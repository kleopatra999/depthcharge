

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
static int uart_wrtie_byte(char byte)
{
    pUART_REG puartRegStart = (pUART_REG)UART2_BASE_ADDR; 
    puartRegStart->UART_RBR = byte;
    while(!(puartRegStart->UART_LSR & UART_LSR_TEMT));
    return (0);
}
 void _print_hex (int hex)
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



