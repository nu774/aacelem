/* 
 * Copyright (C) 2013 nu774
 * For conditions of distribution and use, see copyright notice in COPYING
 */
#ifndef COMPAT_H
#define COMPAT_H

FILE *aacenc_fopen(const char *name, const char *mode);
void aacenc_getmainargs(int *argc, char ***argv);
int aacenc_fprintf(FILE *fp, const char *fmt, ...);

#endif
