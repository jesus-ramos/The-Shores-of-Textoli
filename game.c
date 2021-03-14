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
    game->gunboat_loc = INVALID_LOCATION;

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

    if (intercepted) {
        lieutenant_played = check_play_battle_card(game, &lieutenant_in_pursuit);
    }

    if (lieutenant_played) {
        rolls += 3 * game->patrol_frigates[location];
    } else {
        rolls += FRIGATE_DICE * game->patrol_frigates[location];
    }

    successes = rolld6s(rolls, 6);

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

    if (game_draw(game)) {
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
        if (move->from_zone == HARBOR) {
            game->us_frigates[move->from] -= move->quantity;
        } else {
            game->patrol_frigates[move->from] -= move->quantity;
        }

        if (move->to_zone == HARBOR) {
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

        if (move->from == INVALID_LOCATION || move->to == INVALID_LOCATION) {
            return "Invalid location name";
        }
        if (move->from_zone == INVALID_ZONE ||
            move->to_zone == INVALID_ZONE) {
            return "Invalid move type";
        }

        if (move->from_zone == HARBOR &&
            game->us_frigates[move->from] < move->quantity) {
            return "Invalid move quantity";
        }

        if (move->from_zone == PATROL_ZONE) {
            if (!has_patrol_zone(move->from)) {
                return "Move location does not have a patrol zone";
            }
            if (game->patrol_frigates[move->from] < move->quantity) {
                return "Invalid move quantity";
            }
        }

        if (move->to_zone == PATROL_ZONE && !has_patrol_zone(move->to)) {
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

    err = parse_moves(moves, &num_moves, allowed_moves);
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
    if (!game_strtol(line, &idx) || idx < 0 || idx >= game->hand_size) {
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
        dice = (game->us_frigates[location] + game->us_damaged_frigates) * 3;
    } else {
        dice = (game->us_frigates[location] + game->us_damaged_frigates) *
            FRIGATE_DICE;
    }

    dice += game->assigned_gunboats;
    successes = rolld6s(dice, 6);

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
    int dice = (game->us_frigates[location] + game->us_damaged_frigates) *
        FRIGATE_DICE;
    int successes = 0;
    const char *err;

    assert(has_trip_infantry(location));

    err = parse_assign_gunboats(game, location, HARBOR);
    if (err) {
        return err;
    }

    dice += game->assigned_gunboats;
    successes = rolld6s(dice, 6);

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

        successes = rolld6s(dice, (sharpshooters_played) ? 5 : 6);

        dice = game->arab_infantry[idx];
        successes += rolld6s(dice, 6);

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

    if (battle_loc == INVALID_LOCATION) {
        return "Invalid location";
    }

    btype = location_battle(game, battle_loc);
    if (btype == BTYPE_NONE) {
        return "No battle is occurring at this location";
    }

    if (btype != GROUND_BATTLE) {
        game->gunboat_loc = battle_loc;
    }

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
            check_tripoli_win(game);
        }
        game->gunboat_loc = INVALID_LOCATION;
        game->used_gunboats = 0;
        game->assigned_gunboats = 0;

        if (game->victory_or_death) {
            display_game(game);
            if (game->t_infantry[trip_infantry_idx(TRIPOLI)] == 0) {
                cprintf(ITALIC BLUE, "US Victory via Assault on Tripoli!\n");
            } else {
                cprintf(ITALIC RED, "The assault on Tripoli has failed! "
                        "The tripolitan pirates have claimed victory\n");
            }
            exit(EXIT_SUCCESS);
        }

        tbot_do_turn(game);
        check_tripoli_win(game);

        advance_game_round(game);
    }
}

static bool try_auto_assign_naval_battle(struct game_state *game,
                                         enum locations location,
                                         enum zone zone, int num_hits)
{
    unsigned int *frigate_ptr = us_frigate_ptr(game, location, zone);
    int total_hp = *frigate_ptr * 2 + game->assigned_gunboats +
        game->us_damaged_frigates;

    if (num_hits < total_hp) {
        return false;
    }

    *frigate_ptr = 0;
    game->used_gunboats -= game->assigned_gunboats;
    game->assigned_gunboats = 0;
    game->us_damaged_frigates = 0;

    return true;
}

static bool try_auto_assign_ground_battle(struct game_state *game,
                                          enum locations location,
                                          int num_hits)
{
    int idx = us_infantry_idx(location);
    int total_hp = game->marine_infantry[idx] + game->arab_infantry[idx];

    if (num_hits < total_hp) {
        return false;
    }

    game->marine_infantry[idx] = 0;
    game->arab_infantry[idx] = 0;

    return true;
}

bool try_auto_assign_damage(struct game_state *game, enum locations location,
                            enum zone zone, int num_hits, enum battle_type btype)
{
    if (btype == NAVAL_BATTLE) {
        return try_auto_assign_naval_battle(game, location, zone, num_hits);
    }
    return try_auto_assign_ground_battle(game, location, num_hits);
}

const char *assign_naval_damage(struct game_state *game, enum locations location,
                                enum zone zone, int num_hits,
                                int destroy_frigates, int damage_frigates,
                                int destroy_gunboats)
{
    unsigned int *frigate_ptr = us_frigate_ptr(game, location, zone);
    int rem;

    if (num_hits != destroy_frigates * 2 + damage_frigates + destroy_gunboats) {
        return "You must assign all damage for the battle";
    }

    if ((destroy_frigates + damage_frigates) > (*frigate_ptr +
                                                game->us_damaged_frigates)) {
        return "Destroying and damaging more frigates than we have at the "
            "location";
    }

    if (destroy_gunboats > game->assigned_gunboats) {
        return "Not enough gunboats to destroy at the location";
    }

    game->us_gunboats -= destroy_gunboats;
    game->assigned_gunboats -= destroy_gunboats;
    game->used_gunboats -= destroy_gunboats;

    game->destroyed_us_frigates += destroy_frigates;

    *frigate_ptr -= destroy_frigates;
    if (*frigate_ptr < damage_frigates) {
        assert(game->victory_or_death);
        rem = damage_frigates - *frigate_ptr;
        *frigate_ptr = 0;
        game->destroyed_us_frigates += rem;
        game->us_damaged_frigates -= rem;
        damage_frigates = 0;
    } else {
        *frigate_ptr -= damage_frigates;
    }

    if (game->victory_or_death) {
        game->us_damaged_frigates += damage_frigates;
    } else if (game->year < END_YEAR) {
        game->turn_track_frigates[year_to_frigate_idx(game->year + 1)] +=
            damage_frigates;
    }

    return NULL;
}

const char *assign_ground_damage(struct game_state *game,
                                 enum locations location, int num_hits,
                                 int destroy_marines, int destroy_arabs)
{
    int idx = us_infantry_idx(location);

    if (num_hits != destroy_marines + destroy_arabs) {
        return "You must assign all daamge for the battle";
    }

    if (destroy_marines > game->marine_infantry[idx]) {
        return "Assigning more damage than marines at location";
    }

    if (destroy_arabs > game->arab_infantry[idx]) {
        return "Assigning more damage than arab infantry at location";
    }

    game->marine_infantry[idx] -= destroy_marines;
    game->arab_infantry[idx] -= destroy_arabs;

    return NULL;
}
