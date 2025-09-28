/*
 * modbus_callbacks.h
 *
 *  Created on: Sep 27, 2025
 *      Author: Samet Arslan
 */

#ifndef THIRD_PARTY_MODBUS_MODBUS_CALLBACKS_H_
#define THIRD_PARTY_MODBUS_MODBUS_CALLBACKS_H_

#include "mb.h"   /* for eMBErrorCode, UCHAR, USHORT */

/* Init and background update */
void Modbus_InitCallbacks(void);

/* Encapsulation: Holding Registers */
USHORT Modbus_GetHolding(USHORT index);
void   Modbus_SetHolding(USHORT index, USHORT value);

/* Encapsulation: Coils */
UCHAR  Modbus_GetCoil(USHORT index);
void   Modbus_SetCoil(USHORT index, UCHAR value);

/* Encapsulation: Input Registers */
USHORT Modbus_GetInput(USHORT index);
void   Modbus_SetInput(USHORT index, USHORT value);

/* Encapsulation: Discrete Inputs */
UCHAR  Modbus_GetDiscrete(USHORT index);
void   Modbus_SetDiscrete(USHORT index, UCHAR value);

#endif /* THIRD_PARTY_MODBUS_MODBUS_CALLBACKS_H_ */
