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
#include "drivers/bus/spi/rk3288.h"
#include "drivers/bus/usb/usb.h"
#include "drivers/ec/cros/i2c.h"
#include "drivers/flash/spi.h"

#include "drivers/gpio/sysinfo.h"
#include "board/rksdk/power_ops.h"
#include "drivers/power/rk3288.h"
#include "drivers/power/rk808.h"
#include "drivers/sound/i2s.h"
#include "drivers/sound/max98095.h"
#include "drivers/sound/route.h"
#include "drivers/storage/blockdev.h"
#include "drivers/storage/rk_mmc.h"
#include "drivers/tpm/slb9635_i2c.h"
#include "drivers/tpm/tpm.h"
#include "vboot/util/flag.h"

//static rkmcihost *emmc_host;
static int board_setup(void)
{
	rkmcihost *emmc = new_rkmci_host(0xff0f0000, 24000000, 8, 0, 0x03030001);
	Rk3288Spi *spi2  =  new_rk3288_spi(2, 0, 400000, 0, 0);
	flash_set_ops(&new_spi_flash(&spi2->ops, 0x400000)->ops);
	/*Rk3288I2c *rki2c0 = new_rk3288_i2c(0, 100);*/
	list_insert_after(&emmc->mmc.ctrlr.list_node,
			  &fixed_block_dev_controllers);
		Rk3288I2c *rki2c0 = new_rk3288_i2c(0, 100);
		Rk808Pmic *rk808 = new_rk808_pmic(&rki2c0->ops, 0x1b);
		Rk3288PowerOps *rk3288_power_ops =
						new_rk3288_power_ops(&rk3288_cpu_power_ops, &rk808->ops);
		power_set_ops(&rk3288_power_ops->ops);
	return 0;
}

INIT_FUNC(board_setup);
