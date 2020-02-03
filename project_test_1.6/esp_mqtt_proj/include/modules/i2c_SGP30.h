//#ifndef APP_INCLUDE_USER_DRIVER_I2C_BH1750_H_
//#define APP_INCLUDE_USER_DRIVER_I2C_BH1750_H_

//#define BHT1750_address 0x58
#include "c_types.h"
#define SGP30_read  0xb1
#define SGP30_write 0xb0

void ICACHE_FLASH_ATTR SGP30_Init(void);
void ICACHE_FLASH_ATTR SGP30_ad_write(u8 a,u8 b);
u32  ICACHE_FLASH_ATTR SGP30_ad_read(void);

/*
void BH1750_write(unsigned char Command);
unsigned int BH1750_read(void);
*/

//#endif /* APP_INCLUDE_USER_DRIVER_I2C_BH1750_H_ */
