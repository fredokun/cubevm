
#ifndef CUBE_SCHED_H
#define CUBE_SCHED_H

#include "cubestr.h"
#include "cubeval.h"
#include "cubechan.h"
#include "cubedef.h"
#include "cubeproc.h"

typedef struct _Scheduler {
  StringPool * string_pool; // strings attached to the scheduler
  DefEntry** defs; // program definitions
  int defs_size; // the size of the definition array
  int nb_defs; // the number of active definitions
  Process ** active_procs;  // active (or thought so) processes
  Process ** wait_procs; // waiting (detected as such) processes
  Process ** react_procs;  // reactive (un-scheduled) processes
  Channel ** chans;
  Channel ** free_chans;
  unsigned long process_counter; // to generate unique process id
  // note : unlike channels, we cannot use the index
  //        in the scheduler array because
  //        process indices are swapped during scheduling
  //        (so they would change their id !)//
  unsigned long nb_procs;
  unsigned long nb_rprocs;
  unsigned long nb_active;
  unsigned long nb_wait;
  unsigned long last_proc;
  unsigned long nb_chans;        // current number of channels (allocated/free)
  unsigned long max_chans;       // maximum number of channels (allocatable)
  unsigned long nb_free_chans;   // number of allocated but free channels (cache)
  unsigned long last_chan_index; // last allocated channel
  unsigned long max_procs;
  unsigned long max_rprocs;
  int fuel;
  unsigned long turn;
  // runtime statistics
#ifdef RUNTIME_STAT
  unsigned long nb_proc_spawned;
  unsigned long nb_proc_ended;
  unsigned long nb_channel_alloc;
  unsigned long nb_channel_acquire;
  unsigned long nb_channel_free;
  unsigned long nb_channel_reclaim;
  float nb_ref_per_channel;
#endif
  int argc; // command line argument numbers
  char** argv; // command line arguments
} Scheduler;

////////////////
// Interface //
///////////////

extern Process * spawn_root(Scheduler *sched, Definition *def);

extern void scheduler_add_proc(Scheduler *sched, Process * newproc);

extern void schedule_run(Scheduler *sched);

extern Scheduler * scheduler_create(StringPool *sp, DefEntry** defs, int defs_size, int nb_defs);

extern Scheduler * scheduler_free(Scheduler *sched);


#endif
