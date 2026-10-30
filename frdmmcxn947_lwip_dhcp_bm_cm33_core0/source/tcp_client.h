#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "lwip/ip_addr.h"

/*!
 * @brief Inicia el cliente TCP (API raw). Intenta conectar y reintenta solo si falla.
 */
void tcp_client_init(const ip4_addr_t *server_ip, u16_t server_port);

/*!
 * @brief Debe llamarse en cada vuelta del while(1): lee la consola serie sin bloquear
 *        y, al recibir Enter, envia la linea al servidor.
 */
void tcp_client_poll_console(void);

#endif /* TCP_CLIENT_H */
