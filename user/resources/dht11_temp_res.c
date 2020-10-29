#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"
#include "driver/dht22.h"
#include "driver/gpio16.h"
#include <stdlib.h>

#include "../lobaro-coap/coap.h"
#include "dht11_temp_res.h"

CoAP_Res_t* pDHT11_Temp_Res = NULL;

#define DHT11_OBSERVE_DELAY 1500 //milliseconds
#define BUZZER_DELAY 1000 //microsecs
LOCAL os_timer_t Notify_timer;
//LOCAL os_timer_t Buzzer_timer;
DHT_Sensor sensor;
DHT_Sensor_Data data;
uint8_t pin;

static void ICACHE_FLASH_ATTR Buzz(float TempRead)
{
	float Ref_Temp = 25.0;
	float Temp_Difference;
	int i;

	Temp_Difference = TempRead - Ref_Temp;

	if(abs(Temp_Difference) > 2)
	{
		for(i = 0; i <= 80; i++)
		{
		gpio_output_set(1 << 4, 0, 1 << 4, 0);
		os_delay_us(BUZZER_DELAY);
		gpio_output_set(0, 1 << 4,1 << 4,0);
		os_delay_us(BUZZER_DELAY);
		}
	}
}

LOCAL void notify_cb(void *arg)
{
	CoAP_NotifyResourceObservers(pDHT11_Temp_Res);
}

static void ICACHE_FLASH_ATTR SetDHT11TempPayload(CoAP_Message_t* pReq, CoAP_Message_t* pResp)
{
	if (DHTRead(&sensor, &data))
	{
		char myString[20];
		char buff[10];
		DHTFloat2String(buff,data.temperature);
		coap_sprintf(myString,"Temperature: %sC",buff);
		CoAP_SetPayload(pReq,pResp,myString,coap_strlen(myString),true);
	}
	else
	{
		char info[] = {"Failed to read temperature sensor on GPIO2\n"};
		CoAP_SetPayload(pReq,pResp,info,coap_strlen(info),true);
	}

}

static void ICACHE_FLASH_ATTR SetDHT11TempPayload_Notify(CoAP_Message_t* pReq, CoAP_Message_t* pResp)
{
	if (DHTRead(&sensor, &data))
	{
		char myString[20];
		char buff[10];
		DHTFloat2String(buff,data.temperature);
		Buzz(data.temperature);
		coap_sprintf(myString,"Temperature: %sC",buff);
		CoAP_SetPayload(pReq,pResp,myString,coap_strlen(myString),true);
	}
	else
	{
		char info[] = {"Failed to read temperature sensor on GPIO2\n"};
		CoAP_SetPayload(pReq,pResp,info,coap_strlen(info),true);
	}

}

static CoAP_HandlerResult_t Res_ReqHandler(CoAP_Message_t* pReq, CoAP_Message_t* pResp)
{
/////////////////GET Req Handler
	if(pReq->Code == REQ_GET)
	{
		SetDHT11TempPayload(pReq,pResp);
	}
	return HANDLER_OK;
}

static CoAP_HandlerResult_t ICACHE_FLASH_ATTR NotifyHandler(CoAP_Observer_t* pObserver, CoAP_Message_t* pResp)
{

	SetDHT11TempPayload_Notify(NULL,pResp);

	return HANDLER_OK;
}

CoAP_Res_t* ICACHE_FLASH_ATTR Create_DHT11_Temp_Resource()
{
	CoAP_ResOpts_t Options = {.Cf = COAP_CF_TEXT_PLAIN, .Flags = RES_OPT_GET };

	//set buzzer D2 as output pin for buzzer
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	gpio_output_set(0,0,1 << 4,0);
	///////////////////////////////////////////////
	//start update timer
	os_timer_disarm(&Notify_timer);
	os_timer_setfn(&Notify_timer,(os_timer_func_t*)notify_cb,(void *)0);
	os_timer_arm(&Notify_timer,DHT11_OBSERVE_DELAY,1);


	return (pDHT11_Temp_Res = CoAP_CreateResource("DHT11/Temperature", "Get the temperature from the DHT sensor", Options, Res_ReqHandler,NotifyHandler));
}
