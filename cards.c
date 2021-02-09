#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "cards.h"
#include "display.h"
#include "game.h"
#include "input.h"

static bool battle_playable(struct game_state *game)
{
    return false;
}

void remove_card_from_game(struct game_state *game, int idx)
{
    int i;

    for (i = idx; i < game->hand_size - 1; i++) {
        game->us_hand[i] = game->us_hand[i + 1];
    }
    game->us_hand[--game->hand_size] = NULL;
}

static void move_hamets_army(struct game_state *game, enum locations from,
                             enum locations to)
{
    int from_idx = us_infantry_idx(from);
    int to_idx = us_infantry_idx(to);

    game->marine_infantry[to_idx] += game->marine_infantry[from_idx];
    game->marine_infantry[from_idx] = 0;
    game->arab_infantry[to_idx] += game->arab_infantry[from_idx];
    game->arab_infantry[from_idx] = 0;
}

static int card_in_hand(struct game_state *game, struct card *card)
{
    int i;

    for (i = 0; i < game->hand_size; i++) {
        if (game->us_hand[i] == card) {
            return i;
        }
    }

    return -1;
}

bool check_play_battle_card(struct game_state *game, struct card *card)
{
    int idx = card_in_hand(game, card);
    char msg[128];
    bool play;

    if (idx == -1) {
        return false;
    }

    snprintf(msg, sizeof(msg), "Play [ %s ] as a battle card?", card->name);
    play = yn_prompt(msg);
    if (play) {
        remove_card_from_game(game, idx);
    }

    return play;
}

/* Core US cards */
static const char *play_thomas_jefferson(struct game_state *game)
{
    return game_move_ships(game, 8);
}

struct card thomas_jefferson = {
    .name = "Thomas Jefferson",
    .text = "Move up to eight American frigates. "
    "Resolve any battles that result",
    .remove_after_use = true,
    .playable = always_playable,
    .play = play_thomas_jefferson
};

static const char *play_swedish_frigates(struct game_state *game)
{
    game->swedish_frigates_active = true;
    return NULL;
}

struct card swedish_frigates_arrive = {
    .name = "Swedish Frigates Arrive",
    .text = "Place two swedish frigates in the naval patrol zone of Tripoli.",
    .remove_after_use = true,
    .playable = always_playable,
    .play = play_swedish_frigates
};

static bool hamets_army_playable(struct game_state *game)
{
    return game->year >= 1804 && game->us_frigates[ALEXANDRIA] >= 1;
}

static const char *play_hamets_army(struct game_state *game)
{
    int idx = us_infantry_idx(ALEXANDRIA);

    game->marine_infantry[idx] = 1;
    game->arab_infantry[idx] = 5;

    return NULL;
}

struct card hamets_army_created = {
    .name = "Hamet's Army Created",
    .text = "Playable if it is Spring of 1804 or later "
    "and there is at least one American frigate "
    "in the harbor of Alexandria. Place one "
    "Marine and five Arab infantry units in "
    "the city of Alexandria.",
    .remove_after_use = true,
    .playable = hamets_army_playable,
    .play = play_hamets_army
};

/* Battle cards */
/* All removed after use in battle */
struct card lieutenant_in_pursuit = {
    .name = "Lieutenant Sterett In Pursuit",
    .text = "Playable when making an Interception "
    "Roll. Each American frigate may roll "
    "three dice instead of two.",
    .playable = battle_playable,
    .battle_card = true
};

struct card prebles_boys = {
    .name = "Preble's Boys Take Aim",
    .text = "Playable during a naval battle in a "
    "harbor. Each American frigate may "
    "roll three dice instead of two. If played "
    "during the Assault on Tripoli, only roll "
    "the extra dice in the first round of the "
    "naval battle.",
    .playable = battle_playable,
    .battle_card = true
};

struct card daring_decatur = {
    .name = "The Daring Stephen Decatur",
    .text = "Playable if either Burn the Philadelphia "
    "or Launch the Intrepid is the active event "
    "card this turn. Roll two dice instead of "
    "one and choose the preferred result.",
    .playable = battle_playable,
    .battle_card = true
};

struct card send_in_the_marines = {
    .name = "Send in the Marines",
    .text = "Playable if Assault on Tripoli is the active "
    "event card this turn. Place three Marine "
    "infantry units in the city of Tripoli.",
    .playable = battle_playable,
    .battle_card = true
};

struct card lieutenant_leads_the_charge = {
    .name = "Lieutenant O'Bannon Leads the Charge",
    .text = "Playable at the start of a land battle. "
    "Select one Marine infantry unit to roll "
    "three dice instead of one during each "
    "round of combat.",
    .playable = battle_playable,
    .battle_card = true
};

struct card marine_sharpshooters = {
    .name = "Marine Sharpshooters",
    .text = "Playable at the start of a land battle. All "
    "Marine infantry units hit on a roll of 5 "
    "or 6 for each round of combat.",
    .playable = battle_playable,
    .battle_card = true
};

/* US card deck */

static bool peace_treaty_playable(struct game_state *game)
{
    unsigned int trip_derne_idx = trip_infantry_idx(DERNE);

    return is_date_on_or_past(game, 1805, FALL) &&
        game->t_allies[ALGIERS] == 0 && game->t_allies[TANGIER] == 0 &&
        game->t_allies[TUNIS] == 0 &&
        hamets_army_at(game, DERNE) &&
        game->t_infantry[trip_derne_idx] == 0;
}

static const char *play_treaty(struct game_state *game)
{
    cprintf(ITALIC BLUE, "US Victory via Peace Treaty!\n");
    exit(0);

    return NULL;
}

struct card treaty_of_peace_and_amity = {
    .name = "Treaty of Peace and Amity",
    .text = "Playable if: "
    "1. It is the Fall of 1805 or later, "
    "2. The cities of Algiers, Tangier and Tunis "
    "are all at peace, "
    "3. The city of Derne has been captured, "
    "and "
    "4. There are no Tripolitan frigates in the "
    "harbor of Tripoli. "
    "The game ends immediately in an American victory",
    .remove_after_use = true, /* We win */
    .playable = peace_treaty_playable,
    .play = play_treaty
};

static bool assault_on_tripoli_playable(struct game_state *game)
{
    return is_date_on_or_past(game, 1805, FALL);
}

static const char *play_assault_on_tripoli(struct game_state *game)
{
    int frig_count = 0;
    int i;
    int dest;
    bool marines_played;

    game->victory_or_death = true;

    for (i = 0; i < NUM_LOCATIONS; i++) {
        frig_count += game->us_frigates[i];
        game->us_frigates[i] = 0;
        if (has_patrol_zone(i)) {
            frig_count += game->patrol_frigates[i];
            game->patrol_frigates[i] = 0;
        }
    }

    game->us_frigates[TRIPOLI] = frig_count;

    /* Don't bother prompting, we're going in */
    game->assigned_gunboats = game->us_gunboats;

    dest = us_infantry_idx(TRIPOLI);
    /* Check as a courtesy, if the card is in hand you'd probably just play it */
    marines_played = check_play_battle_card(game, &send_in_the_marines);
    if (marines_played) {
        game->marine_infantry[dest] += 3;
    }

    move_hamets_army(game, BENGHAZI, TRIPOLI);

    return NULL;
}

struct card assault_on_tripoli = {
    .name = "Assault on Tripoli",
    .text = "Playable in Fall of 1805 or later. Move "
    "all American frigates and gunboats to "
    "the harbor of Tripoli. Move Hamet’s "
    "Army from Benghazi to Tripoli and/or "
    "play Send in the Marines. Resolve the "
    "Assault on Tripoli! "
    "Victory or Death!",
    .remove_after_use = true,
    .playable = assault_on_tripoli_playable,
    .play = play_assault_on_tripoli
};

static const char *play_naval_movement(struct game_state *game)
{
    return game_move_ships(game, 4);
}

/* 4 copies */
struct card naval_movement = {
    .name = "Naval Movement",
    .text = "Move up to four American frigates. "
    "Resolve any battles that result.",
    .remove_after_use = false,
    .playable = always_playable,
    .play = play_naval_movement
};

static bool early_deployment_playable(struct game_state *game)
{
    if (game->year == END_YEAR) {
        return false;
    }
    return game->turn_track_frigates[year_to_frigate_idx(game->year + 1)] > 0;
}

static const char *play_early_deployment(struct game_state *game)
{
    char *line;
    enum locations location;

    cprintf(BOLD WHITE, "Choose a patrol zone to deploy to\n");
    prompt();

    line = input_getline();
    location = parse_location(line);
    if (location == NUM_LOCATIONS) {
        free(line);
        return "Invalid location";
    }

    if (!has_patrol_zone(location)) {
        free(line);
        return "Chosen location has no patrol zone";
    }

    game->patrol_frigates[location]++;
    game->turn_track_frigates[year_to_frigate_idx(game->year + 1)]--;

    free(line);
    return NULL;
}

struct card early_deployment = {
    .name = "Early Deployment",
    .text = "Take one American frigate from the "
    "following year of the Year Turn Track "
    "and place it in any naval patrol zone.",
    .remove_after_use = false,
    .playable = early_deployment_playable,
    .play = play_early_deployment
};

static bool ally_active(struct game_state *game, enum locations location)
{
    return game->t_allies[location] > 0;
}

static bool trip_allies_active(struct game_state *game)
{
    int i;

    for (i = 0; i < TRIP_ALLIES; i++) {
        if (ally_active(game, i)) {
            return true;
        }
    }

    return false;
}

static const char *play_show_of_force(struct game_state *game)
{
    char *line;
    char *ally_str;
    enum locations ally_loc;
    struct frigate_move moves[3];
    int num_moves = 0;
    char *from_str;
    char *from_type_str;
    char *quantity_str;
    int i;
    int frigates_moved = 0;
    const char *err;

    cprintf(BOLD WHITE, "Choose which Tripoli ally to return to supply and what frigates to move: "
            "[algiers/tangier/tunis] [location] [patrol/harbor] [quantity]...\n");
    prompt();
    line = input_getline();

    ally_str = strtok(line, sep);
    if (ally_str == NULL) {
        free(line);
        return "No Tripolitan ally selected";
    }

    ally_loc = parse_location(ally_str);
    if (!has_trip_allies(ally_loc)) {
        free(line);
        return "Invalid ally location selected";
    }

    if (game->t_allies[ally_loc] == 0) {
        free(line);
        return "Selected ally is not active";
    }

    while (true) {
        from_str = strtok(NULL, sep);
        if (from_str == NULL) {
            break;
        }
        from_type_str = strtok(NULL, sep);
        if (from_type_str == NULL) {
            free(line);
            return "Missing location zone, patrol or harbor";
        }
        quantity_str = strtok(NULL, sep);
        if (quantity_str == NULL) {
            free(line);
            return "No quantity of frigates to move provided";
        }

        moves[num_moves].from = parse_location(from_str);
        moves[num_moves].from_type = parse_move_type(from_type_str);
        moves[num_moves].quantity = strtol(quantity_str, NULL, 10);
        if (moves[num_moves].from == NUM_LOCATIONS) {
            free(line);
            return "Invalid location for move provided";
        }
        if (moves[num_moves].from_type == INVALID_MOVE_TYPE) {
            free(line);
            return "Invalid location zone provided";
        }
        if (errno != 0) {
            free(line);
            return "Invalid move quantity provided";
        }

        moves[num_moves].to = ally_loc;
        moves[num_moves].to_type = HARBOR;
        num_moves++;
    }

    for (i = 0; i < num_moves; i++) {
        frigates_moved += moves[i].quantity;
    }

    if (frigates_moved != 3) {
        free(line);
        return "You must move exactly 3 frigates";
    }

    err = validate_moves(game, moves, num_moves, 3);
    if (err != NULL) {
        free(line);
        return err;
    }

    move_frigates(game, moves, num_moves);

    game->t_allies[ally_loc] = 0;

    free(line);
    return NULL;
}

struct card a_show_of_force = {
    .name = "A Show of Force",
    .text = "Move three American frigates to the "
    "harbor of an active ally of Tripoli "
    "(Algiers, Tangier or Tunis). Return "
    "all of the corsairs from the harbor to "
    "the Supply",
    .remove_after_use = false,
    .playable = trip_allies_active,
    .play = play_show_of_force
};

static const char *play_tribute_paid(struct game_state *game)
{
    char *line;
    char *from_str;
    char *type_str;
    char *to_str;
    enum locations from_loc;
    enum move_type from_type;
    enum locations to_loc;

    cprintf(BOLD, "Choose location to move frigate from and location to move "
            "to. [location] [harbor/patrol] [algiers/tunis/tangier]\n");
    prompt();
    line = input_getline();

    from_str = strtok(line, sep);
    if (from_str == NULL) {
        free(line);
        return "No location to move frigate from provided";
    }

    type_str = strtok(NULL, sep);
    if (type_str == NULL) {
        free(line);
        return "No harbor or patrol zone specified";
    }

    to_str = strtok(NULL, sep);
    if (to_str == NULL) {
        free(line);
        return "Destination harbor not specified";
    }

    from_loc = parse_location(from_str);
    if (from_loc == NUM_LOCATIONS) {
        free(line);
        return "Invalid location to move frigate from";
    }

    from_type = parse_move_type(type_str);
    if (from_type == INVALID_MOVE_TYPE) {
        free(line);
        return "Invalid harbor or patrol zone specified";
    }

    to_loc = parse_location(to_str);
    if (to_loc != ALGIERS && to_loc != TUNIS && to_loc != TANGIER) {
        free(line);
        return "Invalid destination location, must be one of algiers, tunis, "
            "or tangier";
    }

    if (game->t_allies[to_loc] == 0) {
        free(line);
        return "No tripolitan allies at that location";
    }

    if (from_type == PATROL_ZONE) {
        if (!has_patrol_zone(from_loc)) {
            free(line);
            return "Location to move from does not have a patrol zone";
        }
        if (game->patrol_frigates[from_loc] == 0) {
            free(line);
            return "No frigates at location to move";
        }
        game->patrol_frigates[from_loc]--;
    } else {
        if (game->us_frigates[from_loc] == 0) {
            free(line);
            return "no frigates at location to move";
        }
        game->us_frigates[from_loc]--;
    }

    game->us_frigates[to_loc]++;
    game->t_allies[to_loc] = 0;
    game->pirated_gold += 2;

    free(line);
    return NULL;
}

struct card tribute_paid = {
    .name = "Tribute Paid",
    .text = "Move one American frigate to the "
    "harbor of an active ally of Tripoli "
    "(Algiers, Tangier or Tunis). Return all "
    "of the corsairs from the harbor to the "
    "Supply. The Tripolitan player receives "
    "two Gold Coins.",
    .remove_after_use = false,
    .playable = trip_allies_active,
    .play = play_tribute_paid
};

static bool constantinople_tribute_playable(struct game_state *game)
{
    return game->pirated_gold > 0;
}

static const char *play_constantinople_tribute(struct game_state *game)
{
    if (game->pirated_gold < 2) {
        game->pirated_gold = 0;
    } else {
        game->pirated_gold -= 2;
    }

    return NULL;
}

struct card constantinople_demands_tribute = {
    .name = "Constantinople Demands Tribute",
    .text = "The Tripolitan player must return two "
    "Gold Coins to the Supply",
    .remove_after_use = false,
    .playable = constantinople_tribute_playable,
    .play = play_constantinople_tribute
};

static bool check_hamets_army_created(struct game_state *game)
{
    int i;

    for (i = US_INFANTRY_START; i <= US_INFANTRY_END; i++) {
        if (hamets_army_at(game, i)) {
            return true;
        }
    }

    return false;
}

static const char *play_recruit_bedouins(struct game_state *game)
{
    int i;

    for (i = US_INFANTRY_START; i <= US_INFANTRY_END; i++) {
        if (hamets_army_at(game, i)) {
            game->arab_infantry[us_infantry_idx(i)] += 2;
            return NULL;
        }
    }

    assert(false);

    return NULL;
}

struct card hamet_recruits_bedouins = {
    .name = "Hamet Recruits Bedouins",
    .text = "Playable if Hamet’s Army has been "
    "created. Place two additional Arab "
    "infantry with Hamet’s Army",
    .remove_after_use = false,
    .playable = check_hamets_army_created,
    .play = play_recruit_bedouins
};

static bool brainbridge_playable(struct game_state *game)
{
    return game->discard_size > 0;
}

static const char *play_brainbridge_supplies_intel(struct game_state *game)
{
    char *line;
    char *action;
    char *idx_str;
    int idx;
    const char *err;
    struct card *card;

    print_discard_pile(game);
    cprintf(BOLD WHITE, "Choose which card to take or play "
            "(ex : \"take 3\", \"play 2\", \"t 0\", \"p 3\"):\n");
    prompt();

    line = input_getline();

    action = strtok(line, sep);
    if (action == NULL) {
        free(line);
        return "Invalid action for [ Brainbridge Supplies Intel ]";
    }

    idx_str = strtok(NULL, sep);
    if (idx_str == NULL) {
        free(line);
        return "No card selected";
    }

    idx = strtol(idx_str, NULL, 10);
    if (errno != 0 || idx < 0 || idx >= game->discard_size) {
        free(line);
        return "Invalid card number";
    }

    card = game->us_discard[idx];
    assert(card);
    assert(card->play);

    if (strcmp(action, "play") == 0 || strcmp(action, "p") == 0) {
        if (!card->playable(game)) {
            free(line);
            return "Chosen card not playable";
        }
        err = card->play(game);
        if (err != NULL) {
            free(line);
            return err;
        }
        if (card->remove_after_use) {
            game->us_discard[idx] = game->us_discard[game->discard_size - 1];
            game->us_discard[--game->discard_size] = NULL;
        }
        /* If the card isn't removed after use we just leave it in the discard
         * pile */
    } else if (strcmp(action, "take") == 0 || strcmp(action, "t") == 0) {
        game->us_hand[game->hand_size++] = card;
    }

    free(line);
    return NULL;
}

struct card brainbridge_supplies_intel = {
    .name = "Brainbridge Supplies Intel",
    .text = "Take any card from the American "
    "discard pile and either place it in hand or "
    "play it immediately",
    .remove_after_use = false,
    .playable = brainbridge_playable,
    .play = play_brainbridge_supplies_intel
};

static const char *play_congress_authorizes_action(struct game_state *game)
{
    if (game->year == END_YEAR) {
        return NULL;
    }

    game->turn_track_frigates[year_to_frigate_idx(game->year + 1)] += 2;
    return NULL;
}

struct card congress_authorizes_action = {
    .name = "Congress Authorizes Action",
    .text = "Place two American frigates on the "
    "following year of the Year Turn Track.",
    .remove_after_use = true,
    .playable = always_playable, /* Even on last year as a do nothing */
    .play = play_congress_authorizes_action
};

static bool corsairs_confiscated_playable(struct game_state *game)
{
    return game->t_corsairs_gibraltar > 0;
}

static const char *play_corsairs_confiscated(struct game_state *game)
{
    /* We don't need to remove Murad Reis Breaks out as setting the corsairs to
     * zero will cause the playable() check to fail
    */
    game->t_corsairs_gibraltar = 0;

    return NULL;
}

struct card corsairs_confiscated = {
    .name = "Corsairs Confiscated",
    .text = "Playable if there are Tripolitan corsairs in "
    "the harbor of Gibraltar. Return all of the "
    "corsairs from the harbor to the Supply "
    "and remove the Murad Reis Breaks Out "
    "card from the game.",
    .remove_after_use = true,
    .playable = corsairs_confiscated_playable,
    .play = play_corsairs_confiscated
};

static bool burn_the_philly_playable(struct game_state *game)
{
    return game->t_frigates > 0;
}

static const char *play_burn_the_philly(struct game_state *game)
{
    bool roll_again = check_play_battle_card(game, &daring_decatur);
    unsigned int roll = rolld6();

    if (roll_again) {
        roll = max(roll, rolld6());
    }

    if (roll == 3 || roll == 4) {
        game->t_frigates--;
        if (game->year < 1806) {
            game->t_turn_frigates[year_to_frigate_idx(game->year + 1)]++;
        }
    } else if (roll == 5 || roll == 6) {
        game->t_frigates--;
    }

    return NULL;
}

struct card burn_the_philadelphia = {
    .name = "Burn The Philadelphia",
    .text = "Playable if there is at least one Tripolitan "
    "frigate in the harbor of Tripoli. Roll one "
    "die and apply the result: : "
    "1-2: The raid is a failure. No effect. "
    "3-4: A Tripolitan frigate is damaged. "
    "Place it on the following year of the Year "
    "Turn Track. "
    "5-6: A Tripolitan frigate is sunk.",
    .remove_after_use = true,
    .playable = burn_the_philly_playable,
    .play = play_burn_the_philly
};

static void sink_corsairs_at(struct game_state *game, enum locations location,
                             int count)
{
    unsigned int *corsairs = tripoli_corsair_ptr(game, location);

    if (*corsairs < count) {
        *corsairs = 0;
    } else {
        *corsairs -= count;
    }
}

static const char *sink_corsairs(struct game_state *game, int count)
{
    char *line;
    char *loc_str;
    int loc_count = 1;
    enum locations loc[2];

    /* If there's only one choice don't bother prompting */
    if (game->t_corsairs_gibraltar == 0) {
        sink_corsairs_at(game, TRIPOLI, 2);
        return NULL;
    }

    /* There's probably a simpler way to handle this since there's only 2
     * locations where Tripolitan corsairs can be */
    cprintf(BOLD WHITE, "Choose locations to sink up to 2 corsairs from\n");
    prompt();
    line = input_getline();

    loc_str = strtok(line, sep);
    if (loc_str == NULL) {
        free(line);
        return "No locations provided";
    }

    loc[0] = parse_location(loc_str);
    if (!tripoli_corsair_location(loc[0])) {
        free(line);
        return "Invalid location";
    }

    if (count == 2) {
        loc_str = strtok(NULL, sep);
        if (loc_str != NULL) {
            loc[1] = parse_location(loc_str);
            if (!tripoli_corsair_location(loc[1])) {
                free(line);
                return "Invalid location";
            }
            loc_count++;
        }
    }

    loc_str = strtok(NULL, sep);
    if (loc_str != NULL) {
        free(line);
        return "Too many locations provided";
    }

    if (loc_count == 1) {
        sink_corsairs_at(game, loc[0], count);
    } else {
        sink_corsairs_at(game, loc[0], 1);
        sink_corsairs_at(game, loc[1], 1);
    }

    free(line);
    return NULL;
}

static const char *play_launch_the_intrepid(struct game_state *game)
{
    bool roll_again = check_play_battle_card(game, &daring_decatur);
    unsigned int roll = rolld6();

    if (roll_again) {
        roll = max(roll, rolld6());
    }

    if (roll == 3 || roll == 4) {
        return sink_corsairs(game, 1);
    } else if (roll == 5 || roll == 6) {
        if (game->t_frigates > 0) {
            game->t_frigates--;
        } else {
            return sink_corsairs(game, 2);
        }
    }

    return NULL;
}

struct card launch_the_intrepid = {
    .name = "Launch The Intrepid",
    .text = "Roll one die and apply the result: "
    "1-2: The raid is a failure. No effect. "
    "3-4: One Tripolitan corsair is sunk. "
    "5-6: One Tripolitan frigate is sunk. If no "
    "frigate available, two Tripolitan corsairs "
    "are sunk.",
    .remove_after_use = true,
    .playable = always_playable,
    .play = play_launch_the_intrepid
};

static bool hamets_army_in_alexandria(struct game_state *game)
{
    return hamets_army_at(game, ALEXANDRIA);
}

static const char *hamet_move_frigates(struct game_state *game,
                                       enum locations dest)
{
    char *line;
    struct frigate_move moves[3];
    int move_count = 0;
    bool first = true;
    char *loc_str;
    char *zone_str;
    char *quantity_str;
    enum locations loc;
    enum move_type type;
    int quantity;
    const char *err;

    cprintf(BOLD WHITE, "Move up to three frigates to the %s harbor: "
            "[location] [patrol/harbor] [quantity]...\n", location_str(dest));
    prompt();

    line = input_getline();

    while (true) {
        if (move_count > 3) {
            free(line);
            return "Too many moves specified";
        }
        if (first) {
            loc_str = strtok(line, sep);
            first = false;
        } else {
            loc_str = strtok(NULL, sep);
        }

        if (loc_str == NULL) {
            break;
        }

        zone_str = strtok(NULL, sep);
        if (zone_str == NULL) {
            free(line);
            return "Patrol Zone or Harbor not specified for move";
        }

        quantity_str = strtok(NULL, sep);
        if (quantity_str == NULL) {
            free(line);
            return "No move quantity specified";
        }

        loc = parse_location(loc_str);
        if (loc == NUM_LOCATIONS) {
            free(line);
            return "Invalid location specified";
        }

        type = parse_move_type(zone_str);
        if (type == INVALID_MOVE_TYPE) {
            free(line);
            return "Invalid zone specified. Must be harbor or patrol";
        }

        quantity = strtol(quantity_str, NULL, 10);
        if (errno != 0 || quantity == 0) {
            free(line);
            return "Invalid move quantity specified";
        }

        moves[move_count].from = loc;
        moves[move_count].from_type = type;
        moves[move_count].quantity = quantity;

        moves[move_count].to = dest;
        moves[move_count].to_type = HARBOR;
        move_count++;
    }

    err = validate_moves(game, moves, move_count, 3);
    if (err != NULL) {
        free(line);
        return err;
    }

    move_frigates(game, moves, move_count);

    free(line);
    return NULL;
}

static const char *play_eaton_attacks_derne(struct game_state *game)
{
    const char *err;

    err = hamet_move_frigates(game, DERNE);
    if (err != NULL) {
        return err;
    }

    move_hamets_army(game, ALEXANDRIA, DERNE);

    return NULL;
}

struct card eaton_attacks_derne = {
    .name = "General Eaton Attacks Derne",
    .text = "Move Hamet’s Army from Alexandria "
    "to Derne. Move up to three American "
    "frigates to the harbor of Derne. Resolve "
    "the Battle for Derne!",
    .remove_after_use = true,
    .playable = hamets_army_in_alexandria,
    .play = play_eaton_attacks_derne
};

static bool hamets_army_in_derne(struct game_state *game)
{
    return hamets_army_at(game, DERNE);
}

static const char *play_eaton_attacks_benghazi(struct game_state *game)
{
    const char *err;

    err = hamet_move_frigates(game, BENGHAZI);
    if (err != NULL) {
        return err;
    }

    move_hamets_army(game, DERNE, BENGHAZI);

    return NULL;
}

struct card eaton_attacks_benghazi = {
    .name = "General Eaton Attacks Benghazi",
    .text = "Move Hamet’s Army from Derne to "
    "Benghazi. Move up to three American "
    "frigates to the harbor of Benghazi. "
    "Resolve the Battle for Benghazi!",
    .remove_after_use = true,
    .playable = hamets_army_in_derne,
    .play = play_eaton_attacks_benghazi
};

static struct card *us_core[US_CORE_CARD_COUNT] = {
    &thomas_jefferson,
    &swedish_frigates_arrive,
    &hamets_army_created
};

static struct card *us_deck[US_DECK_SIZE] = {
    &treaty_of_peace_and_amity,
    &assault_on_tripoli,
    /* 4 of these */
    &naval_movement, &naval_movement, &naval_movement, &naval_movement,
    &early_deployment,
    &a_show_of_force,
    &tribute_paid,
    &constantinople_demands_tribute,
    &hamet_recruits_bedouins,
    &brainbridge_supplies_intel,
    &congress_authorizes_action,
    &corsairs_confiscated,
    &burn_the_philadelphia,
    &launch_the_intrepid,
    &eaton_attacks_derne,
    &eaton_attacks_benghazi,
    &lieutenant_in_pursuit,
    &prebles_boys,
    &daring_decatur,
    &send_in_the_marines,
    &lieutenant_leads_the_charge,
    &marine_sharpshooters
};

void init_game_cards(struct game_state *game)
{
    game->us_core_cards = us_core;
    game->us_deck = us_deck;
    game->us_deck_size = US_DECK_SIZE;
}

void draw_from_deck(struct game_state *game, int draw_count)
{
    int i;
    int rand_card;

    assert(game->us_deck_size >= draw_count);

    for (i = 0; i < draw_count; i++) {
        rand_card = rand() % game->us_deck_size;
        game->us_hand[game->hand_size++] = game->us_deck[rand_card];
        game->us_deck[rand_card] = game->us_deck[game->us_deck_size - 1];
        game->us_deck[game->us_deck_size - 1] = NULL;
        game->us_deck_size--;
    }
}

void discard_from_hand(struct game_state *game, int idx)
{
    int i;

    game->us_discard[game->discard_size++] = game->us_hand[idx];
    for (i = idx; i < game->hand_size - 1; i++) {
        game->us_hand[i] = game->us_hand[i + 1];
    }
    game->us_hand[--game->hand_size] = NULL;
}

void shuffle_discard_into_deck(struct game_state *game)
{
    memcpy(game->us_deck, game->us_discard,
           sizeof(struct card *) * game->discard_size);
    memset(game->us_discard, 0, sizeof(struct card *) * US_DECK_SIZE);
    game->us_deck_size = game->discard_size;
    game->discard_size = 0;
}

const char *play_card_from_hand(struct game_state *game, int idx)
{
    struct card *card;
    const char *err;

    if (idx >= game->hand_size || idx < 0) {
        return "Invalid card number";
    }

    card = game->us_hand[idx];

    assert(card != NULL);

    if (!card->playable(game)) {
        return "Card not playable";
    }

    assert(card->play);

    err = card->play(game);
    if (err != NULL) {
        return err;
    }

    if (card->remove_after_use) {
        remove_card_from_game(game, idx);
    } else {
        discard_from_hand(game, idx);
    }

    return NULL;
}

const char *play_core_card(struct game_state *game, int idx)
{
    int i;
    struct card *card;
    const char *err;

    if (idx >= US_CORE_CARD_COUNT || idx < 0) {
        return "Invalid card number";
    }

    card = game->us_core_cards[idx];

    if (card == NULL || !card->playable(game)) {
        return "Card not playable";
    }

    assert(card->play);

    err = card->play(game);
    if (err != NULL) {
        return err;
    }

    for (i = idx; i < US_CORE_CARD_COUNT - 1; i++) {
        game->us_core_cards[i] = game->us_core_cards[i + 1];
    }
    game->us_core_cards[US_CORE_CARD_COUNT - 1] = NULL;

    return NULL;
}
