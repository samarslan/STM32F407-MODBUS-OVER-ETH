/*
 * spi_protocol.c
 *
 *  Created on: Sep 30, 2025
 *      Author: Samet Arslan
 */

/* spi_protocol.c - Bridge MCU (Modbus Slave / SPI Master)
 *
 * This module handles SPI communication with the slave MCU
 * and integrates with Modbus callbacks
 */

/* spi_protocol.c - Bridge MCU (Modbus Slave / SPI Master)
 *
 * This module handles SPI communication with the slave MCU
 * and integrates with Modbus callbacks
 */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include "spi_protocol.h"


/* External SPI handle - configure in CubeMX:
 * - Mode: Master
 * - Data Size: 8 bits
 * - Clock: 1-2 MHz (conservative for reliability)
 * - CPOL: Low, CPHA: 1 Edge
 * - NSS: Software managed (use GPIO for CS)
 */
extern SPI_HandleTypeDef hspi1; // Adjust to your SPI instance

/* Protocol definitions */
#define SPI_TX_BUFFER_SIZE  32
#define SPI_RX_BUFFER_SIZE  32
#define SPI_TIMEOUT_MS      100

/* Internal state */
static uint8_t txBuffer[SPI_TX_BUFFER_SIZE];
static uint8_t rxBuffer[SPI_RX_BUFFER_SIZE];

/* ----------------------------------------------------------------
 * Low-level SPI helpers
 * ----------------------------------------------------------------*/
static inline void SPI_CS_Low(void) {
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);
}

static inline void SPI_CS_High(void) {
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);
}

/* Send command and receive response */
static HAL_StatusTypeDef SPI_SendCommand(const char *cmd, char *response, uint16_t respSize) {
    HAL_StatusTypeDef status;
    uint16_t cmdLen = strlen(cmd);

    if (cmdLen >= SPI_TX_BUFFER_SIZE) {
        return HAL_ERROR;
    }

    /* Prepare TX buffer */
    memset(txBuffer, 0, SPI_TX_BUFFER_SIZE);
    memset(rxBuffer, 0, SPI_RX_BUFFER_SIZE);
    strcpy((char *)txBuffer, cmd);

    /* SPI transaction */
    SPI_CS_Low();
    HAL_Delay(1); // Small delay for slave to detect CS

    status = HAL_SPI_TransmitReceive(&hspi1, txBuffer, rxBuffer,
                                     SPI_TX_BUFFER_SIZE, SPI_TIMEOUT_MS);

    HAL_Delay(1); // Small delay before releasing CS
    SPI_CS_High();

    if (status == HAL_OK && response != NULL) {
        /* Copy response */
        strncpy(response, (char *)rxBuffer, respSize - 1);
        response[respSize - 1] = '\0';
    }

    return status;
}

/* ----------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------*/

void SPI_Protocol_Init(void) {
    /* Initialize CS pin high (inactive) */
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);
}

SPI_Result SPI_SetLED(SPI_LED_Color color, uint8_t state) {
    char cmd[16];
    char response[16];
    char colorChar;

    /* Map color to character */
    switch (color) {
        case SPI_LED_GREEN:  colorChar = 'G'; break;
        case SPI_LED_ORANGE: colorChar = 'O'; break;
        case SPI_LED_RED:    colorChar = 'R'; break;
        case SPI_LED_BLUE:   colorChar = 'B'; break;
        case SPI_LED_ALL:    colorChar = 'A'; break;
        default: return SPI_RESULT_ERROR;
    }

    /* Build command: LED:G1\n */
    snprintf(cmd, sizeof(cmd), "LED:%c%d\n", colorChar, state ? 1 : 0);

    /* Send command */
    if (SPI_SendCommand(cmd, response, sizeof(response)) != HAL_OK) {
        return SPI_RESULT_ERROR;
    }

    /* Check response */
    if (strncmp(response, "OK", 2) == 0) {
        return SPI_RESULT_OK;
    }

    return SPI_RESULT_ERROR;
}

SPI_Result SPI_GetLEDStatus(uint8_t *green, uint8_t *orange, uint8_t *red, uint8_t *blue) {
    char response[16];

    /* Send GET:LED command */
    if (SPI_SendCommand("GET:LED\n", response, sizeof(response)) != HAL_OK) {
        return SPI_RESULT_ERROR;
    }

    /* Parse response: STA:GORB\n where each letter is 0 or 1 */
    if (strncmp(response, "STA:", 4) != 0) {
        return SPI_RESULT_ERROR;
    }

    if (strlen(response) < 8) {
        return SPI_RESULT_ERROR;
    }

    /* Extract LED states */
    *green  = (response[4] == '1') ? 1 : 0;
    *orange = (response[5] == '1') ? 1 : 0;
    *red    = (response[6] == '1') ? 1 : 0;
    *blue   = (response[7] == '1') ? 1 : 0;

    return SPI_RESULT_OK;
}

/* ----------------------------------------------------------------
 * Modbus Integration
 * These functions should be called from modbus_callbacks.c
 * ----------------------------------------------------------------*/

/* Extended coil mapping:
 * Coils 0-3: Bridge LEDs (local)
 * Coils 4-7: Slave LEDs (via SPI)
 */
#define SPI_COIL_OFFSET 4

void SPI_HandleModbusCoilWrite(USHORT coilIndex, UCHAR value) {
    if (coilIndex < SPI_COIL_OFFSET || coilIndex >= SPI_COIL_OFFSET + 4) {
        return; // Not an SPI coil
    }

    SPI_LED_Color color;
    USHORT localIndex = coilIndex - SPI_COIL_OFFSET;

    switch (localIndex) {
        case 0: color = SPI_LED_GREEN; break;
        case 1: color = SPI_LED_ORANGE; break;
        case 2: color = SPI_LED_RED; break;
        case 3: color = SPI_LED_BLUE; break;
        default: return;
    }

    SPI_SetLED(color, value);
}

void SPI_HandleModbusCoilRead(USHORT coilIndex, UCHAR *value) {
    if (coilIndex < SPI_COIL_OFFSET || coilIndex >= SPI_COIL_OFFSET + 4) {
        return; // Not an SPI coil
    }

    uint8_t green, orange, red, blue;

    if (SPI_GetLEDStatus(&green, &orange, &red, &blue) == SPI_RESULT_OK) {
        USHORT localIndex = coilIndex - SPI_COIL_OFFSET;

        switch (localIndex) {
            case 0: *value = green; break;
            case 1: *value = orange; break;
            case 2: *value = red; break;
            case 3: *value = blue; break;
        }
    }
}

/* Special command handler for "turn off all LEDs" via holding register */
void SPI_TurnOffAllLEDs(void) {
    SPI_SetLED(SPI_LED_ALL, 0);
}

void SPI_TurnOnAllLEDs(void) {
    SPI_SetLED(SPI_LED_ALL, 1);
}
