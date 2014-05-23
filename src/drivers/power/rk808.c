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
#include "base/container_of.h"
#include "drivers/power/rk808.h"

#define RK808_DEVCTRL_REG 0x4b
#define DEV_OFF_RST       (1<<3)

static int rk808_set_bit(I2cOps *bus, uint8_t chip, uint8_t reg, uint8_t bit)
{
	uint8_t val;
	if (i2c_readb(bus, chip, reg, &val) ||
	    i2c_writeb(bus, chip, reg, val | bit))
		return -1;
	return 0;
}

static int rk808_power_off(PowerOps *me)
{
	Rk808Pmic *pmic = container_of(me, Rk808Pmic, ops);
	rk808_set_bit(pmic->bus, pmic->chip, RK808_DEVCTRL_REG,
		       DEV_OFF_RST);
	halt();
	return 0;
}

Rk808Pmic *new_rk808_pmic(I2cOps *bus, uint8_t chip)
{
	Rk808Pmic *pmic = xzalloc(sizeof(*pmic));
	pmic->ops.cold_reboot = NULL;
	pmic->ops.power_off = &rk808_power_off;
	pmic->bus = bus;
	pmic->chip = chip;
	return pmic;
}

