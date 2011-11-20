#ifndef __ARGPARSE_H
#define __ARGPARSE_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static int argCopy(const char*, int, char**);
static int argCount(const char*);
static int createArgcArgv(const char*, int*, char***);

#endif
