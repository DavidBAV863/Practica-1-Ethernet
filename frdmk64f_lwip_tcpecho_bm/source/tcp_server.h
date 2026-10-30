#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "lwip/err.h"

/* Puerto TCP en el que escucha la K64. */
#ifndef TCP_SERVER_PORT
#define TCP_SERVER_PORT 7
#endif

/*!
 * @brief Crea el PCB, hace bind al puerto TCP_SERVER_PORT y queda en escucha.
 * @return ERR_OK si el servidor quedo escuchando, codigo de error en caso contrario.
 */
err_t tcp_server_init(void);

#endif /* TCP_SERVER_H */
