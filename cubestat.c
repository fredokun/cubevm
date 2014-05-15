
/* Runtime statistics module */

#ifdef RUNTIME_STAT

#include <stdio.h>

#include "cubestat.h"

void print_runtime_stats(FILE *out, Scheduler *sched) {
  fprintf(out, "Runtime Statistics :\n");
  fprintf(out, "  Number of processes spawned = %ld\n", sched->nb_proc_spawned);
  fprintf(out, "  Number of processes ended = %ld\n\n", sched->nb_proc_ended);
  fprintf(out, "  Number of channels allocated (memory) = %ld\n", sched->nb_channel_alloc);
  fprintf(out, "  Number of channels acquired (cache) = %ld\n", sched->nb_channel_acquire);
  fprintf(out, "  Number of channels freed (cache)= %ld\n", sched->nb_channel_free);
  fprintf(out, "  Number of channels reclaimed (memory) = %ld\n", sched->nb_channel_reclaim);
  fprintf(out, "  Number of average reference per channel = %lf\n",sched->nb_ref_per_channel);
  fprintf(out, "  Memory allocated for processes = %ld bytes\n", sched->nb_proc_spawned*sizeof(Process));
  fprintf(out, "  Memory reclaimed for processes = %ld bytes\n", sched->nb_proc_ended*sizeof(Process));
  fprintf(out, "  Memory allocated for channels (exact) = %ld bytes\n", sched->nb_channel_alloc*sizeof(Channel));
  fprintf(out, "  Memory reclaimed for channels (exact) = %ld bytes\n", sched->nb_channel_reclaim*sizeof(Channel));
}
#endif
