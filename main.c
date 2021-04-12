#include <errno.h>
#if !defined(__CYGWIN__) && !defined(__MINGW32__)
#include <execinfo.h>
#endif /* !defined(__CYGWIN__) && !defined(__MINGW32__) */
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
    const char *usage_str =
        "-s --seed : Set the game seed. Default: time based seed\n"
        "-h --help : Print this usage text\n";

    printf("%s", usage_str);
}

#if !defined(__CYGWIN__) && !defined(__MINGW32__)
static void crash_handler(int signal)
{
    void *callstack[16];
    int frames = 0;

    frames = backtrace(callstack, 16);
    backtrace_symbols_fd(callstack, frames, STDERR_FILENO);
    exit(EXIT_FAILURE);
}
#endif /* !defined(__CYGWIN__) && !defined(__MINGW32__) */

int main(int argc, char **argv)
{
    struct game_state game;
    int seed = 0;
    int ch;

#if !defined(__CYGWIN__) && !defined(__MINGW32__)
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
#endif /* !defined(__CYGWIN__) && !defined(__MINGW32__) */

    while ((ch = getopt_long(argc, argv, "hs:", longopts, NULL)) != -1) {
        switch (ch) {
            case 's':
                if (!game_strtol(optarg, &seed)) {
                    fprintf(stderr, "Invalid seed value\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }

    init_game_state(&game, seed);

    game_loop(&game);

    return 0;
}
