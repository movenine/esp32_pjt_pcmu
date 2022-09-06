
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/rmt.h"

typedef struct
{
	unsigned char	board_id;
	unsigned char	input_button;
	//unsigned int   input_current;
	unsigned char	board_state;
	int	output_relay;
} board_type_e_t;  // type e => pmcu

#define EX_UART_NUM 				UART_NUM_2
#define WCM_UART_NUM				UART_NUM_1
#define WCM_TXD_PIN					21
#define WCM_RXD_PIN					5
#define BUF_SIZE 					(1024)
#define RD_BUF_SIZE 				(BUF_SIZE)

#define TCON_WIFI_SSID      		"CUDO_TCON"
#define TCON_WIFI_PASS      		"pass0001"
#define TCP_PORT       				(6001)
#define ESP_MAXIMUM_RETRY   		(5)

#define WIFI_CONNECTED_BIT 			BIT0
#define WIFI_FAIL_BIT      			BIT1

#define TCON_HOST_IP_ADDR 			"192.168.16.1"

#define CUR_SEN_RESET    			27
#define ACT_LED    					23
#define STATUS_LED_0   				19
#define STATUS_LED_1   				18
// Edited by leedg : 20220823
#define STATE_PWR					14
#define STATE_AC					15
#define GPIO_OUTPUT_PIN_SEL  		((1ULL << CUR_SEN_RESET) | (1ULL << ACT_LED) | (1ULL << STATUS_LED_0) | (1ULL << STATUS_LED_1) | (1ULL << STATE_PWR) | (1ULL << STATE_AC))

#define BUTTON_0    				4
#define GPIO_INPUT_PIN_SEL  		(1ULL << BUTTON_0)
#define ESP_INTR_FLAG_DEFAULT 0		//gpio interrupt flag : 20220727 leedg

#define RELAY_0    					12
#define RELAY_1    					13
#define GPIO_RELAY_PIN_SEL  		((1ULL << RELAY_0) | (1ULL << RELAY_1))

#define CONFIG_RMT_TX_GPIO 			(25)
#define RMT_TX_CHANNEL 				RMT_CHANNEL_0
#define CONFIG_STRIP_LED_NUMBER		(16)

#define EXAMPLE_CHASE_SPEED_MS 		(500)

board_type_e_t pcmu;

xQueueHandle xQueue;
unsigned int   input_current;



