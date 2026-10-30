/*
 * Logica del juego de ruleta rusa (independiente de la red).
 *
 * - El servidor (K64) espera GAME_NUM_PLAYERS clientes y luego va turnando entre ellos.
 * - En su turno, el cliente manda el numero de balas (min..6). El servidor dispara y
 *   responde "1" si le toco bala (eliminado) o "0" si sobrevivio.
 * - Tras cada disparo se avisa a todos cuantos jugadores siguen vivos.
 * - Cada GAME_ROUNDS_PER_LEVEL rondas el minimo de balas sube en 1.
 * - Gana el ultimo jugador vivo; la partida se reinicia sola.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "fsl_debug_console.h"
#include "roulette_game.h"

typedef struct
{
    bool connected;
    bool alive;
} player_t;

typedef enum
{
    GAME_WAITING, /* esperando a que se conecten todos */
    GAME_RUNNING,
    GAME_OVER
} game_state_t;

static game_send_fn send_to;
static player_t players[GAME_NUM_PLAYERS];
static game_state_t state;
static int current;     /* indice del jugador en turno */
static int round_num;   /* ronda actual (1..) */
static int min_bullets; /* minimo de balas permitido en la ronda */
static uint32_t rng;

static char msg[160];

/* ---------- utilidades ---------- */

static int connected_count(void)
{
    int n = 0;
    for (int i = 0; i < GAME_NUM_PLAYERS; i++)
    {
        n += players[i].connected ? 1 : 0;
    }
    return n;
}

static int alive_count(void)
{
    int n = 0;
    for (int i = 0; i < GAME_NUM_PLAYERS; i++)
    {
        n += players[i].alive ? 1 : 0;
    }
    return n;
}

static int first_alive(void)
{
    for (int i = 0; i < GAME_NUM_PLAYERS; i++)
    {
        if (players[i].alive)
        {
            return i;
        }
    }
    return -1;
}

/* Siguiente jugador vivo despues de 'from' (ciclico). -1 si no hay. */
static int next_alive(int from)
{
    for (int k = 1; k <= GAME_NUM_PLAYERS; k++)
    {
        int i = (from + k) % GAME_NUM_PLAYERS;
        if (players[i].alive)
        {
            return i;
        }
    }
    return -1;
}

static void broadcast(const char *text)
{
    for (int i = 0; i < GAME_NUM_PLAYERS; i++)
    {
        if (players[i].connected)
        {
            send_to(i, text);
        }
    }
}

static void broadcast_except(int except, const char *text)
{
    for (int i = 0; i < GAME_NUM_PLAYERS; i++)
    {
        if (players[i].connected && (i != except))
        {
            send_to(i, text);
        }
    }
}

static void broadcast_alive(void)
{
    snprintf(msg, sizeof(msg), ">> Jugadores vivos: %d de %d\r\n", alive_count(), GAME_NUM_PLAYERS);
    broadcast(msg);
    PRINTF(" [JUEGO] %s", msg);
}

/* LCG sembrado con el reloj; se mezcla con la hora de llegada de cada mensaje. */
static int rand_chamber(void)
{
    rng = rng * 1103515245u + 12345u;
    return (int)((rng >> 16) % GAME_CHAMBERS);
}

static int min_bullets_for_round(int r)
{
    int m = 1 + (r - 1) / GAME_ROUNDS_PER_LEVEL;
    return (m > GAME_CHAMBERS) ? GAME_CHAMBERS : m;
}

/* ---------- flujo de partida ---------- */

static void announce_turn(void)
{
    snprintf(msg, sizeof(msg),
             "\r\n== TU TURNO (ronda %d) == Minimo %d bala(s). Envia el numero de balas (%d-%d):\r\n", round_num,
             min_bullets, min_bullets, GAME_CHAMBERS);
    send_to(current, msg);

    snprintf(msg, sizeof(msg), "-- Turno del Jugador %d (ronda %d, minimo %d bala(s)). Esperando...\r\n",
             current + 1, round_num, min_bullets);
    broadcast_except(current, msg);

    PRINTF(" [JUEGO] Ronda %d, turno del Jugador %d, minimo %d bala(s)\r\n", round_num, current + 1, min_bullets);
}

static void start_game(void)
{
    for (int i = 0; i < GAME_NUM_PLAYERS; i++)
    {
        players[i].alive = players[i].connected;
    }

    state       = GAME_RUNNING;
    round_num   = 1;
    min_bullets = min_bullets_for_round(round_num);
    current     = first_alive();

    snprintf(msg, sizeof(msg), "\r\n===== INICIA LA RULETA RUSA: %d jugadores =====\r\n", alive_count());
    broadcast(msg);
    PRINTF(" [JUEGO] Partida iniciada con %d jugadores\r\n", alive_count());

    announce_turn();
}

static void restart_timer_cb(void *arg)
{
    (void)arg;

    if (connected_count() >= 2)
    {
        start_game();
    }
    else
    {
        state = GAME_WAITING;
        snprintf(msg, sizeof(msg), "Esperando jugadores (%d/%d)...\r\n", connected_count(), GAME_NUM_PLAYERS);
        broadcast(msg);
    }
}

static void end_game(int winner)
{
    state = GAME_OVER;

    if (winner >= 0)
    {
        snprintf(msg, sizeof(msg), "\r\n===== GANADOR: Jugador %d =====\r\n", winner + 1);
        broadcast(msg);
        PRINTF(" [JUEGO] Ganador: Jugador %d\r\n", winner + 1);
    }
    else
    {
        broadcast("\r\n===== Partida terminada sin ganador =====\r\n");
    }

    snprintf(msg, sizeof(msg), "Nueva partida en %d segundos...\r\n", GAME_RESTART_DELAY_MS / 1000);
    broadcast(msg);
    sys_timeout(GAME_RESTART_DELAY_MS, restart_timer_cb, NULL);
}

/* Pasa el turno; si se dio la vuelta completa, inicia una nueva ronda. */
static void advance_turn(void)
{
    int next = next_alive(current);

    if (next < 0)
    {
        end_game(-1);
        return;
    }

    if (next <= current)
    {
        round_num++;
        min_bullets = min_bullets_for_round(round_num);
        snprintf(msg, sizeof(msg), "\r\n----- Ronda %d: minimo %d bala(s) -----\r\n", round_num, min_bullets);
        broadcast(msg);
    }

    current = next;
    announce_turn();
}

/* ---------- API publica ---------- */

void game_init(game_send_fn send_fn)
{
    send_to = send_fn;
    memset(players, 0, sizeof(players));
    state       = GAME_WAITING;
    current     = -1;
    round_num   = 0;
    min_bullets = 1;
    rng         = sys_now() ^ 0xA5A5A5A5u;
}

void game_player_joined(int player)
{
    players[player].connected = true;
    players[player].alive     = false;

    snprintf(msg, sizeof(msg), "Bienvenido a la Ruleta Rusa. Eres el Jugador %d.\r\n", player + 1);
    send_to(player, msg);

    snprintf(msg, sizeof(msg), ">> Jugadores conectados: %d de %d\r\n", connected_count(), GAME_NUM_PLAYERS);
    broadcast(msg);
    PRINTF(" [JUEGO] Jugador %d conectado (%d/%d)\r\n", player + 1, connected_count(), GAME_NUM_PLAYERS);

    if (state == GAME_WAITING)
    {
        if (connected_count() == GAME_NUM_PLAYERS)
        {
            start_game();
        }
        else
        {
            send_to(player, "Esperando a los demas jugadores...\r\n");
        }
    }
    else
    {
        send_to(player, "Hay una partida en curso; entraras en la siguiente.\r\n");
    }
}

void game_player_left(int player)
{
    bool was_alive = players[player].alive;

    players[player].connected = false;
    players[player].alive     = false;

    snprintf(msg, sizeof(msg), ">> El Jugador %d se desconecto.\r\n", player + 1);
    broadcast(msg);
    PRINTF(" [JUEGO] Jugador %d desconectado\r\n", player + 1);

    if (state != GAME_RUNNING)
    {
        return;
    }

    broadcast_alive();

    int alive = alive_count();
    if (alive == 1)
    {
        end_game(first_alive());
    }
    else if (alive == 0)
    {
        end_game(-1);
    }
    else if (was_alive && (player == current))
    {
        /* Se fue el jugador en turno: pasa el turno sin contar ronda nueva por el salto. */
        current = next_alive(player);
        announce_turn();
    }
}

void game_player_input(int player, const char *line)
{
    rng ^= sys_now(); /* entropia: el instante en que llega el mensaje */

    if (state == GAME_WAITING)
    {
        snprintf(msg, sizeof(msg), "La partida aun no inicia (%d/%d jugadores).\r\n", connected_count(),
                 GAME_NUM_PLAYERS);
        send_to(player, msg);
        return;
    }
    if (state == GAME_OVER)
    {
        send_to(player, "La partida termino; espera el reinicio.\r\n");
        return;
    }
    if (!players[player].alive)
    {
        send_to(player, "Estas eliminado; solo puedes observar.\r\n");
        return;
    }
    if (player != current)
    {
        snprintf(msg, sizeof(msg), "No es tu turno. Turno del Jugador %d.\r\n", current + 1);
        send_to(player, msg);
        return;
    }

    char *end;
    long bullets = strtol(line, &end, 10);
    if ((end == line) || (bullets < min_bullets) || (bullets > GAME_CHAMBERS))
    {
        snprintf(msg, sizeof(msg), "Valor invalido. Envia un numero entre %d y %d.\r\n", min_bullets,
                 GAME_CHAMBERS);
        send_to(player, msg);
        return;
    }

    /* Disparo: 'bullets' recamaras cargadas de GAME_CHAMBERS. */
    bool hit = rand_chamber() < (int)bullets;

    PRINTF(" [JUEGO] Jugador %d jugo con %ld bala(s): %s\r\n", player + 1, bullets, hit ? "BANG" : "click");

    /* Respuesta de protocolo: "1" = bala, "0" = sobrevive. */
    send_to(player, hit ? "1\r\n" : "0\r\n");

    if (hit)
    {
        players[player].alive = false;
        snprintf(msg, sizeof(msg), "BANG! Te toco bala (%ld/%d). Quedas eliminado.\r\n", bullets, GAME_CHAMBERS);
        send_to(player, msg);
        snprintf(msg, sizeof(msg), "** Jugador %d jugo con %ld bala(s): BANG, eliminado.\r\n", player + 1,
                 bullets);
    }
    else
    {
        snprintf(msg, sizeof(msg), "click... Sobreviviste (%ld/%d).\r\n", bullets, GAME_CHAMBERS);
        send_to(player, msg);
        snprintf(msg, sizeof(msg), "** Jugador %d jugo con %ld bala(s): click, sobrevive.\r\n", player + 1,
                 bullets);
    }
    broadcast_except(player, msg);

    broadcast_alive();

    if (alive_count() <= 1)
    {
        end_game(first_alive());
        return;
    }

    advance_turn();
}
