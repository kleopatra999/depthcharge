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

#ifndef __DRIVERS_BUS_SPI_RK3288_H__
#define __DRIVERS_BUS_SPI_RK3288_H__

#include "drivers/bus/spi/spi.h"

typedef enum {
	CLOCK_POLARITY_LOW,
	CLOCK_POLARITY_HIGN,
} ClockPolarity;

typedef enum {
	CLOCK_PHASE_SECOND,
	CLOCK_PHASE_FIRST,
} ClockPhase;

typedef struct Rk3288Spi {
	SpiOps ops;
	void *reg_addr;
	unsigned int cs;
	unsigned int div;
	ClockPolarity polarity;
	ClockPhase phase;
	int initialized;
} Rk3288Spi;

Rk3288Spi *new_rk3288_spi(int id, unsigned int cs, unsigned int speed, ClockPolarity polarity, ClockPhase phase);

#endif /* __DRIVERS_BUS_SPI_RK3288_H__ */
