/**
 * argparse.c - helper functions for parse arguments
 * refactored from http://www.lemoda.net/c/split-on-whitespace/index.html
 */

#include "./argparse.h"

#define SKIP(p) while (*p && isspace (*p)) p++
#define WANT(p) *p && !isspace (*p)

/**
 *
 * @param input const char* - the full input string
 * @param argc int - the number of arguments
 * @param argv char** - the output buffer for arg array
 * @return int - return -1 on error
 */
static int argCopy(const char* input, int argc, char** argv) {
    int i = 0;
    const char *p;

    p = input;
    while (*p) {
        SKIP(p);
        if (WANT(p)) {
            const char * end = p;
            char * copy;
            while (WANT(end))
                end++;
            copy = argv[i] = malloc(end - p + 1);
            if (!argv[i])
                return -1;
            while (WANT(p))
                *copy++ = *p++;
            *copy = 0;
            i++;
        }
    }

    if (i != argc)
        return -1;

    return 0;
}

/**
 * counts the number of space separated arguments
 * @param input const char* - the full input string
 * @return int - number of args
 */
static int argCount(const char* input) {
    const char * p;
    int argc = 0;
    p = input;
    while (*p) {
        SKIP(p);
        if (WANT(p)) {
            argc++;
            while (WANT(p))
                p++;
        }
    }
    return argc;
}

#undef SKIP
#undef WANT

/**
 *
 * @param input const char* - the full input string
 * @param argc_ptr int* - location of integer to store arg count
 * @param argv_ptr char*** - location of argv array location for arg storage
 * @return int - return -1 on error
 */
static int createArgcArgv(const char* input, int* argc_ptr, char*** argv_ptr) {
    int argc;
    char ** argv;

    argc = argCount(input);
    if (argc == 0)
        return -1;

    argv = malloc(sizeof (char *) * argc);
    if (!argv)
        return -1;
    if (argCopy(input, argc, argv) == -1)
        return -1;

    *argc_ptr = argc;
    *argv_ptr = argv;

    return 0;
}
