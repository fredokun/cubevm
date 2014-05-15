
#ifndef CUBE_TUPLE_H
#define CUBE_TUPLE_H

#include "cubeglobals.h"
#include "cubeval.h"
#include "cubesched.h"
#include "cubeproc.h"

typedef struct _Tuple {
  uint32 size; // size and control
  Value * contents; // contents
} Tuple;

/* First bit : 0 = does not contain reference type
               1 = does contain reference type */

#define TUPLE_MAX_SIZE 2147483648U /* 2^32-1 */

#define TUPLE_HREF_MASK 0x80000000U
#define TUPLE_HAS_REF(t) (((t)->size)&TUPLE_HREF_MASK)
#define TUPLE_SET_HAS_REF(t) (((t)->size)|=TUPLE_HREF_MASK)
#define TUPLE_UNSET_HAS_REF(t) (((t)->size)^=TUPLE_HREF_MASK)

#define TUPLE_SIZE_MASK 0x7FFFFFFFU
#define TUPLE_SIZE_COMPLEMENT 0x80000000U
#define TUPLE_GET_SIZE(t) (((t)->size)&TUPLE_SIZE_MASK)
#define TUPLE_SET_SIZE(t,nsize) (((t)->size)=((nsize)&TUPLE_SIZE_MASK)+(((t)->size)&TUPLE_SIZE_COMPLEMENT))

#define TUPLE_GET_ELEM(t,i) ((t)->contents[(i)])
#define TUPLE_SET_ELEM(t,i,v) (((t)->contents[(i)])=(v))

extern Tuple * tuple_create(uint32 size);

extern Tuple * tuple_clone(Scheduler *sched, const Tuple * source);

extern Tuple * tuple_destroy(Scheduler * sched, Process * proc, Tuple * tuple);

#endif
