/*
 * modbus_callbacks.c
 *
 *  Created on: Sep 27, 2025
 *      Author: arsla
 */


#include "mb.h"
#include "mbport.h"

eMBErrorCode eMBRegHoldingCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    return MB_ENOREG; // no holding registers
}

eMBErrorCode eMBRegInputCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNRegs )
{
    return MB_ENOREG; // no input registers
}

eMBErrorCode eMBRegCoilsCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
    return MB_ENOREG; // no coils
}

eMBErrorCode eMBRegDiscreteCB( UCHAR *pucBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG; // no discrete inputs
}
