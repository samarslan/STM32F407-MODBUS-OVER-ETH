/*
 * tcp_check.h
 *
 *  Created on: Sep 25, 2025
 *      Author: derslik3-07
 */

#ifndef INC_TCP_CHECK_H_
#define INC_TCP_CHECK_H_

#include "lwip/tcp.h"
#include "main.h"   // for HAL, DEBUG_PRINTF, huart4

#ifdef __cplusplus
extern "C" {
#endif

extern struct tcp_pcb *hello_tpcb;
extern ip_addr_t dest_ip;

void tcp_check_start_connection(void);
void tcp_check_send_hello(void);

err_t tcp_check_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t tcp_check_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
void  tcp_check_error(void *arg, err_t err);

#ifdef __cplusplus
}
#endif

#endif /* INC_TCP_CHECK_H_ */
