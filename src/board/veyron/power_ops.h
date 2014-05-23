/*
 * Copyright 2013 Google Inc.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 */

#ifndef __BOARD_RKSDK_POWER_OPS_H__
#define __BOARD_RKSDK_POWER_OPS_H__

#include "drivers/bus/i2c/i2c.h"
#include "drivers/power/power.h"
#include "drivers/power/rk808.h"
#include "drivers/power/rk3288.h"

typedef struct {
	PowerOps ops;
	PowerOps *cpu;
	PowerOps *pmic;
} Rk3288PowerOps;

Rk3288PowerOps *new_rk3288_power_ops(PowerOps *cpu,
				 PowerOps *pmic);

#endif /* __BOARD_RKSDK_POWER_OPS_H__ */
