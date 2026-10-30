/*
 * Cliente TCP para FRDM-MCXN947 (bare-metal, API raw de lwIP).
 *
 * Flujo: tcp_new -> tcp_connect -> (callback connected)
 *        main loop: tcp_client_poll_console() lee la UART sin bloquear;
 *        al recibir Enter hace tcp_write + tcp_output; la respuesta llega por tcp_recv.
 */

#include "lwip/opt.h"

#if LWIP_TCP && LWIP_CALLBACK_API

#include <string.h>
#include "lwip/tcp.h"
#include "lwip/timeouts.h"
#include "fsl_lpuart.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "tcp_client.h"

#define TX_LINE_MAX        128
#define RX_BUF_MAX         256
#define RECONNECT_DELAY_MS 2000

/* UART de la consola (LPUART4 en la FRDM-MCXN947), leida por polling para no bloquear lwIP. */
#define CONSOLE_UART ((LPUART_Type *)BOARD_DEBUG_UART_BASEADDR)

static struct tcp_pcb *client_pcb;
static ip4_addr_t server_addr;
static u16_t server_port;
static bool connected;

static char tx_line[TX_LINE_MAX + 2]; /* +2 para CR LF */
static size_t tx_len;
static char rx_buf[RX_BUF_MAX];

static void tcp_client_connect(void *arg);

static void tcp_client_close(void)
{
    if (client_pcb != NULL)
    {
        tcp_arg(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        tcp_close(client_pcb);
        client_pcb = NULL;
    }
    connected = false;
}

static void schedule_reconnect(void)
{
    PRINTF("Reintentando en %u ms...\r\n", (unsigned)RECONNECT_DELAY_MS);
    sys_timeout(RECONNECT_DELAY_MS, tcp_client_connect, NULL);
}

/* Respuesta del servidor (o cierre si p == NULL). */
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    if (p == NULL)
    {
        PRINTF("El servidor cerro la conexion.\r\n");
        tcp_client_close();
        schedule_reconnect();
        return ERR_OK;
    }

    if (err != ERR_OK)
    {
        pbuf_free(p);
        return err;
    }

    u16_t copied   = pbuf_copy_partial(p, rx_buf, sizeof(rx_buf) - 1, 0);
    rx_buf[copied] = '\0';

    PRINTF("  <- %s", rx_buf);
    if ((copied == 0) || (rx_buf[copied - 1] != '\n'))
    {
        PRINTF("\r\n");
    }
    PRINTF("> ");

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

/* lwIP ya libero el PCB al llamar aqui (connect rechazado, RST, timeout...). */
static void tcp_client_error(void *arg, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    PRINTF("Error TCP: %d (no se pudo conectar o se perdio la conexion).\r\n", (int)err);
    client_pcb = NULL;
    connected  = false;
    schedule_reconnect();
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    if (err != ERR_OK)
    {
        return err;
    }

    connected = true;
    tcp_recv(tpcb, tcp_client_recv);
    PRINTF("Conectado a %s:%u.\r\n", ip4addr_ntoa(&server_addr), (unsigned)server_port);
    PRINTF("Escribe un mensaje y presiona Enter para enviarlo.\r\n> ");
    return ERR_OK;
}

static void tcp_client_connect(void *arg)
{
    LWIP_UNUSED_ARG(arg);

    client_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (client_pcb == NULL)
    {
        PRINTF("No hay memoria para el PCB.\r\n");
        schedule_reconnect();
        return;
    }

    tcp_nagle_disable(client_pcb); /* cada linea sale de inmediato */
    tcp_err(client_pcb, tcp_client_error);

    PRINTF("Conectando a %s:%u ...\r\n", ip4addr_ntoa(&server_addr), (unsigned)server_port);
    err_t err = tcp_connect(client_pcb, &server_addr, server_port, tcp_client_connected);
    if (err != ERR_OK)
    {
        PRINTF("tcp_connect fallo: %d\r\n", (int)err);
        tcp_client_close();
        schedule_reconnect();
    }
}

void tcp_client_init(const ip4_addr_t *server_ip, u16_t port)
{
    ip4_addr_copy(server_addr, *server_ip);
    server_port = port;
    tx_len      = 0;
    tcp_client_connect(NULL);
}

static void send_line(void)
{
    if (!connected || (client_pcb == NULL))
    {
        PRINTF("  (sin conexion, mensaje descartado)\r\n> ");
        return;
    }

    tx_line[tx_len++] = '\r';
    tx_line[tx_len++] = '\n';

    err_t err = tcp_write(client_pcb, tx_line, (u16_t)tx_len, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK)
    {
        tcp_output(client_pcb);
    }
    else
    {
        PRINTF("  tcp_write fallo: %d\r\n> ", (int)err);
    }
}

void tcp_client_poll_console(void)
{
    uint32_t flags = LPUART_GetStatusFlags(CONSOLE_UART);

    if (flags & (uint32_t)kLPUART_RxOverrunFlag)
    {
        (void)LPUART_ClearStatusFlags(CONSOLE_UART, kLPUART_RxOverrunFlag);
    }
    if (!(flags & (uint32_t)kLPUART_RxDataRegFullFlag))
    {
        return;
    }

    uint8_t c = LPUART_ReadByte(CONSOLE_UART);

    if ((c == '\r') || (c == '\n'))
    {
        if (tx_len > 0)
        {
            PRINTF("\r\n");
            send_line();
            tx_len = 0;
        }
        return;
    }

    if ((c == '\b') || (c == 0x7F))
    {
        if (tx_len > 0)
        {
            tx_len--;
            PRINTF("\b \b");
        }
        return;
    }

    if ((tx_len < TX_LINE_MAX) && (c >= 0x20) && (c < 0x7F))
    {
        tx_line[tx_len++] = (char)c;
        PUTCHAR(c);
    }
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
