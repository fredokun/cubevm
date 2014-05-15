
#include <string.h>
#include <math.h>
#include "hash.h"

#define HASH_THETA 0.6180339887  /* value used by the hash value generator */


/***** hash value calculation *****/

int HashValueGenerator( char *name )
{
  int len;
  int i;
  unsigned char cvalue;
  double mvalue;


  /* compression */

  len = strlen(name);
  
  cvalue = 0;

  for(i=0; i<len ; i++)
    cvalue += name[i];

  /* multiplication */

  mvalue = ( fmod((double) cvalue * HASH_THETA, 1.0) ) * (double) HASH_MAX_ENTRIES;

  return ( (int) floor(mvalue) );
} 
  
