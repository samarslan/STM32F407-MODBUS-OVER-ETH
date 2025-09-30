/*
 * spi_protocol.h
 *
 *  Created on: Sep 30, 2025
 *      Author: Samet Arslan
 */

#ifndef INC_SPI_PROTOCOL_H_
#define INC_SPI_PROTOCOL_H_

#include "stm32f4xx_hal.h"
#include "port.h"

/* ----------------------------------------------------------------
 * Types
 * ----------------------------------------------------------------*/
typedef enum {
    SPI_RESULT_OK = 0,
    SPI_RESULT_ERROR
} SPI_Result;

typedef enum {
    SPI_LED_GREEN,
    SPI_LED_ORANGE,
    SPI_LED_RED,
    SPI_LED_BLUE,
    SPI_LED_ALL
} SPI_LED_Color;

/* ----------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------*/

/* Initialize SPI protocol (CS pin, etc.) */
void SPI_Protocol_Init(void);

/* Control slave LEDs over SPI */
SPI_Result SPI_SetLED(SPI_LED_Color color, uint8_t state);

/* Query slave LED status */
SPI_Result SPI_GetLEDStatus(uint8_t *green, uint8_t *orange, uint8_t *red, uint8_t *blue);

/* Modbus integration hooks */
void SPI_HandleModbusCoilWrite(USHORT coilIndex, UCHAR value);
void SPI_HandleModbusCoilRead(USHORT coilIndex, UCHAR *value);

/* Special commands */
void SPI_TurnOffAllLEDs(void);
void SPI_TurnOnAllLEDs(void);

#endif /* INC_SPI_PROTOCOL_H_ */
