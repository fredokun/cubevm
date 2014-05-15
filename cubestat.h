
#ifndef CUBESTAT_H
#define CUBESTAT_H

#ifdef RUNTIME_STAT

#include "cubesched.h"

extern void print_runtime_stats(FILE * out, Scheduler * sched);

#endif

#endif
