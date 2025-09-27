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

/* ---- Modbus mapping ---- */
#define REG_HOLDING_START   1
#define REG_HOLDING_NREGS   1

#define REG_COIL_START      1
#define REG_COIL_NCOILS     4

/* ---- Storage ---- */
static USHORT usHoldingRegs[REG_HOLDING_NREGS];
static UCHAR  ucCoils[REG_COIL_NCOILS]; /* each entry 0 or 1 */

/* ---- Helpers ---- */
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

/* Call from main loop/timer to increment the holding register */
void Modbus_UpdateCounter(void)
{
    usHoldingRegs[0]++; /* 16-bit wrap-around is fine */
}

/* -------------------------------------------------------------------
 * Holding registers callback
 * usAddress is 1-based (FreeModbus calls with address +1 in many flows)
 * ------------------------------------------------------------------*/
eMBErrorCode
eMBRegHoldingCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
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
 * No input registers in this example
 * ------------------------------------------------------------------*/
eMBErrorCode
eMBRegInputCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNRegs )
{
    (void)pucBuffer;
    (void)usAddress;
    (void)usNRegs;
    return MB_ENOREG;
}

/* -------------------------------------------------------------------
 * No discrete inputs in this example
 * ------------------------------------------------------------------*/
eMBErrorCode
eMBRegDiscreteCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    (void)pucBuffer;
    (void)usAddress;
    (void)usNDiscrete;
    return MB_ENOREG;
}
