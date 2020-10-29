//include ESP SDK headers
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <user_interface.h>
#include <espconn.h>
#include <ip_addr.h>
int ets_uart_printf(const char *fmt, ...);
//include custom driver headers
#include "driver/uart.h"
#include "driver/dht22.h"
#include "driver/gpio16.h"
//include lobaro CoAP headers
#include "lobaro-coap/coap.h"
#include "lobaro-coap/interface/esp8266/lobaro-coap_esp8266.h"

//internal ESP8266 Control Structures
esp_udp UdpCoAPConn;
struct espconn CoAP_conn = {.proto.udp = &UdpCoAPConn};

//"Main Loop" Speed
#define DELAY_LOOP 100 // milliseconds
LOCAL os_timer_t MainLoop_timer;

//DHT11 Sensor class and delay timer initialization
//#define DELAY_DHT 2000; //2 seconds delay for every sensor measurement
//LOCAL os_timer_t DHT11_timer;

extern uint8_t pin_num[GPIO_PIN_NUM];
DHT_Sensor sensor;

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
}

LOCAL void ICACHE_FLASH_ATTR mainLoopTimer_cb(void *arg)
{
	if(CoAP_ESP8266_States.TxSocketIdle)
		CoAP_doWork();
}

//setup resouces
void ICACHE_FLASH_ATTR init_done(void) {
	static uint8_t CoAP_WorkMemory[4096]; //Working memory of CoAPs internal memory allocator
	CoAP_Init(CoAP_WorkMemory, 4096);
	CoAP_ESP8266_CreateInterfaceSocket(0, &CoAP_conn, 5683, CoAP_onNewPacketHandler, CoAP_ESP8266_SendDatagram);

	Create_Wifi_IPs_Resource(); 		//example of simple GET resource
	Create_About_Resource();			//example of large resource (blockwise transfers)
	Create_Wifi_Config_Resource(); 		//example of uri-query usage
	Create_RTC_Resource(); 				//example of observable resource
	Create_Led_Resource(); 	//example of observable resource triggered by itself + uri-query usage
	Create_Wifi_Scan_Resource(); //example of longer lasting "postponed" resource with two responses (1. empty ACK, 2. actual resource)
	Create_Led_Switch_Resource(); //new led switch resource
	Create_DHT11_Temp_Resource(); //DHT11 Temperature resource
	Create_DHT11_Hum_Resource(); //DHT11 Humidity resource
	ets_uart_printf("- CoAP init done! Used CoAP ram memory:\r\n"); //note: static ram footprint depends primary on resource count+uri lengths
	coap_mem_determinateStaticMem();
	coap_mem_stats();

	// Set up a "main-loop" timer
	os_timer_disarm(&MainLoop_timer);
	os_timer_setfn(&MainLoop_timer, (os_timer_func_t *)mainLoopTimer_cb, (void *)0);
	os_timer_arm(&MainLoop_timer, DELAY_LOOP, 1);
}

//setup
void user_init(void) {
	// Configure the UART
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	ets_uart_printf("\r\n\r\nLobaro-CoAP on ESP8266 Demo!\r\nwww.lobaro.com\r\n\r\n");

	//Config and init DHT11 Sensor
	sensor.pin = 4;
	sensor.type = DHT11;
	ets_uart_printf("DHT11 connected and initialized on GPIO%d\r\n",pin_num[sensor.pin]);
	DHTInit(&sensor);

	//Config ESP8266 network
	CoAP_ESP8266_ConfigDevice();
	system_init_done_cb(init_done);

}
