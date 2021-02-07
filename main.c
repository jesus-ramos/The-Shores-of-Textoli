#include <errno.h>
#include <execinfo.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

void crash_handler(int signal)
{
    void *callstack[16];
    int frames = 0;

    frames = backtrace(callstack, 16);
    backtrace_symbols_fd(callstack, frames, STDERR_FILENO);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    struct game_state game;
    unsigned int seed = 0;
    int ch;

    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);

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
