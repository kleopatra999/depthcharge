/*
 * Copyright 2012 Google Inc.
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

#include "arch/sign_of_life.h"
#include "base/init_funcs.h"
#include "base/timestamp.h"
#include "config.h"
#include "drivers/input/input.h"
#include "vboot/stages.h"
#include "vboot/util/commonparams.h"


#if 0
///UART_IER
#define   THRE_INT_ENABLE                   (1<<7)
#define   THRE_INT_DISABLE                   (0)
#define   ENABLE_MODEM_STATUS_INT           (1<<3)
#define   DISABLE_MODEM_STATUS_INT           (0)
#define   ENABLE_RECEIVER_LINE_STATUS_INT   (1<<2)
#define   DISABLE_RECEIVER_LINE_STATUS_INT   (0)
#define   ENABLE_TRANSMIT_HOLDING_EM_INT    (1<<1) ///Enable Transmit Holding Register Empty Interrupt.
#define   DISABLE_TRANSMIT_HOLDING_EM_INT    (0)
#define   ENABLE_RECEIVER_DATA_INT           (1)   ////Enable Received Data Available Interrupt.
#define   DISABLE_RECEIVER_DATA_INT          (0)

///UART_IIR
#define   IR_MODEM_STATUS                    (0)
#define   NO_INT_PENDING                     (1)
#define   THR_EMPTY                          (2)
#define   RECEIVER_DATA_AVAILABLE            (0x04)
#define   RECEIVER_LINE_AVAILABLE            (0x06)
#define   BUSY_DETECT                        (0x07)
#define   CHARACTER_TIMEOUT                  (0x0c)

///UART_LCR
#define  LCR_DLA_EN                         (1<<7)
#define  BREAK_CONTROL_BIT                  (1<<6)
#define  PARITY_DISABLED                     (0)
#define  PARITY_ENABLED                     (1<<3)
#define  ONE_STOP_BIT                        (0)
#define  ONE_HALF_OR_TWO_BIT                (1<<2)
#define  LCR_WLS_5                           (0x00)
#define  LCR_WLS_6                           (0x01)
#define  LCR_WLS_7                           (0x02)
#define  LCR_WLS_8                           (0x03)
#define  UART_DATABIT_MASK                   (0x03)


///UART_MCR
#define  IRDA_SIR_DISABLED                   (0)
#define  IRDA_SIR_ENSABLED                  (1<<6)
#define  AUTO_FLOW_DISABLED                  (0)
#define  AUTO_FLOW_ENSABLED                 (1<<5)

///UART_LSR
#define  THRE_BIT_EN                        (1<<5)

///UART_USR
#define  UART_RECEIVE_FIFO_EMPTY             (0)
#define  UART_RECEIVE_FIFO_NOT_EMPTY         (1<<3)
#define  UART_TRANSMIT_FIFO_FULL             (0)
#define  UART_TRANSMIT_FIFO_NOT_FULL         (1<<1)

///UART_SFE
#define  SHADOW_FIFI_ENABLED                 (1)
#define  SHADOW_FIFI_DISABLED                (0)

///UART_SRT
#define  RCVR_TRIGGER_ONE                    (0)
#define  RCVR_TRIGGER_QUARTER_FIFO           (1)
#define  RCVR_TRIGGER_HALF_FIFO              (2)
#define  RCVR_TRIGGER_TWO_LESS_FIFO          (3)

//UART_STET
#define  TX_TRIGGER_EMPTY                    (0)
#define  TX_TRIGGER_TWO_IN_FIFO              (1)
#define  TX_TRIGGER_ONE_FOUR_FIFO            (2)
#define  TX_TRIGGER_HALF_FIFO                (3)

///UART_SRR
#define  UART_RESET                          (1)
#define  RCVR_FIFO_REST                     (1<<1)
#define  XMIT_FIFO_RESET                    (1<<2)





//UART Registers
typedef volatile struct tagUART_STRUCT
{
    int UART_RBR;
    int UART_DLH;
    int UART_IIR;
    int UART_LCR;
    int UART_MCR;
    int UART_LSR;
    int UART_MSR;
    int UART_SCR;
    int RESERVED1[(0x30-0x20)/4];
    int UART_SRBR[(0x70-0x30)/4];
    int UART_FAR;
    int UART_TFR;
    int UART_RFW;
    int UART_USR;
    int UART_TFL;
    int UART_RFL;
    int UART_SRR;
    int UART_SRTS;
    int UART_SBCR;
    int UART_SDMAM;
    int UART_SFE;
    int UART_SRT;
    int UART_STET;
    int UART_HTX;
    int UART_DMASA;
    int RESERVED2[(0xf4-0xac)/4];
    int UART_CPR;
    int UART_UCV;
    int UART_CTR;
} UART_REG, *pUART_REG;

#define UART_THR UART_RBR
#define UART_DLL UART_RBR
#define UART_IER UART_DLH
#define UART_FCR UART_IIR
//#define UART_STHR[(0x6c-0x30)/4]  UART_SRBR[(0x6c-0x30)/4]




#define  UART_LSR_TEMT                0x40 /* Transmitter empty */

#define UART2_BASE_ADDR 0xFF690000
#if 0
static int uart_init()
{
    pUART_REG puartRegStart = (pUART_REG)UART2_BASE_ADDR; 
     //BaudRate
    puartRegStart->UART_LCR = 0x83;
    puartRegStart->UART_RBR = 0xD;   //²¨ÌØÂÊ:115200
    puartRegStart->UART_LCR = 0x03;
	return 0;
}
#endif
static int uart_wrtie_byte(char byte)
{
    pUART_REG puartRegStart = (pUART_REG)UART2_BASE_ADDR; 
    puartRegStart->UART_RBR = byte;
    while(!(puartRegStart->UART_LSR & UART_LSR_TEMT));
    return (0);
}
#if 0
static void print(char *s)
{
	while (*s) 
	{
		if (*s == '\n')
		{
		    uart_wrtie_byte('\r');
		}
	    uart_wrtie_byte(*s);
	    s++;
	}
}
#endif
static void _print_hex (int hex)
{
    int i = 8;
	uart_wrtie_byte('0');
	uart_wrtie_byte('x');
	while (i--) {
		unsigned char c = (hex & 0xF0000000) >> 28;
		uart_wrtie_byte(c < 0xa ? c + '0' : c - 0xa + 'a');
		hex <<= 4;
	}
}
#endif

int main(void)
{
	// Let the world know we're alive.
	sign_of_life(0xaa);
	_print_hex(0x01);

	// Initialize some consoles.
	serial_console_init();
	_print_hex(0x02);
	cbmem_console_init();
	_print_hex(0x03);
	input_init();

	printf("\n\nStarting read-only depthcharge on " CONFIG_BOARD "...\n");

	// Set up time keeping.
	timestamp_init();

	// Run any generic initialization functions that are compiled in.
	if (run_init_funcs())
		halt();
	printf("out run init\n");
	timestamp_add_now(TS_RO_PARAMS_INIT);

	// Set up the common param structure, clearing shared data.
	if (common_params_init(1))
		halt();
	printf("out common_params_initn");

	timestamp_add_now(TS_RO_VB_INIT);

	// Initialize vboot.
	if (vboot_init())
		halt();

	timestamp_add_now(TS_RO_VB_SELECT_FIRMWARE);

	// Select firmware.
	if (vboot_select_firmware())
		halt();

	timestamp_add_now(TS_RO_VB_SELECT_AND_LOAD_KERNEL);

	// Select a kernel and boot it.
	if (vboot_select_and_load_kernel())
		halt();

	// We should never get here.
	printf("Got to the end!\n");
	halt();
	return 0;
}
