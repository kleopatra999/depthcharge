/*
 * Copyright 2013 Google Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <libpayload.h>

#include "drivers/flash/flash.h"



#if  USE_EMMC
static BlockDevOps *mmc_ops;
static int emmc_rom_size = 0x400000;
uint8_t *emmc_data;
void flash_set_ops(BlockDevOps *ops)
{
	die_if(mmc_ops, "Flash ops already set.\n");
	emmc_data = xmalloc(emmc_rom_size);
	mmc_ops = ops;
}
void *flash_read(uint32_t offset, uint32_t size)
{
	uint64_t lba_start;
	uint64_t count;
	uint32_t data_offset = 0;
	uint8_t *data;
	die_if(!mmc_ops, "%s: No flash ops set.\n", __func__);
	lba_start = offset/512;
	count = size/512 + 1;
	data_offset = offset % 512;
	mmc_ops->read(mmc_ops, lba_start, count,emmc_data);
	data = emmc_data + data_offset;
	return data;
}

#else
static FlashOps *flash_ops;
void flash_set_ops(BlockDevOps *ops)
{
	die_if(flash_ops, "Flash ops already set.\n");
	flash_ops = ops;
}

void *flash_read(uint32_t offset, uint32_t size)
{
	die_if(!flash_ops, "%s: No flash ops set.\n", __func__);
	return flash_ops->read(flash_ops, offset, size);
}

#endif

