typedef struct {
    char **data;
    unsigned cap;  // capacity of array
    unsigned len;  // number of items in the list (array)
} arraylist_t;

int al_init(arraylist_t *, unsigned);
void al_destroy(arraylist_t *);

unsigned al_length(arraylist_t *);

int al_push(arraylist_t *, char *);

int al_pop(arraylist_t *, char**);
