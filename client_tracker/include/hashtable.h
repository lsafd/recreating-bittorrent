/* 
 * C dictionary implementation from: 
 * https://stackoverflow.com/questions/4384359/quick-way-to-implement-dictionary-in-c
 * 
 * Written by Vijay Mathew <http://vmathew.in/>.
 *
 *
 * Edited for CS87 @ Swarthmore College by Sasha Casada
 */

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#define HASHSIZE        101
#define MAX_PEERS       256   // max # of peers that a file can support

///////////////////////////////////////////////////////////////////////////////

/* struct that encapsulates all of the peer information
 */
struct peer_info {
  char *peers[MAX_PEERS]; // the list of all peer IPs
  int curr_peers;         // the total # of active peers
};

struct nlist { /* table entry: */
    struct nlist *next; /* next entry in chain */
    char *name; /* defined name */
    struct peer_info *defn; /* replacement text */
};

extern struct nlist *hashtab[HASHSIZE];

/* hash: form hash value for string s */
unsigned hash(char *s);

/* lookup: look for s in hashtab */
struct nlist *hash_lookup(char *s);

/* install: put (name, defn) in hashtab */
struct nlist *hash_install(char *name, struct peer_info *defn);

/* make a duplicate of s */
char *strdupl(char *s);

#endif