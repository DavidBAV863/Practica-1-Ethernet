/*
 * Servidor TCP para FRDM-K64F usando la API raw de lwIP (bare-metal).
 *
 * Flujo: tcp_new -> tcp_bind -> tcp_listen -> tcp_accept
 *        por cada cliente: tcp_recv imprime lo recibido y responde con tcp_write.
 */

#include "lwip/opt.h"

#if LWIP_TCP && LWIP_CALLBACK_API

#include <string.h>
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "fsl_debug_console.h"
#include "tcp_server.h"

/* PCB de escucha (equivalente al socket de servidor). */
static struct tcp_pcb *server_pcb;

/* Buffer para copiar los datos recibidos y terminarlos en '\0' para imprimirlos. */
static char rx_buffer[256];

static void tcp_server_close(struct tcp_pcb *tpcb)
{
    tcp_arg(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    tcp_close(tpcb);
}

/* Se ejecuta cuando llegan datos del cliente o cuando el cliente cierra (p == NULL). */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    static const char reply_prefix[] = "K64 recibio: ";
    u16_t copied;

    LWIP_UNUSED_ARG(arg);

    if (p == NULL)
    {
        PRINTF(" Cliente %s:%u desconectado\r\n", ipaddr_ntoa(&tpcb->remote_ip), tpcb->remote_port);
        tcp_server_close(tpcb);
        return ERR_OK;
    }

    if (err != ERR_OK)
    {
        pbuf_free(p);
        return err;
    }

    copied = pbuf_copy_partial(p, rx_buffer, sizeof(rx_buffer) - 1, 0);

    /* PuTTY/telnet mandan el CR LF en un segmento aparte; se recorta y se ignoran los vacios. */
    while ((copied > 0) && ((rx_buffer[copied - 1] == '\r') || (rx_buffer[copied - 1] == '\n')))
    {
        copied--;
    }
    rx_buffer[copied] = '\0';

    if (copied > 0)
    {
        PRINTF(" [%s:%u] -> %s\r\n", ipaddr_ntoa(&tpcb->remote_ip), tpcb->remote_port, rx_buffer);

        /* Respuesta al cliente: prefijo + los mismos datos + CRLF. */
        if (tcp_sndbuf(tpcb) >= (sizeof(reply_prefix) - 1 + copied + 2))
        {
            tcp_write(tpcb, reply_prefix, sizeof(reply_prefix) - 1, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
            tcp_write(tpcb, rx_buffer, copied, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
            tcp_write(tpcb, "\r\n", 2, TCP_WRITE_FLAG_COPY);
            tcp_output(tpcb);
        }
    }

    /* Informa a lwIP que ya se procesaron los datos para abrir la ventana de recepcion. */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

/* lwIP ya libero el PCB cuando llama a este callback; no se debe usar tpcb aqui. */
static void tcp_server_error(void *arg, err_t err)
{
    LWIP_UNUSED_ARG(arg);
    PRINTF(" Error en conexion TCP: %d\r\n", (int)err);
}

/* Se ejecuta cuando un cliente completa el handshake TCP. */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    static const char welcome[] = "Conectado al servidor TCP de la FRDM-K64F\r\n";

    LWIP_UNUSED_ARG(arg);

    if ((err != ERR_OK) || (newpcb == NULL))
    {
        return ERR_VAL;
    }

    PRINTF(" Cliente conectado desde %s:%u\r\n", ipaddr_ntoa(&newpcb->remote_ip), newpcb->remote_port);

    tcp_setprio(newpcb, TCP_PRIO_MIN);
    tcp_arg(newpcb, NULL);
    tcp_recv(newpcb, tcp_server_recv);
    tcp_err(newpcb, tcp_server_error);

    tcp_write(newpcb, welcome, sizeof(welcome) - 1, TCP_WRITE_FLAG_COPY);
    tcp_output(newpcb);

    return ERR_OK;
}

err_t tcp_server_init(void)
{
    err_t err;

    server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (server_pcb == NULL)
    {
        PRINTF(" tcp_server: no hay memoria para el PCB\r\n");
        return ERR_MEM;
    }

    err = tcp_bind(server_pcb, IP_ANY_TYPE, TCP_SERVER_PORT);
    if (err != ERR_OK)
    {
        PRINTF(" tcp_server: fallo tcp_bind (%d)\r\n", (int)err);
        tcp_close(server_pcb);
        server_pcb = NULL;
        return err;
    }

    /* tcp_listen libera el PCB original y regresa uno nuevo en estado LISTEN. */
    server_pcb = tcp_listen(server_pcb);
    if (server_pcb == NULL)
    {
        PRINTF(" tcp_server: fallo tcp_listen\r\n");
        return ERR_MEM;
    }

    tcp_accept(server_pcb, tcp_server_accept);

    PRINTF(" Servidor TCP escuchando en el puerto %u\r\n", (unsigned)TCP_SERVER_PORT);

    return ERR_OK;
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
