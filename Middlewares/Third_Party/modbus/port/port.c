#include "lwip/tcp.h"
#include "mbport.h"
#include "main.h"

/* ----------------------- TCP globals ----------------------- */
static struct tcp_pcb *mb_listener = NULL;
static struct tcp_pcb *mb_client   = NULL;
static struct pbuf    *rx_pbuf     = NULL;

/* Forward declarations for lwIP callbacks */
static err_t mb_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t mb_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

static volatile BOOL         s_event_pending = FALSE;
static volatile eMBEventType s_event_queued  = EV_READY;

/* ----------------------- TCP functions ---------------------- */
BOOL xMBPortEventInit(void)
{
    __disable_irq();
    s_event_pending = FALSE;
    s_event_queued  = EV_READY;
    __enable_irq();
    return TRUE;
}

BOOL xMBPortEventPost(eMBEventType eEvent)
{
    DEBUG_PRINTF("%s -> post event=%d\r\n", __func__, eEvent);
    __disable_irq();
    s_event_queued  = eEvent;
    s_event_pending = TRUE;
    __enable_irq();

    return TRUE;
}

BOOL xMBPortEventGet(eMBEventType *eEvent)
{
    if (!eEvent) return FALSE;

    __disable_irq();
    if (s_event_pending) {
        *eEvent        = s_event_queued;
        s_event_pending = FALSE;
        __enable_irq();

        DEBUG_PRINTF(" -> got event=%d\r\n", *eEvent);
        return TRUE;
    }
    __enable_irq();
    return FALSE;
}

/* ----------------------- TCP functions ---------------------- */
BOOL xMBTCPPortInit(USHORT usTCPPort)
{
    DEBUG_PRINTF("%s\r\n", __func__);
    mb_listener = tcp_new();
    if (!mb_listener) return FALSE;

    if (tcp_bind(mb_listener, IP_ADDR_ANY, usTCPPort) != ERR_OK)
        return FALSE;

    mb_listener = tcp_listen(mb_listener);
    tcp_accept(mb_listener, mb_accept);

    DEBUG_PRINTF(" -> listening on TCP port %u\r\n", (unsigned)usTCPPort);
    return TRUE;
}

void vMBTCPPortClose(void)
{
    DEBUG_PRINTF("%s\r\n", __func__);
    if (mb_listener) {
        tcp_close(mb_listener);
        mb_listener = NULL;
    }
}

void vMBTCPPortDisable(void)
{
    DEBUG_PRINTF("%s\r\n", __func__);
    if (mb_client) {
        tcp_close(mb_client);
        mb_client = NULL;
    }
    if (rx_pbuf) {
        pbuf_free(rx_pbuf);
        rx_pbuf = NULL;
    }
}

BOOL xMBTCPPortGetRequest(UCHAR **ppucMBTCPFrame, USHORT *pusLength)
{
    DEBUG_PRINTF("%s\r\n", __func__);
    if (!rx_pbuf) return FALSE;

    *ppucMBTCPFrame = rx_pbuf->payload;
    *pusLength      = rx_pbuf->len;

    DEBUG_PRINTF(" -> handing out %u bytes\r\n", (unsigned)*pusLength);

    pbuf_free(rx_pbuf);
    rx_pbuf = NULL;
    return TRUE;
}

BOOL xMBTCPPortSendResponse(const UCHAR *pucMBTCPFrame, USHORT usLength)
{
    DEBUG_PRINTF("%s\r\n", __func__);
    if (!mb_client) {
        DEBUG_PRINTF(" -> no client\r\n");
        return FALSE;
    }

    if (tcp_write(mb_client, pucMBTCPFrame, usLength, TCP_WRITE_FLAG_COPY) != ERR_OK)
        return FALSE;

    tcp_output(mb_client);

    DEBUG_PRINTF(" -> sent %u bytes\r\n", (unsigned)usLength);
    return TRUE;
}

/* ----------------------- lwIP callbacks ---------------------- */
static err_t mb_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    DEBUG_PRINTF("%s\r\n", __func__);
    (void)arg; (void)err;

    mb_client = newpcb;
    tcp_recv(mb_client, mb_recv);

    DEBUG_PRINTF(" -> client connected\r\n");

    xMBPortEventPost(EV_READY);
    return ERR_OK;
}

static err_t mb_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    DEBUG_PRINTF("%s\r\n", __func__);
    (void)arg; (void)tpcb;

    if (!p) {
        mb_client = NULL;
        DEBUG_PRINTF(" -> remote closed\r\n");
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        DEBUG_PRINTF(" -> error %d\r\n", (int)err);
        return err;
    }

    if (rx_pbuf) pbuf_free(rx_pbuf);
    rx_pbuf = p;

    tcp_recved(tpcb, p->tot_len);

    DEBUG_PRINTF(" -> received %u bytes\r\n", (unsigned)p->tot_len);
    if (p->len >= 8) {
        uint8_t *b = (uint8_t*)p->payload;
        DEBUG_PRINTF(" MBAP: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                     b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
    }

    xMBPortEventPost(EV_FRAME_RECEIVED);
    return ERR_OK;
}

/* ----------------------- Stubs for serial/timer ---------------------- */
BOOL xMBPortSerialInit(UCHAR ucPort, ULONG ulBaudRate,
                       UCHAR ucDataBits, eMBParity eParity,
                       UCHAR ucStopBits) {
    DEBUG_PRINTF("%s\r\n", __func__);
    (void)ucPort; (void)ulBaudRate; (void)ucDataBits; (void)eParity; (void)ucStopBits;
    return FALSE;
}
void vMBPortClose(void) { DEBUG_PRINTF("%s\r\n", __func__); }
void xMBPortSerialClose(void) { DEBUG_PRINTF("%s\r\n", __func__); }
void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable) { DEBUG_PRINTF("%s\r\n", __func__); (void)xRxEnable; (void)xTxEnable; }
BOOL xMBPortSerialGetByte(CHAR * pucByte) { DEBUG_PRINTF("%s\r\n", __func__); (void)pucByte; return FALSE; }
BOOL xMBPortSerialPutByte(CHAR ucByte) { DEBUG_PRINTF("%s\r\n", __func__); (void)ucByte; return FALSE; }

BOOL xMBPortTimersInit(USHORT usTimeOut50us) { DEBUG_PRINTF("%s\r\n", __func__); (void)usTimeOut50us; return TRUE; }
void xMBPortTimersClose(void) { DEBUG_PRINTF("%s\r\n", __func__); }
void vMBPortTimersEnable(void) { DEBUG_PRINTF("%s\r\n", __func__); }
void vMBPortTimersDisable(void) { DEBUG_PRINTF("%s\r\n", __func__); }
void vMBPortTimersDelay(USHORT usTimeOutMS) { DEBUG_PRINTF("%s\r\n", __func__); (void)usTimeOutMS; }
