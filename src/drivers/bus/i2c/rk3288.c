/*
 * Copyright 2013 Google Inc. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <assert.h>
#include <libpayload.h>

#include "base/container_of.h"
#include "drivers/bus/i2c/i2c.h"
#include "drivers/bus/i2c/rk3288.h"

#if 0
#define i2c_info(x...)          printf(x)
#else
#define i2c_info(x...)
#endif

#define i2c_err(x...)			printf(x)

#define RKIO_PMU_PHYS        0xFF730000
#define RKIO_GRF_PHYS        0xFF770000

/* gpio iomux control */
#define GRF_GPIO1D_IOMUX	0x000C
#define GRF_GPIO2C_IOMUX	0x0018
#define GRF_GPIO6B_IOMUX	0x0060
#define GRF_GPIO7CL_IOMUX	0x0074
#define GRF_GPIO7CH_IOMUX	0x0078
#define GRF_GPIO8A_IOMUX	0x0080

#define grf_readl(offset)	readl((void *)RKIO_GRF_PHYS + offset)
#define grf_writel(v, offset)  writel(v, (void *)RKIO_GRF_PHYS + offset)

#define pmu_readl(offset)	readl((void *)RKIO_PMU_PHYS + offset)
#define pmu_writel(v, offset) writel(v, (void *)RKIO_PMU_PHYS + offset)

#define RETRY_COUNT 3
#define I2C_TIMEOUT_US  100000
#define I2C_BUS_MAX 6
#define I2C_NOACK				2
#define I2C_TIMEOUT				3

#define DIV_CEIL(x, y) (((x) + (y) - 1) / y)

#define RK_I2C_CEIL(x, y) \
	  ({ unsigned long __x = (x), __y = (y); (__x + __y - 1) / __y; })

/* Con register bits. */
#define I2C_CON_OFF							0x00
#define I2C_ACT2NAK							0x40
#define I2C_NAK								0x20
#define I2C_STOP							0x10
#define I2C_START							0x08
#define I2C_MODE(mode)						((mode) << 1)
#define I2C_MODE_TX							0x00
#define I2C_MODE_TRX						0x01
#define I2C_MODE_RX							0x02
#define I2C_MODE_RRX						0x03
#define I2C_EN								0x01

/* Clkdiv register bits */
#define I2C_CLKDIV_OFF		0x04
#define I2C_DIV(divh, divl)	(((divh) << 16) | (divl))

/* Mrxaddr register bits. */
#define I2C_MRXADDR_OFF		0x08
#define I2C_MRXADDR(vld, addr)	(((vld) << 24) | (addr))

/* Mrxraddr register bits. */
#define I2C_MRXRADDR_OFF	0x0c
#define I2C_MRXRADDR(vld, raddr) (((vld) << 24) | (raddr))

/* Mtxcnt register bits. */
#define I2C_MTXCNT_OFF		0x10
#define I2C_MTXCNT(cnt)		((cnt) & 0x3F)

/* Mrxcnt register bits. */
#define I2C_MRXCNT_OFF		0x14
#define I2C_MRXCNT(cnt)		((cnt) & 0x3F)

/* Ien register bits. */
#define I2C_IEN_OFF			0x18
#define I2C_NAKRCVIEN						0x40
#define I2C_STOPIEN							0x20
#define I2C_STARTIEN						0x10
#define I2C_MBRFIEN							0x08
#define I2C_MBTFIEN							0x04
#define I2C_BRFIEN							0x02
#define I2C_BTFIEN							0x01

/* Ipd register bits. */
#define I2C_IPD_OFF			0x1c
#define I2C_NAKRCVIPD						0x40
#define I2C_STOPIPD							0x20
#define I2C_STARTIPD						0x10
#define I2C_MBRFIPD							0x08
#define I2C_MBTFIPD							0x04
#define I2C_BRFIPD							0x02
#define I2C_BTFIPD							0x01
#define I2C_CLEAN_IPDS						0x7F

/* Fcnt register bits. */
#define I2C_FCNT_OFF		0x20
#define I2C_FCNT_MASK						0x3F

/* Txdata register */
#define I2C_TXDATA0_OFF		0x100

/* Rxdata register */
#define I2C_RXDATA0_OFF		0x200

/* TODO: Define other controller-specific values. */
#define SI2C_MAX_TRANSFER_LENGTH            0x1000

/* TODO: Remove these generic error defines in favor
 *      of real register bit mappings defined above.
 */
#define I2C_STAGE_IDLE						0x00000000
#define I2C_STAGE_SEND_START				0x00010000
#define I2C_STAGE_SEND_ADDR					0x00020000
#define I2C_STAGE_SEND_DATA					0x00040000
#define I2C_STAGE_RECV_DATA					0x00080000
#define I2C_STAGE_SEND_STOP					0x00100000
#define I2C_STATUS_ADDR_NACK				0x10000000
#define I2C_STATUS_DATA_NACK				0x20000000
#define I2C_STATUS_GENERIC_ERROR			0x40000000
#define I2C_STAGE_MASK						0xFFFF0000
#define I2C_GET_STAGE(x)				((x) & I2C_STAGE_MASK)

struct rk_i2c {
	int *regs;
	unsigned int clk_src;
};

static struct rk_i2c rki2c_base[I2C_BUS_MAX] = {
	{.regs = (int *)(0xff650000), 64000000},
	{.regs = (int *)(0xff140000), 64000000},
	{.regs = (int *)(0xff660000), 48000000},
	{.regs = (int *)(0xff150000), 48000000},
	{.regs = (int *)(0xff160000), 48000000},
	{.regs = (int *)(0xff170000), 48000000}
};

static void rk_i2c_iomux_config(int i2c_id)
{
	switch (i2c_id) {
	case 0:
		pmu_writel(pmu_readl(0x88) | (1 << 14), 0x88);
		pmu_writel(pmu_readl(0x8c) | 1, 0x8c);
		break;
	case 1:
		grf_writel((1 << 26) | (1 << 24) | (1 << 10) | (1 << 8),
			   GRF_GPIO8A_IOMUX);
		break;
	case 2:
		grf_writel((1 << 20) | (1 << 18) | (1 << 4) | (1 << 2),
			   GRF_GPIO6B_IOMUX);
		break;
	case 3:
		grf_writel((1 << 20) | (1 << 18) | (1 << 2) | 1,
			   GRF_GPIO2C_IOMUX);
		break;
	case 4:
		grf_writel((1 << 28) | (1 << 26) | (1 << 12) | (1 << 10),
			   GRF_GPIO1D_IOMUX);
		break;
	case 5:
		grf_writel((3 << 28) | (1 << 12), GRF_GPIO7CL_IOMUX);
		grf_writel((3 << 16) | 1, GRF_GPIO7CH_IOMUX);
		break;
	default:
		i2c_err("RK have not this i2c iomux id!\n");
		break;
	}
}

static void i2c_set_clkdiv(void *reg_addr, unsigned int clk_src, int speed)
{
	unsigned int div, divl, divh;

	div = RK_I2C_CEIL(clk_src, speed * 8);
	divh = divl = RK_I2C_CEIL(div, 2);
	writel(I2C_DIV(divl, divh), reg_addr + I2C_CLKDIV_OFF);
}

static int i2c_init(Rk3288I2c *bus)
{
	rk_i2c_iomux_config(bus->bus_id);
	i2c_set_clkdiv(bus->reg_addr, bus->clk_src, 100);
	return 0;
}

static int i2c_send_start_bit(void *reg_addr)
{
	int res = 0;
	int timeout = I2C_TIMEOUT_US;

	i2c_info("I2c Start::Send Start bit\n");
	writel(I2C_CLEAN_IPDS, reg_addr + I2C_IPD_OFF);
	writel(I2C_EN | I2C_START, reg_addr + I2C_CON_OFF);
	writel(I2C_STARTIEN, reg_addr + I2C_IEN_OFF);

	while (timeout--) {
		if (readl(reg_addr + I2C_IPD_OFF) & I2C_STARTIPD) {
			writel(I2C_STARTIPD, reg_addr + I2C_IPD_OFF);
			break;
		}
		udelay(1);
	}

	if (timeout <= 0) {
		i2c_err("I2C Start::Send Start Bit Timeout\n");
		res = I2C_TIMEOUT;
	}

	return res;
}

static int i2c_send_stop_bit(void *reg_addr)
{
	int res = 0;
	int timeout = I2C_TIMEOUT_US;

	i2c_info("I2c Stop::Send Stop bit\n");
	writel(I2C_CLEAN_IPDS, reg_addr + I2C_IPD_OFF);
	writel(I2C_EN | I2C_STOP, reg_addr + I2C_CON_OFF);
	writel(I2C_STOPIEN, reg_addr + I2C_IEN_OFF);

	while (timeout--) {
		if (readl(reg_addr + I2C_IPD_OFF) & I2C_STOPIPD) {
			writel(I2C_STOPIPD, reg_addr + I2C_IPD_OFF);
			break;
		}
		udelay(1);
	}

	if (timeout <= 0) {
		i2c_err("I2C Stop::Send Stop Bit Timeout\n");
		res = I2C_TIMEOUT;
	}

	return res;
}

static void i2c_disable(void *reg_addr)
{
	writel(0, reg_addr + I2C_CON_OFF);
}

static int i2c_read(void *reg_addr, I2cSeg segment, int retry_count)
{
	int res = 0;
	uint8_t *p = segment.buf;
	int timeout = I2C_TIMEOUT_US;
	unsigned int bytes_remaining_to_be_transfered = segment.len;
	unsigned int bytes_to_be_transfered = 0;
	unsigned int words_to_be_transfered = 0;
	unsigned int rxdata = 0;
	unsigned int con = 0;
	unsigned int i, j;

	res = i2c_send_start_bit(reg_addr);
	if (res)
		return res;

	writel(I2C_MRXADDR(1, segment.chip << 1 | 1),
	       reg_addr + I2C_MRXADDR_OFF);
	writel(0, reg_addr + I2C_MRXRADDR_OFF);

	while (bytes_remaining_to_be_transfered) {
		if (bytes_remaining_to_be_transfered > 32) {
			con = I2C_EN | I2C_MODE(I2C_MODE_TRX);
			bytes_to_be_transfered = 32;
		} else {
			con = I2C_EN | I2C_MODE(I2C_MODE_TRX) | I2C_NAK;
			bytes_to_be_transfered =
			    bytes_remaining_to_be_transfered;
		}

		words_to_be_transfered = DIV_CEIL(bytes_to_be_transfered, 4);

		writel(con, reg_addr + I2C_CON_OFF);
		writel(bytes_to_be_transfered, reg_addr + I2C_MRXCNT_OFF);
		writel(I2C_MBRFIPD | I2C_NAKRCVIEN, reg_addr + I2C_IEN_OFF);

		timeout = I2C_TIMEOUT_US;
		while (timeout--) {
			if (readl(reg_addr + I2C_IPD_OFF) & I2C_NAKRCVIPD) {
				writel(I2C_NAKRCVIPD, reg_addr + I2C_IPD_OFF);
				if (retry_count == RETRY_COUNT) {
					i2c_err("I2C Read::Received no ack, Retry %d...\n",
					     retry_count);
				}
				res = I2C_NOACK;
			}

			if (readl(reg_addr + I2C_IPD_OFF) & I2C_MBRFIPD) {
				writel(I2C_MBRFIPD, reg_addr + I2C_IPD_OFF);
				break;
			}

			udelay(1);
		}

		if (timeout <= 0) {
			i2c_err("I2C Read::Recv Data Timeout\n");
			res = I2C_TIMEOUT;
			goto exit;
		}

		for (i = 0; i < words_to_be_transfered; i++) {
			rxdata = readl(reg_addr + I2C_RXDATA0_OFF + i * 4);
			i2c_info("I2c Read::RXDATA[%d] = 0x%x\n", i, rxdata);
			for (j = 0; j < 4; j++) {
				if ((i * 4 + j) == bytes_to_be_transfered)
					break;
				*p++ = (rxdata >> (j * 8)) & 0xff;
			}
		}
		bytes_remaining_to_be_transfered -= bytes_to_be_transfered;
		i2c_info("I2C Read::bytes_remaining_to_be_transfered %d\n",
			 bytes_remaining_to_be_transfered);
	}
exit:
	return res;
}

static int i2c_write(void *reg_addr, I2cSeg segment, int retry_count)
{
	int res = 0;
	uint8_t *p = segment.buf;
	int timeout = I2C_TIMEOUT_US;
	unsigned int bytes_remaining_to_be_transfered = segment.len + 1;
	unsigned int bytes_to_be_transfered = 0;
	unsigned int words_to_be_transfered = 0;
	unsigned int txdata = 0;
	unsigned int i, j;

	res = i2c_send_start_bit(reg_addr);
	if (res)
		return res;

	while (bytes_remaining_to_be_transfered) {
		if (bytes_remaining_to_be_transfered > 32) {
			bytes_to_be_transfered = 32;
		} else {
			bytes_to_be_transfered =
			    bytes_remaining_to_be_transfered;
		}

		words_to_be_transfered = DIV_CEIL(bytes_to_be_transfered, 4);

		for (i = 0; i < words_to_be_transfered; i++) {
			txdata = 0;
			for (j = 0; j < 4; j++) {
				if ((i * 4 + j) == bytes_to_be_transfered)
					break;

				if (i == 0 && j == 0)
					txdata |= (segment.chip << 1);
				else
					txdata |= (*p++) << (j * 8);
			}
			writel(txdata, reg_addr + I2C_TXDATA0_OFF + i * 4);
			i2c_info("I2c Write::TXDATA[%d] = 0x%x\n", i, txdata);
		}

		writel(I2C_EN | I2C_MODE(I2C_MODE_TX), reg_addr + I2C_CON_OFF);
		writel(bytes_to_be_transfered, reg_addr + I2C_MTXCNT_OFF);
		writel(I2C_MBTFIPD | I2C_NAKRCVIEN, reg_addr + I2C_IEN_OFF);

		timeout = I2C_TIMEOUT_US;
		while (timeout--) {
			if (readl(reg_addr + I2C_IPD_OFF) & I2C_NAKRCVIPD) {
				writel(I2C_NAKRCVIPD, reg_addr + I2C_IPD_OFF);
				if (retry_count == RETRY_COUNT) {
					i2c_err("I2C Read::Received no ack, Retry %d...\n",
					     retry_count);
				}
				res = I2C_NOACK;
			}

			if (readl(reg_addr + I2C_IPD_OFF) & I2C_MBTFIPD) {
				writel(I2C_MBTFIPD, reg_addr + I2C_IPD_OFF);
				break;
			}

			udelay(1);
		}

		if (timeout <= 0) {
			i2c_err("I2C Write::Send Data Timeout\n");
			res = I2C_TIMEOUT;
			goto exit;
		}

		bytes_remaining_to_be_transfered -= bytes_to_be_transfered;
		i2c_info("I2C Write::bytes_remaining_to_be_transfered %d\n",
			 bytes_remaining_to_be_transfered);
	}
exit:
	return res;
}

static int i2c_do_xfer(void *reg_addr, I2cSeg segment)
{
	int res = 0;
	int retry_count = 0;

	while (retry_count++ < RETRY_COUNT) {
		if (segment.read)
			res = i2c_read(reg_addr, segment, retry_count);
		else
			res = i2c_write(reg_addr, segment, retry_count);

		if (res != I2C_NOACK)
			break;
	}
	res = i2c_send_stop_bit(reg_addr);

	i2c_disable(reg_addr);

	return res;
}

static int i2c_transfer(I2cOps *me, I2cSeg *segments, int seg_count)
{
	Rk3288I2c *bus = container_of(me, Rk3288I2c, ops);
	int res = 0;
	int i;

	if (!bus->ready) {
		if (i2c_init(bus))
			return 1;
		bus->ready = 1;
	}

	for (i = 0; i < seg_count; i++) {
		res = i2c_do_xfer(bus->reg_addr, segments[i]);
		if (res)
			break;
	}

	return (res != 0);
}

Rk3288I2c *new_rk3288_i2c(int id, int speed)
{
	Rk3288I2c *bus = xzalloc(sizeof(*bus));
	bus->reg_addr = rki2c_base[id].regs;
	bus->clk_src = rki2c_base[id].clk_src;
	bus->ready = 0;
	bus->bus_id = id;
	bus->ops.transfer = &i2c_transfer;
	return bus;
}
