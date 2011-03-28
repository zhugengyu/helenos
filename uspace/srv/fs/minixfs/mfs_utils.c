/*
 * Copyright (c) 2011 Maurizio Lombardi
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

/** @addtogroup fs
 * @{
 */

#include <byteorder.h>
#include <assert.h>
#include <errno.h>
#include "mfs.h"
#include "mfs_utils.h"

uint16_t conv16(bool native, uint16_t n)
{
	if (native)
		return n;

	return uint16_t_byteorder_swap(n);
}

uint32_t conv32(bool native, uint32_t n)
{
	if (native)
		return n;

	return uint32_t_byteorder_swap(n);
}

uint64_t conv64(bool native, uint64_t n)
{
	if (native)
		return n;

	return uint64_t_byteorder_swap(n);
}

/*
 *Read an indirect block from disk and convert its
 *content to the native endian format.
 */
int
read_ind_block(void *data, struct mfs_instance *inst,
			uint32_t block, mfs_version_t version)
{
	int rc;
	unsigned i;
	block_t *b;
	uint32_t *ptr32;
	uint16_t *ptr16;

	assert(inst);
	devmap_handle_t handle = inst->handle;
	struct mfs_sb_info *sbi = inst->sbi;

	assert(sbi);

	rc = block_get(&b, handle, block, BLOCK_FLAGS_NONE);

	if (rc != EOK)
		goto out;

	if (version == MFS_VERSION_V1) {
		uint16_t *p = b->data;
		ptr16 = data;
		for (i = 0; i < sbi->block_size / sizeof(uint16_t); ++i)
			ptr16[i] = conv16(sbi->native, p[i]);
	} else {
		uint32_t *p = b->data;
		ptr32 = data;
		for (i = 0; i < sbi->block_size / sizeof(uint32_t); ++i)
			ptr32[i] = conv32(sbi->native, p[i]);

	}

	rc = EOK;

out:
	block_put(b);
	return rc;
}

/**
 * @}
 */ 

