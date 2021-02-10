#ifndef GAME_H
#define GAME_H

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum locations {
    TANGIER = 0,
    ALGIERS,
    TUNIS,
    GIBRALTAR,
    TRIPOLI,
    BENGHAZI,
    DERNE,
    ALEXANDRIA,
    MALTA,
    NUM_LOCATIONS,
    /* Laid these out in a way that the math on these works kinda nice */
    PATROL_ZONES = TRIPOLI + 1,
    TRIP_ALLIES = TUNIS + 1,
    TRIP_INFANTRY_START = TRIPOLI,
    TRIP_INFANTRY_END = DERNE,
    TRIP_INFANTRY_LOCS = TRIP_INFANTRY_END - TRIP_INFANTRY_START + 1,
    US_INFANTRY_START = TRIPOLI,
    US_INFANTRY_END = ALEXANDRIA,
    US_INFANTRY_LOCS = US_INFANTRY_END - US_INFANTRY_START + 1
};

enum seasons {
    SPRING,
    SUMMER,
    FALL,
    WINTER
};

enum battle_type {
    BTYPE_NONE,
    NAVAL_BATTLE,
    NAVAL_BOMBARDMENT,
    GROUND_BATTLE
};

#define INFANTRY_DICE 1
#define FRIGATE_DICE 2
#define GUNBOAT_DICE 1
#define CORSAIR_DICE 1

struct game_state {
    unsigned int seed;
#define START_YEAR (1801)
#define END_YEAR (1806)
    unsigned int year;
    enum seasons season;

    /* Tripolitan player */
    /* Wincons */
#define DESTROYED_FRIGATES_WIN (4)
    unsigned int destroyed_us_frigates;
#define GOLD_WIN (12)
    unsigned int pirated_gold;
    unsigned int t_frigates;
    unsigned int t_damaged_frigates;
#define MAX_TRIPOLI_CORSAIRS (9)
    unsigned int t_corsairs_tripoli;
    unsigned int t_corsairs_gibraltar;
    unsigned int t_allies[TRIP_ALLIES]; /* Corsair count at each location */
    unsigned int t_infantry[TRIP_INFANTRY_LOCS];
    unsigned int t_turn_frigates[END_YEAR - START_YEAR];
    bool tripoli_attacks;

    /* US Player */
#define MAX_GUNBOATS 3
    unsigned int us_gunboats;
    bool swedish_frigates_active;
    unsigned int patrol_frigates[PATROL_ZONES];
    unsigned int arab_infantry[US_INFANTRY_LOCS];
    unsigned int marine_infantry[US_INFANTRY_LOCS];
    unsigned int us_frigates[NUM_LOCATIONS];
    unsigned int turn_track_frigates[END_YEAR - START_YEAR];

#define US_CORE_CARD_COUNT (3)
#define US_DECK_SIZE (24)
#define MAX_HAND_SIZE (8)
    struct card **us_core_cards;
    struct card **us_deck;
    unsigned int us_deck_size;
    struct card *us_hand[US_DECK_SIZE]; /* A little overkill but its fine */
    unsigned int hand_size;
    struct card *us_discard[US_DECK_SIZE];
    unsigned int discard_size;

    /* Battle info */
    unsigned int used_gunboats;
    unsigned int assigned_gunboats;
    enum locations battle_loc;
    bool victory_or_death;
    /* Track assault on tripoli damaged frigates */
    unsigned int us_damaged_frigates;
};

#define MAX_FRIGATE_MOVES (8)

/* Could use a rename */
enum move_type {
    HARBOR,
    PATROL_ZONE,
    INVALID_MOVE_TYPE
};

struct frigate_move {
    enum locations from;
    enum move_type from_type;
    enum locations to;
    enum move_type to_type;
    unsigned int quantity;
};

static inline unsigned int trip_infantry_idx(enum locations location)
{
    assert(location >= TRIP_INFANTRY_START);
    assert(location <= TRIP_INFANTRY_END);

    return location - TRIP_INFANTRY_START;
}

static inline unsigned int us_infantry_idx(enum locations location)
{
    assert(location >= US_INFANTRY_START);
    assert(location <= US_INFANTRY_END);

    return location - US_INFANTRY_START;
}

static inline bool tripolitan_win(struct game_state *game)
{
    return game->destroyed_us_frigates >= DESTROYED_FRIGATES_WIN ||
        game->pirated_gold >= GOLD_WIN;
}

static inline bool game_draw(struct game_state *game)
{
    return game->year == END_YEAR && game->season == WINTER;
}

static inline unsigned int year_to_frigate_idx(unsigned int year)
{
    assert(year >= 1802);

    return year - 1802;
}

static inline bool is_date_on_or_past(struct game_state *game, unsigned int year,
                                      enum seasons season)
{
    return (game->year == year && game->season >= season) || (game->year > year);
}

static inline unsigned int rolld6()
{
    return rand() % 6 + 1;
}

static inline const char *season_str(enum seasons season)
{
    switch (season) {
        case SPRING:
            return "Spring";
        case SUMMER:
            return "Summer";
        case FALL:
            return "Fall";
        case WINTER:
            return "Winter";
        default:
            assert(false);
            return "";
    }
}

static inline char *move_type_str(enum move_type type)
{
    switch (type) {
        case PATROL_ZONE:
            return "Patrol Zone";
        case HARBOR:
            return "Harbor";
        default:
            assert(false);
            return "";
    }
}

static inline const char *location_str(enum locations location)
{
    switch (location) {
        case TANGIER:
            return "Tangier";
        case ALGIERS:
            return "Algiers";
        case TUNIS:
            return "Tunis";
        case GIBRALTAR:
            return "Gibraltar";
        case TRIPOLI:
            return "Tripoli";
        case BENGHAZI:
            return "Benghazi";
        case DERNE:
            return "Derne";
        case ALEXANDRIA:
            return "Alexandria";
        case MALTA:
            return "Malta";
        default:
            assert(false);
            return "";
    }
}

static inline void str_tolower(char *str)
{
    int i;

    for (i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

static inline enum move_type parse_move_type(char *str)
{
    str_tolower(str);

    if (strcmp(str, "harbor") == 0) {
        return HARBOR;
    } else if (strcmp(str, "patrol") == 0) {
        return PATROL_ZONE;
    } else {
        return INVALID_MOVE_TYPE;
    }
}

static inline enum locations parse_location(char *str)
{
    str_tolower(str);

    if (strcmp(str, "tangier") == 0) {
        return TANGIER;
    } else if (strcmp(str, "algiers") == 0) {
        return ALGIERS;
    } else if (strcmp(str, "tunis") == 0) {
        return TUNIS;
    } else if (strcmp(str, "gibraltar") == 0) {
        return GIBRALTAR;
    } else if (strcmp(str, "tripoli") == 0) {
        return TRIPOLI;
    } else if (strcmp(str, "benghazi") == 0) {
        return BENGHAZI;
    } else if (strcmp(str, "derne") == 0) {
        return DERNE;
    } else if (strcmp(str, "alexandria") == 0) {
        return ALEXANDRIA;
    } else if (strcmp(str, "malta") == 0) {
        return MALTA;
    } else {
        return NUM_LOCATIONS;
    }
}

static inline bool has_patrol_zone(enum locations location)
{
    return location < PATROL_ZONES;
}

static inline bool has_trip_allies(enum locations location)
{
    return location < TRIP_ALLIES;
}

static inline bool has_trip_infantry(enum locations location)
{
    return location >= TRIP_INFANTRY_START && location <= TRIP_INFANTRY_END;
}

static inline bool has_us_infantry(enum locations location)
{
    return location >= US_INFANTRY_START && location <= US_INFANTRY_END;
}

static inline bool hamets_army_at(struct game_state *game,
                                  enum locations location)
{
    int idx = us_infantry_idx(location);

    return game->marine_infantry[idx] > 0 || game->arab_infantry[idx] > 0;
}

static inline bool tripoli_corsair_location(enum locations location)
{
    return location == TRIPOLI || location == GIBRALTAR;
}

static inline unsigned int *tripoli_corsair_ptr(struct game_state *game,
                                                enum locations location)
{
    assert(tripoli_corsair_location(location));

    if (location == TRIPOLI) {
        return &game->t_corsairs_tripoli;
    }
    return &game->t_corsairs_gibraltar;
}

static inline unsigned int max(unsigned int a, unsigned int b)
{
    return (a > b) ? a : b;
}

static inline unsigned int min(unsigned int a, unsigned int b)
{
    return (a < b) ? a : b;
}

void init_game_state(struct game_state *game, unsigned int seed);
void game_loop(struct game_state *game);
bool build_gunboat(struct game_state *game);
const char *game_move_ships(struct game_state *game, int allowed_moves);
bool game_handle_intercept(struct game_state *game, enum locations location);
const char *assign_damage(struct game_state *game, enum locations location,
                          enum move_type type, int destroy_frigates,
                          int damage_frigates, int destroy_gunboats,
                          int destroy_marines, int destroy_arabs);
enum battle_type location_battle(struct game_state *game,
                                 enum locations location);
const char *validate_moves(struct game_state *game, struct frigate_move *moves,
                           int num_moves, int allowed_moves);
void move_frigates(struct game_state *game, struct frigate_move *moves,
                   int num_moves);

#endif /* GAME_H */
