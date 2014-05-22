
#include <assert.h>
#include <libpayload.h>
#include "base/container_of.h"
#include "drivers/bus/spi/spi.h"
#include "drivers/bus/spi/rk3288.h"

#if 0
#define spi_info(x...)          printf(x)
#else
#define spi_info(x...)
#endif

#define spi_err(x...)						printf(x)

#define DIV_CEIL(x, y)              (((x) + (y) - 1) / y)

#define SPI_TIMEOUT_US  100000

#define SPI_CTRLR0_OFF				0x00
#define SPI_CTRLR1_OFF				0x04
#define SPI_ENR_OFF						0x08
#define SPI_SER_OFF						0x0c
#define SPI_BAUDR_OFF					0x10
#define SPI_TXFTLR_OFF				0x1c
#define SPI_RXFTLR_OFF				0x20
#define SPI_SR_OFF						0x24
#define SPI_IPR_OFF						0x28
#define SPI_IMR_OFF						0x2c
#define SPI_ISR_OFF						0x30
#define SPI_RISR_OFF					0x34
#define SPI_ICR_OFF						0x38
#define SPI_DMACR_OFF					0x3c
#define SPI_DMATDLR_OFF				0x40
#define SPI_DMARDLR_OFF				0x44
#define SPI_TXDR_OFF					0x0400
#define SPI_RXDR_OFF					0x0800


#define FIFO_DEPTH		32



/*Control register 0*/

#define CR0_OP_MODE_BIT			20
#define CR0_TS_MODE_BIT			18
enum {
	DUPLEX = 0,
	TXONLY,
	RXONLY
};
#define CR0_HALFWORD_TS_BIT		13
#define CR0_SSN_DELAY_BIT		10
#define CR0_CLOCK_POLARITY_BIT	7
#define CR0_CLOCK_PHASE_BIT		6
#define CR0_FRAME_SIZE_BIT		0
enum {
	FRAME_SIZE_4BIT,
	FRAME_SIZE_8BIT,
	FRAME_SIZE_16BIT
};


/* Control register 1*/

#define CR1_FRAME_NUM_MASK		0xffff


/*Controller enable register*/


enum {
	CONTROLLER_DISABLE,
	CONTROLLER_ENABLE
};


/* Slave select register*/


#define MAX_SLAVE	2
#define SelectSlave(x)	((1<<(x)) & 0x03)


/* Status Register*/


#define SR_RX_FULL			(1<<4)
#define SR_RX_EMPTY			(1<<3)
#define SR_TX_EMPTY			(1<<2)
#define SR_TX_FULL			(1<<1)
#define SR_IDLE				(1<<0)


/* Interrupt Mask Register*/

#define IMR_RX_FULL			(1<<4)
#define IMR_RX_OVERFLOW		(1<<3)
#define IMR_RX_UNDERFLOW	(1<<2)
#define IMR_TX_OVERFLOW		(1<<1)
#define IMR_TX_EMPTY		(1<<0)

/* Interrupt Status Register*/

#define ISR_RX_FULL			(1<<4)
#define ISR_RX_OVERFLOW		(1<<3)
#define ISR_RX_UNDERFLOW	(1<<2)
#define ISR_TX_OVERFLOW		(1<<1)
#define ISR_TX_EMPTY		(1<<0)


/* Interrupt Clear Register*/

#define ICR_TX_OVERFLOW		(1<<3)
#define ICR_RX_OVERFLOW		(1<<2)
#define ICR_RX_UNDERFLOW	(1<<1)
#define ICR_CLEAR_ALL		(1<<0)

static void setbits32(uint32_t *data, uint32_t bits)
{
	writel(readl(data) | bits, data);
}

static void clrbits32(uint32_t *data, uint32_t bits)
{
	writel(readl(data) & ~bits, data);
}
static int rockchip_spi_wait_till_not_busy(Rk3288Spi *bus)
{
	unsigned int delay = 1000;
	while (delay--) {
		if (!(readl(bus->reg_addr + SPI_SR_OFF) & 0x01))
			return 0;
		udelay(1);
	}
	return -1;
}

static int spi_recv(Rk3288Spi *bus, void *in, uint32_t size)
{
	unsigned int cr0 = 0;
	uint32_t bytes_remaining_to_be_transfered;
	int len;
	uint8_t *p = in;
	len = size;
	writel(CONTROLLER_DISABLE, bus->reg_addr + SPI_ENR_OFF);
	assert(!(readl(bus->reg_addr + SPI_ENR_OFF) & 0x01));
	cr0 |= 1 << CR0_HALFWORD_TS_BIT;
	cr0 |= 1 << CR0_SSN_DELAY_BIT;
	cr0 |= RXONLY << CR0_TS_MODE_BIT;
	cr0 |= (bus->polarity << CR0_CLOCK_POLARITY_BIT);
	cr0 |= (bus->phase << CR0_CLOCK_PHASE_BIT);
	cr0 |= FRAME_SIZE_8BIT << CR0_FRAME_SIZE_BIT;
	writel(cr0, bus->reg_addr + SPI_CTRLR0_OFF);
	while (len) {
		writel(CONTROLLER_DISABLE, bus->reg_addr + SPI_ENR_OFF);
		if (len > 0xffff) {
			bytes_remaining_to_be_transfered = 0xffff;
			writel(0xffff, bus->reg_addr + SPI_CTRLR1_OFF);
			len = len - 0xffff;
		} else {
			bytes_remaining_to_be_transfered = len;
			writel(len, bus->reg_addr + SPI_CTRLR1_OFF);
			len = 0;
		}
		writel(CONTROLLER_ENABLE, bus->reg_addr + SPI_ENR_OFF);
		while (bytes_remaining_to_be_transfered) {
			if (readl(bus->reg_addr + SPI_RXFTLR_OFF) & 0x3f) {
				*p++ = readl(bus->reg_addr + SPI_RXDR_OFF) & 0xff;
				bytes_remaining_to_be_transfered--;
			}
		}
	}
	if (rockchip_spi_wait_till_not_busy(bus))
		return -1;
	return  0;
}
static int spi_send(Rk3288Spi *bus, const void *out, uint32_t size)
{
	unsigned int cr0 = 0;
	uint32_t bytes_remaining_to_be_transfered = size;
	uint8_t *p = (uint8_t *)out;
	int len;
	len = size-1;
	writel(CONTROLLER_DISABLE, bus->reg_addr + SPI_ENR_OFF);
	assert(!(readl(bus->reg_addr + SPI_ENR_OFF) & 0x01));
	cr0 |= 1 << CR0_HALFWORD_TS_BIT;
	cr0 |= 1 << CR0_SSN_DELAY_BIT;
	cr0 |= TXONLY << CR0_TS_MODE_BIT;
	cr0 |= (bus->polarity << CR0_CLOCK_POLARITY_BIT);
	cr0 |= (bus->phase << CR0_CLOCK_PHASE_BIT);
	cr0 |= FRAME_SIZE_8BIT << CR0_FRAME_SIZE_BIT;
	writel(cr0, bus->reg_addr + SPI_CTRLR0_OFF);
	writel(len, bus->reg_addr + SPI_CTRLR1_OFF);
	writel(CONTROLLER_ENABLE, bus->reg_addr + SPI_ENR_OFF);
	while (bytes_remaining_to_be_transfered) {
		if ((readl(bus->reg_addr + SPI_TXFTLR_OFF) & 0x3f) < FIFO_DEPTH) {
			writel(*p++, bus->reg_addr + SPI_TXDR_OFF);
			bytes_remaining_to_be_transfered--;
		}
	}
	if (rockchip_spi_wait_till_not_busy(bus))
		return -1;
	return  0;
}

static int spi_transfer(SpiOps *me, void *in, const void *out,
						uint32_t size)
{
	int res = 0;
	Rk3288Spi *bus = container_of(me, Rk3288Spi, ops);
	spi_info("spi:: transfer\n");
	assert((in != NULL) ^ (out != NULL));
	if (in != NULL)
		res = spi_recv(bus, in, size);
	else
		res = spi_send(bus, out, size);
	return res;
}

static int spi_start(SpiOps *me)
{
	int res = 0;
	Rk3288Spi *bus = container_of(me, Rk3288Spi, ops);
	spi_info("spi:: start\n");
	writel(FIFO_DEPTH / 2 - 1, bus->reg_addr + SPI_TXFTLR_OFF);
	writel(FIFO_DEPTH / 2 - 1, bus->reg_addr + SPI_RXFTLR_OFF);
	setbits32(bus->reg_addr + SPI_SER_OFF, 1);
	/*writel(bus->div, bus->reg_addr + SPI_BAUDR_OFF);*/
	return res;
}

static int spi_stop(SpiOps *me)
{
	int res = 0;

	Rk3288Spi *bus = container_of(me, Rk3288Spi, ops);
	spi_info("spi:: stop\n");
	clrbits32(bus->reg_addr + SPI_SER_OFF, 1);
	/*writel(CONTROLLER_DISABLE, bus->reg_addr + SPI_ENR_OFF);*/
	return res;
}
Rk3288Spi *new_rk3288_spi(int id, unsigned int cs, unsigned int speed,
		ClockPolarity polarity, ClockPhase phase)
{
	Rk3288Spi *bus = NULL;
	unsigned int clk_src = 0, div = 0;
	if (cs >= MAX_SLAVE)
		return NULL;
	bus = xzalloc(sizeof(*bus));
	clk_src = 74250000;
	switch (id) {
	case 0:
		bus->reg_addr = (void *)0xff110000;
		writel(0xff005500, (void *)0xff770050);
		writel(0x00030001, (void *)0xff770054);
		break;
	case 1:
		bus->reg_addr = (void *)0xff120000;
		writel(0xff00aa00, (void *)0xff770070);
		break;
	case 2:
		bus->reg_addr = (void *)0xff130000;
		writel(0xf0c05040, (void *)0xff770080);
		writel(0x000f0005, (void *)0xff770084);
		break;
	default:
		free(bus);
		return NULL;
	}
	div = DIV_CEIL(clk_src, speed);
	/* must be even and greate than 1*/
	div = DIV_CEIL(div, 2) * 2;
	if (div <= 2)
		div = 2;
	bus->cs = cs;
	bus->div = div;
	bus->polarity = polarity;
	bus->phase = phase;
	bus->ops.start = &spi_start;
	bus->ops.stop = &spi_stop;
	bus->ops.transfer = &spi_transfer;
	return bus;
}

