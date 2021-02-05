#include <assert.h>

#include "cards.h"
#include "game.h"

static inline bool five_corsair_check(struct game_state *game)
{
    return game->t_corsairs_tripoli >= 5;
}

static void activate_ally(struct game_state *game, enum locations location)
{
    assert(location < TRIP_ALLIES);
    game->t_allies[location] = 3;
}

static bool pirate_raid(struct game_state *game, enum locations location,
                        bool happy_hunting)
{
    int i;
    int successes = 0;
    int raid_count = (location == TRIPOLI) ? game->t_corsairs_tripoli :
        game->t_allies[location];

    if (happy_hunting)
        raid_count += 3;

    for (i = 0; i < raid_count; i++)
        if (rolld6() == 6)
            successes++;

    game->pirated_gold += successes;

    return successes > 0;
}
