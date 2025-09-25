/*
 * tcp_check.c
 *
 *  Created on: Sep 25, 2025
 *      Author: derslik3-07
 */


#include "tcp_check.h"
#include <string.h>

struct tcp_pcb *hello_tpcb = NULL;
ip_addr_t dest_ip;

err_t tcp_check_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    DEBUG_PRINTF("tcp_sent: acked %u bytes\r\n", (unsigned)len);
    return ERR_OK;
}

err_t tcp_check_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err == ERR_OK) {
        hello_tpcb = tpcb;
        tcp_sent(tpcb, tcp_check_sent);
        DEBUG_PRINTF("tcp_connected: OK (err=%d)\r\n", err);
    } else {
        DEBUG_PRINTF("tcp_connected: FAILED (err=%d)\r\n", err);
        hello_tpcb = NULL;
    }
    return err;
}

void tcp_check_error(void *arg, err_t err)
{
    DEBUG_PRINTF("tcp_error_callback: err=%d\r\n", err);
    hello_tpcb = NULL;
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); // Red LED
}

void tcp_check_start_connection(void)
{
    hello_tpcb = tcp_new();
    if (hello_tpcb != NULL) {
        IP4_ADDR(&dest_ip, 192,168,1,100); // adjust as needed
        DEBUG_PRINTF("start_tcp_connection: trying %s:%d\r\n",
                     ipaddr_ntoa(&dest_ip), 502);
        tcp_err(hello_tpcb, tcp_check_error);
        tcp_connect(hello_tpcb, &dest_ip, 502, tcp_check_connected);
    } else {
        DEBUG_PRINTF("start_tcp_connection: tcp_new failed!\r\n");
        Error_Handler();
    }
}

void tcp_check_send_hello(void)
{
    if (hello_tpcb == NULL) {
        DEBUG_PRINTF("send_hello_tcp: no active PCB\r\n");
        return;
    }

    const char *msg = "Hello World from STM32F407\r\n";
    u16_t msg_len = strlen(msg);

    if (tcp_sndbuf(hello_tpcb) < msg_len) {
        DEBUG_PRINTF("send_hello_tcp: not enough sndbuf (%d available, %d needed)\r\n",
                     tcp_sndbuf(hello_tpcb), msg_len);
        return;
    }

    err_t err = tcp_write(hello_tpcb, msg, msg_len, TCP_WRITE_FLAG_COPY);

    if (err == ERR_OK) {
        tcp_output(hello_tpcb);
        DEBUG_PRINTF("send_hello_tcp: sent OK (%d bytes)\r\n", msg_len);
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15); // Blue LED
    }
    else if (err == ERR_MEM) {
        DEBUG_PRINTF("send_hello_tcp: ERR_MEM, flushing\r\n");
        tcp_output(hello_tpcb);
    }
    else if (err == ERR_ABRT || err == ERR_RST || err == ERR_CLSD) {
        DEBUG_PRINTF("send_hello_tcp: connection closed (err=%d)\r\n", err);
        hello_tpcb = NULL;
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); // Red LED
    }
    else {
        DEBUG_PRINTF("send_hello_tcp: unexpected err=%d\r\n", err);
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13); // Orange LED
    }
}
