#include <assert.h>

#include "cards.h"
#include "game.h"

/* 4 cards that can be added to the end of the event line */
#define TBOT_EVENT_MAX (8)
#define TBOT_EVENT_ADD_IDX (4)

#define array_size(arr) (sizeof(arr) / sizeof(arr[0]))

static void activate_ally(struct game_state *game, enum locations location)
{
    assert(location < TRIP_ALLIES);
    game->t_allies[location] = 3;
}

static void pirate_raid(struct game_state *game, enum locations location);

/* Battle cards */
struct card books_overboard = {
    .name = "US Signal Books Overboard",
    .text = "Playable after any Interception Roll that "
    "includes an American frigate. Randomly "
    "draw one card from the American "
    "player’s hand and place the card in the "
    "discard pile."
};

struct card uncharted_waters = {
    .name = "Uncharted Waters",
    .text = "Playable if The Philadelphia Runs "
    "Aground is the active event card this turn. "
    "Roll two dice instead of one and choose "
    "the preferred result."
};

struct card merchant_ship_converted = {
    .name = "Merchant Ship Converted",
    .text = "Playable if a Tripolitan Pirate Raid has "
    "just been successful. Place one Tripolitan "
    "corsair in the harbor of Tripoli."
};

struct card happy_hunting = {
    .name = "Happy Hunting",
    .text = "Playable when making a Pirate Raid "
    "with Tripolitan corsairs. Roll three "
    "additional dice."
};

struct card the_guns_of_tripoli = {
    .name = "The Guns of Tripoli",
    .text = "Playable during a naval battle in the "
    "harbor of Tripoli. The Tripoli fleet may "
    "roll an additional twelve dice. If played "
    "during the Assault on Tripoli, only roll "
    "the extra dice in the first round of the "
    "naval battle."
};

struct card mercenaries_desert = {
    .name = "Mercenaries Desert",
    .text = "Playable at the start of a land battle. "
    "Before the battle starts, roll one die for "
    "each Arab infantry unit. For each 6, "
    "take an Arab infantry unit and return it "
    "to the Supply"
};

/* Yeah these are globals but I'm a bit lazy */
static struct card *tbot_battle_cards[] = {
    &books_overboard,
    &uncharted_waters,
    &merchant_ship_converted,
    &happy_hunting,
    &the_guns_of_tripoli,
    &mercenaries_desert
};

static bool tbot_check_play_battle_card(struct card *card)
{
    int i;

    for (i = 0; i < array_size(tbot_battle_cards); i++) {
        if (tbot_battle_cards[i] == card) {
            tbot_battle_cards[i] = NULL;
            return true;
        }
    }

    return false;
}

static bool yusuf_playable(struct game_state *game)
{
    int ally_count = 0;
    int i;

    for (i = 0; i < TRIP_ALLIES; i++) {
        if (game->t_allies[i] > 0) {
            ally_count++;
        }
    }

    return (game->year >= 1801 && game->year <= 1804 && ally_count >= 2) ||
        (game->year >= 1805 && ally_count >= 1);
}

static const char *play_yusuf(struct game_state *game)
{
    int i;

    pirate_raid(game, TRIPOLI);
    for (i = 0; i < TRIP_ALLIES; i++) {
        if (game->t_allies[i] > 0) {
            pirate_raid(game, i);
        }
    }

    return NULL;
}

/* Tbot core cards */

struct card yusuf_qaramanli = {
    .name = "Yusuf Qaramanli",
    .text = "Pirate Raid with the corsairs from the "
    "harbor of Tripoli and the corsairs from "
    "the harbor of each active ally (Algiers, "
    "Tangier, Tunis).",
    .playable = yusuf_playable,
    .play = play_yusuf
};

static bool break_out_playable(struct game_state *game)
{
    return game->t_corsairs_gibraltar > 0 &&
        (game->patrol_frigates[GIBRALTAR] == 0 ||
         (game->year == 1801 && game->season == WINTER));
}

static const char *play_break_out(struct game_state *game)
{
    game_handle_intercept(game, GIBRALTAR);

    game->t_corsairs_tripoli += game->t_corsairs_gibraltar;
    game->t_corsairs_gibraltar = 0;

    return NULL;
}

struct card murad_reis_breaks_out = {
    .name = "Murad Reis Breaks Out",
    .text = "Move the two Tripolitan corsairs from "
    "the harbor of Gibraltar to the harbor of "
    "Tripoli. Any American frigates in the "
    "naval patrol zone of Gibraltar may first "
    "make an Interception Roll.",
    .playable = break_out_playable,
    .play = play_break_out
};

static bool send_aid_playable(struct game_state *game)
{
    return hamets_army_at(game, DERNE);
}

static const char *play_send_aid(struct game_state *game)
{
    game->t_frigates++;
    game->t_corsairs_tripoli += 2;
    game->t_infantry[trip_infantry_idx(TRIPOLI)] += 2;

    return NULL;
}

struct card constantinople_sends_aid = {
    .name = "Constantinople Sends Aid",
    .text = "Playable if Hamet’s Army has captured "
    "Derne. Place one Tripolitan frigate and "
    "two Tripolitan corsairs in the harbor of "
    "Tripoli. Place two Tripolitan infantry "
    "units in the city of Tripoli.",
    .playable = send_aid_playable,
    .play = play_send_aid
};

/* Tbot deck */

static bool supplies_run_low_playable(struct game_state *game)
{
    return game->patrol_frigates[TRIPOLI] == 2;
}

static const char *play_supplies_run_low(struct game_state *game)
{
    /* XXX: Should the bot choose a zone with the most frigates or just tripoli,
     * likely that most frigates will be in tripoli patrol zone but... you never
     * know
     */

    game->patrol_frigates[TRIPOLI]--;
    game->us_frigates[MALTA]++;

    return NULL;
}

struct card us_supplies_run_low = {
    .name = "US Supplies Run Low",
    .text = "Move one American frigate from any "
    "naval patrol zone to the harbor of Malta.",
    .playable = supplies_run_low_playable,
    .play = play_supplies_run_low
};

/* With the corsairs raid cards, the bot will never play them directly and will
 * always discard to raid or build instead
 */
static bool corsairs_raid_playable(struct game_state *game)
{
    return false;
}

static const char *play_corsairs_raid(struct game_state *game)
{
    assert(false);
    return NULL;
}

/* 2 copies */
struct card algerine_corsairs_raid = {
    .name = "Algerine Corsairs Raid",
    .text = "Pirate Raid with all corsairs from the "
    "harbor of Algiers.",
    .playable = corsairs_raid_playable,
    .play = play_corsairs_raid
};

/* 2 copies */
struct card moroccan_corsairs_raid = {
    .name = "Moroccan Corsairs Raid",
    .text = "Pirate Raid with all corsairs from the "
    "harbor of Tangier.",
    .playable = corsairs_raid_playable,
    .play = play_corsairs_raid
};

/* 2 copies */
struct card tunisian_corsairs_raid = {
    .name = "Tunisian Corsairs Raid",
    .text = "Pirate Raid with all corsairs from the "
    "harbor of Tunis.",
    .playable = corsairs_raid_playable,
    .play = play_corsairs_raid
};

static bool troops_to_derne_playable(struct game_state *game)
{
    return game->t_infantry[trip_infantry_idx(DERNE)] > 0;
}

static const char *play_troops_to_derne(struct game_state *game)
{
    game->t_infantry[trip_infantry_idx(DERNE)] += 2;
    return NULL;
}

struct card troops_to_derne = {
    .name = "Troops to Derne",
    .text = "Playable if Hamet’s Army has not "
    "captured Derne. Place two Tripolitan "
    "infantry in the city of Derne.",
    .playable = troops_to_derne_playable,
    .play = play_troops_to_derne
};

static bool troops_to_benghazi_playable(struct game_state *game)
{
    return game->t_infantry[trip_infantry_idx(BENGHAZI)] > 0;
}

static const char *play_troops_to_benghazi(struct game_state *game)
{
    game->t_infantry[trip_infantry_idx(BENGHAZI)] += 2;
    return NULL;
}

struct card troops_to_benghazi = {
    .name = "Troops to Benghazi",
    .text = "Playable if Hamet’s Army has not "
    "captured Benghazi. Place two Tripolitan "
    "infantry in the city of Benghazi.",
    .playable = troops_to_benghazi_playable,
    .play = play_troops_to_benghazi
};

static const char *play_troops_to_tripoli(struct game_state *game)
{
    game->t_infantry[trip_infantry_idx(TRIPOLI)] += 2;
    return NULL;
}

struct card troops_to_tripoli = {
    .name = "Troops to Tripoli",
    .text = "Place two Tripolitan infantry in the city "
    "of Tripoli.",
    .playable = always_playable,
    .play = play_troops_to_tripoli
};

static bool storms_playable(struct game_state *game)
{
    int i;

    for (i = 0; i < PATROL_ZONES; i++) {
        if (game->patrol_frigates[i] >= 2) {
            return true;
        }
    }

    return false;
}

static int storm_score(struct game_state *game, enum locations location)
{
    assert(has_patrol_zone(location));

    if (game->patrol_frigates[location] < 2) {
        return -1;
    }

    return game->patrol_frigates[location];
}

static const char *play_storms(struct game_state *game)
{
    int i;
    int scores[PATROL_ZONES];
    int max_score = -1;
    int score_idx = -1;
    int score_count = 0;
    int successes = 0;
    int rolls;

    for (i = 0; i < PATROL_ZONES; i++) {
        scores[i] = storm_score(game, i);
        if (scores[i] > max_score) {
            max_score = scores[i];
            score_count = 1;
        } else if (scores[i] == max_score) {
            score_count++;
        }
    }

    assert(max_score != -1);

    score_count = rand() % score_count + 1;

    for (i = 0; i < PATROL_ZONES; i++) {
        if (scores[i] == max_score) {
            if (--score_count == 0) {
                score_idx = i;
                break;
            }
        }
    }

    assert(score_idx != -1);

    rolls = game->patrol_frigates[score_idx];
    while (rolls--) {
        if (rolld6() == 6) {
            successes++;
        }
    }

    if (successes > 0) {
        game->patrol_frigates[score_idx]--;
        game->destroyed_us_frigates++;
        successes--;
    }

    game->patrol_frigates[score_idx] -= successes;
    if (game->year < END_YEAR) {
        game->turn_track_frigates[year_to_frigate_idx(game->year + 1)] +=
            successes;
    }

    return NULL;
}

struct card storms = {
    .name = "Storms",
    .text = "Select a naval patrol zone that contains "
    "at least one American frigate. Roll one "
    "die for each American frigate. The first "
    "6 rolled sinks a frigate. Each additional "
    "6 rolled damages a frigate and is placed "
    "on the following year of the Year Turn "
    "Track.",
    .playable = storms_playable,
    .play = play_storms
};

static bool tripoli_attacks_playable(struct game_state *game)
{
    int trip_dice = game->t_corsairs_tripoli + game->t_frigates * FRIGATE_DICE;

    return trip_dice >= 5 && game->patrol_frigates[TRIPOLI] == 1;
}

struct card tripoli_attacks = {
    .name = "Tripoli Attacks",
    .text = "Move all Tripolitan frigates and corsairs "
    "from the harbor of Tripoli to the naval "
    "patrol zone of Tripoli. Resolve the "
    "battle against the American frigates in "
    "the patrol zone. Any Swedish frigates "
    "in the patrol zone do not participate in "
    "the battle.",
    .playable = tripoli_attacks_playable
};

static bool sweden_pays_tribute_playable(struct game_state *game)
{
    return game->year >= 1803 && game->swedish_frigates_active;
}

static const char *play_sweden_pays_tribute(struct game_state *game)
{
    game->swedish_frigates_active = false;
    game->pirated_gold += 2;

    return NULL;
}

struct card sweden_pays_tribute = {
    .name = "Sweden Pays Tribute",
    .text = "Playable if it is 1803 or later and there "
    "are Swedish frigates in the naval patrol "
    "zone of Tripoli. Return the Swedish "
    "frigates to the Supply and receive two "
    "Gold Coins.",
    .playable = sweden_pays_tribute_playable,
    .play = play_sweden_pays_tribute
};

static bool acquire_corsairs_playable(struct game_state *game)
{
    int corsairs = game->t_corsairs_gibraltar + game->t_corsairs_tripoli;

    return corsairs <= MAX_TRIPOLI_CORSAIRS - 2;
}

static const char *play_tripoli_acquires_corsairs(struct game_state *game)
{
    game->t_corsairs_tripoli += 2;
    return NULL;
}

struct card tripoli_acquires_corsairs = {
    .name = "Tripoli Acquires Corsairs",
    .text = "Place two Tripolitan corsairs in the "
    "harbor of Tripoli.",
    .playable = acquire_corsairs_playable,
    .play = play_tripoli_acquires_corsairs
};

static bool philly_runs_aground_playable(struct game_state *game)
{
    return game->patrol_frigates[TRIPOLI] > 0;
}

static const char *play_philly_runs_aground(struct game_state *game)
{
    int roll = rolld6();
    bool uncharted_waters_played =
        tbot_check_play_battle_card(&uncharted_waters);

    if (uncharted_waters_played) {
        roll = max(roll, rolld6());
    }

    switch (roll) {
        case 5:
        case 6:
            game->t_frigates++;
        case 3:
        case 4:
            game->patrol_frigates[TRIPOLI]--;
            game->destroyed_us_frigates++;
    }

    return NULL;
}

struct card philly_runs_aground = {
    .name = "The Philadelphia Runs Aground",
    .text = "Playable if there is at least one American "
    "frigate in the naval patrol zone of Tripoli. "
    "Roll one die and apply the result: "
    "1-2: Minor damage. Move an American "
    "frigate to the harbor of Malta. "
    "3-4: Frigate sunk. "
    "5-6: Frigate captured. Take the American "
    "frigate as “sunk” and place one Tripolitan "
    "frigate in the harbor of Tripoli.",
    .playable = philly_runs_aground_playable,
    .play = play_philly_runs_aground
};

static const char *play_algiers_declares_war(struct game_state *game)
{
    activate_ally(game, ALGIERS);
    return NULL;
}

struct card algiers_declares_war = {
    .name = "Algiers Declares War",
    .text = "Place three Algerine corsairs in the "
    "harbor of Algiers.",
    .playable = always_playable,
    .play = play_algiers_declares_war
};

static const char *play_morocco_declares_war(struct game_state *game)
{
    activate_ally(game, TANGIER);
    return NULL;
}

struct card morocco_declares_war = {
    .name = "Morocco Declares War",
    .text = "Place three Moroccan corsairs in the "
    "harbor of Tangier.",
    .playable = always_playable,
    .play = play_morocco_declares_war
};

static const char *play_tunis_declares_war(struct game_state *game)
{
    activate_ally(game, TUNIS);
    return NULL;
}

struct card tunis_declares_war = {
    .name = "Tunis Declares War",
    .text = "Place three Tunisian corsairs in the "
    "harbor of Tunis.",
    .playable = always_playable,
    .play = play_tunis_declares_war
};

struct card second_storms = {
    .name = "Second Storms",
    .text = "Select a naval patrol zone that contains "
    "at least one American frigate. Roll one "
    "die for each American frigate. The first "
    "6 rolled sinks a frigate. Each additional "
    "6 rolled damages a frigate and is placed "
    "on the following year of the Year Turn "
    "Track.",
    .playable = storms_playable, /* It's the same card as Storms */
    .play = play_storms
};

/* We leave the last 2 slots open for Storms and Second Storms */
static struct card *tbot_event_line[TBOT_EVENT_MAX] = {
    &yusuf_qaramanli,
    &murad_reis_breaks_out,
    &constantinople_sends_aid,
    &sweden_pays_tribute
};

static struct card *tbot_deck[] = {
    &us_supplies_run_low,
    &algerine_corsairs_raid, &algerine_corsairs_raid,
    &moroccan_corsairs_raid, &moroccan_corsairs_raid,
    &tunisian_corsairs_raid, &tunisian_corsairs_raid,
    &troops_to_derne, &troops_to_benghazi, &troops_to_tripoli,
    &storms, &tripoli_attacks, &tripoli_acquires_corsairs,
    &philly_runs_aground,
    &algiers_declares_war, &morocco_declares_war, &tunis_declares_war,
    &second_storms
};

static unsigned int tbot_deck_size = array_size(tbot_deck);

int tbot_resolve_naval_battle(struct game_state *game, enum locations location,
                              int damage)
{
    /* TODO */
    return 0;
}

int tbot_resolve_ground_combat(struct game_state *game, enum locations location,
                               int damage)
{
    /* TODO */
    return 0;
}

static inline bool five_corsair_check(struct game_state *game)
{
    return game->t_corsairs_tripoli >= 5;
}

static bool check_add_card_to_event_line(struct game_state *game,
                                         struct card *card)
{
    int i;

    if (!card->playable(game) &&
        (card == &storms || card == &second_storms ||
         card == &philly_runs_aground || card == &tripoli_acquires_corsairs)) {
        for (i = TBOT_EVENT_ADD_IDX; i < TBOT_EVENT_MAX; i++) {
            if (tbot_event_line[i] == NULL) {
                tbot_event_line[i] = card;
                return true;
            }
        }
    }


    return false;
}

static bool tbot_draw_play_card(struct game_state *game)
{
    int card_idx;
    struct card *card;

draw_new_card:
    if (tbot_deck_size == 0) {
        return false;
    }

    card_idx = rand() % tbot_deck_size;
    card = tbot_deck[card_idx];

    /* tbot cards go away forever even if unplayed */
    tbot_deck[card_idx] = tbot_deck[tbot_deck_size - 1];
    tbot_deck[--tbot_deck_size] = NULL;

    assert(card && card->playable && card->play);

    if (check_add_card_to_event_line(game, card)) {
        goto draw_new_card;
    }

    if (card->playable(game)) {
        assert(card->play);
        card->play(game);
        return true;
    }

    return false;
}

static bool tbot_process_event_line(struct game_state *game)
{
    int i;
    struct card *card;

    for (i = 0; i < array_size(tbot_event_line); i++) {
        card = tbot_event_line[i];
        assert(!card || (card->play && card->playable));
        if (card && card->playable(game)) {
            card->play(game);
            tbot_event_line[i] = NULL;
            return true;
        }
    }

    return false;
}

static void pirate_raid(struct game_state *game, enum locations location)
{
    int i;
    int successes = 0;
    int raid_count;

    game_handle_intercept(game, location);

    raid_count = (location == TRIPOLI) ? game->t_corsairs_tripoli :
        game->t_allies[location];

    if (tbot_check_play_battle_card(&happy_hunting)) {
        raid_count += 3;
    }

    for (i = 0; i < raid_count; i++) {
        if (rolld6() == 6) {
            successes++;
        }
    }

    game->pirated_gold += successes;

    if (successes > 0 && tbot_check_play_battle_card(&merchant_ship_converted)) {
        game->t_corsairs_tripoli++;
    }
}

static int tbot_raid_score(struct game_state *game, enum locations location)
{
    int corsairs;
    int frigs;

    if (location < TRIP_ALLIES) {
        corsairs = game->t_allies[location];
    } else {
        corsairs = game->t_corsairs_tripoli;
    }

    frigs = game->patrol_frigates[location];
    if (location == TRIPOLI && game->swedish_frigates_active) {
        frigs += 2;
    }

    if (corsairs >= 3 && corsairs > frigs) {
        return corsairs - frigs;
    }

    return -1;
}

static void raid_or_build(struct game_state *game)
{
    int i;
    int scores[TRIP_ALLIES + 1];
    int max_score = -1;
    int score_count = 0;
    int score_idx = -1;

    for (i = 0; i < TRIP_ALLIES; i++) {
        scores[i] = tbot_raid_score(game, i);
    }

    scores[TRIP_ALLIES] = tbot_raid_score(game, TRIPOLI);

    for (i = 0; i <= TRIP_ALLIES; i++) {
        if (scores[i] > max_score) {
            max_score = scores[i];
            score_count = 1;
        } else if (scores[i] == max_score) {
            score_count++;
        }
    }

    if (max_score == -1) {
        game->t_corsairs_tripoli++;
        return;
    }

    assert(score_count > 0);
    score_count = rand() % score_count + 1;

    for (i = 0; i <= TRIP_ALLIES; i++) {
        if (scores[i] == max_score) {
            if (--score_count == 0) {
                score_idx = i;
                break;
            }
        }
    }

    assert(score_idx != -1);

    if (score_idx == TRIP_ALLIES) {
        pirate_raid(game, TRIPOLI);
    } else {
        pirate_raid(game, score_idx);
    }
}

void tbot_do_turn(struct game_state *game)
{
    if (tbot_process_event_line(game)) {
        return;
    }

    if (five_corsair_check(game)) {
        pirate_raid(game, TRIPOLI);
        return;
    }

    if (tbot_draw_play_card(game)) {
        return;
    }

    raid_or_build(game);
}
