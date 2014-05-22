
#include <assert.h>
#include <libpayload.h>
#include <stddef.h>
#include <stdint.h>
#include "drivers/storage/rk_mmc.h"
/* Set block count limit because of 16 bit register limit on some hardware*/
#ifndef CONFIG_SYS_MMC_MAX_BLK_COUNT
#define CONFIG_SYS_MMC_MAX_BLK_COUNT 65535
#endif
static int mmcwaitbusy(void)
{
	int count;
	int ret = 0;
	/* Wait max 100 ms */
	count = MAX_RETRY_COUNT;
	/* before reset ciu, it should check DATA0. if when DATA0 is low and
	it resets ciu, it might make a problem */
	while ((Readl((gMmcBaseAddr + MMC_STATUS)) & MMC_BUSY)) {
		if (count == 0) {
			ret = -1;
			break;
		}
		count--;
		udelay(1);
	}
	return ret;
}
static int  mci_send_cmd(u32 cmd, u32 arg)
{
	unsigned int retrycount = 0;
	retrycount = 1000;
	Writel(gMmcBaseAddr + MMC_CMD, cmd);
	while ((Readl(gMmcBaseAddr + MMC_CMD) & MMC_CMD_START)
		&& (retrycount > 0)) {
		udelay(1);
		retrycount--;
	}
	 if (retrycount == 0)
		return  -1;
	 return 0;
}
static void emmcpoweren(char en)
{
	if (en) {
		Writel(gMmcBaseAddr + MMC_PWREN, 1);
		Writel(gMmcBaseAddr + MMC_RST_N, 1);
	} else {
		Writel(gMmcBaseAddr + MMC_PWREN, 0);
		Writel(gMmcBaseAddr + MMC_RST_N, 0);
	}
}


static void emmcreset()
{
	int data;
	data = ((1<<16)|(1))<<3;
	Writel(gCruBaseAddr + 0x1d8, data);
	udelay(100);
	data = ((1<<16)|(0))<<3;
	Writel(gCruBaseAddr + 0x1d8, data);
	udelay(200);
	emmcpoweren(1);
}

static void emmc_dev_reset(void)
{
	emmcpoweren(0);
	udelay(5000);
	emmcpoweren(1);
	udelay(1000);
}
static void emmc_gpio_init()
{
	Writel(gGrfBaseAddr + 0x20, 0xffffaaaa);
	Writel(gGrfBaseAddr + 0x24, 0x000c0008);
	Writel(gGrfBaseAddr + 0x28, 0x003f002a);
}

static int rk_emmc_init()
{
	int timeout = 10000;
	printf("rk_emmc_init\n");
	emmc_dev_reset();
	emmcreset();
	emmc_gpio_init();
	Writel(gMmcBaseAddr + MMC_CTRL, MMC_CTRL_RESET | MMC_CTRL_FIFO_RESET);
	Writel(gMmcBaseAddr + MMC_PWREN, 1);
	while ((Readl(gMmcBaseAddr + MMC_CTRL) & (MMC_CTRL_FIFO_RESET
		| MMC_CTRL_RESET)) && (timeout > 0)) {
		udelay(1);
		timeout--;
	}
	if (timeout == 0) {
		printf("rk_emmc_init1\n");
		return -1;
	}
	/* Clear the interrupts for the host controller */
	Writel(gMmcBaseAddr + MMC_RINTSTS, 0xFFFFFFFF);
	/* disable all mmc interrupt first */
	Writel(gMmcBaseAddr + MMC_INTMASK, 0);
	/* Put in max timeout */
	Writel(gMmcBaseAddr + MMC_TMOUT, 0xFFFFFFFF);
	Writel(gMmcBaseAddr + MMC_FIFOTH, (0x3 << 28) |
		((FIFO_DETH/2 - 1) << 16) | ((FIFO_DETH/2) << 0));
	Writel(gMmcBaseAddr + MMC_CLKSRC, 0);
	return 0;
}
static u32 rk_mmc_prepare_command(MmcCommand *cmd, MmcData *data)
{
	uint16_t cmdr;
	cmdr = cmd->cmdidx;
	cmdr |= MMC_CMD_PRV_DAT_WAIT;
	if (cmd->resp_type & MMC_RSP_PRESENT) {
		/* We expect a response, so set this bit */
		cmdr |= MMC_CMD_RESP_EXP;
		if (cmd->resp_type & MMC_RSP_136)
			cmdr |= MMC_CMD_RESP_LONG;
	}
	if (cmd->resp_type & MMC_RSP_CRC)
		cmdr |= MMC_CMD_RESP_CRC;
	if (data) {
		cmdr |= MMC_CMD_DAT_EXP;
		if (data->flags & MMC_DATA_WRITE)
			cmdr |= MMC_CMD_DAT_WR;
	}
	return cmdr;
}
static int rk_mmc_start_command(MmcCommand *cmd, unsigned int cmd_flags)
{
	unsigned int retrycount = 0;
	Writel(gMmcBaseAddr + MMC_CMDARG, cmd->cmdarg);
	Writel(gMmcBaseAddr + MMC_CMD, cmd_flags | MMC_CMD_START
		| MMC_USE_HOLD_REG);
	udelay(1);
	for (retrycount = 0; retrycount < 250000; retrycount++) {
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_CMD_DONE) {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_CMD_DONE);
			break;
		}
		udelay(1);
	}
	if (retrycount == MAX_RETRY_COUNT) {
		printf("Emmc::EmmcSendCmd failed, Cmd: 0x%08x, Arg: 0x%08x\n",
			cmd_flags, cmd->cmdarg);
		return MMC_COMM_ERR;
	}
	if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_CMD_RES_TIME_OUT) {
		Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_CMD_RES_TIME_OUT);
		return MMC_TIMEOUT;
	}
	if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_CMD_ERROR_FLAGS) {
		printf("Emmc::EmmcSendCmd error, Cmd: 0x%08x, Arg: 0x%08x,\
				RINTSTS: 0x%08x\n",
		cmd_flags,  cmd->cmdarg, (Readl(gMmcBaseAddr + MMC_RINTSTS)));
		Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_CMD_ERROR_FLAGS);
		return MMC_COMM_ERR;
	}
	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[3] = Readl(gMmcBaseAddr+MMC_RESP0);
			cmd->response[2] = Readl(gMmcBaseAddr+MMC_RESP1);
			cmd->response[1] = Readl(gMmcBaseAddr+MMC_RESP2);
			cmd->response[0] = Readl(gMmcBaseAddr+MMC_RESP3);
		} else {
			cmd->response[0] = Readl(gMmcBaseAddr+MMC_RESP0);
			cmd->response[1] = 0;
			cmd->response[2] = 0;
			cmd->response[3] = 0;
		}
	}
	return 0;
}
static int emmcwritedata(void *buffer, unsigned int blocks)
{
	unsigned int *databuffer = buffer;
	unsigned int fifocount = 0;
	unsigned int count = 0;
	int data_over_flag = 0;
	unsigned int size32 = blocks * BLKSZ / 4;
	while (size32) {
		fifocount = FIFO_DETH/4 - MMC_GET_FCNT(Readl(gMmcBaseAddr
				+ MMC_STATUS));
		for (count = 0; count < fifocount; count++)
			Writel((gMmcBaseAddr + MMC_DATA), *databuffer++);
		size32 -= fifocount;
		if (Readl(gMmcBaseAddr + MMC_RINTSTS)
				& MMC_DATA_ERROR_FLAGS) {
			printf("Emmc::read data error, RINTSTS: 0x%08x\n",
				(Readl(gMmcBaseAddr + MMC_RINTSTS)));
			Writel(gMmcBaseAddr + MMC_RINTSTS,
					MMC_DATA_ERROR_FLAGS);
			return -1;
		}
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_TXDR) {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_TXDR);
			continue;
		}
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_DATA_OVER) {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_DATA_OVER);
			size32 = 0;
			data_over_flag = 1;
			break;
		}
	}
	if (data_over_flag == 0) {
		count = MAX_RETRY_COUNT;
		while ((!(Readl((gMmcBaseAddr + MMC_RINTSTS))
				& MMC_INT_DATA_OVER)) && count) {
			count--;
			udelay(1);
		}
		if (count == 0) {
			printf("write wait DTO timeout\n");
			return -1;
		} else {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_DATA_OVER);
		}
	}
	if (mmcwaitbusy()) {
		printf("in write wait busy time out\n");
		return -1;
	}
	if (size32)
		return -1;
	else
		return 0;
}

static int emmcreaddata(void *buffer, unsigned int blocks)
{
	int status;
	unsigned int *databuffer = buffer;
	unsigned int fifocount = 0;
	unsigned int count = 0;
	int data_over_flag = 0;
	unsigned int size32 = blocks * BLKSZ / 4;
	while (size32) {
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_DATA_ERROR_FLAGS) {
			printf("Emmc::Read data error, RINTSTS: 0x%08x\n",
				(Readl(gMmcBaseAddr + MMC_RINTSTS)));
			Writel(gMmcBaseAddr + MMC_RINTSTS,
					MMC_DATA_ERROR_FLAGS);
			return -1;
		}
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_RXDR) {
			fifocount = MMC_GET_FCNT(Readl(gMmcBaseAddr +
					MMC_STATUS));
			for (count = 0; count < fifocount; count++)
				*databuffer++ = Readl(gMmcBaseAddr + MMC_DATA);
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_RXDR);
			size32 -= fifocount;
		}
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_DATA_OVER) {
			for (count = 0; count < size32; count++)
				*databuffer++ = Readl(gMmcBaseAddr + MMC_DATA);
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_DATA_OVER);
			size32 = 0;
			data_over_flag = 1;
			break;
		}
	}
	if (data_over_flag == 0) {
		count = MAX_RETRY_COUNT;
		while ((!(Readl(gMmcBaseAddr + MMC_RINTSTS) &
				MMC_INT_DATA_OVER))
				&& count){
			count--;
			udelay(1);
		}
		if (count == 0) {
			printf("read wait DTO timeout\n");
			return -1;
		} else {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_DATA_OVER);
		}
	}
	if (mmcwaitbusy()) {
		printf("in read wait busy time out\n");
		return -1;
	}
	if (size32 == 0)
		status = 0;
	else
		status = -1;
	return status;
}

static int rk_emmc_request(MmcCtrlr *ctrlr, MmcCommand *cmd, MmcData *data)
{
	u32 cmdflags;
	int ret;
	int status;
	int value;
	if (data) {
		Writel(gMmcBaseAddr + MMC_BYTCNT, data->blocksize*data->blocks);
		Writel(gMmcBaseAddr + MMC_BLKSIZ, data->blocksize);
		Writel(gMmcBaseAddr + MMC_CTRL, Readl(gMmcBaseAddr + MMC_CTRL)
				|MMC_CTRL_FIFO_RESET);
		Writel((gMmcBaseAddr + MMC_INTMASK),
				MMC_INT_TXDR | MMC_INT_RXDR |
				MMC_INT_CMD_DONE | MMC_INT_DATA_OVER |
				MMC_ERROR_FLAGS);
		status = mmcwaitbusy();
		if (status < 0) {
			printf("Emmc::EmmcPreTransfer failed, data busy\n");
			return status;
		}
	}
	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION) {
		value = Readl(gMmcBaseAddr + MMC_STATUS);
		if (!(value & MMC_FIFO_EMPTY)) {
			value = Readl(gMmcBaseAddr + MMC_CTRL);
			value |= MMC_CTRL_FIFO_RESET;
			Writel(gMmcBaseAddr + MMC_CTRL, value);
			status = mmcwaitbusy();
			if (status < 0) {
				printf("Emmc:stop cmd error\n");
				return status;
			}
		}
	}
	cmdflags = rk_mmc_prepare_command(cmd, data);
	if (cmd->cmdidx == 0)
		cmdflags |= MMC_CMD_INIT;
	ret = rk_mmc_start_command(cmd, cmdflags);
	if (ret)
		return ret;
	if (data) {
		if (data->flags == MMC_DATA_READ)
			ret = emmcreaddata(data->dest, data->blocks);
		else if (data->flags == MMC_DATA_WRITE)
			ret = emmcwritedata((void *)data->src, data->blocks);
	}
	return ret;
}
#define CRU_CLKSEL_CON		0x60
#define CRU_CLKSELS_CON(i)	(CRU_CLKSEL_CON + ((i) * 4))
void rkclk_emmc_set_clk(int div)
{
	Writel((gCruBaseAddr + CRU_CLKSELS_CON(12)),
			(0xFFul<<24)|(div-1)<<8|(1<<14));
}
void rkmci_setup_bus(rkmcihost *host, uint32_t freq)
{
	int suit_clk_div;
	int src_clk;
	int src_clk_div;
	int second_freq;
	int value;
	freq = host->mmc.f_min;
	if ((freq == host->clock) || (freq == 0))
		return;
	Writel(gMmcBaseAddr + MMC_CLKENA, 0);
	/* inform CIU */
	mci_send_cmd(MMC_CMD_START|MMC_CMD_UPD_CLK|MMC_CMD_PRV_DAT_WAIT, 0);
	if (freq > 24000000)
		freq = 24000000/2;
	/*rk32 emmc src generall pll,emmc automic divide
	setting freq to 1/2,
	for get the right freq ,we divide this freq to 1/2*/
	src_clk = 24000000/2;
	src_clk_div = src_clk/freq;
	if (src_clk_div > 0x3e)
		src_clk_div = 0x3e;
	second_freq = src_clk/src_clk_div;
	suit_clk_div = (second_freq/freq);
	if (((suit_clk_div & 0x1) == 1) && (suit_clk_div != 1))
		suit_clk_div++;
	if (suit_clk_div == 1)
		value = 0;
	else
		value = (suit_clk_div >> 1);
	/* set clock to desired speed */
	Writel(gMmcBaseAddr + MMC_CLKDIV, value);
	/* inform CIU */
	mci_send_cmd(MMC_CMD_START|MMC_CMD_UPD_CLK|MMC_CMD_PRV_DAT_WAIT, 0);

	rkclk_emmc_set_clk(src_clk_div);
	/* enable clock */
	Writel(gMmcBaseAddr + MMC_CLKENA, MMC_CLKEN_ENABLE);
	/* inform CIU */
	mci_send_cmd(MMC_CMD_START|MMC_CMD_UPD_CLK|MMC_CMD_PRV_DAT_WAIT, 0);
	host->clock = freq;
}

static void rk_set_ios(MmcCtrlr *ctrlr)
{
	int cfg = 0;
	rkmcihost *host = container_of(ctrlr, rkmcihost, mmc);
	switch (ctrlr->bus_width) {
	case 4:
		cfg = MMC_CTYPE_4BIT;
		break;
	case 8:
		cfg = MMC_CTYPE_8BIT;
		break;
	default:
		cfg = MMC_CTYPE_1BIT;
	}
	rkmci_setup_bus(host, ctrlr->bus_hz);

	Writel(gMmcBaseAddr + MMC_CTYPE, cfg);
}
static int rkmci_update(BlockDevCtrlrOps *me)
{
	rkmcihost *host = container_of(me, rkmcihost, mmc.ctrlr.ops);
	if (!host->initialized && rk_emmc_init())
		return -1;
	host->initialized = 1;
	if (mmc_setup_media(&host->mmc))
		return -1;
	host->mmc.media->dev.name = "rkmmc";
	host->mmc.media->dev.removable = 0;
	host->mmc.media->dev.ops.read = &block_mmc_read;
	host->mmc.media->dev.ops.write = &block_mmc_write;
	list_insert_after(&host->mmc.media->dev.list_node,
			  &fixed_block_devices);
	host->mmc.ctrlr.need_update = 0;
	return 0;
}

rkmcihost *new_rkmci_host(uintptr_t ioaddr, uint32_t src_hz, int bus_width,
			  int removable, uint32_t clksel_val)
{
	rkmcihost *ctrlr = xzalloc(sizeof(*ctrlr));
	ctrlr->mmc.ctrlr.ops.update = &rkmci_update;
	ctrlr->mmc.ctrlr.need_update = 1;
	ctrlr->mmc.voltages = 0x00ff8080;
	ctrlr->mmc.f_min = (MMC_BUS_CLOCK + 510 - 1)/510;
	ctrlr->mmc.f_max = MMC_BUS_CLOCK/2;
	ctrlr->mmc.bus_width = bus_width;
	ctrlr->mmc.bus_hz = ctrlr->mmc.f_min;
	ctrlr->mmc.b_max = 255;
	ctrlr->mmc.caps = MMC_MODE_8BIT|MMC_MODE_4BIT|
				MMC_MODE_HS|MMC_MODE_HS_52MHz;
	ctrlr->mmc.send_cmd = &rk_emmc_request;
	ctrlr->mmc.set_ios = &rk_set_ios;
	ctrlr->mmc.rca = 3;
	ctrlr->ioaddr = (void *)ioaddr;
	ctrlr->src_hz = src_hz;
	ctrlr->clksel_val = clksel_val;
	ctrlr->removable = removable;
	return ctrlr;
}












