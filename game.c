#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cards.h"
#include "display.h"
#include "game.h"
#include "input.h"
#include "tbot.h"

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

    game->t_corsairs_tripoli = 5;
    game->t_corsairs_gibraltar = 2;
    for (i = 0; i < TRIP_INFANTRY_LOCS; i++) {
        game->t_infantry[i] = 4;
    }

    /* No active battle */
    game->battle_loc = NUM_LOCATIONS;

    init_game_cards(game);
}

bool game_handle_intercept(struct game_state *game, enum locations location)
{
    int frig_count;
    bool lieutenant_played = false;
    int rolls = 0;
    int successes = 0;
    unsigned int *corsairs;
    /* Used for signal books overboard check by tbot */
    bool intercepted = false;

    assert(has_patrol_zone(location));

    frig_count = game->patrol_frigates[location];
    if (frig_count) {
        intercepted = true;
    }

    if (location == TRIPOLI && game->swedish_frigates_active) {
        frig_count += 2;
        rolls += 2 * FRIGATE_DICE;
    }

    if (frig_count == 0) {
        return false;;
    }

    if (game->patrol_frigates[location]) {
        lieutenant_played = check_play_battle_card(game, &lieutenant_in_pursuit);
    }

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

    return intercepted;
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

enum battle_type location_battle(struct game_state *game,
                                 enum locations location)
{
    int idx;
    bool us_frigates = game->us_frigates[location] > 0;
    bool us_marines = false;
    bool arab_infantry = false;

    bool trip_infantry = false;
    bool trip_frigates = false;
    bool trip_corsairs = false;
    bool ally_corsairs = false;

    /* Never any fighting in Gibraltar */
    if (location == GIBRALTAR) {
        return false;
    }

    if (has_us_infantry(location)) {
        idx = us_infantry_idx(location);
        us_marines = game->marine_infantry[idx] > 0;
        arab_infantry = game->arab_infantry[idx] > 0;
    }

    if (has_trip_infantry(location)) {
        idx = trip_infantry_idx(location);
        trip_infantry = game->t_infantry[idx] > 0;
    }

    if (location == TRIPOLI) {
        trip_frigates = game->t_frigates > 0;
        trip_corsairs = game->t_corsairs_tripoli > 0;
    }

    if (has_trip_allies(location)) {
        ally_corsairs = game->t_allies[location] > 0;
    }

    if (us_frigates && (trip_corsairs || trip_frigates || ally_corsairs)) {
        return NAVAL_BATTLE;
    }

    if (us_frigates && trip_infantry) {
        return NAVAL_BOMBARDMENT;
    }

    /* Ground battle only happens with Hamet's Army or Assault on Tripoli is
     * played */
    if ((us_marines || arab_infantry) && trip_infantry) {
        return GROUND_BATTLE;
    }

    return BTYPE_NONE;
}

void move_frigates(struct game_state *game, struct frigate_move *moves,
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

const char *validate_moves(struct game_state *game,
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
    if ((idx == 0 && errno != 0) || idx < 0 || idx >= game->hand_size) {
        free(line);
        return "Invalid card number";
    }

    discard_from_hand(game, idx);

    free(line);
    return NULL;
}

static bool battles_to_handle(struct game_state *game)
{
    int i;

    for (i = 0; i < NUM_LOCATIONS; i++) {
        if (location_battle(game, i) != BTYPE_NONE) {
            return true;
        }
    }

    return false;
}

static void return_to_malta(struct game_state *game,
                            enum locations location)
{
    game->us_frigates[MALTA] += game->us_frigates[location];
    game->us_frigates[location] = 0;
    game->used_gunboats += game->assigned_gunboats;
    game->assigned_gunboats = 0;
}

static const char *resolve_naval_battle(struct game_state *game,
                                        enum locations location)
{
    bool prebles_boys_played;
    int dice;
    int successes = 0;
    int damage;
    const char *err;

    err = parse_assign_gunboats(game, location, HARBOR);
    if (err != NULL) {
        return err;
    }

    prebles_boys_played = check_play_battle_card(game, &prebles_boys);
    if (prebles_boys_played) {
        dice = game->us_frigates[location] * 3;
    } else {
        dice = game->us_frigates[location] * FRIGATE_DICE;
    }

    dice += game->assigned_gunboats;

    while (dice--) {
        if (rolld6() == 6) {
            successes++;
        }
    }

    damage = tbot_resolve_naval_battle(game, location, successes);
    display_game(game); /* After resolving the bot battle turn refresh the
                         * display */
    while ((err = parse_damage_assignment(game, location, HARBOR, damage,
                                          NAVAL_BATTLE))) {
        cprintf(BOLD RED, "%s\n", err);
    }

    if (!game->victory_or_death) {
        return_to_malta(game, location);
    }
    return NULL;
}

static const char *resolve_naval_bombardment(struct game_state *game,
                                             enum locations location)
{
    int idx;
    int dice = game->us_frigates[location] * FRIGATE_DICE;
    int successes = 0;
    const char *err;

    assert(has_trip_infantry(location));

    err = parse_assign_gunboats(game, location, HARBOR);
    if (err) {
        return err;
    }

    dice += game->assigned_gunboats;

    while (dice--) {
        if (rolld6() == 6) {
            successes++;
        }
    }

    /* We just handle the damage for the tbot since there's only one way to
     * assign it */
    idx = trip_infantry_idx(location);
    if (successes > game->t_infantry[idx]) {
        game->t_infantry[idx] = 0;
    } else {
        game->t_infantry[idx] -= successes;
    }

    /* Even in the assault on tripoli we want to send them back as there's only
     * 1 round of bombardment and it makes the checks easier */
    return_to_malta(game, location);

    return NULL;
}

static const char *resolve_ground_combat(struct game_state *game,
                                         enum locations location)
{
    int idx;
    int successes;
    int dice;
    int roll;
    int damage;
    const char *err;
    enum battle_type btype;
    bool lieutenant_played;
    bool sharpshooters_played;

    assert(has_us_infantry(location));

    /* The bot will play mercenaries desert before we pick our cards but the
     * player won't see the effects until the next display so the end visibility
     * is the same. The bot also only plays this when general eaton attacks
     * derne so we really only have to check that the location is derne
     */
    if (location == DERNE) {
        tbot_plays_mercenaries_desert(game);
    }

    lieutenant_played =
        check_play_battle_card(game, &lieutenant_leads_the_charge);
    sharpshooters_played = check_play_battle_card(game, &marine_sharpshooters);

    while (true) {
        idx = us_infantry_idx(location);
        successes = 0;
        dice = game->marine_infantry[idx];
        /* We assume you never kill the super marine */
        if (lieutenant_played && game->marine_infantry[idx] > 0) {
            dice += 2;
        }

        while (dice--) {
            roll = rolld6();
            if (roll == 6 || (sharpshooters_played && roll == 5)) {
                successes++;
            }
        }

        dice = game->arab_infantry[idx];
        while (dice--) {
            if (rolld6() == 6) {
                successes++;
            }
        }

        damage = tbot_resolve_ground_combat(game, location, successes);
        display_game(game);

        while ((err = parse_damage_assignment(game, location, HARBOR, damage,
                                              GROUND_BATTLE))) {
            cprintf(BOLD RED, "%s\n", err);
        }

        btype = location_battle(game, location);
        if (btype == BTYPE_NONE) {
            break;
        }
        assert(btype == GROUND_BATTLE);
    }

    return NULL;
}

static const char *resolve_battle(struct game_state *game,
                                  enum locations battle_loc)
{
    enum battle_type btype;

    if (battle_loc == NUM_LOCATIONS) {
        return "Invalid location";
    }

    btype = location_battle(game, battle_loc);
    if (btype == BTYPE_NONE) {
        return "No battle is occurring at this location";
    }

    game->battle_loc = battle_loc;

    switch (btype) {
        case NAVAL_BATTLE:
            return resolve_naval_battle(game, battle_loc);
        case NAVAL_BOMBARDMENT:
            return resolve_naval_bombardment(game, battle_loc);
        case GROUND_BATTLE:
            return resolve_ground_combat(game, battle_loc);
        default:
            assert(false);
    }

    return NULL;
}

static const char *handle_battles(struct game_state *game)
{
    char *line;
    enum locations battle_loc;

    if (game->victory_or_death) {
        return resolve_battle(game, TRIPOLI);
    }

    cprintf(BOLD WHITE, "Enter a location to resolve a battle at: ");
    line = input_getline();
    battle_loc = parse_location(line);

    free(line);
    return resolve_battle(game, battle_loc);
}

static inline void print_err_msg(const char *err_msg)
{
    if (err_msg != NULL) {
        cprintf(UNDERLINE BOLD RED, "%s\n", err_msg);
    }
}

static void check_tripoli_win(struct game_state *game)
{
    if (tripolitan_win(game)) {
        display_game(game); /* Re-render the window to show wincons */
        cprintf(ITALIC RED, "The tripolitan pirates have won!\n");
        exit(0);
    }
}

void game_loop(struct game_state *game)
{
    const char *err_msg = NULL;

    while (true) {
        if (game->season == SPRING) {
            game_draw_cards(game);
        }

    display:
        display_game(game);
        print_err_msg(err_msg);

        if (game->hand_size > MAX_HAND_SIZE) {
            err_msg = discard_down(game);
            goto display;
        }

        err_msg = handle_input(game);
        if (err_msg) {
            goto display;
        }

        while (battles_to_handle(game)) {
            display_game(game);
            print_err_msg(err_msg);
            err_msg = handle_battles(game);
            game->battle_loc = NUM_LOCATIONS;
            check_tripoli_win(game);
        }
        game->battle_loc = NUM_LOCATIONS;
        game->used_gunboats = 0;
        game->assigned_gunboats = 0;

        if (game->victory_or_death &&
            game->t_infantry[trip_infantry_idx(TRIPOLI)] == 0) {
            display_game(game);
            cprintf(ITALIC BLUE, "US Victory via Assault on Tripoli\n");
            exit(0);
        }

        tbot_do_turn(game);
        check_tripoli_win(game);

        advance_game_round(game);
    }
}

const char *assign_damage(struct game_state *game, enum locations location,
                          enum move_type type, int destroy_frigates,
                          int damage_frigates, int destroy_gunboats,
                          int destroy_marines, int destroy_arabs)
{
    unsigned int *frigate_ptr;
    int total_dmg;
    int total_hp;
    int idx = -1;

    if ((destroy_marines || destroy_arabs) &&
        (destroy_frigates || damage_frigates || destroy_gunboats)) {
        return "Can't assign damage from naval battles to ground troops";
    }

    if (destroy_gunboats > game->assigned_gunboats) {
        return "Not enough participating gunboats to destroy";
    }

    if (destroy_marines > 0 || destroy_arabs > 0) {
        if (!has_us_infantry(location)) {
            return "No infantry at this location to destroy";
        }
        idx = us_infantry_idx(location);
    }

    /* Naval combat */
    if (idx == -1) {
        if (type == HARBOR) {
            frigate_ptr = &game->us_frigates[location];
        } else {
            assert(has_patrol_zone(location));
            frigate_ptr = &game->patrol_frigates[location];
        }

        total_dmg = damage_frigates + 2 * destroy_frigates;
        total_hp = *frigate_ptr * 2 + game->used_gunboats +
            game->us_damaged_frigates;

        if (total_dmg > total_hp ||
            (damage_frigates + destroy_frigates > *frigate_ptr)) {
            return "Incorrect damage assignment";
        }

        if (*frigate_ptr < destroy_frigates) {
            return "Destroying too many frigates";
        }

        /* First destroy any frigates */
        *frigate_ptr -= destroy_frigates;

        if (*frigate_ptr < damage_frigates) {
            /* This is only allowed in the assault on tripoli, flow over damaged
             * frigates and destroy previously damaged ones
             */
            assert(game->victory_or_death);
            damage_frigates -= *frigate_ptr;
            *frigate_ptr = 0;
            assert(game->us_damaged_frigates >= damage_frigates);
            game->us_damaged_frigates -= damage_frigates;
            damage_frigates = 0;
        } else {
            *frigate_ptr -= damage_frigates;
        }

        if (game->victory_or_death) {
            game->us_damaged_frigates += damage_frigates;
        } else if (game->year <= 1805) {
            game->turn_track_frigates[year_to_frigate_idx(game->year + 1)] +=
                damage_frigates;
        }

        game->us_gunboats -= destroy_gunboats;
        game->used_gunboats -= destroy_gunboats;

        game->destroyed_us_frigates += destroy_frigates;
    } else { /* Ground combat */
        if (destroy_marines > game->marine_infantry[idx] ||
            destroy_arabs > game->arab_infantry[idx]) {
            return "Incorrect damage assignment";
        }
        game->marine_infantry[idx] -= destroy_marines;
        game->arab_infantry[idx] -= destroy_arabs;
    }

    return NULL;
}
