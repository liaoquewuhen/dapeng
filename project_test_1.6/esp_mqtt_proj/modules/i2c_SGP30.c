#include "modules/i2c_master.h"
#include "modules/i2c_SGP30.h"
#include "osapi.h"

void ICACHE_FLASH_ATTR SGP30_Init(void)
{
	i2c_master_gpio_init();
	i2c_master_init();
	SGP30_ad_write(0x20,0x03);
//	SGP30_ad_write(0x20,0x61);
//	SGP30_ad_write(0x01,0x00);
}

void ICACHE_FLASH_ATTR SGP30_ad_write(u8 a,u8 b)
{
	i2c_master_start();
	i2c_master_writeByte(SGP30_write);
	if(!i2c_master_checkAck()) {
				i2c_master_stop();
				os_printf("sent SGP30_write defeat");}
	i2c_master_writeByte(a);
	if(!i2c_master_checkAck()) {
				i2c_master_stop();
				os_printf("sent a defeat");}
	i2c_master_writeByte(b);
	if(!i2c_master_checkAck()) {
				i2c_master_stop();
				os_printf("sent b defeat");}
	i2c_master_stop();
	os_delay_us(100000);
}

u32 ICACHE_FLASH_ATTR SGP30_ad_read(void)
{
	u32 dat;
	u8 temp1;
	u8 temp2;
	u8 crc;
	i2c_master_start();
	i2c_master_writeByte(SGP30_read);
	if(!i2c_master_checkAck()) {
				//i2c_master_stop();
				os_printf("sent SGP30_read defeat");}
	temp1 = i2c_master_readByte();
	dat = temp1;
//	os_printf("/n/r 1.temp1=",temp1);
	dat <<= 8;
	i2c_master_send_ack();

	temp2 = i2c_master_readByte();
	dat += temp2;
//	os_printf("/n/r 1.temp2=",temp2);
	i2c_master_send_ack();
	crc = i2c_master_readByte();
//	os_printf("/n/r 1.crc=",crc);
//	crc = crc;
	dat <<= 8;
	dat += i2c_master_readByte();
	i2c_master_send_ack();
	dat <<= 8;
	dat += i2c_master_readByte();
	i2c_master_stop();
	return (dat);
}

/*
void ICACHE_FLASH_ATTR
BH1750_write(unsigned char Command){
	i2c_master_start();
	i2c_master_writeByte(BHT1750_address&0xfe);
		if(!i2c_master_checkAck()) {
			i2c_master_stop();
			return;
		}
		i2c_master_writeByte(Command);
		if(!i2c_master_checkAck()) {
			i2c_master_stop();
			return;
		}
		i2c_master_stop();
}

unsigned int ICACHE_FLASH_ATTR
BH1750_read(void){
	unsigned int temp;
	i2c_master_start();
	i2c_master_writeByte(BHT1750_address|0x01);
		if(!i2c_master_checkAck()) {
			i2c_master_stop();
		}
		temp=i2c_master_readByte();
		temp<<=8;
		i2c_master_send_ack();
		temp|=i2c_master_readByte();
		i2c_master_send_nack();
		i2c_master_stop();
		return temp;
}

*/
