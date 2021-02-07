#ifndef TBOT_H
#define TBOT_H

#include "game.h"

int tbot_resolve_naval_battle(struct game_state *game, enum locations location,
                              int damage);
int tbot_resolve_ground_combat(struct game_state *game, enum locations location,
                               int damage);
void tbot_do_turn(struct game_state *game);

#endif /* TBOT_H */
