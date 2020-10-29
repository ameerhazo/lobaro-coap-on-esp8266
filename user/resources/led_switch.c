#include <os_type.h>
#include <ets_sys.h>
#include <osapi.h>
//#include <gpio.h>
//#include <eagle_soc.h>

#include "../lobaro-coap/coap.h"
#include "led_switch.h"

// see eagle_soc.h for these definitions
//#define LED_GPIO 5
//#define LED_GPIO_MUX PERIPHS_IO_MUX_GPIO5_U
//#define LED_GPIO_FUNC FUNC_GPIO5

CoAP_Res_t* pLed_Switch_Res = NULL;

bool LedOnOff = true;

static void ICACHE_FLASH_ATTR led(bool onOff)
{
	if(onOff)
	{
		gpio_output_set(1 << 5, 0, 1 << 5, 0);
	}
	else
	{
		gpio_output_set(0, 1 << 5, 1 << 5, 0);
	}
}

static void ICACHE_FLASH_ATTR SetLedstatePayload(CoAP_Message_t* pReq, CoAP_Message_t* pResp)
{
	if(LedOnOff)
	{
		CoAP_SetPayload(pReq, pResp, "Led is on!", coap_strlen("Led is on!"), true);
	}
	else
	{
		 CoAP_SetPayload(pReq, pResp, "Led is off!", coap_strlen("Led is off!"), true);
	}
}

static CoAP_HandlerResult_t Res_ReqHandler(CoAP_Message_t* pReq, CoAP_Message_t* pResp)
{
////////////////////////////////////////Request POST Handler
//	if(pReq->Code == REQ_POST)
//		{
//			CoAP_option_t* pOpt;
//			bool Found = false;
//
//			for(pOpt =pReq->pOptionsList ; pOpt != NULL; pOpt = pOpt->next)
//			{
//				switch(CoAP_FindUriQueryVal(pOpt,"",3, "on","off", "tgl"))
//				{ //no prefix used -> use /led_gpio12?on or /led_gpio12?off
//					case 0: break; //not found
//					case 1: led(true); Found=true; break; //found "on"
//					case 2: led(false); Found=true; break; //found "off"
//					case 3: led(!LedOnOff); Found=true; break; //found "tgl"
//				}
//				if(Found)
//				{
//					SetLedstatePayload(pReq, pResp);
//					break;
//				}
//			}
//
//			if(!Found)
//			{
//				char info[] = {"usage: coap://.../led_gpio12?on (or \"off\", \"tgl\")"};
//				CoAP_SetPayload(pReq, pResp, info, coap_strlen(info), true);
//				pResp->Code=RESP_ERROR_BAD_REQUEST_4_00;
//			}
//
//		}
////////////////////////////////////////Request PUT Handler
	if(pReq->Code == REQ_PUT)
	{
		CoAP_option_t* pOpt;
		bool PutQueryFound = false;

		for(pOpt = pReq->pOptionsList ; pOpt != NULL ; pOpt = pOpt->next)
		{
			if(CoAP_FindUriQueryVal(pOpt,"",2,"on"))
			{
				if(LedOnOff == true)
				{
					CoAP_SetPayload(pReq, pResp, "Led is already on!", coap_strlen("Led is already on!"), true);
				}
				else
				{
					//gpio_output_set((1 >> LED_GPIO), 0, (1 >> LED_GPIO), 0);
					LedOnOff = true;
					led(LedOnOff);
					SetLedstatePayload(pReq, pResp);
				}
				PutQueryFound = true;
				break;
			}
			else if(CoAP_FindUriQueryVal(pOpt,"",3,"off"))
			{
				if(LedOnOff == false)
				{
					CoAP_SetPayload(pReq, pResp, "Led is already off!", coap_strlen("Led is already off!"), true);
				}
				else
				{
					//gpio_output_set(0,(1 >> LED_GPIO),(1 >> LED_GPIO), 0);
					LedOnOff = false;
					led(LedOnOff);
					SetLedstatePayload(pReq, pResp);
				}
				PutQueryFound = true;
				break;
			}
			else if(CoAP_FindUriQueryVal(pOpt,"",3,"chg"))
			{
				if(LedOnOff == true)
				{
					gpio_output_set(0, 1 >> 5 , 1 >> 5 , 0);
					LedOnOff = false;
					SetLedstatePayload(pReq, pResp);
				}
				else
				{
					gpio_output_set( 1 >> 5 , 0 , 1 >> 5 , 0);
					LedOnOff = true;
					SetLedstatePayload(pReq, pResp);
				}
/////////////////////////////////////////////
//				LedOnOff = !LedOnOff;
//				led(LedOnOff);
//				SetLedstatePayload(pReq,pResp);
///////////////////////////////////////////////
				PutQueryFound = true;
				break;
			}
	  }
		if(PutQueryFound == false)
		{
			char info[] = {"usage: coap://.../led_gpio5?on (or \"off\", \"tgl\")"};
					CoAP_SetPayload(pReq, pResp, info, coap_strlen(info), true);
					pResp->Code= RESP_ERROR_BAD_REQUEST_4_00;
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////GET handler
	if(pReq->Code == REQ_GET)
	{
		SetLedstatePayload(pReq, pResp);
	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	return HANDLER_OK;
}




CoAP_Res_t* ICACHE_FLASH_ATTR Create_Led_Switch_Resource()
{
	CoAP_ResOpts_t Options = {.Cf = COAP_CF_TEXT_PLAIN, .Flags = RES_OPT_GET | RES_OPT_PUT };

	//Enable LED on GPIO5 on ESP
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	led(LedOnOff);

	return (pLed_Switch_Res = CoAP_CreateResource("esp8266/led_switch_gpio5", "Turn led on or off", Options, Res_ReqHandler, NULL));
}
