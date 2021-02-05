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

    card_idx = strtol(idx_str, NULL, 10);
    if (errno != 0) {
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

    card_idx = strtol(idx_str, NULL, 10);
    if (errno != 0 || card_idx < 0 || card_idx >= game->hand_size) {
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
    return "Help text TBD";
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
    } else if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0) {
        return help_command();
    } else if (strcmp(command, "quit") == 0 || strcmp(command, "q") == 0) {
        if (yn_prompt("Quit?")) {
            exit(0);
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

const char *parse_moves(struct frigate_move *moves, int *num_moves)
{
    int move_idx = 0;
    bool first = true;
    char *line;
    char *from_str, *from_type;
    char *to_str, *to_type;
    char *count_str;
    unsigned int count;
    struct frigate_move *move;

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

        from_type = strtok(NULL, sep);
        if (from_type == NULL) {
            free(line);
            return "Missing from type (location or harbor)";
        }

        to_str = strtok(NULL, sep);
        if (to_str == NULL) {
            free(line);
            return "Missing destination location name";
        }

        to_type = strtok(NULL, sep);
        if (to_type == NULL) {
            free(line);
            return "Missing destination type (location or harbor)";
        }

        count_str = strtok(NULL, sep);
        if (count_str == NULL) {
            free(line);
            return "Missing move quantity";
        }

        count = strtol(count_str, NULL, 10);
        if (errno != 0 || count == 0) {
            free(line);
            return "Invalid move value or count";
        }

        move = &moves[move_idx++];
        move->from = parse_location(from_str);
        move->from_type = parse_move_type(from_type);
        move->to = parse_location(to_str);
        move->to_type = parse_move_type(to_type);
        move->quantity = count;
    }

    *num_moves = move_idx;
    free(line);
    return NULL;
}
