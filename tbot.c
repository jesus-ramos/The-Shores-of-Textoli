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

/* Tbot core cards */

struct card yusuf_qaramanli = {
    .name = "Yusuf Qaramanli",
    .text = "Pirate Raid with the corsairs from the "
    "harbor of Tripoli and the corsairs from "
    "the harbor of each active ally (Algiers, "
    "Tangier, Tunis).",
};

struct card murad_reis_breaks_out = {
    .name = "Murad Reis Breaks Out",
    .text = "Move the two Tripolitan corsairs from "
    "the harbor of Gibraltar to the harbor of "
    "Tripoli. Any American frigates in the "
    "naval patrol zone of Gibraltar may first "
    "make an Interception Roll.",
};

struct card constantinople_sends_aid = {
    .name = "Constantinople Sends Aid",
    .text = "Playable if Hamet’s Army has captured "
    "Derne. Place one Tripolitan frigate and "
    "two Tripolitan corsairs in the harbor of "
    "Tripoli. Place two Tripolitan infantry "
    "units in the city of Tripoli."
};

/* Tbot deck */
struct card us_supplies_run_low = {
    .name = "US Supplies Run Low",
    .text = "Move one American frigate from any "
    "naval patrol zone to the harbor of Malta."
};

/* 2 copies */
struct card algerine_corsairs_raid = {
    .name = "Algerine Corsairs Raid",
    .text = "Pirate Raid with all corsairs from the "
    "harbor of Algiers."
};

/* 2 copies */
struct card moroccan_corsairs_raid = {
    .name = "Moroccan Corsairs Raid",
    .text = "Pirate Raid with all corsairs from the "
    "harbor of Tangier."
};

/* 2 copies */
struct card tunisian_corsairs_raid = {
    .name = "Tunisian Corsairs Raid",
    .text = "Pirate Raid with all corsairs from the "
    "harbor of Tunis."
};

struct card troops_to_derne = {
    .name = "Troops to Derne",
    .text = "Playable if Hamet’s Army has not "
    "captured Derne. Place two Tripolitan "
    "infantry in the city of Derne."
};

struct card troops_to_benghazi = {
    .name = "Troops to Benghazi",
    .text = "Playable if Hamet’s Army has not "
    "captured Benghazi. Place two Tripolitan "
    "infantry in the city of Benghazi."
};

struct card troops_to_tripoli = {
    .name = "Troops to Tripoli",
    .text = "Place two Tripolitan infantry in the city "
    "of Tripoli."
};

struct card storms = {
    .name = "Storms",
    .text = "Select a naval patrol zone that contains "
    "at least one American frigate. Roll one "
    "die for each American frigate. The first "
    "6 rolled sinks a frigate. Each additional "
    "6 rolled damages a frigate and is placed "
    "on the following year of the Year Turn "
    "Track."
};

struct card tripoli_attacks = {
    .name = "Tripoli Attacks",
    .text = "Move all Tripolitan frigates and corsairs "
    "from the harbor of Tripoli to the naval "
    "patrol zone of Tripoli. Resolve the "
    "battle against the American frigates in "
    "the patrol zone. Any Swedish frigates "
    "in the patrol zone do not participate in "
    "the battle."
};

struct card sweden_pays_tribute = {
    .name = "Sweden Pays Tribute",
    .text = "Playable if it is 1803 or later and there "
    "are Swedish frigates in the naval patrol "
    "zone of Tripoli. Return the Swedish "
    "frigates to the Supply and receive two "
    "Gold Coins."
};

struct card tripoli_acquires_corsairs = {
    .name = "Tripoli Acquires Corsairs",
    .text = "Place two Tripolitan corsairs in the "
    "harbor of Tripoli."
};

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
    "frigate in the harbor of Tripoli."
};

struct card algiers_declares_war = {
    .name = "Algiers Declares War",
    .text = "Place three Algerine corsairs in the "
    "harbor of Algiers."
};

struct card morocco_declares_war = {
    .name = "Morocco Declares War",
    .text = "Place three Moroccan corsairs in the "
    "harbor of Tangier."
};

struct card tunis_declares_war = {
    .name = "Tunis Declares War",
    .text = "Place three Tunisian corsairs in the "
    "harbor of Tunis."
};

struct card second_storms = {
    .name = "Second Storms",
    .text = "Select a naval patrol zone that contains "
    "at least one American frigate. Roll one "
    "die for each American frigate. The first "
    "6 rolled sinks a frigate. Each additional "
    "6 rolled damages a frigate and is placed "
    "on the following year of the Year Turn "
    "Track."
};

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
