/*================================================================
 * utility routines
 *
 * Copyright (C) 1996-1999 Takashi Iwai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *================================================================*/

#ifndef UTIL_H_DEF
#define UTIL_H_DEF

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define numberof(ary)	(sizeof(ary)/sizeof(ary[0]))
#ifndef offsetof
#define offsetof(s_type,field) ((int)(((char*)(&(((s_type*)NULL)->field)))-((char*)NULL)))
#endif

#define BITON(var,flag)	((var) |= (flag))
#define BITOFF(var,flag) ((var) &= ~(flag))
#define BITSWT(var,flag) ((var) ^= (flag))

extern int awe_verbose, debug;
#define DEBUG(LVL,XXX)	{if (awe_verbose > LVL) { XXX; }}

/* cmpopen.c */
char *CmpGetExtension(char *name);
FILE *CmpOpenFile(char *name, int *flag);
void CmpCloseFile(FILE *fp, int flag);
void CmpAddList(char *ext, char *format);

/* malloc.c */
void *safe_malloc(int size);
void safe_free(void *ptr);
char *safe_strdup(char *src);

/* bool.c */
char *strtoken(char *src);
char *strschr(char *str, char *dels);
int strlcmp(char *ap, char *bp);
int bool_val(char *val);

/* fskip.c */
void fskip(int size, FILE *fd, int seekable);

/* path.c */
int awe_search_file_name(char *fresult, int maxlen, char *fname, char *path, char **ext);


#endif
