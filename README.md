
# MemorEE(PROM) 
[![MIT License](https://img.shields.io/badge/license-MIT-red.svg)](https://choosealicense.com/licenses/mit/)


A HAL-based library for interfacing Microchip's 24xx256 (24AA256/24LC256/24FC256) Electrically Erasable Programmable Read-Only Memory (EEPROM) IC with STM32 via I2C.

## Features

- Read/Write individual bytes
- Read/Write an entire page
- Erase an entire page in one command


## Installation

1. Copy the 24xx256.h file into 
```
    [Your Project Folder] > Core > Inc 
```

2. Copy the 24xx256.c file into 
```
    [Your Project Folder] > Core > Src
```

3. Line 7 of 24xx256.h specifies the HAL library for the STM32 board in use.
    Change it as per your hardware.

    ```c
    #include "stm32f1xx_hal.h"                      //HAL library for the bluepill(STM32F103C8)
    ```

4. The next line specifies the handle of the I2C bus in use. Change it as per your schematic.
    
    ```c
    extern I2C_HandleTypeDef hi2c2;                 //handle for the i2c2 bus connected to my 24LC256
    ```

## API Reference

#### writeByte

Takes a single byte and writes it to the specified memory location.

```c
void writeByte(addr, sent)
```

| Parameter | Type      | Description                
| :-------- | :-------  | :------------------------- 
| `addr`    | `uint16_t`| Address of the memory location to write to.
| `sent`    | `uint8_t` | The data to be written.

#### readByte

Reads a single byte from the specified memory location into [recv].

```c
void readByte(addr, recv)
```

| Parameter | Type      | Description                
| :-------- | :-------  | :------------------------- 
| `addr`    | `uint16_t`| Address of the memory location to read from.
| `recv`    | `uint8_t` | Holds data received from the EEPROM.


####  writePage

Takes 64 bytes of data and writes them to the specified page.

```c
void writePage(page, sent)
```

| Parameter | Type      | Description                
| :-------- | :-------  | :------------------------- 
| `page`    | `uint8_t` | Index of page to be written to (from 0 to 63).
| `sent`    | `uint8_t*`| Pointer to the array holding the data to be written.


### readPage

Reads 64 bytes from the specified page.

```c
void readPage(page, recv)
```

| Parameter | Type      | Description                
| :-------- | :-------  | :------------------------- 
| `page`    | `uint8_t` | Index of page to be read from (from 0 to 63).
| `recv`    | `uint8_t*`| Pointer to the array to hold the data received.


### pageErase

Erases the specified page by filling it with 1's.

```c
void pageErase(uint8_t page)
```

| Parameter | Type      | Description                
| :-------- | :-------  | :------------------------- 
| `page`    | `uint8_t` | Index of page to be erased.


## Usage/Examples
Performing a single-page read:
```c

    uint8_t rData[64] = {0};    //variable to hold received data

    readPage(1, rData);         //read from page one into the variable

```
- rData will be populated with data from page 1


Erasing the entire EEPROM:
```c
for(int i = 0;i < 512; i++){
    pageErase(i);
}
```
- The 24xx256 has 512 physical pages of memory.


## License

[MIT](https://choosealicense.com/licenses/mit/)

