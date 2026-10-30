/*
 * Servidor TCP para FRDM-K64F usando la API raw de lwIP (bare-metal).
 *
 * Flujo: tcp_new -> tcp_bind -> tcp_listen -> tcp_accept
 *        cada cliente aceptado ocupa un slot de jugador; lo que recibe se pasa a
 *        roulette_game.c y las respuestas del juego salen por tcp_write.
 */

#include "lwip/opt.h"

#if LWIP_TCP && LWIP_CALLBACK_API

#include <string.h>
#include <stdint.h>
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "fsl_debug_console.h"
#include "tcp_server.h"
#include "roulette_game.h"

/* PCB de escucha (equivalente al socket de servidor). */
static struct tcp_pcb *server_pcb;

/* Un PCB por jugador; NULL = slot libre. El indice es el numero de jugador. */
static struct tcp_pcb *player_pcb[GAME_NUM_PLAYERS];

/* Buffer para copiar los datos recibidos y terminarlos en '\0'. */
static char rx_buffer[128];

static int find_player(const struct tcp_pcb *tpcb)
{
    for (int i = 0; i < GAME_NUM_PLAYERS; i++)
    {
        if (player_pcb[i] == tpcb)
        {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void)
{
    return find_player(NULL);
}

/* Callback que usa el juego para mandar texto a un jugador. */
static void send_to_player(int player, const char *text)
{
    struct tcp_pcb *tpcb = player_pcb[player];
    u16_t len            = (u16_t)strlen(text);

    if ((tpcb == NULL) || (len == 0))
    {
        return;
    }
    if (tcp_sndbuf(tpcb) < len)
    {
        PRINTF(" [TCP] sin espacio en sndbuf para Jugador %d\r\n", player + 1);
        return;
    }
    if (tcp_write(tpcb, text, len, TCP_WRITE_FLAG_COPY) == ERR_OK)
    {
        tcp_output(tpcb);
    }
}

static void tcp_server_close(struct tcp_pcb *tpcb)
{
    tcp_arg(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    tcp_close(tpcb);
}

static void player_disconnected(int player)
{
    if (player >= 0)
    {
        player_pcb[player] = NULL;
        game_player_left(player);
    }
}

/* Se ejecuta cuando llegan datos del cliente o cuando el cliente cierra (p == NULL). */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    int player = (int)(intptr_t)arg;
    u16_t copied;

    if (p == NULL)
    {
        PRINTF(" Jugador %d (%s:%u) desconectado\r\n", player + 1, ipaddr_ntoa(&tpcb->remote_ip), tpcb->remote_port);
        tcp_server_close(tpcb);
        player_disconnected(player);
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
        PRINTF(" [Jugador %d] -> %s\r\n", player + 1, rx_buffer);
        game_player_input(player, rx_buffer);
    }

    /* Informa a lwIP que ya se procesaron los datos para abrir la ventana de recepcion. */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

/* lwIP ya libero el PCB cuando llama a este callback; no se debe usar tpcb aqui. */
static void tcp_server_error(void *arg, err_t err)
{
    int player = (int)(intptr_t)arg;

    PRINTF(" Error TCP con Jugador %d: %d\r\n", player + 1, (int)err);
    player_disconnected(player);
}

/* Se ejecuta cuando un cliente completa el handshake TCP. */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    static const char full_msg[] = "Servidor lleno, intenta mas tarde.\r\n";
    int slot;

    LWIP_UNUSED_ARG(arg);

    if ((err != ERR_OK) || (newpcb == NULL))
    {
        return ERR_VAL;
    }

    slot = find_free_slot();
    if (slot < 0)
    {
        PRINTF(" Conexion rechazada de %s:%u (servidor lleno)\r\n", ipaddr_ntoa(&newpcb->remote_ip),
               newpcb->remote_port);
        tcp_write(newpcb, full_msg, sizeof(full_msg) - 1, TCP_WRITE_FLAG_COPY);
        tcp_output(newpcb);
        tcp_close(newpcb);
        return ERR_OK;
    }

    PRINTF(" Jugador %d conectado desde %s:%u\r\n", slot + 1, ipaddr_ntoa(&newpcb->remote_ip), newpcb->remote_port);

    player_pcb[slot] = newpcb;
    tcp_setprio(newpcb, TCP_PRIO_MIN);
    tcp_nagle_disable(newpcb); /* los mensajes del juego deben salir de inmediato */
    tcp_arg(newpcb, (void *)(intptr_t)slot);
    tcp_recv(newpcb, tcp_server_recv);
    tcp_err(newpcb, tcp_server_error);

    game_player_joined(slot);

    return ERR_OK;
}

err_t tcp_server_init(void)
{
    err_t err;

    memset(player_pcb, 0, sizeof(player_pcb));
    game_init(send_to_player);

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

    PRINTF(" Servidor TCP escuchando en el puerto %u, esperando %d jugadores\r\n", (unsigned)TCP_SERVER_PORT,
           GAME_NUM_PLAYERS);

    return ERR_OK;
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
