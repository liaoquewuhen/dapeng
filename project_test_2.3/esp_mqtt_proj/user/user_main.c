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
// 工程：	MQTT_JX											//	注：在《esp_mqtt_proj》例程上修改									//
//															//																		//
// 平台：	【技新电子】物联网开发板 ESP8266 V1.0			//	①：添加【详实的注释】唉，不说了，说多了都是泪！！！				//
//															//																		//
// 功能：	①：设置MQTT相关参数							//	②：修改【MQTT参数数组】config.h -> device_id/mqtt_host/mqtt_pass	//
//															//																		//
//			②：与MQTT服务端，建立网络连接(TCP)				//	③：修改【MQTT_CLIENT_ID宏定义】mqtt_config.h -> MQTT_CLIENT_ID		//
//															//																		//
//			③：配置/发送【CONNECT】报文，连接MQTT服务端	//	④：修改【PROTOCOL_NAMEv31宏】mqtt_config.h -> PROTOCOL_NAMEv311	//
//															//																		//
//			④：订阅主题"SW_LED"							//	⑤：修改【心跳报文的发送间隔】mqtt.c ->	[mqtt_timer]函数			//
//															//																		//
//			⑤：向主题"SW_LED"发布"ESP8266_Online"			//	⑥：修改【SNTP服务器设置】user_main.c -> [sntpfn]函数				//
//															//																		//
//			⑥：根据接收到"SW_LED"主题的消息，控制LED亮灭	//	⑦：注释【遗嘱设置】user_main.c -> [user_init]函数					//
//															//																		//
//			⑦：每隔一分钟，向MQTT服务端发送【心跳】		//	⑧：添加【MQTT消息控制LED亮/灭】user_main.c -> [mqttDataCb]函数		//
//															//																		//
//	版本：	V1.1											//																		//
//															//																		//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// 头文件
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

// 类型定义
//=================================
typedef unsigned long 		u32_t;
//=================================


// 全局变量
//============================================================================
MQTT_Client mqttClient;			// MQTT客户端_结构体【此变量非常重要】

static ETSTimer sntp_timer;		// SNTP定时器

u8 i;
char A_JSON_Tree[256] = {0};	// 存放传感器JSON树
char B_JSON_Tree[256] = {0};	// 存放阈值JSON树
char C_JSON_Tree[256] = {0};	// 存放报警JSON树

u16 once_wind_time = 1000;            //每次送风时间，单位ms
u16 once_water_time = 1000;           //每次给水时间，单位ms

u16 led_delta = 1000;                 //每次调节亮度的步幅


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

// 解析阈值JSON树
//===================================================================================================
int ICACHE_FLASH_ATTR Parse_JSON_Tree_JX_VALUE(void)
{
	os_printf("\r\n-------------------- 解析JSON树THRE -------------------\r\n");

	char A_JSONTree_Value[3][32] = {0};	// JSON数据缓存数组

	char * T_Pointer_Head = NULL;		// 临时指针
	char * T_Pointer_end = NULL;		// 临时指针

	u8 T_Value_Len = 0;					// 【"值"】的长度



//	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "{");				// 【{】
//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"temp\"");			// 【"temp"】
	T_Pointer_Head = os_strstr(B_JSON_Tree, ":")+1;			// 【值的首指针】
	T_Pointer_end = os_strstr(B_JSON_Tree, ":")+4;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[0], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[0][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t CO2: %s\n",A_JSONTree_Value[0]);

//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"Humid\"");			// 【"Humid"】
	T_Pointer_Head = os_strstr(B_JSON_Tree, ":") + 10;			// 【值的首指针】
	T_Pointer_end = os_strstr(B_JSON_Tree, ":") + 12;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[1], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t Humid:%s\n",A_JSONTree_Value[1]);

	T_Pointer_Head = os_strstr(B_JSON_Tree, ":") + 18;			// 【值的首指针】
	T_Pointer_end = os_strstr(B_JSON_Tree, ":") + 20;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[2], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[2][T_Value_Len] = '\0';							// 【值后添'\0'】
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
	os_printf("\r\n-------------------- 解析JSON树thre 成功 -------------------\r\n");
	return 0;
}
//===================================================================================================

// 解析JSON树
//===================================================================================================
int ICACHE_FLASH_ATTR Parse_JSON_Tree_JX_WARN(void)
{
	os_printf("\r\n-------------------- 解析JSON树WARN -------------------\r\n");

	char A_JSONTree_Value[5][32] = {0};	// JSON数据缓存数组

	char * T_Pointer_Head = NULL;		// 临时指针
	char * T_Pointer_end = NULL;		// 临时指针

	u8 T_Value_Len = 0;					// 【"值"】的长度



//	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "{");				// 【{】
//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"temp\"");			// 【"temp"】
	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 1;			// 【值的首指针】
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 4;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[0], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[0][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t temp: %s\n",A_JSONTree_Value[0]);

//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"Humid\"");			// 【"Humid"】
	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 10;			// 【值的首指针】
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 12;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[1], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t Humid:%s\n",A_JSONTree_Value[1]);

	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 18;			// 【值的首指针】
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 21;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[2], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[2][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t CO2:%s\n",A_JSONTree_Value[2]);

	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 28;			// 【值的首指针】
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 30;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[3], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t MHMH:%s\n",A_JSONTree_Value[3]);

	T_Pointer_Head = os_strstr(C_JSON_Tree, ":") + 35;			// 【值的首指针】
	T_Pointer_end = os_strstr(C_JSON_Tree, ":") + 38;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[4], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// 【值后添'\0'】
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
	os_printf("\r\n-------------------- 解析JSON树WARN 成功 -------------------\r\n");
	return 0;
}
//===================================================================================================



// 解析JSON树
//===================================================================================================
int ICACHE_FLASH_ATTR Parse_JSON_Tree_JX(void)
{
	os_printf("\r\n-------------------- 解析JSON树 SENERS-------------------\r\n");

	char A_JSONTree_Value[5][32] = {0};	// JSON数据缓存数组

	char * T_Pointer_Head = NULL;		// 临时指针
	char * T_Pointer_end = NULL;		// 临时指针

	u8 T_Value_Len = 0;					// 【"值"】的长度



//	T_Pointer_Head = os_strstr(T_Pointer_Head, ":");				// 【:】
//	T_Pointer_Head = os_strstr(T_Pointer_Head, "{");				// 【{】
//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"temp\"");			// 【"temp"】
	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 9;			// 【值的首指针】
	T_Pointer_end = os_strstr(A_JSON_Tree, ".");			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[0], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[0][T_Value_Len] = '\0';							// 【值后添'\0'】
	os_printf("\t temp: %s\n",A_JSONTree_Value[0]);

//	T_Pointer_Head = os_strstr(A_JSON_Tree, "\"Humid\"");			// 【"Humid"】
	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 22;			// 【值的首指针】
	T_Pointer_end = os_strstr(A_JSON_Tree, ".") + 13;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[1], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t Humid:%s\n",A_JSONTree_Value[1]);

	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 33;			// 【值的首指针】
	T_Pointer_end = os_strstr(A_JSON_Tree, ":") + 36;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[2], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[2][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t CO2:%s\n",A_JSONTree_Value[2]);

	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 44;			// 【值的首指针】
	T_Pointer_end = os_strstr(A_JSON_Tree, ":") + 46;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[3], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// 【值后添'\0'】
//	os_printf("\t MHMH:%s\n",A_JSONTree_Value[3]);

	T_Pointer_Head = os_strstr(A_JSON_Tree, ":") + 55;			// 【值的首指针】
	T_Pointer_end = os_strstr(A_JSON_Tree, ":") + 58;			// 【"值"尾的"】
	T_Value_Len = T_Pointer_end - T_Pointer_Head;					// 计算【值】的长度
	os_memcpy(A_JSONTree_Value[4], T_Pointer_Head, T_Value_Len);		// 获取【值】
	A_JSONTree_Value[1][T_Value_Len] = '\0';							// 【值后添'\0'】
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
	os_printf("\r\n-------------------- 解析JSON树 成功SENERS -------------------\r\n");
	return 0;
}
//===================================================================================================


// 毫秒延时函数
//===========================================
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{	for(;C_time>0;C_time--)
		os_delay_us(1000);
}
///===========================================

// SNTP定时函数：获取当前网络时间
//============================================================================
void sntpfn()
{
	u32_t ts = 0;                         //偏移时间

    ts = sntp_get_current_timestamp();		// 获取当前的偏移时间

    os_printf("current time : %s\n", sntp_get_real_time(ts));	// 获取真实时间

    if (ts == 0)		// 网络时间获取失败
    {
        os_printf("did not get a valid time from sntp server\n");
    }
    else //(ts != 0)	// 网络时间获取成功
    {
            os_timer_disarm(&sntp_timer);	// 关闭SNTP定时器

            MQTT_Connect(&mqttClient);		// 开始MQTT连接
    }
}
//============================================================================


// WIFI连接状态改变：参数 = wifiStatus
//============================================================================
void wifiConnectCb(uint8_t status)
{
	// 成功获取到IP地址
	//---------------------------------------------------------------------
    if(status == STATION_GOT_IP)
    {
    	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));

    	// 在官方例程的基础上，增加2个备用服务器
    	//---------------------------------------------------------------
    	sntp_setservername(0, "us.pool.ntp.org");	// 服务器_0【域名】
    	sntp_setservername(1, "ntp.sjtu.edu.cn");	// 服务器_1【域名】

    	ipaddr_aton("210.72.145.44", addr);	// 点分十进制 => 32位二进制
    	sntp_setserver(2, addr);					// 服务器_2【IP地址】
    	os_free(addr);								// 释放addr

    	sntp_init();	// SNTP初始化


        // 设置SNTP定时器[sntp_timer]
        //-----------------------------------------------------------
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntpfn, NULL);
        os_timer_arm(&sntp_timer, 1000, 1);		// 1s定时
    }

    // IP地址获取失败
	//----------------------------------------------------------------
    else
    {
          MQTT_Disconnect(&mqttClient);	// WIFI连接出错，TCP断开连接
    }
}
//============================================================================


// MQTT已成功连接：ESP8266发送【CONNECT】，并接收到【CONNACK】
//============================================================================
void mqttConnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;	// 获取mqttClient指针

    INFO("MQTT: Connected\r\n");

    // 【参数2：主题过滤器 / 参数3：订阅Qos】
    //-----------------------------------------------------------------
	MQTT_Subscribe(client, "SW_LED", 0);	// 订阅主题"SW_LED"，QoS=0
//	MQTT_Subscribe(client, "SW_LED", 1);
//	MQTT_Subscribe(client, "SW_LED", 2);

	TONE(100);
	TONE(100);
	// 【参数2：主题名 / 参数3：发布消息的有效载荷 / 参数4：有效载荷长度 / 参数5：发布Qos / 参数6：Retain】
	//-----------------------------------------------------------------------------------------------------------------------------------------
	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 0, 0);	// 向主题"SW_LED"发布"ESP8266_Online"，Qos=0、retain=0
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 1, 0);
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 2, 0);
}
//============================================================================

// MQTT成功断开连接
//============================================================================
void mqttDisconnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Disconnected\r\n");
}
//============================================================================

// MQTT成功发布消息
//============================================================================
void mqttPublishedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}
//============================================================================

// 【接收MQTT的[PUBLISH]数据】函数		【参数1：主题 / 参数2：主题长度 / 参数3：有效载荷 / 参数4：有效载荷长度】
//===============================================================================================================
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len+1);		// 申请【主题】空间
    char *dataBuf  = (char*)os_zalloc(data_len+1);		// 申请【有效载荷】空间


    MQTT_Client* client = (MQTT_Client*)args;	// 获取MQTT_Client指针


    os_memcpy(topicBuf, topic, topic_len);	// 缓存主题
    topicBuf[topic_len] = 0;				// 最后添'\0'

    os_memcpy(dataBuf, data, data_len);		// 缓存有效载荷
    dataBuf[data_len] = 0;					// 最后添'\0'

    INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);	// 串口打印【主题】【有效载荷】
    INFO("Topic_len1 = %d, Data_len = %d\r\n", topic_len, data_len);	// 串口打印【主题长度】【有效载荷长度】


// 【技小新】添加
//########################################################################################
    // 根据接收到的主题名/有效载荷，控制LED的亮/灭
    //-----------------------------------------------------------------------------------
    if( os_strcmp(topicBuf,"SW_LED") == 0 )			// 主题 == "SW_LED"
    {
		if(data_len>50){                 //传感器
			strcpy(A_JSON_Tree, dataBuf);
					Parse_JSON_Tree_JX();
			//os_printf("A_JSON_Tree:%s",A_JSON_Tree);
		}

		if(data_len<30&&data_len>20){   //阈值
					strcpy(B_JSON_Tree, dataBuf);
							Parse_JSON_Tree_JX_VALUE();
					//os_printf("B_JSON_Tree:%s",B_JSON_Tree);
				}
		if(data_len<50&&data_len>40){   //阈值
					strcpy(C_JSON_Tree, dataBuf);
							Parse_JSON_Tree_JX_WARN();
					os_printf("C_JSON_Tree:%s",C_JSON_Tree);
				}

    	if( os_strcmp(dataBuf,"water_man") == 0 )		// 有效载荷 == "water_man"
    	{
    		if(!AUTO)
    		{
    	        GPIO_OUTPUT_SET(GPIO_ID_PIN(5),1);		// 手动给水
    	        delay_ms(once_water_time);
    	        GPIO_OUTPUT_SET(GPIO_ID_PIN(5),0);
    	        os_printf("water_man");
    	        os_printf("AUTO:%d",AUTO);
    		}
    		else{
    		os_printf("receive water_man but auto=1");
    		os_printf("AUTO:%d",AUTO);}
    	}

    	if( os_strcmp(dataBuf,"wind_man") == 0 )		// 有效载荷 == "wind_man"
    	{
    		if(!AUTO)
    		{
    			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);		// 手动送风
    			delay_ms(once_wind_time);
    			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);
    		}
    	 }

    	if( os_strcmp(dataBuf,"light_man_up") == 0 )		// 有效载荷 == "light_man_up"
    	{
    		if(!AUTO)
    		{
    			if(led_duty>22222+led_delta) led_duty = 22200-led_delta;
    			    			if(led_duty<led_delta+100) led_duty = led_delta+100;
    			    			led_duty = led_duty - led_delta;


    			pwm_set_period(1000);
    			pwm_set_duty(led_duty,0);//设置 PWM 某个通道信号的占空比, duty 占空比的值, type当前要设置的 PWM 通道
    			pwm_start();//设置完成后，需要调用 pwm_start,PWM 开始
    			os_printf("\nled_duty:%d",led_duty);
    		}
    		os_printf("AUTO:%d",AUTO);
    	}

    	if( os_strcmp(dataBuf,"light_man_down") == 0 )		// 有效载荷 == "light_man_down"
    	{
    		if(!AUTO)
    		{
    			if(led_duty>22222-led_delta) led_duty = 22200-led_delta;
    			    			if(led_duty<led_delta+100) led_duty = led_delta+100;
    			    			led_duty = led_duty + led_delta;

    			pwm_set_period(1000);
    			pwm_set_duty(led_duty,0);//设置 PWM 某个通道信号的占空比, duty 占空比的值, type当前要设置的 PWM 通道
    			pwm_start();//设置完成后，需要调用 pwm_start,PWM 开始
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
			pwm_set_duty(22220,0);//设置 PWM 某个通道信号的占空比, duty 占空比的值, type当前要设置的 PWM 通道
			pwm_start();//设置完成后，需要调用 pwm_start,PWM 开始
    	}
    }
//########################################################################################


    os_free(topicBuf);	// 释放【主题】空间
    os_free(dataBuf);	// 释放【有效载荷】空间
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



// user_init：entry of user application, init user function here
//===================================================================================================================
void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);	// 串口波特率设为74880
    os_delay_us(60000);

    JSON_WARN = 0;                 //表示未接收过报警阈值
    AUTO = 0;                               //默认手动调节
    WARN = 0;                        //默认不进行报警
    JSON_VALUE=0;                 //表示是否接收到过传感器数值,0表示未接收
    JSON_THRE=0;                  //表示是否接受过调整阈值,0表示未接收
    led_duty = 10000;                 //led亮度，范围0~22222
	uint32 pwm_duty_init[3]={0};
	uint32 io_info[][3]={{PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO12,12},};  //12口接LED
	pwm_init(1000,pwm_duty_init,1,io_info);//初始化 PWM，1000周期,pwm_duty_init占空比,3通道数,io_info各通道的 GPIO 硬件参数
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,	FUNC_GPIO0);   //蜂鸣器
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U,	FUNC_GPIO5);   //水泵



//【技小新】添加
//###########################################################################
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// 风机	#
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);						// 将风机设置为关闭状态	#
	GPIO_OUTPUT_SET(GPIO_ID_PIN(5),0);						// 将水泵设置为关闭状态	#
//###########################################################################


    CFG_Load();	// 加载/更新系统参数【WIFI参数、MQTT参数】


    // 网络连接参数赋值：服务端域名【mqtt_test_jx.mqtt.iot.gz.baidubce.com】、网络连接端口【1883】、安全类型【0：NO_TLS】
	//-------------------------------------------------------------------------------------------------------------------
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);

	// MQTT连接参数赋值：客户端标识符【..】、MQTT用户名【..】、MQTT密钥【..】、保持连接时长【120s】、清除会话【1：clean_session】
	//----------------------------------------------------------------------------------------------------------------------------
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);

	// 设置遗嘱参数(如果云端没有对应的遗嘱主题，则MQTT连接会被拒绝)
	//--------------------------------------------------------------
//	MQTT_InitLWT(&mqttClient, "Will", "ESP8266_offline", 0, 0);


	// 设置MQTT相关函数
	//--------------------------------------------------------------------------------------------------
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);			// 设置【MQTT成功连接】函数的另一种调用方式
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);	// 设置【MQTT成功断开】函数的另一种调用方式
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);			// 设置【MQTT成功发布】函数的另一种调用方式
	MQTT_OnData(&mqttClient, mqttDataCb);					// 设置【接收MQTT数据】函数的另一种调用方式


	// 连接WIFI：SSID[..]、PASSWORD[..]、WIFI连接成功函数[wifiConnectCb]
	//--------------------------------------------------------------------------
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);


	INFO("\r\nSystem started ...\r\n");
}
//===================================================================================================================
