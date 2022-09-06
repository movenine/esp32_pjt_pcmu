
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "define.h"
#include "protocol.h"
#include "fs.h"

// static uint8_t tmp_data_buf[128];

void protocol_init(void)
{
	bc_packet_buf_wifi.stxflg = 0;
	bc_packet_buf_uart2.stxflg = 0;
}

// pcmu 상태값 응답함수
void BC_protocol_reset_ack(uint8_t bd_id)
{
	int tx_buf_len = 0;
	uint8_t tx_buf[128];
	uint8_t pk_data_buf[128];
		
	//flag clear
	protocol_status_clear();
	//reset 응답패킷
	pk_data_buf[0] = BD_RESET_EX;
	tx_buf_len = BC_protocol_answer_uart2(bd_id, ASC_ACK, pk_data_buf, 1, tx_buf);
	
	if(tx_buf_len > 0) 
	{
		uart_write_bytes(EX_UART_NUM, (const char*) tx_buf, tx_buf_len);
	}
}

int BC_connect(uint8_t bd_id, uint8_t *tx_buf)
{
	int i, data_len, tx_buf_len = 0;

	data_len = 0;

	tx_buf[0] = ASC_STX;
	tx_buf[1] = bd_id;
	tx_buf[2] = BD_CONNECT;
	tx_buf[3] = (uint8_t)(data_len >> 8);
	tx_buf[4] = (uint8_t)(data_len & 0xff);

	for (i = 1; i < (data_len + 5); i++)
	{
		tx_buf[5 + data_len] = tx_buf[5 + data_len] ^ tx_buf[i];
	}

	tx_buf[6 + data_len] = ASC_ETX;
	tx_buf_len = data_len + 7;

	return tx_buf_len;
}

int BC_protocol_answer_wifi(uint8_t bd_id, uint8_t opcode, uint8_t *data, int data_len, uint8_t *tx_buf)
{
	int i, tx_buf_len = 0;

	tx_buf[0] = ASC_STX;
	tx_buf[1] = bd_id;
	tx_buf[2] = opcode;
	tx_buf[3] = (uint8_t)(data_len >> 8);
	tx_buf[4] = (uint8_t)(data_len & 0xff);

	for (i = 0; i < data_len; i++)
	{
		tx_buf[5 + i] = *data++;
	}

	tx_buf[5 + data_len] = 0;

	for (i = 1; i < (data_len + 5); i++)
	{
		tx_buf[5 + data_len] = tx_buf[5 + data_len] ^ tx_buf[i];
	}

	tx_buf[6 + data_len] = ASC_ETX;

	tx_buf_len = data_len + 7;

	return tx_buf_len;
}

int BC_protocol_processing_wifi(uint8_t bd_id, uint8_t *tx_buf)
{
	int i, tx_buf_len = 0;
	uint8_t tmp_data_buf[128];

	switch (bc_packet_buf_wifi.opcode)
	{
	case ASC_ACK:
		printf("ASC_ACK\r\n");

		break;
	case BD_CONNECT:
		printf("BD_CONNECT\r\n");

		break;
	case BD_RELAY_SET:
		printf("BD_RELAY_SET\r\n");
		tmp_data_buf[0] = BD_RELAY_SET;
		tx_buf_len = BC_protocol_answer_wifi(bd_id, ASC_ACK, tmp_data_buf, 1, tx_buf);

		gpio_set_level(RELAY_0, 1);
		break;
	default:
		printf("<ERROR> OPCODE[0x%2x]\r\n", bc_packet_buf_wifi.opcode);
	}

	return tx_buf_len;
}

int BC_protocol_analysis_wifi(uint8_t bd_id, uint8_t *tx_buf, uint8_t *rx_buf, int rx_buf_len)
{
	int i, tx_buf_len = 0;

	for (i = 0; i < rx_buf_len; i++)
	{
		if ((bc_packet_buf_wifi.stxflg == 0) && (rx_buf[i] == ASC_STX))
		{
			bc_packet_buf_wifi.stxflg = 1;
			bc_packet_buf_wifi.opcode = 0;
			bc_packet_buf_wifi.datalen = 0;
			bc_packet_buf_wifi.cnt = 0;
			bc_packet_buf_wifi.lrc = 0;
		}
		else if (bc_packet_buf_wifi.stxflg == 1)
		{
			bc_packet_buf_wifi.stxflg = 2;
			bc_packet_buf_wifi.lrc = rx_buf[i];
			bc_packet_buf_wifi.bd_id = rx_buf[i];
		}
		else if (bc_packet_buf_wifi.stxflg == 2)
		{
			bc_packet_buf_wifi.stxflg = 3;
			bc_packet_buf_wifi.lrc ^= rx_buf[i];
			bc_packet_buf_wifi.opcode = rx_buf[i];
		}
		else if (bc_packet_buf_wifi.stxflg == 3)
		{
			bc_packet_buf_wifi.stxflg = 4;
			bc_packet_buf_wifi.lrc ^= rx_buf[i];
			bc_packet_buf_wifi.datalen = rx_buf[i] * 256;
		}
		else if (bc_packet_buf_wifi.stxflg == 4)
		{
			bc_packet_buf_wifi.stxflg = 5;
			bc_packet_buf_wifi.lrc ^= rx_buf[i];
			bc_packet_buf_wifi.datalen = bc_packet_buf_wifi.datalen + rx_buf[i];
		}
		else if (bc_packet_buf_wifi.stxflg == 5)
		{
			if (bc_packet_buf_wifi.cnt < bc_packet_buf_wifi.datalen)
			{
				bc_packet_buf_wifi.data[bc_packet_buf_wifi.cnt] = rx_buf[i];
				bc_packet_buf_wifi.lrc ^= rx_buf[i];
				bc_packet_buf_wifi.cnt++;
			}
			else
			{
				bc_packet_buf_wifi.stxflg = 6;
				bc_packet_buf_wifi.crc = rx_buf[i];
			}
		}
		else if ((bc_packet_buf_wifi.stxflg == 6) && (rx_buf[i] == ASC_ETX))
		{
			if (bc_packet_buf_wifi.crc == bc_packet_buf_wifi.lrc)
			{
				bc_packet_buf_wifi.stxflg = 7;

				if (bc_packet_buf_wifi.bd_id == bd_id)
				{
					tx_buf_len = BC_protocol_processing_wifi(bd_id, tx_buf);
				}

				bc_packet_buf_wifi.stxflg = 0;
			}
			else
			{
				bc_packet_buf_wifi.stxflg = 0;
			}
		}
		else
		{
			bc_packet_buf_wifi.stxflg = 0;
		}
	}

	return tx_buf_len;
}

// uart2 RS485 Part

//응답패킷 생성 함수
int BC_protocol_answer_uart2(uint8_t bd_id, uint8_t opcode, uint8_t *data, int data_len, uint8_t *tx_buf)
{
	int i, tx_buf_len = 0;

	tx_buf[0] = ASC_STX;
	tx_buf[1] = bd_id;
	tx_buf[2] = opcode;
	tx_buf[3] = (uint8_t)(data_len >> 8);
	tx_buf[4] = (uint8_t)(data_len & 0xff);

	// tx_buf[5]
	for (i = 0; i < data_len; i++)
	{
		tx_buf[5 + i] = *data++;
	}

	// tx_buf[6]
	tx_buf[5 + data_len] = 0;

	for (i = 1; i < (data_len + 5); i++)
	{
		tx_buf[5 + data_len] = tx_buf[5 + data_len] ^ tx_buf[i];
	}

	tx_buf[6 + data_len] = ASC_ETX;

	tx_buf_len = data_len + 7;

	return tx_buf_len;
}

// TC-1000 요청패킷의 OP CODE에 따른 Action
int BC_protocol_processing_uart2(uint8_t bd_id, uint8_t *tx_buf)
{
	//int i;
	int tx_buf_len = 0;
	uint8_t tmp_data_buf[128];
	uint32_t current_data;

	switch (bc_packet_buf_uart2.opcode)
	{
	case ASC_ACK:
		printf("ASC_ACK\r\n");

		break;
	case BD_CONNECT:
		printf("BD_CONNECT\r\n");

		break;
	case BD_RELAY_SET:
		printf("BD_RELAY_SET[0x%2x]\r\n", bc_packet_buf_uart2.data[0]);
		tmp_data_buf[0] = BD_RELAY_SET;
		tx_buf_len = BC_protocol_answer_uart2(bd_id, ASC_ACK, tmp_data_buf, 1, tx_buf);

		//릴레이 Gpio 포트 high or low 제어
		gpio_set_level(RELAY_0, (bc_packet_buf_uart2.data[0] & 0x01));
		gpio_set_level(RELAY_1, ((bc_packet_buf_uart2.data[0] >> 1) & 0x01));

		//릴레이 포트제어 file 복사 : 20220727 leedg
		if(bc_packet_buf_uart2.data[0] == 0xFF) {
			pcmu.output_relay = 1;
			gpio_set_level(STATE_AC, 1);
		} else if(bc_packet_buf_uart2.data[0] == 0x00) {
			pcmu.output_relay = 0;
			gpio_set_level(STATE_AC, 0);
		}
		fs_write(pcmu.board_id, pcmu.output_relay);
		break;
	case BD_TEMP_HUMI:
		printf("BD_TEMP_HUMI\r\n");
		tmp_data_buf[0] = BD_TEMP_HUMI;
		tx_buf_len = BC_protocol_answer_uart2(bd_id, ASC_ACK, tmp_data_buf, 1, tx_buf);

		break;
	case BD_CUR_SEN_RESET:
		printf("BD_CUR_SEN_RESET[0x%2x]\r\n", bc_packet_buf_uart2.data[0]);
		tmp_data_buf[0] = BD_CUR_SEN_RESET;
		tx_buf_len = BC_protocol_answer_uart2(bd_id, ASC_ACK, tmp_data_buf, 1, tx_buf);

		gpio_set_level(CUR_SEN_RESET, (bc_packet_buf_uart2.data[0] & 0x01));

		break;
	case BD_CURRENT:
		xQueuePeek(xQueue, &current_data, 0);
		printf("#current:%d\n", current_data);

		tmp_data_buf[0] = BD_CURRENT;
		tmp_data_buf[1] = (uint8_t)(current_data / 1000);
		tmp_data_buf[2] = (uint8_t)((current_data / 100) % 10);
		tmp_data_buf[3] = (uint8_t)((current_data / 10) % 10);
		tmp_data_buf[4] = (uint8_t)((current_data) % 10);

		tx_buf_len = BC_protocol_answer_uart2(bd_id, ASC_ACK, tmp_data_buf, 5, tx_buf);

		break;
	case BD_PCMU_STATE:	//pcmu 릴레이 상태
		printf("#status:0x%d\n", pcmu.output_relay);
		//파일에서 정보를 가져옴
		fs_read();
		tmp_data_buf[0] = BD_PCMU_STATE;
		tmp_data_buf[1] = 0x00;
		tmp_data_buf[2] = 0x00;
		tmp_data_buf[3] = 0x00;
		if(pcmu.output_relay == 1) {
			tmp_data_buf[4] = 0xFF;
		} else { 
			tmp_data_buf[4] = 0x00; 
		}
		tx_buf_len = BC_protocol_answer_uart2(bd_id, ASC_ACK, tmp_data_buf, 5, tx_buf);

		break;
	case BD_PCMU_WRITE_ID:
		printf("BD_PCMU_WRITE_ID[0x%2x]\r\n", bc_packet_buf_uart2.data[0]);
		//파일에서 정보를 가져옴
		char id = (uint8_t)bc_packet_buf_uart2.data[0];
		fs_write(id, pcmu.output_relay);

		tmp_data_buf[0] = BD_PCMU_WRITE_ID;
		tmp_data_buf[1] = 0x00;
		tmp_data_buf[2] = 0x00;
		tmp_data_buf[3] = 0x00;
		tmp_data_buf[4] = id;
		
		tx_buf_len = BC_protocol_answer_uart2(bd_id, ASC_ACK, tmp_data_buf, 5, tx_buf);

		break;
	// case BD_RESET_EX:
	// 	printf("Board reseting...\n");
	// 	//set resetFlag is 1... when start app_main, be clear
	// 	pcmu_status.sw_reset_flg = 1;
	// 	//software resetting
	// 	esp_restart();
	// 	break;
	default:
		printf("<ERROR> OPCODE[0x%2x]\r\n", bc_packet_buf_uart2.opcode);
	}

	return tx_buf_len;
}

// TC-1000 요청패킷을 Parsing하여 tx_buf에 적재
//  rx_buf : request packet
//  tx_buf : parsing datas
int BC_protocol_analysis_uart2(uint8_t bd_id, uint8_t *tx_buf, uint8_t *rx_buf, int rx_buf_len)
{
	int i, tx_buf_len = 0;

	for (i = 0; i < rx_buf_len; i++)
	{
		if ((bc_packet_buf_uart2.stxflg == 0) && (rx_buf[i] == ASC_STX))
		{
			bc_packet_buf_uart2.stxflg = 1;
			bc_packet_buf_uart2.opcode = 0;
			bc_packet_buf_uart2.datalen = 0;
			bc_packet_buf_uart2.cnt = 0;
			bc_packet_buf_uart2.lrc = 0;
		}
		else if (bc_packet_buf_uart2.stxflg == 1)
		{
			bc_packet_buf_uart2.stxflg = 2;
			bc_packet_buf_uart2.lrc = rx_buf[i];
			bc_packet_buf_uart2.bd_id = rx_buf[i];
		}
		else if (bc_packet_buf_uart2.stxflg == 2)
		{
			bc_packet_buf_uart2.stxflg = 3;
			bc_packet_buf_uart2.lrc ^= rx_buf[i];
			bc_packet_buf_uart2.opcode = rx_buf[i];
		}
		else if (bc_packet_buf_uart2.stxflg == 3)
		{
			bc_packet_buf_uart2.stxflg = 4;
			bc_packet_buf_uart2.lrc ^= rx_buf[i];
			bc_packet_buf_uart2.datalen = rx_buf[i] * 256;
		}
		else if (bc_packet_buf_uart2.stxflg == 4)
		{
			bc_packet_buf_uart2.stxflg = 5;
			bc_packet_buf_uart2.lrc ^= rx_buf[i];
			bc_packet_buf_uart2.datalen = bc_packet_buf_uart2.datalen + rx_buf[i];
		}
		else if (bc_packet_buf_uart2.stxflg == 5)
		{
			if (bc_packet_buf_uart2.cnt < bc_packet_buf_uart2.datalen)
			{
				bc_packet_buf_uart2.data[bc_packet_buf_uart2.cnt] = rx_buf[i];
				bc_packet_buf_uart2.lrc ^= rx_buf[i];
				bc_packet_buf_uart2.cnt++;
			}
			else
			{
				bc_packet_buf_uart2.stxflg = 6;
				bc_packet_buf_uart2.crc = rx_buf[i];
			}
		}
		else if ((bc_packet_buf_uart2.stxflg == 6) && (rx_buf[i] == ASC_ETX))
		{
			if (bc_packet_buf_uart2.crc == bc_packet_buf_uart2.lrc)
			{
				bc_packet_buf_uart2.stxflg = 7;

				if ((bc_packet_buf_uart2.bd_id == bd_id) || (bc_packet_buf_uart2.bd_id == 0xff))
				{
					tx_buf_len = BC_protocol_processing_uart2(bd_id, tx_buf);
				}

				bc_packet_buf_uart2.stxflg = 0;
			}
			else
			{
				bc_packet_buf_uart2.stxflg = 0;
			}
		}
		else
		{
			bc_packet_buf_uart2.stxflg = 0;
		}
	}

	return tx_buf_len;
}
