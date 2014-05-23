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
#include <stdint.h>

#include "drivers/power/rk3288.h"
#include "drivers/power/power.h"

enum rk_plls_id {
	APLL_ID = 0,
	DPLL_ID,
	CPLL_ID,
	GPLL_ID,
	NPLL_ID,
	END_PLL_ID
};

#define RKIO_SECURE_GRF_PHYS 0xFF740000
#define RKIO_CRU_PHYS 0xFF760000
#define RKIO_GRF_PHYS 0xFF770000

#define SGRF_SOC_CON0		0x00
#define CRU_MODE_CON		0x50
#define CRU_GLB_SRST_SND	0x1B4
#define PLL_MODE_SLOW(id)	((id == NPLL_ID) ? \
		(0x0<<14)|(0x3<<(16+14)) : \
		((0x0<<((id)*4))|(0x3<<(16+(id)*4))))


static int rk3288_cold_reboot(PowerOps *me)
{
    /* disable remap */
	/* rk3288 address remap control bit: SGRF soc con0 bit 11 */
	writel(1 << (11 + 16),
		(uint32_t *)(RKIO_SECURE_GRF_PHYS + SGRF_SOC_CON0));

	/* pll enter slow mode */
	writel(PLL_MODE_SLOW(APLL_ID) | PLL_MODE_SLOW(GPLL_ID)
		| PLL_MODE_SLOW(CPLL_ID) | PLL_MODE_SLOW(NPLL_ID),
		(uint32_t *)(RKIO_GRF_PHYS + CRU_MODE_CON));

	/* soft reset */
	writel(0xeca8, (uint32_t *)(RKIO_CRU_PHYS + CRU_GLB_SRST_SND));

	halt();
	return 0;
}

PowerOps rk3288_cpu_power_ops = {
	.cold_reboot = &rk3288_cold_reboot,
	.power_off = NULL
};
