#ifndef CARDS_H
#define CARDS_H

#include <stdbool.h>

struct game_state;

typedef bool (*playable_fn)(struct game_state *game);
typedef const char* (*play_fn)(struct game_state *game);

struct card {
    const char *name;
    const char *text;
    bool remove_after_use;
    bool battle_card;
    playable_fn playable;
    play_fn play;
};

static inline bool always_playable(struct game_state *game)
{
    return true;
}

/* Battle cards usable in battle resolution */
extern struct card lieutenant_in_pursuit;
extern struct card prebles_boys;
extern struct card lieutenant_leads_the_charge;
extern struct card marine_sharpshooters;

void init_game_cards(struct game_state *game);
void draw_from_deck(struct game_state *game, int draw_count);
void discard_from_hand(struct game_state *game, int idx);
void shuffle_discard_into_deck(struct game_state *game);
void remove_card_from_game(struct game_state *game, int idx);
const char *play_card_from_hand(struct game_state *game, int idx);
const char *play_core_card(struct game_state *game, int idx);
bool check_play_battle_card(struct game_state *game, struct card *card);

#endif /* CARDS_H */
