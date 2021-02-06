#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cards.h"
#include "display.h"
#include "game.h"
#include "input.h"

void init_game_state(struct game_state *game, unsigned int seed)
{
    unsigned int year;
    unsigned int i;

    if (seed == 0) {
        seed = time(NULL);
    }

    srand(seed);

    memset(game, 0, sizeof(*game));
    game->seed = seed;

    game->year = START_YEAR;
    game->season = SPRING;

    game->us_frigates[GIBRALTAR] = 3;
    for (year = 1802; year <= 1804; year++) {
        game->turn_track_frigates[year_to_frigate_idx(year)] = 1;
    }

    /* For now setup the solo game */
    game->t_corsairs_tripoli = 5;
    game->t_corsairs_gibraltar = 2;
    for (i = 0; i < TRIP_INFANTRY_LOCS; i++) {
        game->t_infantry[i] = 4;
    }

    init_game_cards(game);

    /* Setup the tbot */
}

void game_handle_intercept(struct game_state *game, enum locations location)
{
    int frig_count = 0;
    bool lieutenant_played;
    int rolls = 0;
    int successes = 0;
    unsigned int *corsairs;

    assert(has_patrol_zone(location));

    if (location == TRIPOLI && game->swedish_frigates_active) {
        frig_count += 2;
        rolls += 2 * FRIGATE_DICE;
    }

    frig_count += game->patrol_frigates[location];

    if (frig_count == 0) {
        return;
    }

    lieutenant_played = check_play_battle_card(game, &lieutenant_in_pursuit);
    if (lieutenant_played) {
        rolls += 3 * game->patrol_frigates[location];
    } else {
        rolls += FRIGATE_DICE * game->patrol_frigates[location];
    }

    while (rolls--) {
        if (rolld6() == 6) {
            successes++;
        }
    }

    if (has_trip_allies(location)) {
        corsairs = &game->t_allies[location];
    } else if (location == TRIPOLI) {
        corsairs = &game->t_corsairs_tripoli;
    } else { /* Gibraltar */
        corsairs = &game->t_corsairs_gibraltar;
    }

    if (successes > *corsairs) {
        *corsairs = 0;
    } else {
        *corsairs -= successes;
    }
}

static void advance_game_round(struct game_state *game)
{
    unsigned int frig_idx;

    if (game->year == END_YEAR && game->season == WINTER) {
        cprintf(ITALIC WHITE, "Game ended in a draw!\n");
        exit(0);
    }

    if (game->season == WINTER) {
        game->year++;
        game->season = SPRING;

        frig_idx = year_to_frigate_idx(game->year);
        game->us_frigates[GIBRALTAR] += game->turn_track_frigates[frig_idx];
        game->turn_track_frigates[frig_idx] = 0;

        game->t_frigates += game->t_turn_frigates[frig_idx];
        game->t_turn_frigates[frig_idx] = 0;
    } else {
        game->season++;
    }
}

static void move_frigates(struct game_state *game, struct frigate_move *moves,
                          int num_moves)
{
    int i;
    struct frigate_move *move;

    for (i = 0; i < num_moves; i++) {
        move = &moves[i];
        if (move->from_type == HARBOR) {
            game->us_frigates[move->from] -= move->quantity;
        } else {
            game->patrol_frigates[move->from] -= move->quantity;
        }

        if (move->to_type == HARBOR) {
            game->us_frigates[move->to] += move->quantity;
        } else {
            game->patrol_frigates[move->to] += move->quantity;
        }
    }
}

static const char *validate_moves(struct game_state *game,
                                  struct frigate_move *moves,
                                  int num_moves, int allowed_moves)
{
    int i;
    struct frigate_move *move;
    int count = 0;

    if (num_moves > allowed_moves) {
        return "Too many moves";
    }

    for (i = 0; i < num_moves; i++) {
        move = &moves[i];

        if (move->from == NUM_LOCATIONS || move->to == NUM_LOCATIONS) {
            return "Invalid location name";
        }
        if (move->from_type == INVALID_MOVE_TYPE ||
            move->to_type == INVALID_MOVE_TYPE) {
            return "Invalid move type";
        }

        if (move->from_type == HARBOR &&
            game->us_frigates[move->from] < move->quantity) {
            return "Invalid move quantity";
        }

        if (move->from_type == PATROL_ZONE) {
            if (!has_patrol_zone(move->from)) {
                return "Move location does not have a patrol zone";
            }
            if (game->patrol_frigates[move->from] < move->quantity) {
                return "Invalid move quantity";
            }
        }

        if (move->to_type == PATROL_ZONE && !has_patrol_zone(move->to)) {
            return "Move location does not have a patrol zone";
        }

        count += move->quantity;
        if (count > allowed_moves) {
            return "Too many moves";
        }
    }

    return NULL;
}

const char *game_move_ships(struct game_state *game, int allowed_moves)
{
    struct frigate_move moves[MAX_FRIGATE_MOVES];
    int num_moves;
    const char *err;

    err = parse_moves(moves, &num_moves);
    if (err) {
        return err;
    }

    err = validate_moves(game, moves, num_moves, allowed_moves);
    if (err) {
        return err;
    }

    move_frigates(game, moves, num_moves);

    /* TODO Resolve battles and move gunboats */

    return NULL;
}

static bool can_build_gunboat(struct game_state *game)
{
    return game->us_gunboats < MAX_GUNBOATS;
}

bool build_gunboat(struct game_state *game)
{
    if (can_build_gunboat(game)) {
        game->us_gunboats++;
        return true;
    }

    return false;
}

static void game_draw_cards(struct game_state *game)
{
    int draw_count = 6;

    if (game->year == 1806) {
        draw_count = game->us_deck_size;
    }

    if (game->year == 1805) {
        shuffle_discard_into_deck(game);
    }

    draw_from_deck(game, draw_count);
}

static const char *discard_down(struct game_state *game)
{
    char *line;
    int idx;

    cprintf(BOLD ITALIC RED, "Too many cards in hand choose a card to "
            "discard\n");
    prompt();

    line = input_getline();
    idx = strtol(line, NULL, 10);
    if (errno != 0 || idx < 0 || idx >= game->hand_size) {
        free(line);
        return "Invalid card number";
    }

    discard_from_hand(game, idx);

    free(line);
    return NULL;
}

void game_loop(struct game_state *game)
{
    const char *err_msg = NULL;
    /* TODO might have to allocate some memory for tbot actions */
    const char *tbot_action = NULL;

    while (true) {
        if (game->season == SPRING) {
            game_draw_cards(game);
        }

    display:
        display_game(game);
        if (tbot_action) {
            cprintf(ITALIC BOLD RED, "%s\n", tbot_action);
        }

        if (err_msg) {
            cprintf(UNDERLINE BOLD RED, "%s\n", err_msg);
        }

        if (game->hand_size > MAX_HAND_SIZE) {
            err_msg = discard_down(game);
            goto display;
        }

        /* TODO Check for battles to resolve and hop back up to display */

        err_msg = handle_input(game);
        if (err_msg) {
            goto display;
        }

        /* Tripoli turn */

        advance_game_round(game);
    }
}
