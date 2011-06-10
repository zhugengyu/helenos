/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * Functions needed by hub drivers.
 *
 * For class specific requests, see usb/classes/hub.h.
 */
#ifndef LIBUSBDEV_HUB_H_
#define LIBUSBDEV_HUB_H_

#include <sys/types.h>
#include <usb/hc.h>

int usb_hc_new_device_wrapper(ddf_dev_t *, usb_hc_connection_t *, usb_speed_t,
    int (*)(int, void *), int, void *,
    usb_address_t *, devman_handle_t *,
    ddf_dev_ops_t *, void *, ddf_fun_t **);

/** Info about device attached to host controller.
 *
 * This structure exists only to keep the same signature of
 * usb_hc_register_device() when more properties of the device
 * would have to be passed to the host controller.
 */
typedef struct {
	/** Device address. */
	usb_address_t address;
	/** Devman handle of the device. */
	devman_handle_t handle;
} usb_hc_attached_device_t;

usb_address_t usb_hc_request_address(usb_hc_connection_t *, usb_speed_t);
int usb_hc_register_device(usb_hc_connection_t *,
    const usb_hc_attached_device_t *);
int usb_hc_unregister_device(usb_hc_connection_t *, usb_address_t);

#endif
/**
 * @}
 */
