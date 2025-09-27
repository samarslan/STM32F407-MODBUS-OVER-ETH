#ifndef _PORT_H
#define _PORT_H

#include <stdint.h>
#include <stdbool.h>

/* ----------------------- Basic types ------------------------------------- */
#define BOOL    uint8_t
#define UCHAR   uint8_t
#define CHAR    int8_t
#define USHORT  uint16_t
#define SHORT   int16_t
#define ULONG   uint32_t
#define LONG    int32_t

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

/* ----------------------- Critical section handling ----------------------- */
/* These macros should disable/enable interrupts, or enter/exit mutex */
#define ENTER_CRITICAL_SECTION()   __disable_irq()
#define EXIT_CRITICAL_SECTION()    __enable_irq()

/* ----------------------- Compiler specifics ------------------------------ */
#define INLINE  inline
#define STATIC  static

/* ----------------------- Misc -------------------------------------------- */
#define MB_TCP_DEFAULT_PORT   502

#endif /* _PORT_H */

