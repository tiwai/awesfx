/*================================================================
 * slist.h:
 *	S-expression list
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

#ifndef SLIST_H_DEF
#define SLIST_H_DEF

enum { S_ATOM_NIL, S_ATOM_LIST, S_ATOM_CHR, S_ATOM_INT, S_ATOM_STR, S_ATOM_FUN, };

typedef struct _SAtom *SList;

typedef union {
	char c;
	int i;
	char *s;
	char *f;
	SList p;
} SAtomVal;

typedef struct _SAtom {
	int type;
	SAtomVal val, nxt;
} SAtom;

SList SCons(void);
SList SReadFile(FILE *fp);
void SFree(SList);

#define NIL	(SList)0

#define SIsNil(at)	((at)->type == S_ATOM_NIL)
#define SListP(at)	((at)->type == S_ATOM_LIST)
#define SChrP(at)	((at)->type == S_ATOM_CHR)
#define SIntP(at)	((at)->type == S_ATOM_INT)
#define SStrP(at)	((at)->type == S_ATOM_STR)
#define SFunP(at)	((at)->type == S_ATOM_FUN)

#define SCar(at)	((at)->val.p)
#define SCdr(at)	((at)->nxt.p)

#define SChr(at)	((at)->val.c)
#define SStr(at)	((at)->val.s)
#define SInt(at)	((at)->val.i)
#define SFun(at)	((at)->val.f)
#define SFunIs(at,fun)	(strcmp(SFun(at),fun) == 0)

int SIndex(SList);

#endif
