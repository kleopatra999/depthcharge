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

#include <assert.h>
#include <libpayload.h>
#include <stdint.h>

#include "base/container_of.h"
#include "drivers/gpio/rk3288.h"
#include "drivers/gpio/gpio.h"

static int rk_gpio_bank_reg[GPIO_BANK_NUM] = {
	0xFF750000,   /* gpio0 */
	0xFF780000,   /* gpio1 */
	0xFF790000,   /* gpio2 */
	0xFF7A0000,   /* gpio3 */
	0xFF7B0000,   /* gpio4 */
	0xFF7C0000,   /* gpio5 */
	0xFF7D0000,   /* gpio6 */
	0xFF7E0000,   /* gpio7 */
	0xFF7F0000,   /* gpio8 */
};

static inline void rk_gpio_bit_op(int *regbase,
						unsigned int offset,
						unsigned int bit,
						unsigned char flag)
{
	unsigned int val = readl(regbase + offset);

	if (flag)
		val |= bit;
	else
		val &= ~bit;

	writel(val, regbase + offset);
}

static inline void rk_gpio_set_pin_level(int *regbase,
							unsigned int bit,
							eGPIOPinLevel_t level)
{
	rk_gpio_bit_op(regbase, GPIO_SWPORT_DR, bit, level);
}

static inline int rk_gpio_get_pin_level(int *regbase, unsigned int bit)
{
	return (readl(regbase + GPIO_EXT_PORT) & bit) != 0;
}

static inline void rk_gpio_set_pin_direction(int *regbase,
						unsigned int bit,
						eGPIOPinDirection_t direction)
{
	rk_gpio_bit_op(regbase, GPIO_SWPORT_DDR, bit, direction);
}

static int rk3288_gpio_get_value(GpioOps *me)
{
	assert(me);
	Rk3288Gpio *gpio = container_of(me, Rk3288Gpio, ops);

	if (!gpio->dir_set) {
		rk_gpio_set_pin_direction((int *)rk_gpio_bank_reg[gpio->bank],
								gpio->index,
								GPIO_IN);
		gpio->dir_set = 1;
	}

	return rk_gpio_get_pin_level((int *)rk_gpio_bank_reg[gpio->bank],
		gpio->index);
}


static int rk3288_gpio_set_value(GpioOps *me, unsigned value)
{
	assert(me);
	Rk3288Gpio *gpio = container_of(me, Rk3288Gpio, ops);
	rk_gpio_set_pin_level((int *)rk_gpio_bank_reg[gpio->bank],
						gpio->index,
						value);

	if (!gpio->dir_set) {
		rk_gpio_set_pin_direction((int *)rk_gpio_bank_reg[gpio->bank],
								gpio->index,
								GPIO_OUT);
		gpio->dir_set = 1;
	}

	return 0;
}

Rk3288Gpio *new_rk3288_gpio(unsigned bank, unsigned index)
{
	die_if(bank >= GPIO_BANK_NUM ||
	       index >= GPIO_INDEX_NUM,
	       "GPIO parameters (%d, %d) out of bounds.\n",
	       bank, index);

	Rk3288Gpio *gpio = xzalloc(sizeof(*gpio));
	gpio->bank = bank;
	gpio->index = index;
	return gpio;
}

Rk3288Gpio *new_rk3288_gpio_input(unsigned bank,	unsigned index)
{
	Rk3288Gpio *gpio = new_rk3288_gpio(bank, index);
	gpio->ops.get = &rk3288_gpio_get_value;
	return gpio;
}

Rk3288Gpio *new_rk3288_gpio_output(unsigned bank, unsigned index)
{
	Rk3288Gpio *gpio = new_rk3288_gpio(bank, index);
	gpio->ops.set = &rk3288_gpio_set_value;
	return gpio;
}
