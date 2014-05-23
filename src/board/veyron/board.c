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

#include <arch/io.h>

#include "base/init_funcs.h"
#include "board/daisy/i2c_arb.h"
#include "drivers/bus/i2c/rk3288.h"
#include "drivers/bus/i2s/exynos5.h"
#include "drivers/bus/i2s/i2s.h"
#include "drivers/bus/spi/exynos5.h"
#include "drivers/bus/usb/usb.h"
#include "drivers/ec/cros/i2c.h"
#include "drivers/flash/spi.h"
#include "drivers/gpio/exynos5250.h"
#include "drivers/gpio/sysinfo.h"
#include "drivers/power/exynos.h"
#include "drivers/sound/i2s.h"
#include "drivers/sound/max98095.h"
#include "drivers/sound/route.h"
#include "drivers/storage/blockdev.h"
#include "drivers/storage/rk_mmc.h"
#include "drivers/tpm/slb9635_i2c.h"
#include "drivers/tpm/tpm.h"
#include "vboot/util/flag.h"

static RkmciHost *emmc_host; ;
static int board_setup(void)
{
	emmc_host = new_rkmci_host(0xff0f0000, 24000000,
					 8, 0, 0x03030001);
	list_insert_after(&emmc_host->mmc.ctrlr.list_node,
			  &fixed_block_dev_controllers);
	{
		ListNode *ctrlrs;
		ctrlrs = &fixed_block_dev_controllers;
		BlockDevCtrlr *ctrlr;
		list_for_each(ctrlr, *ctrlrs, list_node) {
			if (ctrlr->ops.update && ctrlr->need_update &&
			    ctrlr->ops.update(&ctrlr->ops)) {
				
			}
		}
		flash_set_ops(&emmc_host->mmc.media->dev.ops);
		//Rk3288I2c *rki2c0 = new_rk3288_i2c(0, 100);		
	#if 0
		BlockDev *bdev;
		uint64_t lba_start;
		uint64_t lba_count;
		int hl;
		char buffer[100] = {0};
		lba_start = 0x2000;
		lba_count = 1;
		
		
		BlockDevOps *ops = &emmc_host->mmc.media->dev.ops;
		printf("ops=%x\n",ops);
		if (ops->read(ops, lba_start, lba_count, buffer) != lba_count) {
			printf("Read failed.\n");
			return 0;
		}
	#endif
	}
	return 0;
}

INIT_FUNC(board_setup);

