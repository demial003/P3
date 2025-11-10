#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "arraylist.h"

#ifndef SAFE
#define SAFE 0
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

int
al_init (arraylist_t *L, unsigned capacity)
{
    assert(capacity > 0);
    L->data = malloc(capacity * sizeof(char *));
    if (!L->data) return 1;
    L->cap = capacity;
    L->len = 0;
    
    return 0;
}

void
al_destroy(arraylist_t *L)
{
    free(L->data);
    if (SAFE) {
	L->data = NULL;
	L->len = 0;
	L->cap = 0;
    }
}

unsigned
al_length (arraylist_t *L)
{
    return L->len;
}


int
al_push (arraylist_t *L, char *item)
{
    if (DEBUG) printf("[list cap=%d, len=%d]\n", L->cap, L->len);
    if (L->len == L->cap) {
	// increase capacity
	L->cap *= 2;
	char **d = realloc(L->data, L->cap * sizeof(char *));
	if (!d) return 1;
	L->data = d;
	if (DEBUG) printf("[doubled cap to %d]\n", L->cap);
    }
    
    L->data[L->len] = item;
    L->len++;

    if (DEBUG) printf("[increased len to %d]\n", L->len);

    return 0;
}

int
al_pop (arraylist_t *L, char **dest)
{
    if (L->len == 0) return 0;

    L->len--;
    *dest = L->data[L->len];

    if (DEBUG) printf("[decreased len to %d]\n", L->len);

    return 1;
}
