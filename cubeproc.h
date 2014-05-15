
#ifndef CUBE_PROC_H
#define CUBE_PROC_H

#include "cubeval.h"
#include "cubedef.h"

typedef struct _Environment {
  Value * entries;
  int nb_entries;
} Environment;

typedef enum { ACTIVE=0,             // process is (thought of as) runnable
	       ACTIVE_WAIT_INPUT=1,  // process waits to perform input in active queue
	       ACTIVE_WAIT_OUTPUT=2, // process waits to perform output in active queue
	       ACTIVE_WAIT_CHOICE=3, // process waits to perform choice in active queue
	       WAIT_INPUT=4,         // process waits to perform input
	       WAIT_OUTPUT=5,        // process waits to perform output (another is writing)
	       WAIT_OUTPUT_SYNC=6,   // process waits to synchronize (wait a reader)
	       WAIT_CHOICE=7,        // process waits on multiple condition
	                             // xxx: may extend this, for the moment
	                             //      mark choice as runnable
	                             //      but put in wait queue if wait
	                             //      too many times with choice
	       REACT=8,              // process run reactive subprocess 
               REACT_CHILD=9,        // reactive subprocess
	       REACT_END=10          // end of reaction
               
} ProcState;

// The running state of a process
typedef struct _Process {
  unsigned long id;
  Definition * def;
  Value val;  // for synchronization // XXX: FixMe put back value in channel ?
  int last_choice;
  ProcState state;
  int nb_wait; // passivate only if number of wait excess some constant
  unsigned long sched_ref; // reference in the scheduler
  Environment env;
  struct _Process * partner; // current child for parent, parent for children
} Process;

#endif
