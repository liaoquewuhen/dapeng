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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//															//																		//
// ���̣�	MQTT_JX											//	ע���ڡ�esp_mqtt_proj���������޸�									//
//															//																		//
// ƽ̨��	�����µ��ӡ������������� ESP8266 V1.0			//	�٣���ӡ���ʵ��ע�͡�������˵�ˣ�˵���˶����ᣡ����				//
//															//																		//
// ���ܣ�	�٣�����MQTT��ز���							//	�ڣ��޸ġ�MQTT�������顿config.h -> device_id/mqtt_host/mqtt_pass	//
//															//																		//
//			�ڣ���MQTT����ˣ�������������(TCP)				//	�ۣ��޸ġ�MQTT_CLIENT_ID�궨�塿mqtt_config.h -> MQTT_CLIENT_ID		//
//															//																		//
//			�ۣ�����/���͡�CONNECT�����ģ�����MQTT�����	//	�ܣ��޸ġ�PROTOCOL_NAMEv31�꡿mqtt_config.h -> PROTOCOL_NAMEv311	//
//															//																		//
//			�ܣ���������"SW_LED"							//	�ݣ��޸ġ��������ĵķ��ͼ����mqtt.c ->	[mqtt_timer]����			//
//															//																		//
//			�ݣ�������"SW_LED"����"ESP8266_Online"			//	�ޣ��޸ġ�SNTP���������á�user_main.c -> [sntpfn]����				//
//															//																		//
//			�ޣ����ݽ��յ�"SW_LED"�������Ϣ������LED����	//	�ߣ�ע�͡��������á�user_main.c -> [user_init]����					//
//															//																		//
//			�ߣ�ÿ��һ���ӣ���MQTT����˷��͡�������		//	�ࣺ��ӡ�MQTT��Ϣ����LED��/��user_main.c -> [mqttDataCb]����		//
//															//																		//
//	�汾��	V1.1											//																		//
//															//																		//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// ͷ�ļ�
//==============================
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
//==============================

// ���Ͷ���
//=================================
typedef unsigned long 		u32_t;
//=================================


// ȫ�ֱ���
//============================================================================
MQTT_Client mqttClient;			// MQTT�ͻ���_�ṹ�塾�˱����ǳ���Ҫ��

static ETSTimer sntp_timer;		// SNTP��ʱ��

u8 i;
char A_JSON_Tree[256] = {0};	// ��Ŵ�����JSON��
char B_JSON_Tree[256] = {0};	// �����ֵJSON��
char C_JSON_Tree[256] = {0};	// ��ű���JSON��

u16 once_wind_time = 1000;            //ÿ���ͷ�ʱ�䣬��λms
u16 once_water_time = 1000;           //ÿ�θ�ˮʱ�䣬��λms

u16 led_delta = 1000;                 //ÿ�ε������ȵĲ���


//============================================================================

void ICACHE_FLASH_ATTR TONE(u16 delay_time)
{
	u32 c = delay_time;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(0),1);
	for(;c>0;c--)
	    os_delay_us(1000);
	c = delay_time;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(0),0);
	for(;c>0;c--)
		os_delay_us(1000);
}

// ������ֵJSON��
//===================================================================================================
int ICACHE_FLASH_ATTR Parse_JSON_Tree_JX_VALUE(void)
{
	os_printf("\r\n-------------------- ����JSON��THRE -------------------\r\n");

	char A_JSONTree_Value[3][32] = {0};	// JSON���ݻ�������

	char * T_Pointer_Head = NULL;		// ��ʱָ��
	char * T_Pointer_end = NULL;		// ��ʱָ��

	u8 T_Value_Len = 0;					// ��"ֵ"���ĳ���



//	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// ��:��
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "{");				// ��{��
//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"temp\"");			// ��"temp"��
	T_Pointer_Head = os_strstr(B_JSON_Tree, ":")+1;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(B_JSON_Tree, ":")+4;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[0], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[0][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t CO2: %s\n",A_JSONTree_Value[0]);

//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"Humid\"");			// ��"Humid"��
	T_Pointer_Head = os_strstr(B_JSON_Tree, ":") + 10;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(B_JSON_Tree, ":") + 12;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[1], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t Humid:%s\n",A_JSONTree_Value[1]);

	T_Pointer_Head = os_strstr(B_JSON_Tree, ":") + 18;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(B_JSON_Tree, ":") + 20;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[2], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[2][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t CO2:%s\n",A_JSONTree_Value[2]);

	LED_THRE_Value[0] = atoi(A_JSONTree_Value[0]);
	LED_THRE_Value[1] = atoi(A_JSONTree_Value[1]);
	LED_THRE_Value[2] = atoi(A_JSONTree_Value[2]);



	os_printf("LIGHT_u16:%d\n",LED_THRE_Value[0]);
	os_printf("Temp_u16:%d\n",LED_THRE_Value[1]);
	os_printf("MHMH_u16:%d\n",LED_THRE_Value[2]);
	/*
	os_printf("MHMH_u16:%d/n",LED_SW_Value[3]);
	os_printf("light_u16:%d/n",LED_SW_Value[4]);
	*/
	JSON_THRE = 1;
	os_printf("\r\n-------------------- ����JSON��thre �ɹ� -------------------\r\n");
	return 0;
}
//===================================================================================================

// ����JSON��
//===================================================================================================
int ICACHE_FLASH_ATTR Parse_JSON_Tree_JX_WARN(void)
{
	os_printf("\r\n-------------------- ����JSON��WARN -------------------\r\n");

	char A_JSONTree_Value[5][32] = {0};	// JSON���ݻ�������

	char * T_Pointer_Head = NULL;		// ��ʱָ��
	char * T_Pointer_end = NULL;		// ��ʱָ��

	u8 T_Value_Len = 0;					// ��"ֵ"���ĳ���



//	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// ��:��
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "{");				// ��{��
//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"temp\"");			// ��"temp"��
	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 1;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 4;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[0], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[0][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t temp: %s\n",A_JSONTree_Value[0]);

//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"Humid\"");			// ��"Humid"��
	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 10;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 12;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[1], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t Humid:%s\n",A_JSONTree_Value[1]);

	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 18;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 21;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[2], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[2][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t CO2:%s\n",A_JSONTree_Value[2]);

	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 28;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 30;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[3], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t MHMH:%s\n",A_JSONTree_Value[3]);

	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 35;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 38;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[4], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t light:%s\n",A_JSONTree_Value[4]);

	LED_WARN_Value[0] = atoi(A_JSONTree_Value[0]);
	LED_WARN_Value[1] = atoi(A_JSONTree_Value[1]);
	LED_WARN_Value[2] = atoi(A_JSONTree_Value[2]);
	LED_WARN_Value[3] = atoi(A_JSONTree_Value[3]);
	LED_WARN_Value[4] = atoi(A_JSONTree_Value[4]);

	os_printf("Temp_u16:%d/n",LED_WARN_Value[0]);
	os_printf("Humid_u16:%d/n",LED_WARN_Value[1]);
	os_printf("CO2_u16:%d/n",LED_WARN_Value[2]);
	os_printf("MHMH_u16:%d/n",LED_WARN_Value[3]);
	os_printf("light_u16:%d/n",LED_WARN_Value[4]);


	JSON_WARN = 1;
	os_printf("\r\n-------------------- ����JSON��WARN �ɹ� -------------------\r\n");
	return 0;
}
//===================================================================================================



// ����JSON��
//===================================================================================================
int ICACHE_FLASH_ATTR Parse_JSON_Tree_JX(void)
{
	os_printf("\r\n-------------------- ����JSON�� SENERS-------------------\r\n");

	char A_JSONTree_Value[5][32] = {0};	// JSON���ݻ�������

	char * T_Pointer_Head = NULL;		// ��ʱָ��
	char * T_Pointer_end = NULL;		// ��ʱָ��

	u8 T_Value_Len = 0;					// ��"ֵ"���ĳ���



//	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// ��:��
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "{");				// ��{��
//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"temp\"");			// ��"temp"��
	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 9;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(A_JSON_Tree, ".");			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[0], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[0][T_Value_Len] = '\0';							// ��ֵ����'\0'��
	os_printf("\t temp: %s\n",A_JSONTree_Value[0]);

//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"Humid\"");			// ��"Humid"��
	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 22;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(A_JSON_Tree, ".") + 13;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[1], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t Humid:%s\n",A_JSONTree_Value[1]);

	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 33;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(A_JSON_Tree, ":") + 36;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[2], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[2][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t CO2:%s\n",A_JSONTree_Value[2]);

	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 44;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(A_JSON_Tree, ":") + 46;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[3], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t MHMH:%s\n",A_JSONTree_Value[3]);

	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 55;			// ��ֵ����ָ�롿
	T_Pointer_end = os_strstr(A_JSON_Tree, ":") + 58;			// ��"ֵ"β��"��
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// ���㡾ֵ���ĳ���
	os_memcpy(A_JSONTree_Value[4], T_Pointer_Head, T_Value_Len);		// ��ȡ��ֵ��
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// ��ֵ����'\0'��
//	os_printf("\t light:%s\n",A_JSONTree_Value[4]);

	LED_SW_Value[0] = atoi(A_JSONTree_Value[0]);
	LED_SW_Value[1] = atoi(A_JSONTree_Value[1]);
	LED_SW_Value[2] = atoi(A_JSONTree_Value[2]);
	LED_SW_Value[3] = atoi(A_JSONTree_Value[3]);
	LED_SW_Value[4] = atoi(A_JSONTree_Value[4]);
	/*
	os_printf("Temp_u16:%d/n",LED_SW_Value[0]);
	os_printf("Humid_u16:%d/n",LED_SW_Value[1]);
	os_printf("CO2_u16:%d/n",LED_SW_Value[2]);
	os_printf("MHMH_u16:%d/n",LED_SW_Value[3]);
	os_printf("light_u16:%d/n",LED_SW_Value[4]);
	*/

	JSON_VALUE = 1;
	os_printf("\r\n-------------------- ����JSON�� �ɹ�SENERS -------------------\r\n");
	return 0;
}
//===================================================================================================


// ������ʱ����
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
///===========================================

// SNTP��ʱ��������ȡ��ǰ����ʱ��
//============================================================================
void sntpfn()
{
	u32_t ts = 0;                         //ƫ��ʱ��

    ts = sntp_get_current_timestamp();		// ��ȡ��ǰ��ƫ��ʱ��

    os_printf("current time : %s\n", sntp_get_real_time(ts));	// ��ȡ��ʵʱ��

    if (ts == 0)		// ����ʱ���ȡʧ��
    {
        os_printf("did not get a valid time from sntp server\n");
    }
    else //(ts != 0)	// ����ʱ���ȡ�ɹ�
    {
            os_timer_disarm(&sntp_timer);	// �ر�SNTP��ʱ��

            MQTT_Connect(&mqttClient);		// ��ʼMQTT����
    }
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
	MQTT_Subscribe(client, "SW_LED", 0);	// ��������"SW_LED"��QoS=0
//	MQTT_Subscribe(client, "SW_LED", 1);
//	MQTT_Subscribe(client, "SW_LED", 2);

	TONE(100);
	TONE(100);
	// ������2�������� / ����3��������Ϣ����Ч�غ� / ����4����Ч�غɳ��� / ����5������Qos / ����6��Retain��
	//-----------------------------------------------------------------------------------------------------------------------------------------
	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 0, 0);	// ������"SW_LED"����"ESP8266_Online"��Qos=0��retain=0
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
    INFO("Topic_len1 = %d, Data_len = %d\r\n", topic_len, data_len);	// ���ڴ�ӡ�����ⳤ�ȡ�����Ч�غɳ��ȡ�


// ����С�¡����
//########################################################################################
    // ���ݽ��յ���������/��Ч�غɣ�����LED����/��
    //-----------------------------------------------------------------------------------
    if( os_strcmp(topicBuf,"SW_LED") == 0 )			// ���� == "SW_LED"
    {
		if(data_len>50){                 //������
			strcpy(A_JSON_Tree, dataBuf);
					Parse_JSON_Tree_JX();
			//os_printf("A_JSON_Tree:%s",A_JSON_Tree);
		}

		if(data_len<30&&data_len>20){   //��ֵ
					strcpy(B_JSON_Tree, dataBuf);
							Parse_JSON_Tree_JX_VALUE();
					//os_printf("B_JSON_Tree:%s",B_JSON_Tree);
				}
		if(data_len<50&&data_len>40){   //��ֵ
					strcpy(C_JSON_Tree, dataBuf);
							Parse_JSON_Tree_JX_WARN();
					os_printf("C_JSON_Tree:%s",C_JSON_Tree);
				}

    	if( os_strcmp(dataBuf,"water_man") == 0 )		// ��Ч�غ� == "water_man"
    	{
    		if(!AUTO)
    		{
    	        GPIO_OUTPUT_SET(GPIO_ID_PIN(5),1);		// �ֶ���ˮ
    	        delay_ms(once_water_time);
    	        GPIO_OUTPUT_SET(GPIO_ID_PIN(5),0);
    	        os_printf("water_man");
    	        os_printf("AUTO:%d",AUTO);
    		}
    		else{
    		os_printf("receive water_man but auto=1");
    		os_printf("AUTO:%d",AUTO);}
    	}

    	if( os_strcmp(dataBuf,"wind_man") == 0 )		// ��Ч�غ� == "wind_man"
    	{
    		if(!AUTO)
    		{
    			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);		// �ֶ��ͷ�
    			delay_ms(once_wind_time);
    			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);
    		}
    	 }

    	if( os_strcmp(dataBuf,"light_man_up") == 0 )		// ��Ч�غ� == "light_man_up"
    	{
    		if(!AUTO)
    		{
    			if(led_duty>22222+led_delta) led_duty = 22200-led_delta;
    			    			if(led_duty<led_delta+100) led_duty = led_delta+100;
    			    			led_duty = led_duty - led_delta;


    			pwm_set_period(1000);
    			pwm_set_duty(led_duty,0);//���� PWM ĳ��ͨ���źŵ�ռ�ձ�, duty ռ�ձȵ�ֵ, type��ǰҪ���õ� PWM ͨ��
    			pwm_start();//������ɺ���Ҫ���� pwm_start,PWM ��ʼ
    			os_printf("\nled_duty:%d",led_duty);
    		}
    		os_printf("AUTO:%d",AUTO);
    	}

    	if( os_strcmp(dataBuf,"light_man_down") == 0 )		// ��Ч�غ� == "light_man_down"
    	{
    		if(!AUTO)
    		{
    			if(led_duty>22222-led_delta) led_duty = 22200-led_delta;
    			    			if(led_duty<led_delta+100) led_duty = led_delta+100;
    			    			led_duty = led_duty + led_delta;

    			pwm_set_period(1000);
    			pwm_set_duty(led_duty,0);//���� PWM ĳ��ͨ���źŵ�ռ�ձ�, duty ռ�ձȵ�ֵ, type��ǰҪ���õ� PWM ͨ��
    			pwm_start();//������ɺ���Ҫ���� pwm_start,PWM ��ʼ
    			os_printf("\nled_duty:%d",led_duty);
    		}
    	}

    	if( os_strcmp(dataBuf,"AUTO_ALL") == 0 )
    	{
    		AUTO = 1;

    	}

    	if( os_strcmp(dataBuf,"MAN_ALL") == 0 )
    	{
    		AUTO = 0;

    	}
    	if(os_strcmp(dataBuf,"B") == 0)
    	{
    		TONE(100);
    		TONE(50);

    	}
    	if(os_strcmp(dataBuf,"warning_on") == 0)
    	{
    		WARN = 1;

    	}
    	if(os_strcmp(dataBuf,"warning_off") == 0)
    	{
    		WARN = 0;

    	}
    	if(os_strcmp(dataBuf,"test") == 0)
    	{
			pwm_set_period(1000);
			pwm_set_duty(22220,0);//���� PWM ĳ��ͨ���źŵ�ռ�ձ�, duty ռ�ձȵ�ֵ, type��ǰҪ���õ� PWM ͨ��
			pwm_start();//������ɺ���Ҫ���� pwm_start,PWM ��ʼ
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
    uart_init(BIT_RATE_115200, BIT_RATE_115200);	// ���ڲ�������Ϊ74880
    os_delay_us(60000);

    JSON_WARN = 0;                 //��ʾδ���չ�������ֵ
    AUTO = 0;                               //Ĭ���ֶ�����
    WARN = 0;                        //Ĭ�ϲ����б���
    JSON_VALUE=0;                 //��ʾ�Ƿ���յ�����������ֵ,0��ʾδ����
    JSON_THRE=0;                  //��ʾ�Ƿ���ܹ�������ֵ,0��ʾδ����
    led_duty = 10000;                 //led���ȣ���Χ0~22222
	uint32 pwm_duty_init[3]={0};
	uint32 io_info[][3]={{PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO12,12},};  //12�ڽ�LED
	pwm_init(1000,pwm_duty_init,1,io_info);//��ʼ�� PWM��1000����,pwm_duty_initռ�ձ�,3ͨ����,io_info��ͨ���� GPIO Ӳ������
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,	FUNC_GPIO0);   //������
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U,	FUNC_GPIO5);   //ˮ��



//����С�¡����
//###########################################################################
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// ���	#
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);						// ���������Ϊ�ر�״̬	#
	GPIO_OUTPUT_SET(GPIO_ID_PIN(5),0);						// ��ˮ������Ϊ�ر�״̬	#
//###########################################################################


    CFG_Load();	// ����/����ϵͳ������WIFI������MQTT������


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
