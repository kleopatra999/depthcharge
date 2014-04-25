
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

typedef volatile struct tagUART_STRUCT
{
    u32 UART_RBR;
    u32 UART_DLH;
    u32 UART_IIR;
    u32 UART_LCR;
    u32 UART_MCR;
    u32 UART_LSR;
    u32 UART_MSR;
    u32 UART_SCR;
    u32 RESERVED1[(0x30-0x20)/4];
    u32 UART_SRBR[(0x70-0x30)/4];
    u32 UART_FAR;
    u32 UART_TFR;
    u32 UART_RFW;
    u32 UART_USR;
    u32 UART_TFL;
    u32 UART_RFL;
    u32 UART_SRR;
    u32 UART_SRTS;
    u32 UART_SBCR;
    u32 UART_SDMAM;
    u32 UART_SFE;
    u32 UART_SRT;
    u32 UART_STET;
    u32 UART_HTX;
    u32 UART_DMASA;
    u32 RESERVED2[(0xf4-0xac)/4];
    u32 UART_CPR;
    u32 UART_UCV;
    u32 UART_CTR;
} UART_REG, *pUART_REG;

#define  UART_RECEIVE_FIFO_EMPTY             (0)
#define  UART_RECEIVE_FIFO_NOT_EMPTY         (1<<3)
#define  UART_TRANSMIT_FIFO_FULL             (0)
#define  UART_TRANSMIT_FIFO_NOT_FULL         (1<<1)

#define UART_THR UART_RBR
#define UART_DLL UART_RBR
#define UART_IER UART_DLH
#define UART_FCR UART_IIR


static pUART_REG pUartReg;

void serial_putchar(unsigned int c)
{
	u32 uartTimeOut;
	pUART_REG puartRegStart;
	if(!pUartReg) return;
	puartRegStart = (pUART_REG)pUartReg;
	uartTimeOut = 0xFFFF;
	while((puartRegStart->UART_USR & UART_TRANSMIT_FIFO_NOT_FULL) != UART_TRANSMIT_FIFO_NOT_FULL)
	{
		if(uartTimeOut == 0)
		{
			return ;
		}
		uartTimeOut--;
	}
	puartRegStart->UART_THR = c;
}


int serial_havechar(void)
{
	pUART_REG puartRegStart = (pUART_REG)pUartReg;
	return (puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY);
}

int serial_getchar(void)
{
	pUART_REG puartRegStart = (pUART_REG)pUartReg;
	char pdata;
 	while (!serial_havechar())
	{;}
	pdata = (char )puartRegStart->UART_RBR;
    	return (pdata);
}
static struct console_output_driver rk_serial_output =
{
	.putchar = &serial_putchar
};

static struct console_input_driver rk_serial_input =
{
	.havekey = &serial_havechar,
	.getchar = &serial_getchar
};

void serial_init(void)
{
	if (!lib_sysinfo.serial || !lib_sysinfo.serial->baseaddr)
		return;

	pUartReg = (pUART_REG )lib_sysinfo.serial->baseaddr;
}

void serial_console_init(void)
{
	serial_init();

	if (pUartReg) {
		console_add_output_driver(&rk_serial_output);
		console_add_input_driver(&rk_serial_input);
	}
}









