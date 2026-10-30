# Practica-1-Ethernet

Proyecto base: `frdmk64f_lwip_tcpecho_bm` (TCP Echo *bare-metal* con lwIP para la tarjeta FRDM-K64F).

Archivos clave:

- `frdmk64f_lwip_tcpecho_bm/source/lwip_tcpecho_bm.c` → `main()`, configuración de red (MAC/IP) e inicialización de lwIP.
- `frdmk64f_lwip_tcpecho_bm/lwip/contrib/apps/tcpecho_raw/tcpecho_raw.c` → lógica de la aplicación TCP Echo (API *raw* de lwIP).
- `frdmk64f_lwip_tcpecho_bm/source/lwipopts.h` → opciones de lwIP (`NO_SYS = 1`, `LWIP_SOCKET = 0`).

---

## 1. MAC Address

Definida en `source/lwip_tcpecho_bm.c` mediante la macro `configMAC_ADDR`:

```c
/* MAC address configuration. */
#ifndef configMAC_ADDR
#define configMAC_ADDR                     \
    {                                      \
        0x02, 0x12, 0x13, 0x10, 0x15, 0x11 \
    }
#endif
```

**MAC = `02:12:13:10:15:11`** (el primer byte `0x02` indica una dirección administrada localmente).

Se pasa al driver Ethernet dentro de `main()` a través de la estructura `ethernetif_config_t`:

```c
ethernetif_config_t enet_config = {
    .phyHandle  = &phyHandle,
    .macAddress = configMAC_ADDR,
};
```

---

## 2. IP Address

También en `source/lwip_tcpecho_bm.c`, con las macros `configIP_ADDRx`, `configNET_MASKx` y `configGW_ADDRx`:

| Parámetro        | Macros               | Valor           |
|------------------|----------------------|-----------------|
| IP de la tarjeta | `configIP_ADDR0..3`  | `192.168.0.102` |
| Máscara de red   | `configNET_MASK0..3` | `255.255.255.0` |
| Gateway          | `configGW_ADDR0..3`  | `192.168.0.100` |

La IP es **estática** (no se usa DHCP). Se construye y se asigna a la interfaz en `main()`:

```c
IP4_ADDR(&netif_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
IP4_ADDR(&netif_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
IP4_ADDR(&netif_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);

lwip_init();

netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN, ethernet_input);
netif_set_default(&netif);
netif_set_up(&netif);
```

---

## 3. ¿Es cliente o servidor?

**Es un SERVIDOR TCP** (echo server en el puerto 7).

Evidencia en `tcpecho_raw.c`:

- Hace `tcp_bind()` a un puerto fijo (7).
- Llama a `tcp_listen()` para quedarse escuchando.
- Registra un callback con `tcp_accept()` para atender conexiones entrantes.
- Nunca llama a `tcp_connect()`, por lo que no inicia conexiones hacia otro equipo.

Comportamiento: espera a que un cliente (p. ej. la PC con `echotool`, `nc` o `telnet`) se conecte, y todo lo que recibe lo regresa tal cual (eco). Cuando el cliente cierra, el servidor cierra pasivamente la conexión.

---

## 4. ¿Dónde es la creación del socket?

Este proyecto usa la **API raw/callback de lwIP** (no sockets BSD, ya que es *bare-metal* sin RTOS: `NO_SYS = 1` y `LWIP_SOCKET = 0` en `lwipopts.h`). El equivalente al "socket" es el **PCB (Protocol Control Block)** `struct tcp_pcb`.

La creación ocurre en `tcpecho_raw_init()` dentro de `lwip/contrib/apps/tcpecho_raw/tcpecho_raw.c`:

```c
void
tcpecho_raw_init(void)
{
  tcpecho_raw_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);      /* <-- creación del "socket" (PCB) */
  if (tcpecho_raw_pcb != NULL) {
    err_t err;

    err = tcp_bind(tcpecho_raw_pcb, IP_ANY_TYPE, 7);        /* bind a cualquier IP local, puerto 7 */
    if (err == ERR_OK) {
      tcpecho_raw_pcb = tcp_listen(tcpecho_raw_pcb);        /* pasa a modo escucha */
      tcp_accept(tcpecho_raw_pcb, tcpecho_raw_accept);      /* callback para conexiones entrantes */
    }
  }
}
```

Equivalencia con sockets BSD:

| API raw lwIP          | Socket BSD   |
|-----------------------|--------------|
| `tcp_new_ip_type()`   | `socket()`   |
| `tcp_bind()`          | `bind()`     |
| `tcp_listen()`        | `listen()`   |
| `tcp_accept()` (cb)   | `accept()`   |
| `tcp_recv()` (cb)     | `recv()`     |
| `tcp_write()`         | `send()`     |
| `tcp_close()`         | `close()`    |
| `tcp_connect()`       | `connect()`  |

`tcpecho_raw_init()` es invocada desde `main()` en `source/lwip_tcpecho_bm.c`, justo después de levantar la interfaz de red:

```c
netif_set_up(&netif);

tcpecho_raw_init();
```

---

## 5. ¿Dónde se empieza la comunicación?

### a) Inicio de la escucha (lado servidor)

En `tcpecho_raw_init()` con `tcp_listen()` + `tcp_accept()`. A partir de ahí la tarjeta acepta conexiones en el puerto 7.

### b) Inicio real del intercambio de datos

Ocurre cuando un cliente se conecta. lwIP invoca el callback **`tcpecho_raw_accept()`** (`tcpecho_raw.c`), que crea el estado de la conexión y registra los callbacks para esa conexión:

```c
static err_t
tcpecho_raw_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  ...
  es->state = ES_ACCEPTED;
  es->pcb = newpcb;
  tcp_arg(newpcb, es);
  tcp_recv(newpcb, tcpecho_raw_recv);    /* datos recibidos */
  tcp_err(newpcb, tcpecho_raw_error);    /* errores */
  tcp_poll(newpcb, tcpecho_raw_poll, 0); /* reintentos periódicos */
  tcp_sent(newpcb, tcpecho_raw_sent);    /* confirmación de envío */
  ...
}
```

Cuando llegan datos se ejecuta `tcpecho_raw_recv()`, que guarda el `pbuf` y llama a `tcpecho_raw_send()`, donde se hace el envío (eco) con `tcp_write()`:

```c
wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
```

### c) Bucle principal que mueve los paquetes

Al ser *bare-metal*, nada ocurre si no se hace *polling*. El `while(1)` de `main()` recoge las tramas del driver ENET y dispara los timers de lwIP, provocando que se ejecuten los callbacks anteriores:

```c
while (1)
{
    /* Poll the driver, get any outstanding frames */
    ethernetif_input(&netif);

    sys_check_timeouts(); /* Handle all system timeouts for all core protocols */
}
```

Flujo resumido:

```
main() → tcpecho_raw_init() → tcp_listen()/tcp_accept()
                                    ↓ (cliente se conecta)
                           tcpecho_raw_accept()
                                    ↓ (cliente envía datos)
                           tcpecho_raw_recv() → tcpecho_raw_send() → tcp_write()
                                    ↓ (cliente cierra)
                           tcpecho_raw_recv(p == NULL) → tcpecho_raw_close()
```

---

## 6. ¿Cómo se puede modificar para hacer un servidor/cliente?

### 6.1 Ajustar el servidor actual

- **Puerto**: cambiar el `7` en `tcp_bind(tcpecho_raw_pcb, IP_ANY_TYPE, 7);`.
- **IP/MAC**: modificar las macros `configIP_ADDRx`, `configGW_ADDRx`, `configNET_MASKx` y `configMAC_ADDR` en `lwip_tcpecho_bm.c`.
- **Lógica de respuesta**: en `tcpecho_raw_recv()`, en lugar de reenviar el mismo `pbuf`, procesar `p->payload` y responder con `tcp_write()` el dato que se desee.

### 6.2 Convertirlo en CLIENTE TCP

En la API raw, el cliente **no hace `bind`/`listen`/`accept`**; usa `tcp_connect()` y registra un callback de conexión establecida. Pasos:

1. Crear un nuevo archivo (p. ej. `tcpclient_raw.c`) o modificar `tcpecho_raw_init()`.
2. Reemplazar la secuencia `bind → listen → accept` por `connect`:

```c
#include "lwip/tcp.h"
#include "fsl_debug_console.h"

static struct tcp_pcb *client_pcb;

/* Respuesta del servidor */
static err_t
tcpclient_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  if (p == NULL) {                                /* el servidor cerró */
    tcp_close(tpcb);
    client_pcb = NULL;
    return ERR_OK;
  }
  PRINTF("Recibido: %.*s\r\n", p->len, (char *)p->payload);
  tcp_recved(tpcb, p->tot_len);
  pbuf_free(p);
  return ERR_OK;
}

/* Se ejecuta cuando el servidor acepta la conexión */
static err_t
tcpclient_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  const char msg[] = "Hola desde FRDM-K64F\r\n";

  if (err != ERR_OK) {
    return err;
  }

  tcp_recv(tpcb, tcpclient_recv);
  tcp_write(tpcb, msg, sizeof(msg) - 1, TCP_WRITE_FLAG_COPY);
  tcp_output(tpcb);                               /* fuerza el envío inmediato */
  return ERR_OK;
}

static void
tcpclient_error(void *arg, err_t err)
{
  PRINTF("Error TCP: %d\r\n", err);
  client_pcb = NULL;                              /* lwIP ya liberó el pcb */
}

void
tcpclient_raw_init(void)
{
  ip4_addr_t server_ip;

  IP4_ADDR(&server_ip, 192, 168, 0, 100);         /* IP del servidor (p. ej. la PC) */

  client_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);  /* socket() */
  if (client_pcb != NULL) {
    tcp_err(client_pcb, tcpclient_error);
    tcp_connect(client_pcb, &server_ip, 7, tcpclient_connected);   /* connect() */
  }
}
```

3. En `main()` de `lwip_tcpecho_bm.c` sustituir (o añadir junto a) `tcpecho_raw_init();` por `tcpclient_raw_init();`.
4. Mantener el `while(1)` con `ethernetif_input()` y `sys_check_timeouts()`, pues `tcp_connect()` y sus callbacks solo progresan si el stack se sigue atendiendo.
5. En la PC levantar un servidor para probar, por ejemplo: `echotool /p tcp /s 7` (Windows) o `nc -l -p 7` (Linux).

> Nota: en un cliente no es obligatorio `tcp_bind()`; lwIP asigna un puerto efímero automáticamente.

### 6.3 Servidor y cliente a la vez

Como la API raw es asíncrona, pueden coexistir: llamar a `tcpecho_raw_init()` (servidor en puerto 7) y a `tcpclient_raw_init()` (cliente hacia otro host) desde `main()`. El `while(1)` atenderá ambos PCBs.

### 6.4 Alternativa con sockets BSD

Para usar `socket()/bind()/listen()/accept()/connect()` reales hay que compilar con `USE_RTOS` (proyecto `lwip_tcpecho_freertos`), lo que en `lwipopts.h` activa `NO_SYS = 0` y `LWIP_SOCKET = 1`. La API de sockets requiere un sistema operativo; el proyecto actual (*bare-metal*) solo soporta la API raw.

