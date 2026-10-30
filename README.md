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

| Parámetro        | Macros               | Valor original  | Valor actual   |
|------------------|----------------------|-----------------|----------------|
| IP de la tarjeta | `configIP_ADDR0..3`  | `192.168.0.102` | `10.10.10.102` |
| Máscara de red   | `configNET_MASK0..3` | `255.255.255.0` | `255.255.255.0`|
| Gateway          | `configGW_ADDR0..3`  | `192.168.0.100` | `10.10.10.100` |

> Se cambió la subred a `10.10.10.x` porque la VPN corporativa (GlobalProtect) inyecta una ruta `192.168.0.0/24` con métrica 0 que captura el tráfico hacia la K64 y lo manda por el túnel; con `10.10.10.0/24` el tráfico sale por el cable Ethernet.

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

  IP4_ADDR(&server_ip, 10, 10, 10, 100);          /* IP del servidor (p. ej. la PC) */

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

---

## 7. Modificación realizada: la K64 como servidor TCP

Se reemplazó la aplicación de ejemplo `tcpecho_raw` por un servidor propio en `source/`:

| Archivo | Cambio |
|---------|--------|
| `source/tcp_server.h` | Nuevo. Declara `tcp_server_init()` y el puerto `TCP_SERVER_PORT` (7). |
| `source/tcp_server.c` | Nuevo. Implementa el servidor con la API raw de lwIP. |
| `source/lwip_tcpecho_bm.c` | Incluye `tcp_server.h` en lugar de `tcpecho_raw.h`, imprime la MAC y llama a `tcp_server_init()` tras configurar la red. |

### Funcionamiento de `tcp_server.c`

```
tcp_server_init()
  ├─ tcp_new_ip_type()  → crea el PCB (socket)
  ├─ tcp_bind(…, 7)     → puerto 7 en cualquier IP local
  ├─ tcp_listen()       → modo escucha
  └─ tcp_accept(tcp_server_accept)

tcp_server_accept()  ← un cliente se conecta
  ├─ registra tcp_recv(tcp_server_recv) y tcp_err(tcp_server_error)
  └─ envía "Conectado al servidor TCP de la FRDM-K64F"

tcp_server_recv()    ← llegan datos
  ├─ copia el pbuf a rx_buffer y lo imprime por consola (PRINTF)
  ├─ responde "K64 recibio: <datos>" con tcp_write() + tcp_output()
  ├─ tcp_recved() para liberar ventana de recepción
  └─ si p == NULL el cliente cerró → tcp_close()
```

### Prueba desde la PC

1. Asignar IP estática a la PC en el adaptador Ethernet conectado a la K64 (PowerShell como administrador; ajustar el nombre del adaptador con `Get-NetAdapter`):

```powershell
New-NetIPAddress -InterfaceAlias "Ethernet 5" -IPAddress 10.10.10.100 -PrefixLength 24
```

2. Verificar que el tráfico sale por el cable y que la K64 responde:

```powershell
Find-NetRoute -RemoteIPAddress 10.10.10.102 | Select-Object -First 1 -ExpandProperty IPAddress   # debe mostrar 10.10.10.100
ping 10.10.10.102
```

3. Abrir la consola serie del OpenSDA (115200 8N1) para ver los mensajes de la tarjeta.
4. Conectarse como cliente:

```powershell
# PuTTY: Connection type = Raw, Host = 10.10.10.102, Port = 7

# Windows (telnet)
telnet 10.10.10.102 7

# Linux / macOS
nc 10.10.10.102 7
```

5. Todo lo que se escriba en el cliente se mostrará en la consola de la K64 y se recibirá de vuelta con el prefijo `K64 recibio:`.

#### Problemas encontrados durante la prueba

| Síntoma | Causa | Solución |
|---------|-------|----------|
| PuTTY: *Network error: connection timeout* y `ping` sin respuesta. | La PC tenía la misma IP que la K64 (`192.168.0.102`). | Asignar a la PC la IP del gateway (`.100`). |
| Sigue sin responder; `Find-NetRoute` devuelve la IP de la VPN (`10.124.x.x`). | GlobalProtect inyecta `192.168.0.0/24` con métrica 0 y se lleva el tráfico por el túnel. | Cambiar la subred de la K64 a `10.10.10.0/24` (ver sección 8). |
| Tras cambiar a `10.10.10.x`, `Get-NetIPAddress` muestra `AddressState = Duplicate`. | Se asignó a la PC `.102` (la IP de la K64) en vez de `.100`. Windows detectó el conflicto por ARP y no instaló la ruta. | Corregir a `10.10.10.100`. Regla: **K64 = .102, PC = .100**. |
| `Find-NetRoute` sigue devolviendo la IP de la VPN aunque la subred sea nueva. | GlobalProtect copia **cualquier** subred local de la PC al túnel con métrica 0 (política "sin acceso a red local"). | Ruta `/32` hacia la K64 por el cable o desconectar la VPN (ver sección 8). |

Para cambiar el puerto basta con definir `TCP_SERVER_PORT` antes de incluir `tcp_server.h` o editar el valor por defecto en el header.

---

## 8. Cambios para poder probar el ejemplo con la VPN corporativa (GlobalProtect)

### 8.1 Diagnóstico

Con la K64 corriendo y el cable conectado, `ping` y PuTTY daban *timeout*. Los comandos de diagnóstico mostraron:

```powershell
PS> Find-NetRoute -RemoteIPAddress 192.168.0.102 | Select-Object -First 1 -ExpandProperty IPAddress
10.124.82.95          # <- IP del adaptador de la VPN, no la del cable

PS> Get-NetRoute -AddressFamily IPv4 | Where-Object DestinationPrefix -like '192.168.0.*'
DestinationPrefix NextHop InterfaceAlias RouteMetric InterfaceMetric
192.168.0.0/24    0.0.0.0 Ethernet 2 (PANGP VPN)    0        1     # <- gana
192.168.0.0/24    0.0.0.0 Ethernet 5 (cable)      256       35
```

GlobalProtect (adaptador *PANGP Virtual Ethernet Adapter*) inyecta una copia de la subred local con métrica 0, así que todo el tráfico hacia la K64 se iba por el túnel VPN y nunca llegaba al cable. Al cambiar la K64 a `10.10.10.x` el comportamiento se repitió: la VPN volvió a copiar `10.10.10.0/24`. Es decir, **no depende de la subred elegida**.

### 8.2 Cambio en el firmware (`source/lwip_tcpecho_bm.c`)

Se movió la K64 fuera de `192.168.0.x` para evitar choques con redes domésticas/corporativas típicas:

```c
/* IP address configuration. Subred 10.10.10.x para no chocar con rutas de VPN (192.168.0.0/24). */
#define configIP_ADDR0 10
#define configIP_ADDR1 10
#define configIP_ADDR2 10
#define configIP_ADDR3 102

/* Gateway address configuration (IP de la PC). */
#define configGW_ADDR0 10
#define configGW_ADDR1 10
#define configGW_ADDR2 10
#define configGW_ADDR3 100
```

Resultado: **K64 = `10.10.10.102/24`**, **PC = `10.10.10.100/24`**.

### 8.3 Cambio en la PC: IP estática + ruta que esquiva la VPN

PowerShell **como administrador** (ajustar `"Ethernet 5"` al nombre real del adaptador, ver `Get-NetAdapter`):

```powershell
# 1. IP estática en el adaptador conectado a la K64
New-NetIPAddress -InterfaceAlias "Ethernet 5" -IPAddress 10.10.10.100 -PrefixLength 24

# 2. Ruta /32 hacia la K64 por el cable. Un /32 siempre gana sobre el /24 que inyecta la VPN.
New-NetRoute -DestinationPrefix 10.10.10.102/32 -InterfaceAlias "Ethernet 5" -NextHop 0.0.0.0 -RouteMetric 0

# 3. Limpiar ARP viejo y verificar
arp -d 10.10.10.102
Find-NetRoute -RemoteIPAddress 10.10.10.102 | Select-Object -First 1 -ExpandProperty IPAddress   # debe decir 10.10.10.100
ping 10.10.10.102
```

Alternativa sin tocar rutas: desconectar GlobalProtect (icono en la bandeja → *Disconnect*) mientras dura la prueba.

> La ruta `/32` no es persistente; se pierde al reiniciar o al reconectar la VPN. Para hacerla permanente agregar `-PolicyStore PersistentStore` al `New-NetRoute`, aunque GlobalProtect puede eliminarla según la política corporativa.

### 8.4 Resultado

Consola serie de la K64 (COM6, 115200 8N1):

```
 IPv4 Address     : 10.10.10.102
 IPv4 Subnet mask : 255.255.255.0
 IPv4 Gateway     : 10.10.10.100
 MAC Address      : 02:12:13:10:15:11
************************************************
 Servidor TCP escuchando en el puerto 7
 Cliente conectado desde 10.10.10.100:58255
 [10.10.10.100:58255] -> hola
 [10.10.10.100:58255] -> hola amigos!!!!!!!!!!
```

PuTTY (Raw, `10.10.10.102:7`):

```
Conectado al servidor TCP de la FRDM-K64F
K64 recibio: hola
K64 recibio: hola amigos!!!!!!!!!!
```

### 8.5 Ajustes posteriores en `tcp_server.c`

Durante la prueba se observó que PuTTY envía la línea y el `CR LF` en segmentos TCP distintos, lo que generaba respuestas duplicadas y vacías. `tcp_server_recv()` ahora recorta `\r\n` del final, ignora segmentos vacíos y termina cada respuesta con `\r\n`.

---

## 9. Cliente TCP en la FRDM-MCXN947

Proyecto base: `frdmmcxn947_lwip_dhcp_bm_cm33_core0` (lwIP *bare-metal*, API **raw**, igual que la K64). Se reutiliza la inicialización de red, se quita DHCP y se agrega un cliente TCP que lee lo que el usuario escribe en la consola serie y lo envía al servidor de la K64.

> Inicialmente se intentó con `frdmmcxn947_lwip_https_client_freertos_cm33_core0` (FreeRTOS + sockets BSD + mbedTLS), pero el proyecto era demasiado pesado; se descartó en favor del ejemplo DHCP bare-metal, que es mucho más ligero y comparte el mismo modelo de programación que el servidor.

### 9.1 Topología

```
 PC (consolas serie por USB)
  ├── USB ──► FRDM-K64F      10.10.10.102  ── Servidor TCP, puerto 7
  └── USB ──► FRDM-MCXN947   10.10.10.101  ── Cliente TCP
                   │                 │
                   └── cable Ethernet ┘   (directo, o ambas a un switch junto con la PC en 10.10.10.100)
```

Ambos PHY (KSZ8081 en la K64, LAN8741 en la MCX) soportan Auto-MDIX, por lo que un cable directo entre tarjetas funciona.

### 9.2 Archivos modificados / nuevos

| Archivo | Cambio |
|---------|--------|
| `source/tcp_client.h` | Nuevo. `tcp_client_init(ip, puerto)` y `tcp_client_poll_console()`. |
| `source/tcp_client.c` | Nuevo. Cliente con API raw: `tcp_new_ip_type` → `tcp_connect` → callbacks `connected` / `recv` / `err`; lectura **no bloqueante** de la UART por *polling* del LPUART4; reconexión automática con `sys_timeout`. |
| `source/lwip_dhcp_bm.c` | Se quita `dhcp_start()` y `print_dhcp_state()`. IP estática `10.10.10.101/24`, gateway `10.10.10.100`. Se llama `tcp_client_init()` y, en el `while(1)`, `tcp_client_poll_console()`. |

### 9.3 Por qué la consola se lee por *polling* y no con `GETCHAR()`

En *bare-metal* el `while(1)` de `main()` es el único hilo: si se bloquea en `GETCHAR()` esperando una tecla, `ethernetif_input()` y `sys_check_timeouts()` dejan de ejecutarse y lwIP no procesa tramas ni temporizadores (el ARP, los reintentos de TCP y la respuesta del servidor se congelan). Por eso `tcp_client_poll_console()` consulta directamente el registro de estado del LPUART4 y solo lee un byte si hay uno disponible:

```c
uint32_t flags = LPUART_GetStatusFlags(CONSOLE_UART);
if (!(flags & kLPUART_RxDataRegFullFlag)) return;   /* nada que leer, regresar al loop */
uint8_t c = LPUART_ReadByte(CONSOLE_UART);
```

La línea se acumula en `tx_line[]`; al recibir Enter se envía con `tcp_write()` + `tcp_output()`.

### 9.4 Comparación servidor (K64) vs. cliente (MCX), ambos con API raw

| Paso | K64 — servidor (`tcp_server.c`) | MCXN947 — cliente (`tcp_client.c`) |
|------|---------------------------------|------------------------------------|
| Crear PCB | `tcp_new_ip_type(IPADDR_TYPE_ANY)` | `tcp_new_ip_type(IPADDR_TYPE_V4)` |
| Dirección | `tcp_bind(pcb, IP_ANY_TYPE, 7)` | `IP4_ADDR(&server_ip, 10,10,10,102)` |
| Inicio | `tcp_listen()` + `tcp_accept(cb)` | `tcp_connect(pcb, &server_ip, 7, cb_connected)` |
| Recibir | callback `tcp_recv()` | callback `tcp_recv()` |
| Enviar | `tcp_write()` + `tcp_output()` | `tcp_write()` + `tcp_output()` (Nagle deshabilitado) |
| Error / cierre | `tcp_err()`, `tcp_close()` | `tcp_err()` → `sys_timeout(2000, reconectar)` |
| Entrada de usuario | — | *polling* del LPUART4 en el `while(1)` |

### 9.5 Flujo del cliente

```
main()
  ├─ lwip_init()
  ├─ netif_add(IP 10.10.10.101, mask /24, gw 10.10.10.100)
  ├─ ethernetif_wait_linkup()
  ├─ tcp_client_init(10.10.10.102, 7) ──► tcp_connect()
  └─ while(1)
       ├─ ethernetif_input()
       ├─ sys_check_timeouts()
       └─ tcp_client_poll_console()  ← eco, backspace; Enter → send_line()

callbacks (lwIP los invoca desde ethernetif_input / sys_check_timeouts)
  ├─ tcp_client_connected()  → "Conectado a 10.10.10.102:7"
  ├─ tcp_client_recv()       → "  <- K64 recibio: ..."  |  p == NULL → cierre + reconexión
  └─ tcp_client_error()      → PCB ya liberado por lwIP → programa reconexión en 2 s
```

### 9.6 Prueba

1. Flashear la K64 (servidor) y la MCXN947 (cliente). Conectar el cable Ethernet entre ambas.
2. Abrir dos consolas serie (115200 8N1): COM de la K64 y COM de la MCX.
3. En la consola de la MCX se verá:

```
************************************************
 FRDM-MCXN947 TCP Client
************************************************
 IPv4 Address     : 10.10.10.101
 IPv4 Subnet mask : 255.255.255.0
 IPv4 Gateway     : 10.10.10.100
 MAC Address      : xx:xx:xx:xx:xx:xx
 Servidor TCP     : 10.10.10.102:7
************************************************

Conectando a 10.10.10.102:7 ...
Conectado a 10.10.10.102:7.
Escribe un mensaje y presiona Enter para enviarlo.
>   <- Conectado al servidor TCP de la FRDM-K64F
> hola desde la MCX
  <- K64 recibio: hola desde la MCX
>
```

4. En la consola de la K64:

```
 Cliente conectado desde 10.10.10.101:49153
 [10.10.10.101:49153] -> hola desde la MCX
```

Si la K64 no está encendida o rechaza la conexión, el cliente imprime `Error TCP: -13` (`ERR_RST`) o `-15` (`ERR_ABRT`) y reintenta cada 2 s hasta conectar. Si se escribe sin conexión, el mensaje se descarta con aviso.

### 9.7 Parámetros configurables

| Macro | Archivo | Valor por defecto |
|-------|---------|-------------------|
| `configIP_ADDR0..3` | `lwip_dhcp_bm.c` | `10.10.10.101` |
| `configGW_ADDR0..3` | `lwip_dhcp_bm.c` | `10.10.10.100` |
| `configSERVER_ADDR0..3` | `lwip_dhcp_bm.c` | `10.10.10.102` |
| `configSERVER_PORT` | `lwip_dhcp_bm.c` | `7` |
| `RECONNECT_DELAY_MS` | `tcp_client.c` | `2000` |
| `TX_LINE_MAX` | `tcp_client.c` | `128` |
| `CONSOLE_UART` | `tcp_client.c` | `BOARD_DEBUG_UART_BASEADDR` (LPUART4) |


