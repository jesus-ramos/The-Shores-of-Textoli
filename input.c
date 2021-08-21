#include <errno.h>
#include <string.h>

#include "cards.h"
#include "input.h"

static const char *play_command(struct game_state *game, bool core)
{
    char *idx_str = strtok(NULL, sep);
    int card_idx;

    if (idx_str == NULL) {
        return "Missing card number";
    }

    if (!game_strtol(idx_str, &card_idx)) {
        return "Invalid card index";
    }

    if (core) {
        return play_core_card(game, card_idx);
    }

    return play_card_from_hand(game, card_idx);
}

static const char *discard_command(struct game_state *game)
{
    char *idx_str = strtok(NULL, sep);
    char *action;
    int card_idx;
    const char *ret;

    if (idx_str == NULL) {
        return "Missing card number";
    }

    if (!game_strtol(idx_str, &card_idx) || card_idx < 0 ||
        card_idx >= game->hand_size) {
        return "Invalid card index";
    }

    action = strtok(NULL, sep);
    if (action == NULL) {
        return "Missing discard action";
    }

    if (strcmp(action, "build") == 0 || strcmp(action, "b") == 0 ||
        strcmp(action, "gunboat") == 0 || strcmp(action, "g") == 0) {
        if (!build_gunboat(game)) {
            return "Cannot build any more gunboats";
        }
    } else if (strcmp(action, "move") == 0 || strcmp(action, "m") == 0) {
        ret = game_move_ships(game, 2);
        if (ret) {
            return ret;
        }
    } else {
        return "Invalid discard command";
    }

    discard_from_hand(game, card_idx);

    return NULL;
}

static const char *help_command()
{
    return "Commands:\n"
        "[play/p] [card number] : play a card from hand. ex: p 0, play 2\n"
        "[core/c] [card number] : play a core card. ex: c 1, core 0\n"
        "[discard/d] [card number] [move/m,gunboat/g] : "
        "discard a card to move 2 frigates or build a gunboat. ex: d 2 g, "
        "discard 3 move\n"
        "[help/h/?] : print this useful message\n"
        "[quit/q] : quit the game";
}

static const char *parse_command(struct game_state *game, char *line)
{
    char *command = strtok(line, sep);

    if (command == NULL) {
        return "No command specified";
    }

    if (strcmp(command, "play") == 0 || strcmp(command, "p") == 0) {
        return play_command(game, false);
    } else if (strcmp(command, "core") == 0 || strcmp(command, "c") == 0) {
        return play_command(game, true);
    } else if (strcmp(command, "discard") == 0 || strcmp(command, "d") == 0) {
        return discard_command(game);
    } else if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0 ||
               strcmp(command, "?") == 0) {
        return help_command();
    } else if (strcmp(command, "quit") == 0 || strcmp(command, "q") == 0) {
        if (yn_prompt("Quit?")) {
            exit(EXIT_SUCCESS);
        }
        return NULL;
    } else {
        return "Invalid Command";
    }
}

const char *handle_input(struct game_state *game)
{
    char *line = NULL;
    const char *ret;

    prompt();

    line = input_getline();
    ret = parse_command(game, line);

    free(line);
    return ret;
}

const char *parse_moves(struct frigate_move *moves, int *num_moves,
                        int allowed_moves)
{
    int move_idx = 0;
    bool first = true;
    char *line;
    char *from_str, *from_zone;
    char *to_str, *to_zone;
    char *count_str;
    int count;
    struct frigate_move *move;

    cprintf(BOLD WHITE, "You can move up to %d frigates\n", allowed_moves);
    cprintf(BOLD WHITE, "Specify moves in the form [from location] "
            "[harbor/patrol] [destination location] [harbor/patrol] "
            "[quantity]\n");
    prompt();

    line = input_getline();

    while (true) {
        if (first) {
            from_str = strtok(line, sep);
            first = false;
        } else {
            from_str = strtok(NULL, sep);
        }
        if (from_str == NULL) {
            break;
        }

        if (move_idx == MAX_FRIGATE_MOVES) {
            free(line);
            return "Too many moves";
        }

        from_zone = strtok(NULL, sep);
        if (from_zone == NULL) {
            free(line);
            return "Missing from zone (location or harbor)";
        }

        to_str = strtok(NULL, sep);
        if (to_str == NULL) {
            free(line);
            return "Missing destination location name";
        }

        to_zone = strtok(NULL, sep);
        if (to_zone == NULL) {
            free(line);
            return "Missing destination zone (location or harbor)";
        }

        count_str = strtok(NULL, sep);
        if (count_str == NULL) {
            free(line);
            return "Missing move quantity";
        }

        if (!game_strtol(count_str, &count) || count == 0) {
            free(line);
            return "Invalid move value or count";
        }

        move = &moves[move_idx++];
        move->from = parse_location(from_str);
        move->from_zone = parse_zone(from_zone);
        move->to = parse_location(to_str);
        move->to_zone = parse_zone(to_zone);
        move->quantity = count;
    }

    *num_moves = move_idx;
    free(line);
    return NULL;
}

const char *parse_damage_assignment(struct game_state *game,
                                    enum locations location,
                                    enum zone zone, int num_hits,
                                    enum battle_type btype)
{
    char *line;
    int destroy_frigates = 0;
    int damage_frigates = 0;
    int destroy_gunboats = 0;
    int destroy_marines = 0;
    int destroy_arabs = 0;
    int len;
    int i;
    const char *err;
    const char *btype_str = (btype == NAVAL_BATTLE) ? "naval" : "ground";

    assert(btype == NAVAL_BATTLE || btype == GROUND_BATTLE);

    if (num_hits == 0) {
        return NULL;
    }

    if (try_auto_assign_damage(game, location, zone, num_hits, btype)) {
        return NULL;
    }

    cprintf(BOLD WHITE, "Assign %d hits for %s battle at %s %s:"
            "(F to destroy a frigate, f to damage a frigate or destroy a damaged"
            "frigate, G/g to destroy a gunboat, A/a for arab infantry, "
            "M/m for US marines)\n", num_hits, btype_str,
            location_str(location), zone_str(zone));
    prompt();
    line = input_getline();

    len = strlen(line);
    for (i = 0; i < len; i++) {
        if (line[i] == 'g' || line[i] == 'G') {
            destroy_gunboats++;
        } else if (line[i] == 'f') {
            damage_frigates++;
        } else if (line[i] == 'F') {
            destroy_frigates++;
        } else if (line[i] == 'M' || line[i] == 'm') {
            destroy_marines++;
        } else if (line[i] == 'A' || line[i] == 'a') {
            destroy_arabs++;
        } else if (line[i] != ' ') {
            free(line);
            return "Invalid character found for assigning damage";
        }
    }

    if (btype == NAVAL_BATTLE) {
        err = assign_naval_damage(game, location, zone, num_hits,
                                  destroy_frigates, damage_frigates,
                                  destroy_gunboats);
    } else {
        err = assign_ground_damage(game, location, num_hits, destroy_marines,
                                   destroy_arabs);
    }

    free(line);
    return err;
}

const char *parse_assign_gunboats(struct game_state *game,
                                  enum locations location,
                                  enum zone zone)
{
    char *line;
    int gunboats;

    /* No boats to assign */
    if (game->used_gunboats == game->us_gunboats) {
        return NULL;
    }

    cprintf(BOLD WHITE, "Choose how many gunboats to bring to the battle at "
            "%s %s\n", location_str(location), zone_str(zone));
    prompt();
    line = input_getline();

    if (!game_strtol(line, &gunboats)) {
        free(line);
        return "Invalid number provided";
    }

    if (gunboats > (game->us_gunboats - game->used_gunboats)) {
        free(line);
        return "Too many gunboats chosen";
    }

    game->assigned_gunboats = gunboats;
    game->used_gunboats += gunboats;

    free(line);
    return NULL;
}
