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
// 工程：	MQTT_Data-Visualization						//	注：在《MQTT_JX》例程上修改											//
//														//																		//
// 平台：	【技新电子】物联网开发板 ESP8266 V1.0		//	①：按照创建【物影子】时获取到的MQTT连接参数，修改<mqtt_config.h>	//
//														//																		//
// 功能：	①：设置【物影子】MQTT相关参数				//	②：将<dht11.c>添加到<modules>，将<dht11.h>添加到<include/modules>	//
//														//																		//
//			②：ESP8266接入百度云【物影子】				//	③：需注意，<dht11.c>中，须改为<#include modules/dht11.h>			//
//														//																		//
//			③：每5秒向【物影子】上报【温湿度数据】		//	④：在<mqtt.c>定时函数中，添加：每5秒向【物影子】上报【温湿度数据】	//
//														//																		//
//	版本：	V1.0										//	⑤：取消【订阅"SW_LED"】【向"SW_LED"发布消息】操作					//
//														//																		//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// 头文件
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

// 类型定义
//=================================
typedef unsigned long 		u32_t;
//=================================


// 宏定义
//==================================================================================
#define		ProjectName			"1.6"	// 工程名宏定义

#define		Sector_STA_INFO		0x8D			// 【STA参数】保存扇区
//==================================================================================

// 全局变量
//==================================================================================
struct station_config STA_INFO;		// 【STA】参数结构体

os_timer_t OS_Timer_IP;				// 软件定时器

struct ip_info ST_ESP8266_IP;		// 8266的IP信息
u8 ESP8266_IP[4];					// 8266的IP地址

u8 C_LED_Flash = 0;					// LED闪烁计次

MQTT_Client mqttClient;			// MQTT客户端_结构体【此变量非常重要】

static ETSTimer sntp_timer;		// SNTP定时器

//============================================================================

/*
//数据转字符串
//=============================================================================
void ICACHE_FLASH_ATTR data2str()
{
	u8 C_char = 0;		// 字符计数
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

// GPIO中断函数【注意：中断函数前不要有"ICACHE_FLASH_ATTR"宏】
//=============================================================================
void GPIO_INTERRUPT(void)
{
	u32	S_GPIO_INT;		// 所有IO口的中断状态
	u32 F_GPIO_0_INT;	// GPIO_0的中断状态


	// 读取GPIO中断状态
	//---------------------------------------------------
	S_GPIO_INT = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	// 清除中断状态位(如果不清除状态位，则会持续进入中断)
	//----------------------------------------------------
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, S_GPIO_INT);


	F_GPIO_0_INT = S_GPIO_INT & (0x01<<0);	// 获取GPIO_0中断状态


	// 判断是否是KEY中断(未做消抖)
	//------------------------------------------------------------
	if(F_GPIO_0_INT)	// GPIO_0的下降沿中断
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
    		OLED_ShowString(48,4,DHT11_Data_Char[0]);	// DHT11_Data_Char[0] == 【湿度字符串】
    		OLED_ShowString(48,6,DHT11_Data_Char[1]);	// DHT11_Data_Char[1] == 【温度字符串】
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

// SmartConfig状态发生改变时，进入此回调函数
//--------------------------------------------
// 参数1：sc_status status / 参数2：无类型指针【在不同状态下，[void *pdata]的传入参数是不同的】
//=================================================================================================================
void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata)
{
	os_printf("\r\n------ smartconfig_done ------\r\n");	// ESP8266网络状态改变

    switch(status)
    {
    	// CmartConfig等待
    	//……………………………………………………
        case SC_STATUS_WAIT:		// 初始值
            os_printf("\r\nSC_STATUS_WAIT\r\n");
        break;
        //……………………………………………………

        // 发现【WIFI信号】（8266在这种状态下等待配网）
        //…………………………………………………………………………………………………
        case SC_STATUS_FIND_CHANNEL:
            os_printf("\r\nSC_STATUS_FIND_CHANNEL\r\n");

    		os_printf("\r\n---- Please Use WeChat to SmartConfig ------\r\n\r\n");

    		OLED_ShowString(0,2,"Use WeChat        ");
//    		OLED_ShowString(0,6,"SmartConfig     ");
    	break;
    	//…………………………………………………………………………………………………

        // 正在获取【SSID】【PSWD】（8266正在抓取并解密【SSID+PSWD】）
        //…………………………………………………………………………………………………
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("\r\nSC_STATUS_GETTING_SSID_PSWD\r\n");

            // 【SC_STATUS_GETTING_SSID_PSWD】状态下，参数2==SmartConfig类型指针
            //-------------------------------------------------------------------
			sc_type *type = pdata;		// 获取【SmartConfig类型】指针

			// 配网方式 == 【ESPTOUCH】
			//-------------------------------------------------
            if (*type == SC_TYPE_ESPTOUCH)
            { os_printf("\r\nSC_TYPE:SC_TYPE_ESPTOUCH\r\n"); }

            // 配网方式 == 【AIRKISS】||【ESPTOUCH_AIRKISS】
            //-------------------------------------------------
            else
            { os_printf("\r\nSC_TYPE:SC_TYPE_AIRKISS\r\n"); }

	    break;
	    //…………………………………………………………………………………………………

        // 成功获取到【SSID】【PSWD】，保存STA参数，并连接WIFI
	    //…………………………………………………………………………………………………
        case SC_STATUS_LINK:
            os_printf("\r\nSC_STATUS_LINK\r\n");

            // 【SC_STATUS_LINK】状态下，参数2 == STA参数结构体指针
            //------------------------------------------------------------------
            struct station_config *sta_conf = pdata;	// 获取【STA参数】指针

            // 将【SSID】【PASS】保存到【外部Flash】中
            //--------------------------------------------------------------------------
			spi_flash_erase_sector(Sector_STA_INFO);						// 擦除扇区
			spi_flash_write(Sector_STA_INFO*4096, (uint32 *)sta_conf, 96);	// 写入扇区
			//--------------------------------------------------------------------------

			wifi_station_set_config(sta_conf);			// 设置STA参数【Flash】
	        wifi_station_disconnect();					// 断开STA连接
	        wifi_station_connect();						// ESP8266连接WIFI

	    	OLED_ShowString(0,2,"WIFI Connecting ");	// OLED显示：
	    	//OLED_ShowString(0,6,"................");	// 正在连接WIFI

	    break;
	    //…………………………………………………………………………………………………


        // ESP8266作为STA，成功连接到WIFI
	    //…………………………………………………………………………………………………
        case SC_STATUS_LINK_OVER:
            os_printf("\r\nSC_STATUS_LINK_OVER\r\n");

            smartconfig_stop();		// 停止SmartConfig，释放内存

            //**************************************************************************************************

            wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取8266_STA的IP地址

			ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
			ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
			ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
			ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

			// 显示ESP8266的IP地址
			//-----------------------------------------------------------------------------------------------
//			os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
//			OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
//			OLED_ShowString(0,4,"Connect to WIFI ");
//			OLED_ShowString(0,6,"Successfully    ");
			//-----------------------------------------------------------------------------------------------

			// 接入WIFI成功后，LED快闪3次
			//----------------------------------------------------
			for(; C_LED_Flash<=5; C_LED_Flash++)
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
				delay_ms(100);
			}

			os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");

			//*****************************************************
			// WIFI连接成功，执行后续功能。	如：SNTP/UDP/TCP/DNS等
			//*****************************************************

			//**************************************************************************************************

	    break;
	    //…………………………………………………………………………………………………

    }
}
//=================================================================================================================


// IP定时检查的回调函数
//========================================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_cb(void)
{
	u8 S_WIFI_STA_Connect;			// WIFI接入状态标志


	// 查询STA接入WIFI状态
	//-----------------------------------------------------
	S_WIFI_STA_Connect = wifi_station_get_connect_status();
	//---------------------------------------------------
	// Station连接状态表
	// 0 == STATION_IDLE -------------- STATION闲置
	// 1 == STATION_CONNECTING -------- 正在连接WIFI
	// 2 == STATION_WRONG_PASSWORD ---- WIFI密码错误
	// 3 == STATION_NO_AP_FOUND ------- 未发现指定WIFI
	// 4 == STATION_CONNECT_FAIL ------ 连接失败
	// 5 == STATION_GOT_IP ------------ 获得IP，连接成功
	//---------------------------------------------------


	// 成功接入WIFI
	//----------------------------------------------------------------------------------------
	if( S_WIFI_STA_Connect == STATION_GOT_IP )	// 判断是否获取IP
	{
		wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 获取8266_STA的IP地址

		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

		// 显示ESP8266的IP地址
		//-----------------------------------------------------------------------------------------------
		os_printf("ESP8266_IP = %d.%d.%d.%d\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
//		OLED_ShowIP(24,2,ESP8266_IP);	// OLED显示ESP8266的IP地址
//		OLED_ShowString(0,4,"Connect to WIFI ");
//		OLED_ShowString(0,6,"Successfully    ");
		//-----------------------------------------------------------------------------------------------

		// 接入WIFI成功后，LED快闪3次
		//----------------------------------------------------
		for(; C_LED_Flash<=5; C_LED_Flash++)
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),(C_LED_Flash%2));
			delay_ms(100);
		}

		os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");

		os_timer_disarm(&OS_Timer_IP);	// 关闭定时器

		//*****************************************************
		// WIFI连接成功，执行后续功能。	如：SNTP/UDP/TCP/DNS等
		//*****************************************************
	}

	// ESP8266无法连接WIFI
	//------------------------------------------------------------------------------------------------
	else if(	S_WIFI_STA_Connect==STATION_NO_AP_FOUND 	||		// 未找到指定WIFI
				S_WIFI_STA_Connect==STATION_WRONG_PASSWORD 	||		// WIFI密码错误
				S_WIFI_STA_Connect==STATION_CONNECT_FAIL		)	// 连接WIFI失败
	{
		os_timer_disarm(&OS_Timer_IP);			// 关闭定时器

		os_printf("\r\n---- S_WIFI_STA_Connect=%d-----------\r\n",S_WIFI_STA_Connect);
		os_printf("\r\n---- ESP8266 Can't Connect to WIFI-----------\r\n");

		// 微信智能配网设置
		//…………………………………………………………………………………………………………………………
		//wifi_set_opmode(STATION_MODE);			// 设为STA模式						//【第①步】

		smartconfig_set_type(SC_TYPE_AIRKISS); 	// ESP8266配网方式【AIRKISS】			//【第②步】

		smartconfig_start(smartconfig_done);	// 进入【智能配网模式】,并设置回调函数	//【第③步】
		//…………………………………………………………………………………………………………………………
	}
}
//========================================================================================================


// 软件定时器初始化(ms毫秒)
//========================================================================================
void ICACHE_FLASH_ATTR OS_Timer_IP_Init_JX(u32 time_ms, u8 time_repetitive)
{

	os_timer_disarm(&OS_Timer_IP);	// 关闭定时器
	os_timer_setfn(&OS_Timer_IP,(os_timer_func_t *)OS_Timer_IP_cb, NULL);	// 设置定时器
	os_timer_arm(&OS_Timer_IP, time_ms, time_repetitive);  // 使能定时器
}

// SNTP定时函数：获取当前网络时间
//============================================================================
void sntpfn()
{
	// 字符串整理 相关变量
	//------------------------------------------------------

	u8 C_Str = 0;				// 字符串字节计数

	char A_Str_Data[20] = {0};	// 【"日期"】字符串数组

	char *T_A_Str_Data = A_Str_Data;	// 缓存数组指针

	char A_Str_Clock[10] = {0};	// 【"时间"】字符串数组


	char * Str_Head_Week;		// 【"星期"】字符串首地址

	char * Str_Head_Month;		// 【"月份"】字符串首地址

	char * Str_Head_Day;		// 【"日数"】字符串首地址

	char * Str_Head_Clock;		// 【"时钟"】字符串首地址

	char * Str_Head_Year;		// 【"年份"】字符串首地址

    u32_t ts = 0;
    u32_t TimeStamp=0;

    char * Str_RealTime;	// 实际时间的字符串

    ts = sntp_get_current_timestamp();		// 获取当前的偏移时间

    os_printf("current time : %s\n", sntp_get_real_time(ts));	// 获取真实时间

    if (ts == 0)		// 网络时间获取失败
    {
        os_printf("did not get a valid time from sntp server\n");
    }
    else //(ts != 0)	// 网络时间获取成功
    {
            os_timer_disarm(&sntp_timer);	// 关闭SNTP定时器
            //os_timer_disarm(&OS_Timer_SNTP);	// 关闭SNTP定时器
            TimeStamp = ts;
            		 // 查询实际时间(GMT+8):东八区(北京时间)
            		 //--------------------------------------------
            		 Str_RealTime = sntp_get_real_time(TimeStamp);


            		 // 【实际时间】字符串 == "周 月 日 时:分:秒 年"
            		 //------------------------------------------------------------------------
            		 os_printf("\r\n----------------------------------------------------\r\n");
            		 os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		// 时间戳
            		 os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// 实际时间
            		 os_printf("--------------------------------------------------------\r\n");


            		 // 时间字符串整理，OLED显示【"日期"】、【"时间"】字符串
            		 //…………………………………………………………………………………………………………………

            		 // 【"年份" + ' '】填入日期数组
            		 //---------------------------------------------------------------------------------
            		 Str_Head_Year = Str_RealTime;	// 设置起始地址

            		 while( *Str_Head_Year )		// 找到【"实际时间"】字符串的结束字符'\0'
            			 Str_Head_Year ++ ;

            		 // 【注：API返回的实际时间字符串，最后还有一个换行符，所以这里 -5】
            		 //-----------------------------------------------------------------
            		 Str_Head_Year -= 5 ;			// 获取【"年份"】字符串的首地址

            		 T_A_Str_Data[4] = ' ' ;
            		 os_memcpy(T_A_Str_Data, Str_Head_Year, 4);		// 【"年份" + ' '】填入日期数组

            		 T_A_Str_Data += 5;				// 指向【"年份" + ' '】字符串的后面的地址
            		 //---------------------------------------------------------------------------------

            		 // 获取【日期】字符串的首地址
            		 //---------------------------------------------------------------------------------
            		 Str_Head_Week 	= Str_RealTime;							// "星期" 字符串的首地址
            		 Str_Head_Month = os_strstr(Str_Head_Week,	" ") + 1;	// "月份" 字符串的首地址
            		 Str_Head_Day 	= os_strstr(Str_Head_Month,	" ") + 1;	// "日数" 字符串的首地址
            		 Str_Head_Clock = os_strstr(Str_Head_Day,	" ") + 1;	// "时钟" 字符串的首地址


            		 // 【"月份" + ' '】填入日期数组
            		 //---------------------------------------------------------------------------------
            		 C_Str = Str_Head_Day - Str_Head_Month;				// 【"月份" + ' '】的字节数

            		 os_memcpy(T_A_Str_Data, Str_Head_Month, C_Str);	// 【"月份" + ' '】填入日期数组

            		 T_A_Str_Data += C_Str;		// 指向【"月份" + ' '】字符串的后面的地址


            		 // 【"日数" + ' '】填入日期数组
            		 //---------------------------------------------------------------------------------
            		 C_Str = Str_Head_Clock - Str_Head_Day;				// 【"日数" + ' '】的字节数

            		 os_memcpy(T_A_Str_Data, Str_Head_Day, C_Str);		// 【"日数" + ' '】填入日期数组

            		 T_A_Str_Data += C_Str;		// 指向【"日数" + ' '】字符串的后面的地址


            		 // 【"星期" + ' '】填入日期数组
            		 //---------------------------------------------------------------------------------
            		 C_Str = Str_Head_Month - Str_Head_Week - 1;		// 【"星期"】的字节数

            		 os_memcpy(T_A_Str_Data, Str_Head_Week, C_Str);		// 【"星期"】填入日期数组

            		 T_A_Str_Data += C_Str;		// 指向【"星期"】字符串的后面的地址


            		 // OLED显示【"日期"】、【"时钟"】字符串
            		 //---------------------------------------------------------------------------------
            		 *T_A_Str_Data = '\0';		// 【"日期"】字符串后面添加'\0'

            		 OLED_ShowString(0,0,A_Str_Data);		// OLED显示日期


            		 os_memcpy(A_Str_Clock, Str_Head_Clock, 8);		// 【"时钟"】字符串填入时钟数组
            		 A_Str_Clock[8] = '\0';

            		 //OLED_ShowString(64,2,A_Str_Clock);		// OLED显示时间

            		 //…………………………………………………………………………………………………………………
            	 }

            MQTT_Connect(&mqttClient);		// 开始MQTT连接

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
//	MQTT_Subscribe(client, "SW_LED", 0);	// 订阅主题"SW_LED"，QoS=0
//	MQTT_Subscribe(client, "SW_LED", 1);
//	MQTT_Subscribe(client, "SW_LED", 2);

	// 【参数2：主题名 / 参数3：发布消息的有效载荷 / 参数4：有效载荷长度 / 参数5：发布Qos / 参数6：Retain】
	//-----------------------------------------------------------------------------------------------------------------------------------------
//	MQTT_Publish(client, "SW_LED", "ESP8266_Online", strlen("ESP8266_Online"), 0, 0);	// 向主题"SW_LED"发布"ESP8266_Online"，Qos=0、retain=0
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


// 【技小新】添加
//########################################################################################
	// 根据接收到的主题名/有效载荷，控制LED的亮/灭
	//-----------------------------------------------------------------------------------
	if( os_strcmp(topicBuf,"SW_LED") == 0 )			// 主题 == "SW_LED"
	{
		if( os_strcmp(dataBuf,"LED_ON") == 0 )		// 有效载荷 == "LED_ON"
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);		// LED亮
		}

		else if( os_strcmp(dataBuf,"LED_OFF") == 0 )	// 有效载荷 == "LED_OFF"
		{
			GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);			// LED灭
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
    uart_init(BIT_RATE_74880, BIT_RATE_74880);	// 串口波特率设为74880
    os_delay_us(60000);

    pages = 0;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,	FUNC_GPIO0);	// GPIO_0作为GPIO口
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));       // GPIO_0失能输出(默认)
	ETS_GPIO_INTR_DISABLE();										// 关闭GPIO中断功能
	ETS_GPIO_INTR_ATTACH((ets_isr_t)GPIO_INTERRUPT,NULL);			// 注册中断回调函数
	gpio_pin_intr_state_set(GPIO_ID_PIN(0),GPIO_PIN_INTR_NEGEDGE);	// GPIO_0下降沿中断
	ETS_GPIO_INTR_ENABLE();


    OLED_Init();
    SGP30_Init();
//【技小新】添加
//###########################################################################
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U,	FUNC_GPIO4);	// GPIO4输出高	#
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,	FUNC_GPIO12);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(12),1);						// LED初始化	#
//###########################################################################


    CFG_Load();	// 加载/更新系统参数【WIFI参数、MQTT参数】

    OS_Timer_IP_Init_JX(1000,1);

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
