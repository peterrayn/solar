/*
 * bh1750.h
 *
 *  Created on: Apr 25, 2026
 *      Author: MAGIC
 */

#ifndef INC_BH1750_H_
#define INC_BH1750_H_

#include "main.h"

#define BH1750_PORT hi2c2	/*使用端口*/

#define BH1750_ADDRESS 0x46		/*ADDRESS引脚接地时地址为0x46，接电源时地址为0xB8*/

#define BH1750_POW_OFF				0X00
#define BH1750_POW_ON				0X01
#define BH1750_POW_RST				0X07
#define BH1750_CONT_HI_RSLT_1		0X10	/*连续测量，1lx精度开始测量，周期120ms*/
#define BH1750_CONT_HI_RSLT_2		0X11	/*连续测量，0.5lx精度开始测量，周期120ms*/
#define BH1750_CONT_LOW_RSLT		0X13	/*连续测量，4lx精度开始测量，周期16ms*/
#define BH1750_ONE_HI_RSLT_1		0X20	/*一次测量，1lx精度开始测量，周期120ms，测量完后自动进入POWER DOWN*/
#define BH1750_ONE_HI_RSLT_2		0X21	/*一次测量，0.5lx精度开始测量，周期120ms，测量完后自动进入POWER DOWN*/
#define BH1750_ONE_LOW_RSLT			0X23	/*一次测量，4lx精度开始测量，周期16ms，测量完后自动进入POWER DOWN*/


HAL_StatusTypeDef BH1750_Init(void);

HAL_StatusTypeDef BH1750_WriteOpecode(uint8_t* pData, uint16_t size);
HAL_StatusTypeDef BH1750_ReadData(uint8_t* pData, uint16_t size);

#endif /* INC_BH1750_H_ */
