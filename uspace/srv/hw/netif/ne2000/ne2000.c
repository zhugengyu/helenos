/*
 * Copyright (c) 2009 Lukas Mejdrech
 * Copyright (c) 2011 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup ne2000
 *  @{
 */

/** @file
 *  NE2000 network interface implementation.
 */

#include <assert.h>
#include <async.h>
#include <async_obsolete.h>
#include <ddi.h>
#include <errno.h>
#include <err.h>
#include <malloc.h>
#include <sysinfo.h>
#include <ipc/services.h>
#include <ns.h>
#include <ns_obsolete.h>
#include <ipc/irc.h>
#include <net/modules.h>
#include <packet_client.h>
#include <adt/measured_strings.h>
#include <net/device.h>
#include <netif_skel.h>
#include <nil_remote.h>
#include "dp8390.h"

/** Return the device from the interrupt call.
 *
 *  @param[in] call The interrupt call.
 *
 */
#define IRQ_GET_DEVICE(call)  ((device_id_t) IPC_GET_IMETHOD(call))

/** Return the ISR from the interrupt call.
 *
 * @param[in] call The interrupt call.
 *
 */
#define IRQ_GET_ISR(call)  ((int) IPC_GET_ARG2(call))

/** Return the TSR from the interrupt call.
 *
 * @param[in] call The interrupt call.
 *
 */
#define IRQ_GET_TSR(call)  ((int) IPC_GET_ARG3(call))

static bool irc_service = false;
static int irc_phone = -1;

/** NE2000 kernel interrupt command sequence.
 *
 */
static irq_cmd_t ne2k_cmds[] = {
	{
		/* Read Interrupt Status Register */
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,
		.dstarg = 2
	},
	{
		/* Mask supported interrupt causes */
		.cmd = CMD_BTEST,
		.value = (ISR_PRX | ISR_PTX | ISR_RXE | ISR_TXE | ISR_OVW |
		    ISR_CNT | ISR_RDC),
		.srcarg = 2,
		.dstarg = 3,
	},
	{
		/* Predicate for accepting the interrupt */
		.cmd = CMD_PREDICATE,
		.value = 4,
		.srcarg = 3
	},
	{
		/*
		 * Mask future interrupts via
		 * Interrupt Mask Register
		 */
		.cmd = CMD_PIO_WRITE_8,
		.addr = NULL,
		.value = 0
	},
	{
		/* Acknowledge the current interrupt */
		.cmd = CMD_PIO_WRITE_A_8,
		.addr = NULL,
		.srcarg = 3
	},
	{
		/* Read Transmit Status Register */
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,
		.dstarg = 3
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** NE2000 kernel interrupt code.
 *
 */
static irq_code_t ne2k_code = {
	sizeof(ne2k_cmds) / sizeof(irq_cmd_t),
	ne2k_cmds
};

/** Handle the interrupt notification.
 *
 * This is the interrupt notification function. It is quarantied
 * that there is only a single instance of this notification
 * function running at one time until the return from the
 * ne2k_interrupt() function (where the interrupts are unmasked
 * again).
 *
 * @param[in] iid  Interrupt notification identifier.
 * @param[in] call Interrupt notification.
 *
 */
static void irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	device_id_t device_id = IRQ_GET_DEVICE(*call);
	netif_device_t *device;
	int nil_phone;
	ne2k_t *ne2k;
	
	fibril_rwlock_read_lock(&netif_globals.lock);
	
	if (find_device(device_id, &device) == EOK) {
		nil_phone = device->nil_phone;
		ne2k = (ne2k_t *) device->specific;
	} else
		ne2k = NULL;
	
	fibril_rwlock_read_unlock(&netif_globals.lock);
	
	if (ne2k != NULL) {
		link_t *frames =
		    ne2k_interrupt(ne2k, IRQ_GET_ISR(*call), IRQ_GET_TSR(*call));
		
		if (frames != NULL) {
			while (!list_empty(frames)) {
				frame_t *frame =
				    list_get_instance(frames->next, frame_t, link);
				
				list_remove(&frame->link);
				nil_received_msg(nil_phone, device_id, frame->packet,
				    SERVICE_NONE);
				free(frame);
			}
			
			free(frames);
		}
	}
}

/** Change the network interface state.
 *
 * @param[in,out] device Network interface.
 * @param[in]     state  New state.
 *
 */
static void change_state(netif_device_t *device, device_state_t state)
{
	if (device->state != state) {
		device->state = state;
		
		const char *desc;
		switch (state) {
		case NETIF_ACTIVE:
			desc = "active";
			break;
		case NETIF_STOPPED:
			desc = "stopped";
			break;
		default:
			desc = "unknown";
		}
		
		printf("%s: State changed to %s\n", NAME, desc);
	}
}

int netif_specific_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *count)
{
	return ENOTSUP;
}

int netif_get_device_stats(device_id_t device_id, device_stats_t *stats)
{
	if (!stats)
		return EBADMEM;
	
	netif_device_t *device;
	int rc = find_device(device_id, &device);
	if (rc != EOK)
		return rc;
	
	ne2k_t *ne2k = (ne2k_t *) device->specific;
	
	memcpy(stats, &ne2k->stats, sizeof(device_stats_t));
	return EOK;
}

int netif_get_addr_message(device_id_t device_id, measured_string_t *address)
{
	if (!address)
		return EBADMEM;
	
	netif_device_t *device;
	int rc = find_device(device_id, &device);
	if (rc != EOK)
		return rc;
	
	ne2k_t *ne2k = (ne2k_t *) device->specific;
	
	address->value = ne2k->mac;
	address->length = ETH_ADDR;
	return EOK;
}

int netif_probe_message(device_id_t device_id, int irq, void *io)
{
	netif_device_t *device =
	    (netif_device_t *) malloc(sizeof(netif_device_t));
	if (!device)
		return ENOMEM;
	
	ne2k_t *ne2k = (ne2k_t *) malloc(sizeof(ne2k_t));
	if (!ne2k) {
		free(device);
		return ENOMEM;
	}
	
	void *port;
	int rc = pio_enable((void *) io, NE2K_IO_SIZE, &port);
	if (rc != EOK) {
		free(ne2k);
		free(device);
		return rc;
	}
	
	bzero(device, sizeof(netif_device_t));
	bzero(ne2k, sizeof(ne2k_t));
	
	device->device_id = device_id;
	device->nil_phone = -1;
	device->specific = (void *) ne2k;
	device->state = NETIF_STOPPED;
	
	rc = ne2k_probe(ne2k, port, irq);
	if (rc != EOK) {
		printf("%s: No ethernet card found at I/O address %p\n",
		    NAME, port);
		free(ne2k);
		free(device);
		return rc;
	}
	
	rc = netif_device_map_add(&netif_globals.device_map, device->device_id, device);
	if (rc != EOK) {
		free(ne2k);
		free(device);
		return rc;
	}
	
	printf("%s: Ethernet card at I/O address %p, IRQ %d, MAC ",
	    NAME, port, irq);
	
	unsigned int i;
	for (i = 0; i < ETH_ADDR; i++)
		printf("%02x%c", ne2k->mac[i], i < 5 ? ':' : '\n');
	
	return EOK;
}

int netif_start_message(netif_device_t *device)
{
	if (device->state != NETIF_ACTIVE) {
		ne2k_t *ne2k = (ne2k_t *) device->specific;
		
		ne2k_cmds[0].addr = ne2k->port + DP_ISR;
		ne2k_cmds[3].addr = ne2k->port + DP_IMR;
		ne2k_cmds[4].addr = ne2k_cmds[0].addr;
		ne2k_cmds[5].addr = ne2k->port + DP_TSR;
		
		int rc = register_irq(ne2k->irq, device->device_id,
		    device->device_id, &ne2k_code);
		if (rc != EOK)
			return rc;
		
		rc = ne2k_up(ne2k);
		if (rc != EOK) {
			unregister_irq(ne2k->irq, device->device_id);
			return rc;
		}
		
		change_state(device, NETIF_ACTIVE);
		
		if (irc_service)
			async_obsolete_msg_1(irc_phone, IRC_ENABLE_INTERRUPT, ne2k->irq);
	}
	
	return device->state;
}

int netif_stop_message(netif_device_t *device)
{
	if (device->state != NETIF_STOPPED) {
		ne2k_t *ne2k = (ne2k_t *) device->specific;
		
		ne2k_down(ne2k);
		unregister_irq(ne2k->irq, device->device_id);
		change_state(device, NETIF_STOPPED);
	}
	
	return device->state;
}

int netif_send_message(device_id_t device_id, packet_t *packet,
    services_t sender)
{
	netif_device_t *device;
	int rc = find_device(device_id, &device);
	if (rc != EOK)
		return rc;
	
	if (device->state != NETIF_ACTIVE) {
		netif_pq_release(packet_get_id(packet));
		return EFORWARD;
	}
	
	ne2k_t *ne2k = (ne2k_t *) device->specific;
	
	/*
	 * Process the packet queue
	 */
	
	do {
		packet_t *next = pq_detach(packet);
		ne2k_send(ne2k, packet);
		netif_pq_release(packet_get_id(packet));
		packet = next;
	} while (packet);
	
	return EOK;
}

int netif_initialize(void)
{
	sysarg_t apic;
	sysarg_t i8259;
	
	if (((sysinfo_get_value("apic", &apic) == EOK) && (apic))
	    || ((sysinfo_get_value("i8259", &i8259) == EOK) && (i8259)))
		irc_service = true;
	
	if (irc_service) {
		while (irc_phone < 0)
			irc_phone = service_obsolete_connect_blocking(SERVICE_IRC, 0, 0);
	}
	
	async_set_interrupt_received(irq_handler);
	
	return service_register(SERVICE_NE2000);
}

int main(int argc, char *argv[])
{
	/* Start the module */
	return netif_module_start();
}

/** @}
 */
