/*================================================================
 * slist.c:
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "slist.h"
#include "util.h"

/*----------------------------------------------------------------
 * prototypes
 *----------------------------------------------------------------*/

static SList SReadAtom(FILE *fp, int in_list);
static SList SReadList(FILE *fp);
static int SReadInt(FILE *fp);
static int SReadChr(FILE *fp);
static char *SReadFun(FILE *fp);
static char *SReadStr(FILE *fp);


/*----------------------------------------------------------------
 * allocate an atom
 *----------------------------------------------------------------*/

SList SCons(void)
{
	SList p = (SList)safe_malloc(sizeof(SAtom));
	memset(p, 0, sizeof(SAtom));
	return p;
}


/*----------------------------------------------------------------
 * read s-expression file
 *----------------------------------------------------------------*/

SList SReadFile(FILE *fp)
{
	return SReadAtom(fp, FALSE);
}


/*----------------------------------------------------------------
 * free a list
 *----------------------------------------------------------------*/

void SFree(SList at)
{
	SList nx;
	while (at) {
		switch (at->type) {
		case S_ATOM_LIST:
			SFree(at->val.p);
			break;
		case S_ATOM_STR:
		case S_ATOM_FUN:
			free(at->val.s);
			break;
		}
		nx = at->nxt.p;
		free(at);
		at = nx;
	}
}


/*----------------------------------------------------------------
 * return number of members
 *----------------------------------------------------------------*/

int SIndex(SList list)
{
	int n;
	if (list == NIL || !SListP(list)) return -1;
	n = 0;
	for (list = SCar(list); list; list = SCdr(list))
		n++;
	return n;
}


/*----------------------------------------------------------------
 * file operation
 *----------------------------------------------------------------*/

/* skip one character */
#define skipc(fp)	getc(fp)

/* skip one line */
static void skipline(FILE *fp)
{
	int c;
	while ((c = getc(fp)) != EOF && c != '\n')
		;
}

#define ESC_CHAR	'\033'
#define todigit(c)	((c) - '0')

/* read a character */
static int readchar(int delim, int accept_empty, FILE *fp)
{
	int c, c2, c3;

	if ((c = getc(fp)) == ESC_CHAR) {
		if ((c = getc(fp)) == EOF || c == '\n')
			return EOF;
		if (isdigit(c)) {
			if ((c2 = getc(fp)) == EOF || !isdigit(c2))
				return EOF;
			if ((c3 = getc(fp)) == EOF || !isdigit(c3))
				return EOF;
			c = (todigit(c) << 6) | (todigit(c2) << 3) | (todigit(c3));
		}
	} else if (c == delim) {
		if (! accept_empty)
			fprintf(stderr, "SList: empty chracter\n");
		return EOF;
	}

	return c;
}

/* read a token */
static char *readtoken(char *buf, int delim, FILE *fp)
{
	char *p;
	int c;

	p = buf;
	for (;;) {
		if (delim) {
			if ((c = readchar(delim, TRUE, fp)) == EOF)
				break;
		} else {
			if ((c = getc(fp)) == EOF)
				break;
			if (isspace(c) || c == '\n')
				break;
			if (c == ')') {
				ungetc(c, fp);
				break;
			}
		}
		*p++ = c;
	}
	*p = 0;
	return buf;
}

/* read a character and restore it again */
static int readhead(FILE *fp)
{
	int c;
	while ((c = getc(fp)) != EOF) {
		if (!isspace(c) && c != '\n') {
			if (c == ';' || c == '#') /* comment */
				skipline(fp);
			else {
				ungetc(c, fp);
				return c;
			}
		}
	}
	return EOF;
}

/*----------------------------------------------------------------
 * read an atom
 *----------------------------------------------------------------*/

static SList SReadAtom(FILE *fp, int in_list)
{
	SList cur;
	int c;

	/* skip space and comments */
	if ((c = readhead(fp)) == EOF) {
		if (in_list)
			fprintf(stderr, "SList: non-closed list\n");
		return NIL;
	}
	if (c == ')') { /* close list */
		skipc(fp); /* skip it */
		if (!in_list)
			fprintf(stderr, "SList: non-matched parenthesis\n");
		return NIL;
	}

	cur = SCons();
	switch (c) {
	case '(': /* open list */
		cur->type = S_ATOM_LIST;
		cur->val.p = SReadList(fp);
		break;
	case '"': /* string */
		cur->type = S_ATOM_STR;
		cur->val.s = SReadStr(fp);
		break;
	case '\'': /* char */
		cur->type = S_ATOM_CHR;
		cur->val.c = SReadChr(fp);
		break;
	default:
		if (isdigit(c) || c == '-') { /* int */
			cur->type = S_ATOM_INT;
			cur->val.i = SReadInt(fp);
		} else { /* func */
			cur->type = S_ATOM_FUN;
			cur->val.f = SReadFun(fp);
		}
		break;
	}
	return cur;
}

/* read a list */
static SList SReadList(FILE *fp)
{
	SList head, cur;

	skipc(fp); /* skip open parenthesis */
	head = cur = SReadAtom(fp, TRUE);
	while (cur) {
		cur->nxt.p = SReadAtom(fp, TRUE);
		cur = cur->nxt.p;
	}
	return head;
}

/* read an integer value */
static int SReadInt(FILE *fp)
{
	char str[256];
	readtoken(str, 0, fp);
	return (int)strtol(str, NULL, 0);
	return 0;
}
	
/* read a character value */
static int SReadChr(FILE *fp)
{
	int c;
	skipc(fp); /* skip leading quote */
	c = readchar('\'', FALSE, fp);
	if (getc(fp) != '\'')
		fprintf(stderr, "SList: non-closed character\n");
	return c;

}
		
/* read a constant string */
static char *SReadFun(FILE *fp)
{
	char str[256];
	readtoken(str, 0, fp);
	return safe_strdup(str);
}

/* read a string */
static char *SReadStr(FILE *fp)
{
	char str[256];
	skipc(fp); /* skip reading double quote */
	readtoken(str, '"', fp);
	return safe_strdup(str);
}


#ifdef DEBUG_MODE

/*----------------------------------------------------------------
 * print a list recursively
 *----------------------------------------------------------------*/

void print_list(SList at)
{
	while (at) {
		switch (at->type) {
		case S_ATOM_LIST:
			printf("(");
			print_list(at->val.p);
			printf(")\n");
			break;
		case S_ATOM_STR:
			printf("\"%s\"", at->val.s);
			break;
		case S_ATOM_FUN:
			printf("%s", at->val.s);
			break;
		case S_ATOM_INT:
			printf("%d", at->val.i);
			break;
		case S_ATOM_CHR:
			printf("'%c'", at->val.c);
			break;
		default:
			printf("*unknown*");
			break;
		}
		at = at->nxt.p;
		if (at)
			printf(" ");
	}
}

#endif
