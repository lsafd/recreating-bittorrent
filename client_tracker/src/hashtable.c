/* 
 * C dictionary implementation from: 
 * https://stackoverflow.com/questions/4384359/quick-way-to-implement-dictionary-in-c
 * 
 * Written by Vijay Mathew <http://vmathew.in/>.
 *
 *
 * Edited for CS87 @ Swarthmore College by Sasha Casada
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "hashtable.h"

/////////////////////////////////////////////////////////////////////

struct nlist *hashtab[HASHSIZE];

unsigned hash(char *s)
{
	unsigned hashval;
	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;
	return hashval % HASHSIZE;
}

struct nlist *hash_lookup(char *s)
{
	struct nlist *np;
	for (np = hashtab[hash(s)]; np != NULL; np = np->next)
		if (strcmp(s, np->name) == 0)
			return np; /* found */
	return NULL; /* not found */
}

struct nlist *hash_install(char *name, struct peer_info *defn)
{
	struct nlist *np;
	unsigned hashval;
	if ((np = hash_lookup(name)) == NULL) { /* not found */
		np = (struct nlist *) malloc(sizeof(*np));
		if (np == NULL || (np->name = strdupl(name)) == NULL)
			return NULL;
		hashval = hash(name);
		np->next = hashtab[hashval];
		hashtab[hashval] = np;
	} else /* already there */
		free((void *) np->defn); /*free previous defn */
	return np;
}

char *strdupl(char *s) /* make a duplicate of s */
{
	char *p;
	p = (char *) malloc(strlen(s)+1); /* +1 for ’\0’ */
	if (p != NULL)
		strcpy(p, s);
	return p;
}