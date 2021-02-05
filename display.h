#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>

#include "game.h"

/* ANSI Color codes */
#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE    "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN    "\x1B[36m"
#define WHITE   "\x1B[37m"
#define RESET   "\x1B[0m"
/* Extended color code */
#define ORANGE "\x1B[38;5;208m"

#define UNDERLINE "\e[4m"
#define BOLD      "\e[1m"
#define ITALIC    "\e[3m"

#define cprintf(color, ...)                     \
    printf(color);                              \
    printf(__VA_ARGS__);                        \
    printf(RESET);

#define print_centered(color, text, width)                      \
    {                                                           \
        int pad = (width - strlen(text)) / 2;                   \
        cprintf(color, "%*s%s%*s\n", pad, "", text, pad, "");   \
    }

static inline void clear_screen()
{
    printf("\e[1;1H\e[2J");
}

void display_game(struct game_state *game);
void print_discard_pile(struct game_state *game);

#endif /* DISPLAY_H */
