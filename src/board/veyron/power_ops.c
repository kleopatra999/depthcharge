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

#include <libpayload.h>
#include "base/container_of.h"
#include "board/veyron/power_ops.h"

static int rk3288_cpu_reboot(PowerOps *me)
{
	Rk3288PowerOps *power = container_of(me, Rk3288PowerOps, ops);
	return power->cpu->cold_reboot(power->cpu);
}

static int rk3288_pmic_power_off(PowerOps *me)
{
	Rk3288PowerOps *power = container_of(me, Rk3288PowerOps, ops);
	return power->pmic->power_off(power->pmic);
}

Rk3288PowerOps *new_rk3288_power_ops(PowerOps *cpu,
				 PowerOps *pmic)
{
	Rk3288PowerOps *power = xzalloc(sizeof(*power));
	power->ops.cold_reboot = &rk3288_cpu_reboot;
	power->ops.power_off = &rk3288_pmic_power_off;
	power->cpu = cpu;
	power->pmic = pmic;
	return power;
}
