#ifndef INPUT_H
#define INPUT_H

#include "display.h"
#include "game.h"

#define sep " "

static inline char *input_getline()
{
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen = 0;

    linelen = getline(&line, &linecap, stdin);
    line[linelen - 1] = 0;

    return line;
}

static inline void prompt()
{
    cprintf(BOLD WHITE, "> ");
}

static inline bool yn_prompt(const char *msg)
{
    char *line;
    bool ret = false;

    cprintf(BOLD WHITE, "%s [y/n] ", msg);
    line = input_getline();
    if (strcmp(line, "y") == 0 || strcmp(line, "yes") == 0) {
        ret = true;
    }

    free(line);
    return ret;
}

const char *handle_input(struct game_state *game);
const char *parse_moves(struct frigate_move *moves, int *num_moves);
const char *parse_damage_assignment(struct game_state *game,
                                    enum locations location,
                                    enum zone zone, int num_hits,
                                    enum battle_type btype);
const char *parse_assign_gunboats(struct game_state *game,
                                  enum locations location,
                                  enum zone zone);

#endif /* INPUT_H */
