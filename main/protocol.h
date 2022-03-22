

// ASCII CODE
#define ASC_SOH                 0x01
#define ASC_STX                 0x02
#define ASC_ETX                 0x03
#define ASC_EOT                 0x04
#define ASC_ENQ                 0x05
#define ASC_ACK                 0x06
#define ASC_BS                  0x08
#define ASC_CR                  0x0d
#define ASC_LF                  0x0a
#define ASC_DLE                 0x10
#define ASC_XON                 0x11
#define ASC_XOFF                0x13
#define ASC_NAK                 0x15
#define ASC_CAN                 0x18
#define ASC_CTLZ                0x1a
#define ASC_ESC                 0x1b

//BC 프로토콜 command  선언  
#define BD_RESET				0x20
#define BD_TIME_SET				0x21
#define BD_FW_REQ				0x22
#define BD_FORMAT				0x23
#define BD_CONNECT				0x24
#define BD_DEBUG_ON				0x25
#define BD_DEBUG_OFF			0x26

#define BD_GPIO_WRITE			0x30
#define BD_IN_EEPROM_WRITE		0x31
#define BD_EX_EEPROM_WRITE		0x32

#define BD_GPIO_READ			0x40
#define BD_IN_EEPROM_READ		0x41
#define BD_EX_EEPROM_READ		0x42

#define BD_FILE_START_SEND		0x50
#define BD_FILE_DATA_SEND		0x51
#define BD_FILE_END_SEND		0x52
#define BD_FILE_LIST_REQ		0x53
#define BD_FILE_REMOVE_REQ		0x54
#define BD_FILE_REMOVE_OK		0x55
#define BD_FILE_SEND_REQ      	0x56

#define BD_FILE_LIST			0x5A

#define BD_TEMP_HUMI			0x60
#define BD_CDS					0x61
#define BD_PIR					0x62
#define BD_IMPACT				0x63
#define BD_DOOR					0x64
#define BD_RELAY				0x65
#define BD_LED_PORT				0x66
#define BD_CUR_SEN_RESET		0x67
#define BD_CURRENT				0x68

#define BD_TEMP_HUMI_EX			0x6A
#define BD_CDS_EX				0x6B
#define BD_PIR_EX				0x6C
#define BD_IMPACT_EX			0x6D
#define BD_DOOR_EX				0x6E
#define BD_RELAY_EX				0x6F

#define BD_RELAY_SET			0x70
#define BD_LED_PORT_SET			0x71

#define BD_RELAY_EX_SET			0x7A

#define BD_DATA_BYPASS_COM1		0x80
#define BD_DATA_BYPASS_COM2		0x81
#define BD_DATA_BYPASS_COM3		0x82
#define BD_DATA_BYPASS_COM4		0x83
#define BD_DATA_BYPASS_COM5		0x84
#define BD_DATA_BYPASS_COM6		0x85
#define BD_DATA_BYPASS_COM7		0x86
#define BD_DATA_BYPASS_COM8		0x87

#define BD_DATA_BYPASS_LED1		0x88
#define BD_DATA_BYPASS_LED2		0x89

#define BD_BIT_WDT				0xA0
#define BD_BIT_FAN				0xA1
#define BD_BIT_CFAN				0xA2
#define BD_BIT_HEATER			0xA3
#define BD_BIT_PC				0xA4
#define BD_BIT_DOOR				0xA5
#define BD_BIT_BUTTON			0xA6
#define BD_BIT_SSR				0xA7
#define BD_BIT_FAN_SET  		0xA8
#define BD_BIT_HEATER_SET		0xA9
#define BD_BIT_CDS_SET			0xAA
#define BD_BIT_IMPACT_SET		0xAB

#define BD_PW_PC				0xB0
#define BD_PW_SIGN				0xB1

#define BD_SCH_PW_SIGN			0xC0

#define BC_PACKET_SIZE	        512

typedef struct 
{
	unsigned char  bd_id;
    unsigned char  opcode;
    unsigned short  datalen;
    unsigned char data[BC_PACKET_SIZE];
	unsigned short  cnt;
	unsigned char  lrc;
	unsigned char  crc;
	unsigned char stxflg;
} protocol_bc_t;

static protocol_bc_t bc_packet_buf_wifi;
static protocol_bc_t bc_packet_buf_uart2;

void protocol_init(void);
int BC_connect(uint8_t bd_id, uint8_t *tx_buf);

int BC_protocol_answer_wifi(uint8_t bd_id, uint8_t opcode, uint8_t *data, int data_len, uint8_t *tx_buf);
int BC_protocol_processing_wifi(uint8_t bd_id, uint8_t *tx_buf);
int BC_protocol_analysis_wifi(uint8_t bd_id, uint8_t *tx_buf, uint8_t *rx_buf, int rx_buf_len);

int BC_protocol_answer_uart2(uint8_t bd_id, uint8_t opcode, uint8_t *data, int data_len, uint8_t *tx_buf);
int BC_protocol_processing_uart2(uint8_t bd_id, uint8_t *tx_buf);
int BC_protocol_analysis_uart2(uint8_t bd_id, uint8_t *tx_buf, uint8_t *rx_buf, int rx_buf_len);





