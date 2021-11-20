/*
 * 24xx256.c
 *
 *  Created on: Nov 19, 2021
 *      Author: @skuodi
 */
#include "24xx265.h"


#define DEV_ADDR 0b10100000						//the base address of the 24LC256

/**
 @brief This function writes a single byte of data to the specified memory address

 @param addr: The 2-byte memory address to be written to
 @param sent: The data to be written
 @retval None
 */
void writeByte(uint16_t addr, uint8_t sent){
	HAL_I2C_Mem_Write(&hi2c2, DEV_ADDR, addr, 2, &sent, 1, 5);
}

/**
 @brief This function reads a single byte of data from the specified memory address

 @param addr: The 2-byte memory address to be read from
 @param recv: Will hold data received from the 24LC256
 @retval None
 */
void readByte(uint16_t addr, uint8_t* recv){
	HAL_I2C_Mem_Write(&hi2c2, DEV_ADDR, addr, 2, recv, 1, 5);
}

/**
 @brief 	This function writes data to an entire page

 @param 	page: The page to be written to (from 0 - 63)
 @param 	sent: The 64 bytes of data to be written
 */

void writePage(uint8_t page, uint8_t* sent){
	HAL_I2C_Mem_Write(&hi2c2, DEV_ADDR, (page<<6), 2, sent, 64, 5);
}

/**
 @brief This function writes data to an entire page

 @param 	page: The page to be read from (0 - 63)
 @param 	recv: Holds the 64 bytes of data read
 @retval 	None
 */
void readPage(uint8_t page, uint8_t* recv){
	HAL_I2C_Mem_Read(&hi2c2, DEV_ADDR, (page<<6), 2, recv, 64, 5);
}

/**
 @brief 	This function erases a page of memory by filling it with 1's
 @retval	None
 */
void pageErase(uint8_t page){
	for(int i=0;i<64;i++){
		writeByte((page<<6)|i, 0b11111111);
	}
}
