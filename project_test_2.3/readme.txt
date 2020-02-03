project_test_2.0 是以_MQTT_JX为蓝本复制而成
2.0在本版本将ESP8266的功能一分为二，从而增加二次开发的条理性。
2.x的这个项目系列是给另外一块ESP8266（下面称为实行板），主要实现的功能是，接受SW_LED主题（以及控制用的主题传入的控制信息）传入的各路传感器的数值，进行自动或者手动控制给风、给水、给光的操作。
其中，云端设备信息：


实例名：                                      iot_light_jx_                              
MQTT服务端域名：                    uudp5h2.mqtt.iot.gz.baidubce.com
MQTT服务端端口号：                 1883
MQTT密码（身份秘钥）：         bmCyRZ07ne2QZXvW
身份：                                         aviator_shuke
策略：                                         SW_LED，Will
MQTT客户端：
                 ①：iot_light_esp8266_01_jx              MQTT用户名：uudp5h2/iot_light_esp8266_01_jx
                 ②：iot_light_mqttfx_baidu_jx             MQTT用户名：uudp5h2/iot_light_mqttfx_baidu_jx

version = 2.3
2.1 加入了蜂鸣器响的主题关键字“B”，并且以IO4/IO5作为水风的控制IO，IO12作为LED调光的pwm口
2.2 解析了云端传来的JSON串，得到了各个传感器数据
2.3 解析了云端传来的警告阈值，自动阈值,并且发送warning，warning_cancel