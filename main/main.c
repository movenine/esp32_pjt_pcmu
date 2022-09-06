#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_dev.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>

#include "driver/gpio.h"

#include "define.h"
#include "protocol.h"
#include "fs.h"

static const char *TAG = "tcon_events";

// static uint8_t protocol_tx_buf[BUF_SIZE];
// static int protocol_tx_buf_len = 0;

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
// static int cnt = 0;

static QueueHandle_t gpio_evt_queue = NULL;
//static uint8_t tx_buf[128];
//static int tx_buf_len = 0;

// A Type 20~35, B Type 40~55, C Type 60~75, D Type 80~95, E Type 100~115
void init_dev(void)
{
    //20220726 leedg : file system �ʱ�ȭ �߰�
    init_fileconfig();
    //load board_id & relay status
    fs_read();
	//pcmu.board_id = 254;    //defualt value = 254
	gpio_set_level(CUR_SEN_RESET, 1);
    //SMPS_STATUS_0/1 
    gpio_set_level(STATUS_LED_0, 1);
    gpio_set_level(STATUS_LED_1, 1);
    
    if(pcmu.output_relay == 1) {
        gpio_set_level(RELAY_0, 1);
	    gpio_set_level(RELAY_1, 1);
        gpio_set_level(STATE_AC, 1);
    } else {
        gpio_set_level(STATE_AC, 0);
        ESP_LOGI(TAG, "relay status : '%x'", pcmu.output_relay);
    }
}

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
	{
        esp_wifi_connect();
    } 
	else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
	{
        if (s_retry_num < ESP_MAXIMUM_RETRY) 
		{
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } 
		else 
		{
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }

        ESP_LOGI(TAG,"connect to the AP fail");
    } 
	else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
	{
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
	esp_netif_ip_info_t ipInfo;

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_netif_t* wifista = esp_netif_create_default_wifi_sta();
	
    IP4_ADDR(&ipInfo.ip, 192, 168, 16, pcmu.board_id);
	IP4_ADDR(&ipInfo.gw, 192, 168, 16, 1);
	IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);

	esp_netif_dhcpc_stop(wifista);
	esp_netif_set_ip_info(wifista, &ipInfo);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL));

    wifi_config_t wifi_config = 
    {
        .sta = {
            .ssid = TCON_WIFI_SSID,
            .password = TCON_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if(bits & WIFI_CONNECTED_BIT) 
	{
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", TCON_WIFI_SSID, TCON_WIFI_PASS);
    } 
	else if(bits & WIFI_FAIL_BIT) 
	{
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", TCON_WIFI_SSID, TCON_WIFI_PASS);
    } 
	else 
	{
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

// static void tcp_client_task(void *pvParameters)
// {
// 	uint8_t tx_buf[128];
// 	int tx_buf_len;

//     uint8_t rx_buffer[128];
//     char addr_str[128];
//     int addr_family;
//     int ip_protocol;
    
//     char tmp_str[256];

// 	protocol_init();


//     while(1) 
// 	{
//         struct sockaddr_in dest_addr;

//         dest_addr.sin_addr.s_addr = inet_addr(TCON_HOST_IP_ADDR);
//         dest_addr.sin_family = AF_INET;
//         dest_addr.sin_port = htons(TCP_PORT);
//         addr_family = AF_INET;
//         ip_protocol = IPPROTO_IP;
//         inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

//         int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        
//         if(sock < 0) 
//         {
//             ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
//             break;
//         }
        
//         ESP_LOGI(TAG, "Socket created, connecting to %s:%d", TCON_HOST_IP_ADDR, TCP_PORT);

//         int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        
//         if(err != 0) 
//         {
//             ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
//             break;
//         }
        
//         ESP_LOGI(TAG, "Successfully connected");
		
// 		tx_buf_len = BC_connect(pcmu.board_id, tx_buf);
// 		err = send(sock, tx_buf, tx_buf_len, 0);

//         if(err < 0) 
//         {
//             ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
//             break;
//         }
            
//         while(1) 
//         {
//             int rx_buffer_len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            
//             if(rx_buffer_len < 0) 		// Error occurred during receiving
//             {
//                 ESP_LOGE(TAG, "recv failed: errno %d", errno);
//                 break;
//             }					// Data received
//             else 
//             {
//                 tmp_str[rx_buffer_len] = 0;
//                 ESP_LOGI(TAG, "Received %d bytes from %s:", rx_buffer_len, addr_str);
//                 sprintf(tmp_str, "[%2x][%2x][%2x][%2x][%2x][%2x][%2x]", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], rx_buffer[4], rx_buffer[5], rx_buffer[6]);
//                 ESP_LOGI(TAG, "%s", tmp_str);

// 				tx_buf_len = 0;
// 				tx_buf_len = BC_protocol_analysis_wifi(pcmu.board_id, tx_buf, rx_buffer, rx_buffer_len);
				
// 				if(tx_buf_len > 0)
// 				{
// 					err = send(sock, tx_buf, tx_buf_len, 0);
// 				}
//             }

// 			//gpio_set_level(ACT_LED, cnt % 2);
// 			//cnt++;
//             //vTaskDelay(5000 / portTICK_PERIOD_MS);
//         }

//         if(sock != -1) 
//         {
//             ESP_LOGE(TAG, "Shutting down socket and restarting...");
//             shutdown(sock, 0);
//             close(sock);
//         }
//     }
    
//     vTaskDelete(NULL);
// }

// Uart1 �������� ������?
static void uart1_rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "UART1_RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE+1);

	uint8_t sensing_cnt = 0;
	uint32_t sensing_sum = 0;
	uint32_t current_val = 0;

    while (1) 
	{
        const int rxBytes = uart_read_bytes(WCM_UART_NUM, data, 32, 100 / portTICK_RATE_MS);
        //const int rxBytes = uart_read_bytes(UART_NUM_0, data, 32, 100 / portTICK_RATE_MS);

        if(rxBytes > 0) 
		{
			sensing_cnt++;
            data[rxBytes] = 0;
			sensing_sum += (data[1] - 0x30) * 1000 + (data[2] - 0x30) * 100 + (data[3] - 0x30) * 10 + (data[4] - 0x30);
			
			if(sensing_cnt == 100)
			{
				//pcmu.input_current = sensing_sum / 100;
				//printf("current:%d\n", pcmu.input_current);
				current_val = sensing_sum / 100;
				xQueueOverwrite(xQueue, &current_val);
				printf("current:%d\n", current_val);

				sensing_cnt = 0;
				sensing_sum = 0;				
			}
			//printf("%s\n", data);
            //ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

//Uart2 Rs485 ������?
static void uart2_rx_task(void *arg)
{
	uint8_t tx_buf[128];
	int tx_buf_len;

	int rxBytes;

    static const char *RX_TASK_TAG = "UART2_RX_TASK";
	static const char *TX_BUF_TAG = "TX_BUF";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE+1);  //1Mbyte ��û��Ŷ ������ ���� �޸� �Ҵ�

    //stxflag �ʱ�ȭ - �������� ��Ŷ Index
	protocol_init();

    //byte ������ ��û��Ŷ�� �а� ����ü�� ����
    while(1) 
	{
        //����Ʈ ���� ����
        rxBytes = uart_read_bytes(EX_UART_NUM, data, BUF_SIZE, 1000 / portTICK_RATE_MS);

        if(rxBytes > 0) 
		{
            data[rxBytes] = 0;  //���̸�ŭ ������ ���� �ʱ�ȭ

			tx_buf_len = 0;
			tx_buf_len = BC_protocol_analysis_uart2(pcmu.board_id, tx_buf, data, rxBytes); //������ ���ۿ� ���� �־��ְ� �м�
			
			printf("tx_buf_len: %d\n", tx_buf_len);
			//ESP_LOG_BUFFER_HEXDUMP(TX_BUF_TAG, tx_buf, tx_buf_len, ESP_LOG_INFO);	

		    if(tx_buf_len > 0) 
			{
	 			uart_write_bytes(EX_UART_NUM, (const char*) tx_buf, tx_buf_len);
		    }

            //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

//Gpio ���ͷ�Ʈ ó�� �Լ� : 20220727 leedg
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_button_task(void* arg)
{
    uint32_t io_num;
    while(1) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, pcmu.output_relay);
            if(pcmu.output_relay) {
                gpio_set_level(RELAY_0, 0);
                gpio_set_level(RELAY_1, 0);
                gpio_set_level(STATE_AC, 0);
                pcmu.output_relay = 0;
            }
            else {
                gpio_set_level(RELAY_0, 1);
                gpio_set_level(RELAY_1, 1);
                gpio_set_level(STATE_AC, 1);
                pcmu.output_relay = 1;
            }
        }
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
    	ESP_ERROR_CHECK(nvs_flash_erase());
    	ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    uart_config_t uart_config = 
	{
        .baud_rate = 38400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE, 0, NULL, 0);
    uart_param_config(EX_UART_NUM, &uart_config);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    uart_set_pin(EX_UART_NUM, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_config_t uart_config1 = 
	{
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
	
    //uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE, 0, NULL, 0);
    if(uart_driver_install(WCM_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Driver installation failed");
    }
    //uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(WCM_UART_NUM, &uart_config1);
	esp_log_level_set(TAG, ESP_LOG_INFO);
    uart_set_pin(WCM_UART_NUM, WCM_TXD_PIN, WCM_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    gpio_config_t io_conf;	
	
	// ACT_LED, CURRENT SENSOR RESET, STATE_AC, STATE_PWR					
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;		//disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;				//set as output mode
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;		//bit mask of the pins that you want to set
    io_conf.pull_down_en = 0;						//disable pull-down mode
    io_conf.pull_up_en = 0;							//disable pull-up mode
    gpio_config(&io_conf);							//configure GPIO with the given settings
	// BUTTON						
    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;		//enable interrupt (high -> low)
    io_conf.mode = GPIO_MODE_INPUT;					//set as input mode
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;		//bit mask of the pins that you want to set
    io_conf.pull_down_en = 0;						//disable pull-down mode
    io_conf.pull_up_en = 0;							//disable pull-up mode
    gpio_config(&io_conf);							//configure GPIO with the given settings
    // RELAY
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;		//disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;				//set as output mode
    io_conf.pin_bit_mask = GPIO_RELAY_PIN_SEL;		//bit mask of the pins that you want to set
    io_conf.pull_down_en = 0;						//disable pull-down mode
    io_conf.pull_up_en = 0;							//disable pull-up mode
    gpio_config(&io_conf);							//configure GPIO with the given settings

	gpio_set_level(ACT_LED, 0);
	gpio_set_level(CUR_SEN_RESET, 1);

	gpio_set_level(RELAY_0, 0);
	gpio_set_level(RELAY_1, 0);

    //gpio14/15 intialized : 20220823 leedg
    gpio_set_level(STATE_PWR, 0);
    gpio_set_level(STATE_AC, 0);
    
    //gpio4 button event handler : 20220727 leedg
    gpio_set_intr_type(BUTTON_0, GPIO_INTR_NEGEDGE);    //interrupt nagative edge (high -> low)
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_button_task, "gpio_button_task", 2048, NULL, 10, NULL);
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(BUTTON_0, gpio_isr_handler, (void*) BUTTON_0);

    //remove isr handler for gpio number.
    gpio_isr_handler_remove(BUTTON_0);
    //hook isr handler for specific gpio pin again
    gpio_isr_handler_add(BUTTON_0, gpio_isr_handler, (void*) BUTTON_0);

	init_dev();

	xQueue = xQueueCreate(1, sizeof(input_current));

	xTaskCreate(uart1_rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
	xTaskCreate(uart2_rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);

    //ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    //wifi_init_sta();

	//xTaskCreate(tcp_client_task, "tcp_client", 8192, NULL, 5, NULL);

    int cnt = 0;

    while(1) 
	{
        if(cnt == 100) {
            cnt = 0;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
		
        gpio_set_level(ACT_LED, cnt % 2);
        gpio_set_level(STATE_PWR, cnt % 2);
		
/*

		if((cnt % 2) == 0)
		{
			gpio_set_level(RELAY_0, 1);
		}
		else
		{
			gpio_set_level(RELAY_0, 0);
		}
			
		if((cnt % 2) == 1)
		{
			gpio_set_level(RELAY_1, 1);
		}
		else
		{
			gpio_set_level(RELAY_1, 0);
		}
*/
        cnt++;
    }
}
