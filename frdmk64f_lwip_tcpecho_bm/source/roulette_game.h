#ifndef ROULETTE_GAME_H
#define ROULETTE_GAME_H

#include <stdbool.h>

/* Numero de clientes (MCX) que deben conectarse para iniciar la partida. */
#ifndef GAME_NUM_PLAYERS
#define GAME_NUM_PLAYERS 3
#endif

/* Recamaras del revolver digital. */
#define GAME_CHAMBERS 6

/* Cada cuantas rondas sube el minimo de balas. */
#ifndef GAME_ROUNDS_PER_LEVEL
#define GAME_ROUNDS_PER_LEVEL 2
#endif

/* Tiempo antes de reiniciar la partida tras declarar un ganador. */
#ifndef GAME_RESTART_DELAY_MS
#define GAME_RESTART_DELAY_MS 10000
#endif

/* Callback que el servidor provee para enviar texto a un jugador (indice 0..N-1). */
typedef void (*game_send_fn)(int player, const char *msg);

void game_init(game_send_fn send_fn);
void game_player_joined(int player);
void game_player_left(int player);
void game_player_input(int player, const char *line);

#endif /* ROULETTE_GAME_H */
