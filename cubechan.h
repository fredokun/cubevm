
#ifndef CUBE_CHAN_H
#define CUBE_CHAN_H

#include "cubeglobals.h"

typedef struct _Channel {
  unsigned long id;
  uint32 ctl;
  struct _Process *owner; // last writer
} Channel;

/* ID accessor */
#define CHANP_GET_ID(c) (c->id)
/* maximum of 2^32 channels */
#define CHAN_MAX_ID 4294967295UL // 2^32 - 1

/* CTL bit 31 : Free channel (cache) */ /* XXX: FixMe maybe redundant with Scheduler.free_chans[] */
#define CHAN_FREE_MASK 0x80000000UL
#define CHANP_IS_FREE(c) (((c)->ctl)&CHAN_FREE_MASK)
#define CHANP_SET_FREE(c) (((c)->ctl)|=CHAN_FREE_MASK)

/* CTL bit 30 : Full channel (active output) */
#define CHAN_FULL_MASK 0x40000000UL
#define CHANP_IS_FULL(c) (((c)->ctl)&CHAN_FULL_MASK)
#define CHANP_IS_EMPTY(c) (!CHANP_IS_FULL(c))
#define CHANP_SET_FULL(c) (((c)->ctl)|=CHAN_FULL_MASK)
#define CHANP_UNSET_FULL(c) (((c)->ctl)^=CHAN_FULL_MASK)

/* CTL bit 29 : Reactive channel (deterministic) */
#define CHAN_REACT_MASK 0x20000000UL
#define CHANP_IS_REACT(c) (((c)->ctl)&CHAN_REACT_MASK)
#define CHANP_SET_REACT(c) (((c)->ctl)|=CHAN_REACT_MASK)
#define CHANP_UNSET_REACT(c) (((c)->ctl)^=CHAN_REACT_MASK)

/* CTL bit 28 : Volatile channel (automatic GC) */
#define CHAN_STATIC_MASK 0x10000000UL
#define CHANP_IS_STATIC(c) (((c)->ctl)&CHAN_STATIC_MASK)
#define CHANP_IS_VOLATILE(c) (!CHANP_IS_STATIC(c))
#define CHANP_SET_STATIC(c) (((c)->ctl)|=CHAN_STATIC_MASK)

/* CTL bits 27 <- 0 : reference count */
#define CHAN_REFCOUNT_MASK 0x0FFFFFFFUL
#define CHAN_REFCOUNT_COMPLEMENT 0xF0000000UL
#define CHANP_GET_REFCOUNT(c) (((c)->ctl)&CHAN_REFCOUNT_MASK)
#define CHANP_REFCOUNT_ZERO(c) ((((c)->ctl)&CHAN_REFCOUNT_MASK)==0)
#define CHANP_REFCOUNT_ONE(c) ((((c)->ctl)&CHAN_REFCOUNT_MASK)==1)
#define CHANP_INCR_REFCOUNT(c) (((c)->ctl)=(((((c)->ctl)&CHAN_REFCOUNT_MASK)+1)+(((c)->ctl)&CHAN_REFCOUNT_COMPLEMENT)))
#define CHANP_DECR_REFCOUNT(c) (((c)->ctl)=(((((c)->ctl)&CHAN_REFCOUNT_MASK)-1)+(((c)->ctl)&CHAN_REFCOUNT_COMPLEMENT)))

/* maximum of 2^28 references per channel */
#define CHAN_MAX_REFCOUNT 268435455UL // 2^28 - 1
 
/* ownership (reactive channels) */
#define CHANP_GET_OWNER(c) ((c)->owner)
#define CHANP_SET_OWNER(c,o) ((c)->owner=(o))

#define CHAN_INIT_MASK 0x00000000UL
extern Channel * chan_init(Channel *chan, unsigned long index);

#endif
