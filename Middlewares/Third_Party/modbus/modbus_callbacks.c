/* modbus_callbacks.c - UPDATED WITH SPI INTEGRATION
 *
 * Created on: Sep 27, 2025
 * Author: Samet Arslan
 * Modified: Added SPI slave LED control via extended coils
 *
 * Modbus Mapping:
 * - Coils 00001–00004 (0-3) -> Bridge LEDs (local)
 * - Coils 00005–00008 (4-7) -> Slave LEDs (via SPI)
 * - Holding Register 40001 (0) -> counter
 * - Holding Register 40002 (1) -> command register
 *     Write 0xAA00 -> Turn OFF all slave LEDs
 *     Write 0xAA01 -> Turn ON all slave LEDs
 */

#include "main.h"
#include "mb.h"
#include "mbport.h"
#include <string.h>
#include "modbus_callbacks.h"
#include "spi_protocol.h" // Include SPI protocol header

/* ---- Modbus mapping ---- */
#define REG_HOLDING_START     1
#define REG_HOLDING_NREGS     2
#define REG_COIL_START        1
#define REG_COIL_NCOILS       8  // 4 local + 4 SPI
#define REG_INPUT_START       1
#define REG_INPUT_NREGS       2
#define REG_DISC_START        1
#define REG_DISC_NDISCRETES   4

/* Special command codes for holding register 1 */
#define CMD_SPI_ALL_OFF  0xAA00
#define CMD_SPI_ALL_ON   0xAA01

/* ---- Storage ---- */
static USHORT usHoldingRegs[REG_HOLDING_NREGS];
static UCHAR  ucCoils[REG_COIL_NCOILS];
static USHORT usInputRegs[REG_INPUT_NREGS];
static UCHAR  ucDiscreteInputs[REG_DISC_NDISCRETES];

/* ---- Helpers ---- */
static void Modbus_UpdateCounter(void) {
    usHoldingRegs[0]++;
}

static void apply_led_from_coil(USHORT coilIndex, UCHAR val) {
    if (coilIndex < 4) {
        /* Local bridge LEDs */
        GPIO_PinState state = (val ? GPIO_PIN_SET : GPIO_PIN_RESET);
        switch (coilIndex) {
            case 0: HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN, state); break;
            case 1: HAL_GPIO_WritePin(LED_PORT, LED_ORANGE_PIN, state); break;
            case 2: HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, state); break;
            case 3: HAL_GPIO_WritePin(LED_PORT, LED_BLUE_PIN, state); break;
        }
    }
    else if (coilIndex >= 4 && coilIndex < 8) {
        /* Forward SPI LED control */
        SPI_HandleModbusCoilWrite(coilIndex, val);
    }
}

void Modbus_InitCallbacks(void) {
    memset(usHoldingRegs, 0, sizeof(usHoldingRegs));
    memset(ucCoils, 0, sizeof(ucCoils));
    memset(usInputRegs, 0, sizeof(usInputRegs));
    memset(ucDiscreteInputs, 0, sizeof(ucDiscreteInputs));

    /* Init local LEDs OFF */
    for (USHORT i = 0; i < 4; i++) {
        apply_led_from_coil(i, 0);
    }

    /* Init SPI */
    SPI_Protocol_Init();

    /* Init SPI LEDs OFF */
    for (USHORT i = 4; i < 8; i++) {
        apply_led_from_coil(i, 0);
    }
}

/* ----------------------------------------------------------------
 * Holding registers callback
 * ----------------------------------------------------------------*/
eMBErrorCode eMBRegHoldingCB(UCHAR *pucBuffer, USHORT usAddress,
                             USHORT usNRegs, eMBRegisterMode eMode) {
    if ((usAddress < REG_HOLDING_START) ||
        (usAddress + usNRegs - 1) > (REG_HOLDING_START + REG_HOLDING_NREGS - 1)) {
        return MB_ENOREG;
    }

    USHORT iRegIndex = (USHORT)(usAddress - REG_HOLDING_START);

    if (eMode == MB_REG_READ) {
        for (USHORT i = 0; i < usNRegs; i++) {
            USHORT val = usHoldingRegs[iRegIndex + i];

            if ((iRegIndex + i) == 0) {
                Modbus_UpdateCounter(); // auto increment counter
            }

            *pucBuffer++ = (UCHAR)(val >> 8);
            *pucBuffer++ = (UCHAR)(val & 0xFF);
        }
    }
    else if (eMode == MB_REG_WRITE) {
        for (USHORT i = 0; i < usNRegs; i++) {
            USHORT hi = (USHORT)(*pucBuffer++);
            USHORT lo = (USHORT)(*pucBuffer++);
            USHORT val = (USHORT)((hi << 8) | lo);

            usHoldingRegs[iRegIndex + i] = val;

            /* Special commands in register 1 */
            if ((iRegIndex + i) == 1) {
                if (val == CMD_SPI_ALL_OFF) {
                    SPI_TurnOffAllLEDs();
                    for (USHORT j = 4; j < 8; j++) ucCoils[j] = 0;
                }
                else if (val == CMD_SPI_ALL_ON) {
                    SPI_TurnOnAllLEDs();
                    for (USHORT j = 4; j < 8; j++) ucCoils[j] = 1;
                }
            }
        }
    }
    else {
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

/* ----------------------------------------------------------------
 * Coils callback (8 coils: 0-3 local, 4-7 SPI)
 * ----------------------------------------------------------------*/
eMBErrorCode eMBRegCoilsCB(UCHAR *pucBuffer, USHORT usAddress,
                           USHORT usNCoils, eMBRegisterMode eMode) {
    if ((usAddress < REG_COIL_START) ||
        (usAddress + usNCoils - 1) > (REG_COIL_START + REG_COIL_NCOILS - 1)) {
        return MB_ENOREG;
    }

    USHORT iCoilIndex = (USHORT)(usAddress - REG_COIL_START);

    if (eMode == MB_REG_READ) {
        /* Refresh SPI coil states */
        if (iCoilIndex < 8 && (iCoilIndex + usNCoils) > 4) {
            uint8_t g, o, r, b;
            if (SPI_GetLEDStatus(&g, &o, &r, &b) == SPI_RESULT_OK) {
                ucCoils[4] = g;
                ucCoils[5] = o;
                ucCoils[6] = r;
                ucCoils[7] = b;
            }
        }

        /* Pack bits into buffer */
        UCHAR currentByte = 0;
        UCHAR bitPos = 0;
        UCHAR *pOut = pucBuffer;

        for (USHORT i = 0; i < usNCoils; i++) {
            UCHAR val = (ucCoils[iCoilIndex + i] & 0x01) ? 1 : 0;
            currentByte |= (val << bitPos);
            bitPos++;
            if (bitPos == 8) {
                *pOut++ = currentByte;
                currentByte = 0;
                bitPos = 0;
            }
        }
        if (bitPos != 0) {
            *pOut++ = currentByte;
        }
    }
    else if (eMode == MB_REG_WRITE) {
        /* Unpack and apply */
        UCHAR *pIn = pucBuffer;
        USHORT byteIndex = 0;
        UCHAR bitIndex = 0;

        for (USHORT i = 0; i < usNCoils; i++) {
            UCHAR byte = pIn[byteIndex];
            UCHAR bit = (byte >> bitIndex) & 0x01;

            ucCoils[iCoilIndex + i] = bit ? 1 : 0;
            apply_led_from_coil(iCoilIndex + i, ucCoils[iCoilIndex + i]);

            bitIndex++;
            if (bitIndex == 8) {
                bitIndex = 0;
                byteIndex++;
            }
        }
    }
    else {
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

/* ----------------------------------------------------------------
 * Input registers callback
 * ----------------------------------------------------------------*/
eMBErrorCode eMBRegInputCB(UCHAR *pucBuffer, USHORT usAddress, USHORT usNRegs) {
    if ((usAddress < REG_INPUT_START) ||
        (usAddress + usNRegs - 1) > (REG_INPUT_START + REG_INPUT_NREGS - 1)) {
        return MB_ENOREG;
    }

    USHORT iRegIndex = (USHORT)(usAddress - REG_INPUT_START);

    for (USHORT i = 0; i < usNRegs; i++) {
        USHORT val = usInputRegs[iRegIndex + i];
        *pucBuffer++ = (UCHAR)(val >> 8);
        *pucBuffer++ = (UCHAR)(val & 0xFF);
    }

    return MB_ENOERR;
}

/* ----------------------------------------------------------------
 * Discrete inputs callback
 * ----------------------------------------------------------------*/
eMBErrorCode eMBRegDiscreteCB(UCHAR *pucBuffer, USHORT usAddress, USHORT usNDiscrete) {
    if ((usAddress < REG_DISC_START) ||
        (usAddress + usNDiscrete - 1) > (REG_DISC_START + REG_DISC_NDISCRETES - 1)) {
        return MB_ENOREG;
    }

    USHORT iDiscIndex = (USHORT)(usAddress - REG_DISC_START);
    UCHAR currentByte = 0;
    UCHAR bitPos = 0;
    UCHAR *pOut = pucBuffer;

    for (USHORT i = 0; i < usNDiscrete; i++) {
        UCHAR val = (ucDiscreteInputs[iDiscIndex + i] & 0x01) ? 1 : 0;
        currentByte |= (val << bitPos);
        bitPos++;
        if (bitPos == 8) {
            *pOut++ = currentByte;
            currentByte = 0;
            bitPos = 0;
        }
    }

    if (bitPos != 0) {
        *pOut++ = currentByte;
    }

    return MB_ENOERR;
}

/* ----------------------------------------------------------------
 * Simple getters/setters for app logic
 * ----------------------------------------------------------------*/
USHORT Modbus_GetHolding(USHORT index) { return (index < REG_HOLDING_NREGS) ? usHoldingRegs[index] : 0; }
void   Modbus_SetHolding(USHORT index, USHORT value) { if (index < REG_HOLDING_NREGS) usHoldingRegs[index] = value; }
UCHAR  Modbus_GetCoil(USHORT index) { return (index < REG_COIL_NCOILS) ? ucCoils[index] : 0; }
void   Modbus_SetCoil(USHORT index, UCHAR value) { if (index < REG_COIL_NCOILS) { ucCoils[index] = value; apply_led_from_coil(index, value); } }
USHORT Modbus_GetInput(USHORT index) { return (index < REG_INPUT_NREGS) ? usInputRegs[index] : 0; }
void   Modbus_SetInput(USHORT index, USHORT value) { if (index < REG_INPUT_NREGS) usInputRegs[index] = value; }
UCHAR  Modbus_GetDiscrete(USHORT index) { return (index < REG_DISC_NDISCRETES) ? ucDiscreteInputs[index] : 0; }
void   Modbus_SetDiscrete(USHORT index, UCHAR value) { if (index < REG_DISC_NDISCRETES) ucDiscreteInputs[index] = value ? 1 : 0; }
