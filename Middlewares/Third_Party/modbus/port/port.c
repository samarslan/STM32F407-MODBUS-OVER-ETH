#include "lwip/tcp.h"
#include "mbport.h"

/* ----------------------- TCP globals ----------------------- */
static struct tcp_pcb *mb_listener = NULL;
static struct tcp_pcb *mb_client   = NULL;
static struct pbuf    *rx_pbuf     = NULL;

/* Forward declarations for lwIP callbacks */
static err_t mb_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t mb_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

/* ----------------------- TCP functions ---------------------- */
BOOL xMBTCPPortInit(USHORT usTCPPort)
{
    mb_listener = tcp_new();
    if (mb_listener == NULL) return FALSE;

    if (tcp_bind(mb_listener, IP_ADDR_ANY, usTCPPort) != ERR_OK)
        return FALSE;

    mb_listener = tcp_listen(mb_listener);
    tcp_accept(mb_listener, mb_accept);

    return TRUE;
}

void vMBTCPPortClose(void)
{
    if (mb_listener) {
        tcp_close(mb_listener);
        mb_listener = NULL;
    }
}

void vMBTCPPortDisable(void)
{
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
    if (!rx_pbuf) return FALSE;

    *ppucMBTCPFrame = rx_pbuf->payload;
    *pusLength      = rx_pbuf->len;

    pbuf_free(rx_pbuf);
    rx_pbuf = NULL;

    return TRUE;
}

BOOL xMBTCPPortSendResponse(const UCHAR *pucMBTCPFrame, USHORT usLength)
{
    if (!mb_client) return FALSE;

    if (tcp_write(mb_client, pucMBTCPFrame, usLength, TCP_WRITE_FLAG_COPY) != ERR_OK)
        return FALSE;

    tcp_output(mb_client);
    return TRUE;
}

/* ----------------------- lwIP callbacks ---------------------- */
static err_t mb_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    (void)arg;
    (void)err;

    /* Only one client at a time */
    mb_client = newpcb;
    tcp_recv(mb_client, mb_recv);

    return ERR_OK;
}

static err_t mb_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    (void)arg;
    (void)tpcb;

    if (!p) {
        mb_client = NULL;  /* remote closed */
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    if (rx_pbuf) {
        pbuf_free(rx_pbuf);
    }
    rx_pbuf = p;

    tcp_recved(tpcb, p->tot_len);
    return ERR_OK;
}

/* ----------------------- Stubs for RTU/ASCII ---------------------- */
BOOL xMBPortEventInit(void) { return TRUE; }
BOOL xMBPortEventPost(eMBEventType eEvent) { (void)eEvent; return TRUE; }
BOOL xMBPortEventGet(eMBEventType *eEvent) { (void)eEvent; return FALSE; }

BOOL xMBPortSerialInit(UCHAR ucPort, ULONG ulBaudRate,
                       UCHAR ucDataBits, eMBParity eParity,
                       UCHAR ucStopBits) {
    (void)ucPort; (void)ulBaudRate; (void)ucDataBits; (void)eParity; (void)ucStopBits;
    return FALSE;
}
void vMBPortClose(void) {}
void xMBPortSerialClose(void) {}
void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable) { (void)xRxEnable; (void)xTxEnable; }
BOOL xMBPortSerialGetByte(CHAR * pucByte) { (void)pucByte; return FALSE; }
BOOL xMBPortSerialPutByte(CHAR ucByte) { (void)ucByte; return FALSE; }

BOOL xMBPortTimersInit(USHORT usTimeOut50us) { (void)usTimeOut50us; return TRUE; }
void xMBPortTimersClose(void) {}
void vMBPortTimersEnable(void) {}
void vMBPortTimersDisable(void) {}
void vMBPortTimersDelay(USHORT usTimeOutMS) { (void)usTimeOutMS; }
