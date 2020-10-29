#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"
#include "driver/dht22.h"
#include "driver/gpio16.h"
#include "../lobaro-coap/coap.h"
#include "dht11_hum_res.h"

CoAP_Res_t*  pDHT11_Hum_Res = NULL;

#define DHT11_OBSERVE_DELAY 1500 //milliseconds
LOCAL os_timer_t Notify_timer;
DHT_Sensor sensor;
DHT_Sensor_Data data;
uint8_t pin;

LOCAL void notify_cb(void *arg)
{
	CoAP_NotifyResourceObservers(pDHT11_Hum_Res);
}

static void ICACHE_FLASH_ATTR SetDHT11HumPayload(CoAP_Message_t* pReq, CoAP_Message_t* pResp)
{
	if (DHTRead(&sensor, &data))
	{
		char myString[20];
		char buff[10];
		DHTFloat2String(buff,data.humidity);
		coap_sprintf(myString,"Humidity: %s%",buff);
		CoAP_SetPayload(pReq,pResp,myString,coap_strlen(myString),true);
	}
	else
	{
		char info[] = {"Failed to read humidity sensor on GPIO2\n"};
		CoAP_SetPayload(pReq,pResp,info,coap_strlen(info),true);
	}
}

static CoAP_HandlerResult_t Res_ReqHandler(CoAP_Message_t* pReq, CoAP_Message_t* pResp)
{
/////////////////PUT Req Handler
	if(pReq->Code == REQ_GET)
	{
		SetDHT11HumPayload(pReq,pResp);
	}
	return HANDLER_OK;
}

static CoAP_HandlerResult_t ICACHE_FLASH_ATTR NotifyHandler(CoAP_Observer_t* pObserver, CoAP_Message_t* pResp)
{
	SetDHT11HumPayload(NULL,pResp);

	return HANDLER_OK;
}



CoAP_Res_t* ICACHE_FLASH_ATTR Create_DHT11_Hum_Resource()
{
	CoAP_ResOpts_t Options = {.Cf = COAP_CF_TEXT_PLAIN, .Flags = RES_OPT_GET };

	//start update timer
	os_timer_disarm(&Notify_timer);
	os_timer_setfn(&Notify_timer,(os_timer_func_t*)notify_cb,(void *)0);
	os_timer_arm(&Notify_timer,DHT11_OBSERVE_DELAY,1);

	return (pDHT11_Hum_Res = CoAP_CreateResource("DHT11/Humidity", "Get the humidity from the DHT sensor", Options, Res_ReqHandler, NotifyHandler));
}
