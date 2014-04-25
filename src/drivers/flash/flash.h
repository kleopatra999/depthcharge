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

#ifndef __DRIVERS_FLASH_FLASH_H__
#define __DRIVERS_FLASH_FLASH_H__

#include <stdint.h>
#define USE_EMMC  1
typedef struct FlashOps
{
	void *(*read)(struct FlashOps *me, uint32_t offset, uint32_t size);
} FlashOps;
#if USE_EMMC
#include "drivers/storage/blockdev.h"
void flash_set_ops(BlockDevOps *ops);
void *flash_read(uint32_t offset, uint32_t size);
#else
void flash_set_ops(FlashOps *ops);
void *flash_read(uint32_t offset, uint32_t size);
#endif
#endif /* __DRIVERS_FLASH_FLASH_H__ */
