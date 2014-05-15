
#ifndef HASH_H
#define HASH_H

/* DON'T MODIFY THE FOLLOWING DEFINES ! */

#define HASH_LENGTH 8   /* the number of bits of the hash values */
#define HASH_MAX_ENTRIES (1 << HASH_LENGTH) /* number of elements in a hash table */

/* hash value function */

int HashValueGenerator( char *name );

#endif /* end of hash.h */

