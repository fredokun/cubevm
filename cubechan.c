
#include <stdlib.h>

#include "cubechan.h"

Channel * chan_init(Channel * chan, unsigned long index) {
  chan->id = index;
  chan->ctl = CHAN_INIT_MASK;
  chan->owner = NULL;
  return chan;
}
