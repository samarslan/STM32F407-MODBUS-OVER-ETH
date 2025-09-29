/* modbus_callbacks.c
 *
 *  Created on: Sep 27, 2025
 *      Author: Samet Arslan
 * FreeModbus application callbacks
 *
 * - 4 Coils (00001–00004) -> control LEDs
 * - 1 Holding Register (40001) -> counter
 */

#include "main.h"   /* contains LED macros and HAL include */
#include "mb.h"
#include "mbport.h"
#include <string.h> /* memset */
#include "modbus_callbacks.h"


/* ---- Modbus mapping ---- */
#define REG_HOLDING_START   1
#define REG_HOLDING_NREGS   2	// [0]=counter, [1]=temperature

#define REG_COIL_START      1
#define REG_COIL_NCOILS     4

#define REG_INPUT_START    1
#define REG_INPUT_NREGS    2

#define REG_DISC_START     1
#define REG_DISC_NDISCRETES 4
/* ---- Storage ---- */
static volatile USHORT usHoldingRegs[REG_HOLDING_NREGS];
static volatile UCHAR  ucCoils[REG_COIL_NCOILS]; /* each entry 0 or 1 */

static volatile USHORT usInputRegs[REG_INPUT_NREGS];
static volatile UCHAR  ucDiscreteInputs[REG_DISC_NDISCRETES];

/* ---- Helpers ---- */

static void Modbus_UpdateCounter(void){
	usHoldingRegs[0]++;
}

static void apply_led_from_coil(USHORT coilIndex, UCHAR val)
{
    /* coilIndex is 0-based index into ucCoils (0..3) */
    GPIO_PinState state = (val ? GPIO_PIN_SET : GPIO_PIN_RESET);
    switch (coilIndex) {
        case 0:
            HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN, state);
            break;
        case 1:
            HAL_GPIO_WritePin(LED_PORT, LED_ORANGE_PIN, state);
            break;
        case 2:
            HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, state);
            break;
        case 3:
            HAL_GPIO_WritePin(LED_PORT, LED_BLUE_PIN, state);
            break;
        default:
            /* nothing */
            break;
    }
}

void Modbus_InitCallbacks(void)
{
    /* initialise storage and apply LEDs to the current coil values (0) */
    memset(usHoldingRegs, 0, sizeof(usHoldingRegs));
    memset(ucCoils, 0, sizeof(ucCoils));
    for (USHORT i = 0; i < REG_COIL_NCOILS; i++) {
        apply_led_from_coil(i, ucCoils[i]);
    }
}

/* -------------------------------------------------------------------
 * Holding registers callback
 * usAddress is 1-based (FreeModbus calls with address +1 in many flows)
 * ------------------------------------------------------------------*/
eMBErrorCode
eMBRegHoldingCB(UCHAR *pucBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode)
{
    /* range check */
    if ((usAddress < REG_HOLDING_START) ||
        (usAddress + usNRegs - 1) > (REG_HOLDING_START + REG_HOLDING_NREGS - 1)) {
        return MB_ENOREG;
    }

    USHORT iRegIndex = (USHORT)(usAddress - REG_HOLDING_START);

    if (eMode == MB_REG_READ) {
        /* copy register(s) to buffer (big-endian: high byte first) */
        for (USHORT i = 0; i < usNRegs; i++) {
            USHORT val = usHoldingRegs[iRegIndex + i];

            /* If this is register 0, increment AFTER reading */
            if ((iRegIndex + i) == 0) {
            	Modbus_UpdateCounter();            }

            *pucBuffer++ = (UCHAR)(val >> 8);
            *pucBuffer++ = (UCHAR)(val & 0xFF);
        }
    } else if (eMode == MB_REG_WRITE) {
        /* write register(s) from buffer */
        for (USHORT i = 0; i < usNRegs; i++) {
            USHORT hi = (USHORT)(*pucBuffer++);
            USHORT lo = (USHORT)(*pucBuffer++);
            usHoldingRegs[iRegIndex + i] = (USHORT)((hi << 8) | lo);
        }
    } else {
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

/* -------------------------------------------------------------------
 * Coils callback
 * usAddress is 1-based
 * Note: Modbus coils are bit-packed in pucBuffer LSB-first.
 * ------------------------------------------------------------------*/
eMBErrorCode
eMBRegCoilsCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
    if ((usAddress < REG_COIL_START) ||
        (usAddress + usNCoils - 1) > (REG_COIL_START + REG_COIL_NCOILS - 1)) {
        return MB_ENOREG;
    }

    USHORT iCoilIndex = (USHORT)(usAddress - REG_COIL_START);

    if (eMode == MB_REG_READ) {
        /* pack coils into pucBuffer, LSB-first per byte */
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
        /* any remaining partial byte */
        if (bitPos != 0) {
            *pOut++ = currentByte;
        }
    } else if (eMode == MB_REG_WRITE) {
        /* unpack pucBuffer and write to coil storage and LEDs */
        UCHAR *pIn = pucBuffer;
        USHORT byteIndex = 0;
        UCHAR bitIndex = 0;

        for (USHORT i = 0; i < usNCoils; i++) {
            /* ensure we read correct byte */
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
    } else {
        return MB_ENOREG;
    }

    return MB_ENOERR;
}

/* -------------------------------------------------------------------
 * Input registers callback
 * usAddress is 1-based
 * ------------------------------------------------------------------*/
eMBErrorCode
eMBRegInputCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNRegs )
{
    if ((usAddress < REG_INPUT_START) ||
        (usAddress + usNRegs - 1) > (REG_INPUT_START + REG_INPUT_NREGS - 1)) {
        return MB_ENOREG;
    }

    USHORT iRegIndex = (USHORT)(usAddress - REG_INPUT_START);

    /* copy register(s) to buffer (big-endian) */
    for (USHORT i = 0; i < usNRegs; i++) {
        USHORT val = usInputRegs[iRegIndex + i];
        *pucBuffer++ = (UCHAR)(val >> 8);
        *pucBuffer++ = (UCHAR)(val & 0xFF);
    }

    return MB_ENOERR;
}

/* -------------------------------------------------------------------
 * Discrete inputs callback
 * usAddress is 1-based
 * Note: packed into pucBuffer LSB-first per byte
 * ------------------------------------------------------------------*/
eMBErrorCode
eMBRegDiscreteCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNDiscrete )
{
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

    /* flush last partial byte */
    if (bitPos != 0) {
        *pOut++ = currentByte;
    }

    return MB_ENOERR;
}


/* -------------------------------------------------------------------
 * Encapsulation: Holding Registers
 * ------------------------------------------------------------------*/
USHORT Modbus_GetHolding(USHORT index)
{
    if (index < REG_HOLDING_NREGS) {
        return usHoldingRegs[index];
    }
    return 0; /* out-of-range safe default */
}

void Modbus_SetHolding(USHORT index, USHORT value)
{
    if (index < REG_HOLDING_NREGS) {
        usHoldingRegs[index] = value;
    }
}

/* -------------------------------------------------------------------
 * Encapsulation: Coils
 * ------------------------------------------------------------------*/
UCHAR Modbus_GetCoil(USHORT index)
{
    if (index < REG_COIL_NCOILS) {
        return ucCoils[index];
    }
    return 0;
}

void Modbus_SetCoil(USHORT index, UCHAR value)
{
    if (index < REG_COIL_NCOILS) {
        ucCoils[index] = value ? 1 : 0;
        apply_led_from_coil(index, ucCoils[index]); /* sync LEDs */
    }
}

/* -------------------------------------------------------------------
 * Encapsulation: Input Registers
 * ------------------------------------------------------------------*/
USHORT Modbus_GetInput(USHORT index)
{
    if (index < REG_INPUT_NREGS) {
        return usInputRegs[index];
    }
    return 0;
}

void Modbus_SetInput(USHORT index, USHORT value)
{
    if (index < REG_INPUT_NREGS) {
        usInputRegs[index] = value;
    }
}

/* -------------------------------------------------------------------
 * Encapsulation: Discrete Inputs
 * ------------------------------------------------------------------*/
UCHAR Modbus_GetDiscrete(USHORT index)
{
    if (index < REG_DISC_NDISCRETES) {
        return ucDiscreteInputs[index];
    }
    return 0;
}

void Modbus_SetDiscrete(USHORT index, UCHAR value)
{
    if (index < REG_DISC_NDISCRETES) {
        ucDiscreteInputs[index] = value ? 1 : 0;
    }
}

