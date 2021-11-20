/*
 * 24xx265.h
 *
 *  Created on: Nov 19, 2021
 *      Author: @skuodi
 */
#include "stm32f1xx_hal.h"						//HAL library for the STM32 board in use
#include "stdint.h"								//in order to use uint8_t
extern I2C_HandleTypeDef hi2c2;					//handle for the i2c bus connected to the 24LC456

#ifndef INC_24LC265_H_
#define INC_24LC265_H_

void writeByte(uint16_t addr, uint8_t sent);
void readByte(uint16_t add, uint8_t* recv);
void writePage(uint8_t page, uint8_t* sent);
void readPage(uint8_t page, uint8_t* sent);
void pageErase(uint8_t page);

#endif /* INC_24LC265_H_ */
