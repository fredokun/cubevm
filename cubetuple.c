
#include <assert.h>

#include "cubeglobals.h"
#include "cubetuple.h"
#include "cubealloc.h"
#include "cubemisc.h"

Tuple * tuple_create(uint32 size) {
  Tuple * tuple = (Tuple*) MEM_ALLOC_SINGLE(sizeof(Tuple),"Tuple","cubetuple.c","tuple_create");
  if(tuple==NULL) {
    fatal_error("cubetuple.c","tuple_create",__LINE__,"Cannot allocate tuple, memory exhausted");
  }

  // PRECONDITION : size is less than max size
  assert(size>=0 && size<=TUPLE_MAX_SIZE);

  TUPLE_SET_SIZE(tuple,size);

  // POSTCONDITION: setsize works
  assert(TUPLE_GET_SIZE(tuple)==size);

  tuple->contents = (Value*) MEM_ALLOC_ARRAY(sizeof(Value),size,"Value*","cubetuple.c","tuple_create");
  if(tuple->contents==NULL) {
    fatal_error("cubetuple.c","tuple_create",__LINE__,"Cannot allocate tuple contents, memory exhausted");
  }

  // no reference value in tuple
  TUPLE_UNSET_HAS_REF(tuple);

  // POSTCONDITION : size does not change
  assert(TUPLE_GET_SIZE(tuple)==size);

  // POSTCONDITION : has_ref is unset
  assert(!TUPLE_HAS_REF(tuple));

  return tuple;
}

Tuple * tuple_clone(Scheduler * sched, const Tuple * source) {
  Tuple * tuple = NULL;
  unsigned int i;

  /* PRECONDITION : source tuple is not NULL */
  assert(source!=NULL);

  tuple = tuple_create(TUPLE_GET_SIZE(source));
  /* POSTCONDITION : new tuple and old tuple have same size */
  assert(TUPLE_GET_SIZE(tuple)==TUPLE_GET_SIZE(source));

  for(i=0;i<TUPLE_GET_SIZE(source);i++) {
    TUPLE_SET_ELEM(tuple,i,TUPLE_GET_ELEM(source,i));
  }

  if(TUPLE_HAS_REF(source)) {
    TUPLE_SET_HAS_REF(tuple);
    for(i=0;i<TUPLE_GET_SIZE(tuple);i++) {
      Value val = TUPLE_GET_ELEM(tuple,i);
      /* Copy/Paste from cubesched::register_value */
      switch(VALUE_GET_TYPE(val)) {
      case VALUE_CHAN:
	CHANP_INCR_REFCOUNT(VALUE_AS_CHANP(val));
	break;
      case VALUE_STRING:
	STRREF(sched->string_pool, VALUE_AS_STRINGREF(val));
	break;
      default:
	// do nothing
	break;
      }
    }
  }
  
  /* POSTCONDITION : same HAS_REF flag */
  assert(TUPLE_HAS_REF(tuple)==TUPLE_HAS_REF(source));

  return tuple;
}

Tuple * tuple_destroy(Scheduler * sched, Process * proc, Tuple *tuple) {
  /* PRECONDITION : tuple is not NULL */
  assert(tuple!=NULL);

  /* unreference if has references */
  if(TUPLE_HAS_REF(tuple)) {
    int i;
    Channel * chan;
    for(i=0;i<TUPLE_GET_SIZE(tuple);i++) {
      Value prev = tuple->contents[i];
      /* Copy/Paste from cubesched.c::proc_update_env */
      switch(VALUE_GET_TYPE(prev)) {
      case VALUE_CHAN:
	// dereference channel count if necessary
	chan = VALUE_AS_CHANP(prev);
	
	// PRECONDITION: channel reference must be >0
	assert(!CHANP_REFCOUNT_ZERO(chan));
	
	CHANP_DECR_REFCOUNT(chan);
	if(CHANP_REFCOUNT_ZERO(chan)) {
	  reclaim_channel(sched,proc,chan);
	}
	break;
      case VALUE_STRING: 
	// reference counting for strings
	
	// PRECONDITION: string reference must be >0
	assert(STRREFCOUNT(sched->string_pool,VALUE_AS_STRINGREF(prev))>0 ||
	       STRREFCOUNT(sched->string_pool,VALUE_AS_STRINGREF(prev)==-1)); // for permanent strings
	
	STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(prev));
	if(STRREFCOUNT(sched->string_pool,VALUE_AS_STRINGREF(prev))==0)
	  str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(prev));
	break;
      default:
	// do nothing
	break;
      }
    }
  }

  MEM_FREE_ARRAY(tuple->contents,sizeof(Value),tuple->size,"Value*","cubetuple.c","tuple_destroy");
  tuple->contents=NULL;

  MEM_FREE_SINGLE(tuple,sizeof(Tuple),"Tuple","cubetuple.c","tuple_destroy");
  tuple = NULL;

  return tuple; // ENSURE : tuple==NULL
}
