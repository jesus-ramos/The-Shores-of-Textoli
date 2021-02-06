#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "cards.h"
#include "display.h"

/* Might need adjusting */
#define HARBOR_PAD_SIZE (16)
static void print_harbor(struct game_state *game, enum locations location)
{
    int i;
    int trip_corsairs = 0;
    int count = 0;
    int idx;
    int padlen;

    cprintf(BOLD WHITE, "Harbor: ");
    for (i = 0; i < game->us_frigates[location]; i++) {
        cprintf(BOLD BLUE, "F");
        count++;
    }
    /* TODO handle displaying gunboats at battle location for assaul on tripoli */
    if (location == MALTA) {
        for (i = 0; i < game->us_gunboats - game->used_gunboats; i++) {
            cprintf(BOLD BLUE, "G");
            count++;
        }
        for (i = 0; i < game->used_gunboats; i++) {
            cprintf(ITALIC BLUE, "G");
            count++;
        }
    }

    if (has_us_infantry(location)) {
        idx = us_infantry_idx(location);

        for (i = 0; i < game->marine_infantry[idx]; i++) {
            cprintf(BOLD BLUE, "M");
            count++;
        }
        for (i = 0; i < game->arab_infantry[idx]; i++) {
            cprintf(BOLD GREEN, "A");
            count++;
        }
    }

    if (count > 0) {
        printf(" ");
        count++;
    }

    if (location == TRIPOLI) {
        trip_corsairs = game->t_corsairs_tripoli;
    } else if (location == GIBRALTAR) {
        trip_corsairs = game->t_corsairs_gibraltar;
    }
    for (i = 0; i < trip_corsairs; i++) {
        cprintf(BOLD RED, "C");
        count++;
    }

    if (location == TRIPOLI) {
        for (i = 0; i < game->t_frigates; i++) {
            cprintf(BOLD RED, "F");
            count++;
        }
    }

    if (has_trip_allies(location)) {
        for (i = 0; i < game->t_allies[location]; i++) {
            cprintf(BOLD ORANGE, "C");
            count++;
        }
    }

    if (has_trip_infantry(location)) {
        idx = trip_infantry_idx(location);
        for (i = 0; i < game->t_infantry[idx]; i++) {
            cprintf(BOLD RED, "P");
            count++;
        }
    }

    if (count <= HARBOR_PAD_SIZE) {
        padlen = HARBOR_PAD_SIZE - count;
        printf("%*s", padlen, "");
    }
}

static void print_patrol_zone(struct game_state *game, enum locations location)
{
    int i;

    if (!has_patrol_zone(location)) {
        return;
    }
    cprintf(BOLD WHITE, " Patrol Zone: ");

    for (i = 0; i < game->patrol_frigates[location]; i++) {
        cprintf(BOLD BLUE, "F");
    }

    if (location == TRIPOLI && game->swedish_frigates_active) {
        cprintf(BOLD YELLOW, "FF");
    }
}

#define LOC_PAD_SIZE (10)
static void print_location(struct game_state *game, enum locations location)
{
    const char *loc_str = location_str(location);
    int len = strlen(loc_str);
    int pad = LOC_PAD_SIZE - len;

    assert(pad >= 0);

    cprintf(BOLD CYAN, "[%s] %*s", loc_str, pad, "");
    print_harbor(game, location);
    print_patrol_zone(game, location);
    printf("\n");
}

static void print_locations(struct game_state *game)
{
    int i;

    cprintf(BOLD BLUE, "[ Map ]\n");
    for (i = 0; i < NUM_LOCATIONS; i++) {
        print_location(game, i);
    }
}

static void print_card(struct game_state *game, struct card *card, int idx)
{
    const char *remove = "";

    if (card->battle_card) {
            remove = " If played as a battle card, this card is removed from "
                "the game.";
    } else if (card->remove_after_use) {
        remove = " After playing as an event, this card is removed from "
                "the game.";
    }

    if (card->playable(game)) {
        cprintf(BOLD, "%d) [%s]", idx, card->name);
    } else {
        if (card->battle_card) {
            cprintf(ITALIC RED, "%d) [%s]", idx, card->name);
        } else {
            cprintf(BOLD STRIKETHROUGH RED, "%d) [%s]", idx, card->name);
        }
    }
    cprintf(ITALIC WHITE, " - %s%s\n", card->text, remove);
}

static void print_hand(struct game_state *game)
{
    int i;

    cprintf(BOLD BLUE, "[ Hand ]\n");

    for (i = 0; i < game->hand_size; i++) {
        print_card(game, game->us_hand[i], i);
    }
}

static void print_core_cards(struct game_state *game)
{
    int i;

    if (game->us_core_cards[0] == NULL) {
        return;
    }

    cprintf(BOLD BLUE, "[ Core Cards ]\n");

    for (i = 0; i < US_CORE_CARD_COUNT; i++) {
        if (game->us_core_cards[i]) {
            print_card(game, game->us_core_cards[i], i);
        } else {
            break;
        }
    }
}

void print_discard_pile(struct game_state *game)
{
    int i;

    cprintf(BOLD BLUE, "[ Discard Pile ]\n");

    for (i = 0; i < game->discard_size; i++) {
        print_card(game, game->us_discard[i], i);
    }
}

static void print_separator(int count)
{
    int i;

    for (i = 0; i < count; i++) {
        cprintf(BOLD CYAN, "-");
    }
}

static void print_season(struct game_state *game)
{
    const char *fmtstring = "[ Season : %s ] ";
    const char *str = season_str(game->season);

    if (game->season == SPRING) {
        cprintf(BOLD GREEN, fmtstring, str);
    } else if (game->season == SUMMER) {
        cprintf(BOLD YELLOW, fmtstring, str);
    } else if (game->season == FALL) {
        cprintf(BOLD ORANGE, fmtstring, str);
    } else {
        cprintf(BOLD CYAN, fmtstring, str);
    }
}

static void print_year_track(struct game_state *game)
{
    int year;
    int i;
    int frigates;

    cprintf(BOLD WHITE, "[ Year:");
    for (year = START_YEAR; year <= END_YEAR; year++) {
        if (year == game->year) {
            cprintf(BOLD WHITE, " (%u)", year);
        } else {
            cprintf(WHITE, " (%u", year);
            if (year >= 1802) {
                frigates = game->turn_track_frigates[year_to_frigate_idx(year)];
                if (frigates > 0) {
                    printf(" ");
                    for (i = 0; i < frigates; i++) {
                        cprintf(BLUE, "F");
                    }
                }

                frigates = game->t_turn_frigates[year_to_frigate_idx(year)];
                if (frigates > 0) {
                    printf(" ");
                    for (i = 0; i < frigates; i++) {
                        cprintf(RED, "F");
                    }
                }
            }
            cprintf(WHITE, ")");
        }
    }
    cprintf(BOLD WHITE, "] ");
}

void display_game(struct game_state *game)
{
    struct winsize size;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

    clear_screen();

    print_centered(BOLD UNDERLINE WHITE, "THE SHORES OF TRIPOLI", size.ws_col);
    print_year_track(game);
    print_season(game);
    printf("\n");
    cprintf(BOLD RED, "[ Destroyed US Frigates : %u ] ",
            game->destroyed_us_frigates);
    cprintf(BOLD YELLOW, "[ Tripolitan Gold : %u ] ", game->pirated_gold);
    cprintf(BOLD MAGENTA, "[ Game Seed : %u ]\n", game->seed);

    print_separator(size.ws_col);
    print_locations(game);
    print_separator(size.ws_col);
    print_core_cards(game);
    print_separator(size.ws_col);
    print_hand(game);
    print_separator(size.ws_col);
}
