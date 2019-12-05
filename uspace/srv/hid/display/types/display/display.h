/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server display type
 */

#ifndef TYPES_DISPLAY_DISPLAY_H
#define TYPES_DISPLAY_DISPLAY_H

#include <adt/list.h>
#include <io/input.h>
#include "window.h"

/** Display server display */
typedef struct ds_display {
	/** Clients (of ds_client_t) */
	list_t clients;

	/** Next ID to assign to a window.
	 *
	 * XXX Window IDs need to be unique per display just because
	 * we don't have a way to match GC connection to the proper
	 * client. Really this should be in ds_client_t and the ID
	 * space should be per client.
	 */
	ds_wnd_id_t next_wnd_id;
	/** Input service */
	input_t *input;

	/** Seats (of ds_seat_t) */
	list_t seats;

	/** Windows (of ds_window_t) in stacking order */
	list_t windows;

	/** Display devices (of ds_ddev_t) */
	list_t ddevs;
} ds_display_t;

#endif

/** @}
 */
