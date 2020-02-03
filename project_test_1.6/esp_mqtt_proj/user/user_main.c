/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														//																		//
// ���̣�	MQTT_Data-Visualization						//	ע���ڡ�MQTT_JX���������޸�											//
//														//																		//
// ƽ̨��	�����µ��ӡ������������� ESP8266 V1.0		//	�٣����մ�������Ӱ�ӡ�ʱ��ȡ����MQTT���Ӳ������޸�<mqtt_config.h>	//
//														//																		//
// ���ܣ�	�٣����á���Ӱ�ӡ�MQTT��ز���				//	�ڣ���<dht11.c>��ӵ�<modules>����<dht11.h>��ӵ�<include/modules>	//
//														//																		//
//			�ڣ�ESP8266����ٶ��ơ���Ӱ�ӡ�				//	�ۣ���ע�⣬<dht11.c>�У����Ϊ<#include modules/dht11.h>			//
//														//																		//
//			�ۣ�ÿ5������Ӱ�ӡ��ϱ�����ʪ�����ݡ�		//	�ܣ���<mqtt.c>��ʱ�����У���ӣ�ÿ5������Ӱ�ӡ��ϱ�����ʪ�����ݡ�	//
//														//																		//
//	�汾��	V1.0										//	�ݣ�ȡ��������"SW_LED"������"SW_LED"������Ϣ������					//
//														//																		//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// ͷ�ļ�
//==============================
#include <stdio.h>
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"
#include "smartconfig.h"

#include "modules/i2c_SGP30.h"

#include "dht11.h"	// DHT11
#include "oled.h"
//==============================

// ���Ͷ���
//=================================
typedef unsigned long 		u32_t;
//=================================


// �궨��
//==================================================================================
#define		ProjectName			"1.6"	// �������궨��

#define		Sector_STA_INFO		0x8D			// ��STA��������������
//==================================================================================

// ȫ�ֱ���
//==================================================================================
struct station_config STA_INFO;		// ��STA�������ṹ��

os_timer_t OS_Timer_IP;				// �����ʱ��

struct ip_info ST_ESP8266_IP;		// 8266��IP��Ϣ
u8 ESP8266_IP[4];					// 8266��IP��ַ

u8 C_LED_Flash = 0;					// LED��˸�ƴ�

MQTT_Client mqttClient;			// MQTT�ͻ���_�ṹ�塾�˱����ǳ���Ҫ��

static ETSTimer sntp_timer;		// SNTP��ʱ��

//============================================================================

/*
//����ת�ַ���
//=============================================================================
void ICACHE_FLASH_ATTR data2str()
{
	u8 C_char = 0;		// �ַ�����
  //  os_printf("/n%d,%d,%d",co2Data/100,(co2Data%100)/10,co2Data%10);
    Data_Char_myself[0][C_char++] = co2Data/100 + 48;
    Data_Char_myself[0][C_char++] = (co2Data%100)/10 + 48;
    Data_Char_myself[0][C_char++] = co2Data%10 + 48;
    C_char = 0;
    Data_Char_myself[1][C_char++] = MHMH/100 + 48;
    Data_Char_myself[1][C_char++] = (MHMH%100)/10 + 48;
    Data_Char_myself[1][C_char++] = MHMH%10 + 48;
    C_char = 0;
    Data_Char_myself[2][C_char++] = light/100 + 48;
    Data_Char_myself[2][C_char++] = (light%100)/10 + 48;
    Data_Char_myself[2][C_char++] = light%10 + 48;

}
//=============================================================================
*/

// GPIO�жϺ�����ע�⣺�жϺ���ǰ��Ҫ��"ICACHE_FLASH_ATTR"�꡿
//=============================================================================
void GPIO_INTERRUPT(void)
{
	u32	S_GPIO_INT;		// ����IO�ڵ��ж�״̬
	u32 F_GPIO_0_INT;	// GPIO_0���ж�״̬


	// ��ȡGPIO�ж�״̬
	//---------------------------------------------------
	S_GPIO_INT = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	// ����ж�״̬λ(��������״̬λ�������������ж�)
	//----------------------------------------------------
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, S_GPIO_INT);


	F_GPIO_0_INT = S_GPIO_INT & (0x01<<0);	// ��ȡGPIO_0�ж�״̬


	// �ж��Ƿ���KEY�ж�(δ������)
	//------------------------------------------------------------
	if(F_GPIO_0_INT)	// GPIO_0���½����ж�
	{
		DHT11_NUM_Char();
        os_printf("\r\n====exit0 ======\r\n");
        pages = !pages;
        if(pages){
        	OLED_ShowString(0,0,"P1/2");
        	OLED_ShowString(0,2,"                ");
        	OLED_ShowString(48,4,"      ");
        	OLED_ShowString(48,6,"      ");
        	if(wifi_station_get_connect_status() == STATION_GOT_IP)
        	     OLED_ShowString(0,2,"connect to WIFI!");
        	OLED_ShowString(0,4,"Humid:          ");
        	OLED_ShowString(0,6,"Temp :          ");
    		OLED_ShowString(48,4,DHT11_Data_Char[0]);	// DHT11_Data_Char[0] == ��ʪ���ַ�����
    		OLED_ShowString(48,6,DHT11_Data_Char[1]);	// DHT11_Data_Char[1] == ���¶��ַ�����
        }
        else{
        	data2str();
        	OLED_ShowString(0,0,"P2/2");
        	OLED_ShowString(0,2,"                ");
        	OLED_ShowString(48,4,"      ");
        	OLED_ShowString(48,6,"      ");
        	OLED_ShowString(0,2,"CO2  :   ppm    ");
        	OLED_ShowString(0,4,"MH   :          ");
        	OLED_ShowString(0,6,"light:          ");
        	OLED_ShowString(48,2,Data_Char_myself[0]);
        	OLED_ShowString(48,4,Data_Char_myself[1]);
        	OLED_ShowString(48,6,Data_Char_myself[2]);
        }

	}

}

// SmartConfig״̬�����ı�ʱ������˻ص�����
//--------------------------------------------
// ����1��sc_status status / ����2��������ָ�롾�ڲ�ͬ״̬�£�[void *pdata]�Ĵ�������ǲ�ͬ�ġ�
//=================================================================================================================
void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata)
{
	os_printf("\r\n------ smartconfig_done ------\r\n");	// ESP8266����״̬�ı�

    switch(status)
    {
    	// CmartConfig�ȴ�
    	//����������������������������������������
        case SC_STATUS_WAIT:		// ��ʼֵ
            os_printf("\r\nSC_STATUS_WAIT\r\n");
        break;
        //����������������������������������������

        // ���֡�WIFI�źš���8266������״̬�µȴ�������
        //��������������������������������������������������������������������������
        case SC_STATUS_FIND_CHANNEL:
            os_printf("\r\nSC_STATUS_FIND_CHANNEL\r\n");

    		os_printf("\r\n---- Please Use WeChat to SmartConfig ------\r\n\r\n");

    		OLED_ShowString(0,2,"Use WeChat        ");
//    		OLED_ShowString(0,6,"SmartConfig     ");
    	break;
    	//��������������������������������������������������������������������������

        // ���ڻ�ȡ��SSID����PSWD����8266����ץȡ�����ܡ�SSID+PSWD����
        //��������������������������������������������������������������������������
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("\r\nSC_STATUS_GETTING_SSID_PSWD\r\n");

            // ��SC_STATUS_GETTING_SSID_PSWD��״̬�£�����2==SmartConfig����ָ��
            //-------------------------------------------------------------------
			sc_type *type = pdata;		// ��ȡ��SmartConfig���͡�ָ��

			// ������ʽ == ��ESPTOUCH��
			//-------------------------------------------------
            if (*type == SC_TYPE_ESPTOUCH)
            { os_printf("\r\nSC_TYPE:SC_TYPE_ESPTOUCH\r\n"); }

            // ������ʽ == ��AIRKISS��||��ESPTOUCH_AIRKISS��
            //-------------------------------------------------
            else
            { os_printf("\r\nSC_TYPE:SC_TYPE_AIRKISS\r\n"); }

	    break;
	    //��������������������������������������������������������������������������

        // �ɹ���ȡ����SSID����PSWD��������STA������������WIFI
	    //��������������������������������������������������������������������������
        case SC_STATUS_LINK:
            os_printf("\r\nSC_STATUS_LINK\r\n");

            // ��SC_STATUS_LINK��״̬�£�����2 == STA�����ṹ��ָ��
            //------------------------------------------------------------------
            struct station_config *sta_conf = pdata;	// ��ȡ��STA������ָ��

            // ����SSID����PASS�����浽���ⲿFlash����
            //--------------------------------------------------------------------------
			spi_flash_erase_sector(Sector_STA_INFO);						// ��������
			spi_flash_write(Sector_STA_INFO*4096, (uint32 *)sta_conf, 96);	// д������
			//--------------------------------------------------------------------------

			wifi_station_set_config(sta_conf);			// ����STA������Flash��
	        wifi_station_disconnect();					// �Ͽ�STA����
	        wifi_station_connect();						// ESP8266����WIFI

	    	OLED_ShowString(0,2,"WIFI Connecting ");	// OLED��ʾ��
	    	//OLED_ShowString(0,6,"................");	// ��������WIFI

	    break;
	    //��������������������������������������������������������������������������


        // ESP8266��ΪSTA���ɹ����ӵ�WIFI
	    //��������������������������������������������������������������������������
        case SC_STATUS_LINK_OVER:
            os_printf("\r\nSC_STATUS_LINK_OVER\r\n");

            smartconfig_stop();		// ֹͣSmartConfig���ͷ��ڴ�

            //**************************************************************************************************

            wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// ��ȡ8266_STA��IP��ַ

			ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP��ַ�߰�λ == addr�Ͱ�λ
			ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP��ַ�θ߰�λ == addr�εͰ�λ
			ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP��ַ�εͰ�λ == addr�θ߰�λ
			ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP��ַ�Ͱ�λ == addr�߰�λ

			// ��ʾESP8266��IP��ַ
			//-----------------------------------------------------------------------------------------------
//			os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
//			OLED_ShowIP(24,2,ESP8266_IP);	// OLED��ʾESP8266��IP��ַ
//			OLED_ShowString(0,4,"Connect to WIFI ");
//			OLED_ShowString(0,6,"Successfully    ");
			//-----------------------------------------------------------------------------------------------

			// ����WIFI�ɹ���LED����3��
			//----------------------------------------------------
			for(; C_LED_Flash<=5; C_LED_Flash++)
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
				delay_ms(100);
			}

			os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");

			//*****************************************************
			// WIFI���ӳɹ���ִ�к������ܡ�	�磺SNTP/UDP/TCP/DNS��
			//*****************************************************

			//**************************************************************************************************

	    break;
	    //��������������������������������������������������������������������������

    }
}
//=================================================================================================================


// IP��ʱ���Ļص�����
//========================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_cb(void)
{
	u8 S_WIFI_STA_Connect;			// WIFI����״̬��־


	// ��ѯSTA����WIFI״̬
	//-----------------------------------------------------
	S_WIFI_STA_Connect = wifi_station_get_connect_status();
	//---------------------------------------------------
	// Station����״̬��
	// 0 == STATION_IDLE -------------- STATION����
	// 1 == STATION_CONNECTING -------- ��������WIFI
	// 2 == STATION_WRONG_PASSWORD ---- WIFI�������
	// 3 == STATION_NO_AP_FOUND ------- δ����ָ��WIFI
	// 4 == STATION_CONNECT_FAIL ------ ����ʧ��
	// 5 == STATION_GOT_IP ------------ ���IP�����ӳɹ�
	//---------------------------------------------------


	// �ɹ�����WIFI
	//----------------------------------------------------------------------------------------
	if( S_WIFI_STA_Connect == STATION_GOT_IP )	// �ж��Ƿ��ȡIP
	{
		wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// ��ȡ8266_STA��IP��ַ

		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP��ַ�߰�λ == addr�Ͱ�λ
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP��ַ�θ߰�λ == addr�εͰ�λ
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP��ַ�εͰ�λ == addr�θ߰�λ
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP��ַ�Ͱ�λ == addr�߰�λ

		// ��ʾESP8266��IP��ַ
		//-----------------------------------------------------------------------------------------------
		os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
//		OLED_ShowIP(24,2,ESP8266_IP);	// OLED��ʾESP8266��IP��ַ
//		OLED_ShowString(0,4,"Connect to WIFI ");
//		OLED_ShowString(0,6,"Successfully    ");
		//-----------------------------------------------------------------------------------------------

		// ����WIFI�ɹ���LED����3��
		//----------------------------------------------------
		for(; C_LED_Flash<=5; C_LED_Flash++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
			delay_ms(100);
		}

		os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");

		os_timer_disarm(&OS_Timer_IP);	// �رն�ʱ��

		//*****************************************************
		// WIFI���ӳɹ���ִ�к������ܡ�	�磺SNTP/UDP/TCP/DNS��
		//*****************************************************
	}

	// ESP8266�޷�����WIFI
	//------------------------------------------------------------------------------------------------
	else if(	S_WIFI_STA_Connect==STATION_NO_AP_FOUND 	||		// δ�ҵ�ָ��WIFI
				S_WIFI_STA_Connect==STATION_WRONG_PASSWORD 	||		// WIFI�������
				S_WIFI_STA_Connect==STATION_CONNECT_FAIL		)	// ����WIFIʧ��
	{
		os_timer_disarm(&OS_Timer_IP);			// �رն�ʱ��

		os_printf("\r\n---- S_WIFI_STA_Connect=%d-----------\r\n",S_WIFI_STA_Connect);
		os_printf("\r\n---- ESP8266 Can't Connect to WIFI-----------\r\n");

		// ΢��������������
		//��������������������������������������������������������������������������������������������
		//wifi_set_opmode(STATION_MODE);			// ��ΪSTAģʽ						//���ڢٲ���

		smartconfig_set_type(SC_TYPE_AIRKISS); 	// ESP8266������ʽ��AIRKISS��			//���ڢڲ���

		smartconfig_start(smartconfig_done);	// ���롾��������ģʽ��,�����ûص�����	//���ڢ۲���
		//��������������������������������������������������������������������������������������������
	}
}
//========================================================================================================


// �����ʱ����ʼ��(ms����)
//========================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_Init_JX(u32 time_ms, u8 time_repetitive)
{

	os_timer_disarm(&OS_Timer_IP);	// �رն�ʱ��
	os_timer_setfn(&OS_Timer_IP,(os_timer_func_t *)OS_Timer_IP_cb, NULL);	// ���ö�ʱ��
	os_timer_arm(&OS_Timer_IP, time_ms, time_repetitive);  // ʹ�ܶ�ʱ��
}

// SNTP��ʱ��������ȡ��ǰ����ʱ��
//============================================================================
void sntpfn()
{
	// �ַ������� ��ر���
	//------------------------------------------------------

	u8 C_Str = 0;				// �ַ����ֽڼ���

	char A_Str_Data[20] = {0};	// ��"����"���ַ�������

	char *T_A_Str_Data = A_Str_Data;	// ��������ָ��

	char A_Str_Clock[10] = {0};	// ��"ʱ��"���ַ�������


	char * Str_Head_Week;		// ��"����"���ַ����׵�ַ

	char * Str_Head_Month;		// ��"�·�"���ַ����׵�ַ

	char * Str_Head_Day;		// ��"����"���ַ����׵�ַ

	char * Str_Head_Clock;		// ��"ʱ��"���ַ����׵�ַ

	char * Str_Head_Year;		// ��"���"���ַ����׵�ַ

    u32_t ts = 0;
    u32_t TimeStamp=0;

    char * Str_RealTime;	// ʵ��ʱ����ַ���

    ts = sntp_get_current_timestamp();		// ��ȡ��ǰ��ƫ��ʱ��

    os_printf("current time : %s\n", sntp_get_real_time(ts));	// ��ȡ��ʵʱ��

    if (ts == 0)		// ����ʱ���ȡʧ��
    {
        os_printf("did not get a valid time from sntp server\n");
    }
    else //(ts != 0)	// ����ʱ���ȡ�ɹ�
    {
            os_timer_disarm(&sntp_timer);	// �ر�SNTP��ʱ��
            //os_timer_disarm(&OS_Timer_SNTP);	// �ر�SNTP��ʱ��
            TimeStamp = ts;
            		 // ��ѯʵ��ʱ��(GMT+8):������(����ʱ��)
            		 //--------------------------------------------
            		 Str_RealTime = sntp_get_real_time(TimeStamp);


            		 // ��ʵ��ʱ�䡿�ַ��� == "�� �� �� ʱ:��:�� ��"
            		 //------------------------------------------------------------------------
            		 os_printf("\r\n----------------------------------------------------\r\n");
            		 os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		// ʱ���
            		 os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// ʵ��ʱ��
            		 os_printf("--------------------------------------------------------\r\n");


            		 // ʱ���ַ�������OLED��ʾ��"����"������"ʱ��"���ַ���
            		 //��������������������������������������������������������������������������������������

            		 // ��"���" + ' '��������������
            		 //---------------------------------------------------------------------------------
            		 Str_Head_Year = Str_RealTime;	// ������ʼ��ַ

            		 while( *Str_Head_Year )		// �ҵ���"ʵ��ʱ��"���ַ����Ľ����ַ�'\0'
            			 Str_Head_Year ++ ;

            		 // ��ע��API���ص�ʵ��ʱ���ַ����������һ�����з����������� -5��
            		 //-----------------------------------------------------------------
            		 Str_Head_Year -= 5 ;			// ��ȡ��"���"���ַ������׵�ַ

            		 T_A_Str_Data[4] = ' ' ;
            		 os_memcpy(T_A_Str_Data, Str_Head_Year, 4);		// ��"���" + ' '��������������

            		 T_A_Str_Data += 5;				// ָ��"���" + ' '���ַ����ĺ���ĵ�ַ
            		 //---------------------------------------------------------------------------------

            		 // ��ȡ�����ڡ��ַ������׵�ַ
            		 //---------------------------------------------------------------------------------
            		 Str_Head_Week 	= Str_RealTime;							// "����" �ַ������׵�ַ
            		 Str_Head_Month = os_strstr(Str_Head_Week,	" ") + 1;	// "�·�" �ַ������׵�ַ
            		 Str_Head_Day 	= os_strstr(Str_Head_Month,	" ") + 1;	// "����" �ַ������׵�ַ
            		 Str_Head_Clock = os_strstr(Str_Head_Day,	" ") + 1;	// "ʱ��" �ַ������׵�ַ


            		 // ��"�·�" + ' '��������������
            		 //---------------------------------------------------------------------------------
            		 C_Str = Str_Head_Day - Str_Head_Month;				// ��"�·�" + ' '�����ֽ���

            		 os_memcpy(T_A_Str_Data, Str_Head_Month, C_Str);	// ��"�·�" + ' '��������������

            		 T_A_Str_Data += C_Str;		// ָ��"�·�" + ' '���ַ����ĺ���ĵ�ַ


            		 // ��"����" + ' '��������������
            		 //---------------------------------------------------------------------------------
            		 C_Str = Str_Head_Clock - Str_Head_Day;				// ��"����" + ' '�����ֽ���

            		 os_memcpy(T_A_Str_Data, Str_Head_Day, C_Str);		// ��"����" + ' '��������������

            		 T_A_Str_Data += C_Str;		// ָ��"����" + ' '���ַ����ĺ���ĵ�ַ


            		 // ��"����" + ' '��������������
            		 //---------------------------------------------------------------------------------
            		 C_Str = Str_Head_Month - Str_Head_Week - 1;		// ��"����"�����ֽ���

            		 os_memcpy(T_A_Str_Data, Str_Head_Week, C_Str);		// ��"����"��������������

            		 T_A_Str_Data += C_Str;		// ָ��"����"���ַ����ĺ���ĵ�ַ


            		 // OLED��ʾ��"����"������"ʱ��"���ַ���
            		 //---------------------------------------------------------------------------------
            		 *T_A_Str_Data = '\0';		// ��"����"���ַ����������'\0'

            		 OLED_ShowString(0,0,A_Str_Data);		// OLED��ʾ����


            		 os_memcpy(A_Str_Clock, Str_Head_Clock, 8);		// ��"ʱ��"���ַ�������ʱ������
            		 A_Str_Clock[8] = '\0';

            		 //OLED_ShowString(64,2,A_Str_Clock);		// OLED��ʾʱ��

            		 //��������������������������������������������������������������������������������������
            	 }

            MQTT_Connect(&mqttClient);		// ��ʼMQTT����

}
//============================================================================


// WIFI����״̬�ı䣺���� = wifiStatus
//============================================================================
void wifiConnectCb(uint8_t status)
{
	// �ɹ���ȡ��IP��ַ
	//---------------------------------------------------------------------
    if(status == STATION_GOT_IP)
    {
    	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));

    	// �ڹٷ����̵Ļ����ϣ�����2�����÷�����
    	//---------------------------------------------------------------
    	sntp_setservername(0, "us.pool.ntp.org");	// ������_0��������
    	sntp_setservername(1, "ntp.sjtu.edu.cn");	// ������_1��������

    	ipaddr_aton("210.72.145.44", addr);	// ���ʮ���� => 32λ������
    	sntp_setserver(2, addr);					// ������_2��IP��ַ��
    	os_free(addr);								// �ͷ�addr

    	sntp_init();	// SNTP��ʼ��


        // ����SNTP��ʱ��[sntp_timer]
        //-----------------------------------------------------------
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntpfn, NULL);
        os_timer_arm(&sntp_timer, 1000, 1);		// 1s��ʱ
    }

    // IP��ַ��ȡʧ��
	//----------------------------------------------------------------
    else
    {
          MQTT_Disconnect(&mqttClient);	// WIFI���ӳ���TCP�Ͽ�����
    }
}
//============================================================================


// MQTT�ѳɹ����ӣ�ESP8266���͡�CONNECT���������յ���CONNACK��
//============================================================================
void mqttConnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;	// ��ȡmqttClientָ��

    INFO("MQTT: Connected\r\n");

    // ������2����������� / ����3������Qos��
    //-----------------------------------------------------------------
//	MQTT_Subscribe(client, "SW_LED", 0);	// ��������"SW_LED"��QoS=0
//	MQTT_Subscribe(client, "SW_LED", 1);
//	MQTT_Subscribe(client, "SW_LED", 2);

	// ������2�������� / ����3��������Ϣ����Ч�غ� / ����4����Ч�غɳ��� / ����5������Qos / ����6��Retain��
	//-----------------------------------------------------------------------------------------------------------------------------------------
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 0, 0);	// ������"SW_LED"����"ESP8266_Online"��Qos=0��retain=0
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 1, 0);
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 2, 0);
}
//============================================================================

// MQTT�ɹ��Ͽ�����
//============================================================================
void mqttDisconnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Disconnected\r\n");
}
//============================================================================

// MQTT�ɹ�������Ϣ
//============================================================================
void mqttPublishedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}
//============================================================================

// ������MQTT��[PUBLISH]���ݡ�����		������1������ / ����2�����ⳤ�� / ����3����Ч�غ� / ����4����Ч�غɳ��ȡ�
//===============================================================================================================
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len+1);		// ���롾���⡿�ռ�
    char *dataBuf  = (char*)os_zalloc(data_len+1);		// ���롾��Ч�غɡ��ռ�


    MQTT_Client* client = (MQTT_Client*)args;	// ��ȡMQTT_Clientָ��


    os_memcpy(topicBuf, topic, topic_len);	// ��������
    topicBuf[topic_len] = 0;				// �����'\0'

    os_memcpy(dataBuf, data, data_len);		// ������Ч�غ�
    dataBuf[data_len] = 0;					// �����'\0'

    INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);	// ���ڴ�ӡ�����⡿����Ч�غɡ�


// ����С�¡����
//########################################################################################
	// ���ݽ��յ���������/��Ч�غɣ�����LED����/��
	//-----------------------------------------------------------------------------------
	if( os_strcmp(topicBuf,"SW_LED") == 0 )			// ���� == "SW_LED"
	{
		if( os_strcmp(dataBuf,"LED_ON") == 0 )		// ��Ч�غ� == "LED_ON"
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);		// LED��
		}

		else if( os_strcmp(dataBuf,"LED_OFF") == 0 )	// ��Ч�غ� == "LED_OFF"
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);			// LED��
		}
	}
//########################################################################################


    os_free(topicBuf);	// �ͷš����⡿�ռ�
    os_free(dataBuf);	// �ͷš���Ч�غɡ��ռ�
}
//===============================================================================================================

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
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

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

// user_init��entry of user application, init user function here
//===================================================================================================================
void user_init(void)
{
    uart_init(BIT_RATE_74880, BIT_RATE_74880);	// ���ڲ�������Ϊ74880
    os_delay_us(60000);

    pages = 0;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,	FUNC_GPIO0);	// GPIO_0��ΪGPIO��
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));       // GPIO_0ʧ�����(Ĭ��)
	ETS_GPIO_INTR_DISABLE();										// �ر�GPIO�жϹ���
	ETS_GPIO_INTR_ATTACH((ets_isr_t)GPIO_INTERRUPT,NULL);			// ע���жϻص�����
	gpio_pin_intr_state_set(GPIO_ID_PIN(0),GPIO_PIN_INTR_NEGEDGE);	// GPIO_0�½����ж�
	ETS_GPIO_INTR_ENABLE();


    OLED_Init();
    SGP30_Init();
//����С�¡����
//###########################################################################
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4�����	#
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,	FUNC_GPIO12);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(12),1);						// LED��ʼ��	#
//###########################################################################


    CFG_Load();	// ����/����ϵͳ������WIFI������MQTT������

    OS_Timer_IP_Init_JX(1000,1);

    // �������Ӳ�����ֵ�������������mqtt_test_jx.mqtt.iot.gz.baidubce.com�����������Ӷ˿ڡ�1883������ȫ���͡�0��NO_TLS��
	//-------------------------------------------------------------------------------------------------------------------
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);

	// MQTT���Ӳ�����ֵ���ͻ��˱�ʶ����..����MQTT�û�����..����MQTT��Կ��..������������ʱ����120s��������Ự��1��clean_session��
	//----------------------------------------------------------------------------------------------------------------------------
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);

	// ������������(����ƶ�û�ж�Ӧ���������⣬��MQTT���ӻᱻ�ܾ�)
	//--------------------------------------------------------------
//	MQTT_InitLWT(&mqttClient, "Will", "ESP8266_offline", 0, 0);


	// ����MQTT��غ���
	//--------------------------------------------------------------------------------------------------
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);			// ���á�MQTT�ɹ����ӡ���������һ�ֵ��÷�ʽ
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);	// ���á�MQTT�ɹ��Ͽ�����������һ�ֵ��÷�ʽ
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);			// ���á�MQTT�ɹ���������������һ�ֵ��÷�ʽ
	MQTT_OnData(&mqttClient, mqttDataCb);					// ���á�����MQTT���ݡ���������һ�ֵ��÷�ʽ


	// ����WIFI��SSID[..]��PASSWORD[..]��WIFI���ӳɹ�����[wifiConnectCb]
	//--------------------------------------------------------------------------
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);


	INFO("\r\nSystem started ...\r\n");
}
//===================================================================================================================
