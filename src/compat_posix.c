/* 
 * Copyright (C) 2013 nu774
 * For conditions of distribution and use, see copyright notice in COPYING
 */
#if HAVE_CONFIG_H
#  include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "compat.h"

FILE *aacenc_fopen(const char *name, const char *mode)
{
    FILE *fp;
    if (strcmp(name, "-") == 0)
        fp = (mode[0] == 'r') ? stdin : stdout;
    else
        fp = fopen(name, mode);
    return fp;
}

void aacenc_getmainargs(int *argc, char ***argv)
{
    return;
}

int aacenc_fprintf(FILE *fp, const char *fmt, ...)
{
    va_list ap;
    int cnt;

    va_start(ap, fmt);
    cnt = vfprintf(fp, fmt, ap);
    va_end(ap);
    return cnt;
}
