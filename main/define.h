
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
	unsigned char  board_id;
	unsigned char  input_button;
	//unsigned int   input_current;
	unsigned char  output_relay;
} board_type_e_t;  // type e => pmcu

#define EX_UART_NUM 				UART_NUM_2      
#define BUF_SIZE 					(1024)
#define RD_BUF_SIZE 				(BUF_SIZE)

#define TCON_WIFI_SSID      		"CUDO_TCON"
#define TCON_WIFI_PASS      		"cudo0509"
#define TCP_PORT       				(6001)
#define ESP_MAXIMUM_RETRY   		(5)

#define WIFI_CONNECTED_BIT 			BIT0
#define WIFI_FAIL_BIT      			BIT1

#define TCON_HOST_IP_ADDR 			"192.168.16.1"

#define CUR_SEN_RESET    			27
#define ACT_LED    					23
#define STATUS_LED_0   				19
#define STATUS_LED_1   				18
#define GPIO_OUTPUT_PIN_SEL  		((1ULL << CUR_SEN_RESET) | (1ULL << ACT_LED) | (1ULL << STATUS_LED_0) | (1ULL << STATUS_LED_1))

#define BUTTON_0    				4
#define GPIO_INPUT_PIN_SEL  		(1ULL << BUTTON_0)

#define RELAY_0    					12
#define RELAY_1    					13
#define GPIO_RELAY_PIN_SEL  		((1ULL << RELAY_0) | (1ULL << RELAY_1))

#define CONFIG_RMT_TX_GPIO 			(25)
#define RMT_TX_CHANNEL 				RMT_CHANNEL_0
#define CONFIG_STRIP_LED_NUMBER		(16)

#define EXAMPLE_CHASE_SPEED_MS 		(1000)

static board_type_e_t pcmu;
xQueueHandle xQueue;
unsigned int   input_current;



