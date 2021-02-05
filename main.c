#include <errno.h>
#include <getopt.h>
#include <stdio.h>

#include "game.h"

static struct option longopts[] =
{
    {"seed", required_argument, NULL, 's'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

static void usage()
{
    printf("Something\n");
}

int main(int argc, char **argv)
{
    struct game_state game;
    unsigned int seed = 0;
    int ch;

    while ((ch = getopt_long(argc, argv, "s:", longopts, NULL)) != -1) {
        switch (ch) {
            case 's':
                seed = strtol(optarg, NULL, 10);
                if (errno != 0) {
                    fprintf(stderr, "Invalid seed value\n");
                    exit(1);
                }
                break;
            case 'h':
                usage();
                exit(0);
            default:
                usage();
                exit(1);
        }
    }

    init_game_state(&game, seed);

    game_loop(&game);

    return 0;
}
