
#ifndef CUBE_PRIM_H
#define CUBE_PRIM_H

#include "cubeglobals.h"
#include "cubeval.h"
#include "cubesched.h"

extern char * GLOBAL_primitive_names[];
extern int GLOBAL_primitive_arity[];
extern Bool GLOBAL_primitive_value[];
extern int GLOBAL_nb_primitives;

extern int GLOBAL_get_primitive_ref(char *pname);

extern void prim_print(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_println(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_not(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_and(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_or(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_add(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_sub(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_mul(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_div(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_mod(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_eq(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_ineq(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_gt(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_gte(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_lw(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_lwe(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_umin(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_rclock(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_clock(Scheduler *sched, Process *proc, Definition *expr, Value args[]);
extern void prim_arg(Scheduler *sched, Process * proc, Definition *expr, Value args[]);
extern void prim_intfromstr(Scheduler *sched, Process * proc, Definition *expr, Value args[]);

typedef void (*PrimitivePtrType) (Scheduler *, Process *, Definition *, Value[]);

extern PrimitivePtrType GLOBAL_primitives[];

#endif
