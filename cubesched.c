
/************************************
 * INCLUDES
 ************************************/

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cubechan.h"
#include "cubetuple.h"
#include "cubesched.h"
#include "cubemisc.h"
#include "cubeprim.h"
#include "cubestr.h"
#include "cubealloc.h"
#include "cubecfg.h"

static
void reclaim_process(Scheduler *sched, Process * proc);

static
Process * new(Scheduler *sched, Process * proc, Definition * def);

static
Process * let(Scheduler *sched, Process * proc, Definition * def);

static
Process * call(Scheduler *sched, Process * proc, Definition * def);

static
Process * pown(Scheduler *sched, Process * proc);

static
Process * own(Scheduler *sched, Process * proc);

static
Process * interpret_primitive(Scheduler *sched, Process * proc, Definition * def, int call_id);

static
Process * interpret_if(Scheduler *sched, Process * proc, Definition * econd, Definition *ethen, Definition *eelse);

/********************************************************
 * CHANNEL MANAGEMENT
 ********************************************************/

/** Grow the channel list in scheduler (pre-allocations)
 *  --
 *  @param[in,out] sched : scheduler to update
 **/  
void sched_grow_chans(Scheduler *sched) {
  unsigned long old_max_chans = sched->max_chans;
  unsigned long i;
  
  // no need to grow ?
  assert(sched->nb_chans>=sched->max_chans);
  
  sched->max_chans+=GLOBAL_CFG_AS_ULONG(CFG_GROW_CHANS_FACTOR);
  if(sched->max_chans>CHAN_MAX_ID) {
    fatal_error("cubesched.c","sched_grow_chans",__LINE__,"Too many channels allocated, reached ID is %ld , max is %ld", sched->max_chans, CHAN_MAX_ID);
  }
  sched->chans = (Channel **) MEM_REALLOC_ARRAY(sched->chans,sizeof(Channel *),old_max_chans,sched->max_chans,"Channel*","cubesched.c","sched_grow_chans");
  if(sched->chans==NULL)
    fatal_error("cubesched.c","sched_grow_chans",__LINE__,"Cannot grow channel list in scheduler (memory exhausted)");
  // don't forget to empty the newly allocated entries
  for(i=old_max_chans;i<sched->max_chans;i++) {
    sched->chans[i] = NULL;
  }
}


/** Find the first empty channel
 *  Complexity (worst) = LINEAR TIME in number of channels
 *  --
 *  @param[in,out] sched : scheduler to update
 **/
unsigned long seek_first_allocatable_chan(Scheduler *sched) {
  unsigned long i;
  Channel *chan =NULL;
  
  // PRECONDITION: there are some allocatable channels, they should be NULL
  assert(sched->nb_chans < sched->max_chans);

  // start from last allocated, if possible
  if(sched->last_chan_index<sched->max_chans)
    sched->last_chan_index++;
  
  for(i=sched->last_chan_index;i<sched->max_chans;i++) {
    chan = sched->chans[i];
    if(chan==NULL) { // allocatable channel is NULL
      sched->last_chan_index = i;
      return i;
    }
  }
  
  // if not found, restart from start of channel list
  for(i=0;i<sched->max_chans;i++) {
    chan = sched->chans[i];
    if(chan==NULL) { // allocatable channel is NULL
      sched->last_chan_index = i;
      return i;
    }
  }
  
  // we should not reach this point since there should
  // be a difference between nb_chans and max_chans or at least, 
  // there should be free channels
  fatal_error("cubesched.c","seek_first_allocatable_chan",__LINE__,"No allocatable channel (please Report)");
  
  // will never arrive here (but removes compiler warning)
  return 0;
}

/** Channel deallocation
 *  @param[in,out] sched : scheduler to update
 *  @param[in] proc : process reclaiming the channel
 *  @paral[in,out] chan : channel to reclaim 
 **/
void reclaim_channel(Scheduler *sched,const Process *proc, Channel *chan) {

  // precondition: the reference count must be 0
  assert(CHANP_GET_REFCOUNT(chan)==0);

  // if we can cache the freed channel
  if(sched->nb_free_chans<GLOBAL_CFG_AS_ULONG(CFG_MAX_FREE_CHANS)) {
    CHANP_SET_FREE(chan);
    sched->free_chans[sched->nb_free_chans] = chan;
    sched->nb_free_chans++;
#ifdef RUNTIME_STAT
    sched->nb_channel_free++;
#endif
#ifdef PROC_TRACE
    char trace[128];
    snprintf(trace, 128,"Channel '%ld' reclaimed (and cached)", chan->id);
    trace[127]=(char)0;
    PTRACE(proc->id, trace);
#endif
  } else { // too many freed channels
    sched->chans[CHANP_GET_ID(chan)] = NULL;
#ifdef RUNTIME_STAT
      sched->nb_channel_reclaim++;
#endif
      MEM_FREE_SINGLE(chan,sizeof(Channel),"Channel","cubesched.c","reclaim_channel");
      chan = NULL;
#ifdef PROC_TRACE
  char trace[128];
  snprintf(trace, 128,"Channel '%ld' reclaimed (and destroyed)", chan->id);
  trace[127]=(char)0;
  PTRACE(proc->id, trace);
#endif
  }
}

/** Count the number of references by a process to a given channel
 *  @param[in] proc : the process
 *  @param[in] chan : the channel
 *  @return : the number of references by the process to the channel
 **/
static inline
int proc_count_chan_refs(const Process * proc, const Channel * chan) {
  unsigned int i=0;
  unsigned int j=0;
  int count=0;
  Tuple * tuple;
  for(i=0;i<proc->env.nb_entries;i++) {
    Value val = proc->env.entries[i];
    switch(VALUE_GET_TYPE(val)) {
    case VALUE_CHAN:
      if(VALUE_AS_CHANP(val)==chan)
	count++;
      break;
    case VALUE_TUPLE:
      tuple = VALUE_AS_TUPLE(val);
      if(TUPLE_HAS_REF(tuple)) { /* only if tuple contains references */
	for(j=0;j<TUPLE_GET_SIZE(tuple);j++) {
	  if(VALUE_IS_CHANP(TUPLE_GET_ELEM(tuple,j)) && (VALUE_AS_CHANP(TUPLE_GET_ELEM(tuple,j))==chan))
	    count++;
	}
      }
      break;
    default:
      break;
    }
  }
  return count;
}

/** Reclaim a process if volatile channel is only referenced by the process
 *  @param[in,out] sched : the scheduler of the process
 *  @param[in] proc : the process to tset to reclaim
 *  @param[in] chan : the input or output channel
 *  @return : the process if not reclaimed, NULL if reclaimed
 **/
static inline
Process * proc_test_reclaim(Scheduler *sched, Process *proc,Channel *chan) {
  // PRECONDITION : channel must be volatile
  assert(CHANP_IS_VOLATILE(chan));

  // count the number of references
  int nb_proc_refs = proc_count_chan_refs(proc,chan);
  if(nb_proc_refs==CHANP_GET_REFCOUNT(chan)) {
#ifdef PROC_TRACE
    unsigned long pid = proc->id;
    unsigned long cid = CHANP_GET_ID(chan);
#endif
    // if only the process references the channel then reclaim
    reclaim_process(sched,proc);
#ifdef PROC_TRACE
    char trace[128];
    snprintf(trace, 64, "Input/Output channel '%ld' only referenced by self, reclaim ", cid);
    PTRACE(pid, trace);
#endif
    return NULL; // the process does not exist anymore
  }

  return proc;
}

/** Set a value in environment (possibly overwriting)
 *  @param[in,out] sched : the scheduler of the process
 *  @param[in,out] proc : the process whose environment is to update
 *  @param[in] env_ref : the lexical index in environment
 *  @param[in] val : the value to put in environment
 **/
static inline
void proc_update_env(Scheduler *sched, Process *proc, int env_ref, Value val) {

  Channel* chan = NULL;

  // PRECONDITION: the environment index exists
  assert(env_ref>=0 && env_ref<proc->env.nb_entries);

  Value prev = proc->env.entries[env_ref];

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
  case VALUE_TUPLE:
    /* dereference everything in the tuple, and destroy the tuple */
    tuple_destroy(sched,proc,VALUE_AS_TUPLE(prev));
    break;    	
  default:
    // do nothing
    break;
  }
  
  // then replace the entry
  proc->env.entries[env_ref] = val;
}

/** Register a new channel in a process
 *  @param[in,out] sched : scheduler to update
 *  @param[in,out] proc : process registering the channel
 *  @param[in,out] chan : channel to register
 *  @param[in] env_ref : lexical location in process environment
 **/
void proc_register_channel(Scheduler *sched, Process *proc, Channel *chan, int env_ref) {
  
  // put the channel in the environment
  Value entry = VALUE_CREATE_NONE();
  VALUE_SET_TYPE(entry,VALUE_CHAN);
  VALUE_SET_CHANP(entry,chan);
  
  // update the environment
  proc_update_env(sched,proc,env_ref,entry);
  
  // don't forget to register from channel side also
  CHANP_INCR_REFCOUNT(chan);
#ifdef RUNTIME_STAT
  sched->nb_ref_per_channel += 1 / (sched->nb_channel_alloc+sched->nb_channel_acquire);
#endif
}

/** Channel creation
 *  @param[in,out] sched : scheduler to update
 *  @param[in,out] proc : process performing the creation
 *  @param[in] def : program definition of creation
 *  @return : updated process pointer
 **/
Process * new(Scheduler *sched, Process *proc,Definition *def) {
  Channel * chan = NULL;
  unsigned long index;
  int i;

  // PRECONDITION : arity must be more than 0 for a new prefix
  assert(def->arity>0);

  /* arity is greater than one for polyadic creations */
  for(i=def->arity-1;i>=0;i--) {

    // PRECONDITION: definition is correct
    assert(def->type==DEF_NEW);
    assert(def->nb_children==1);
    assert(def->binder_ref>=0);
    assert(def->binder_ref<proc->env.nb_entries);
    // PRECONDITION : we create more than one channel
    assert(def->arity>0);
    // PRECONDITION : the indicated arity must be equal to the counter
    assert(i==def->arity-1);

    // if there are some free channels available
    if(sched->nb_free_chans!=0) {
      chan = sched->free_chans[sched->nb_free_chans-1];
      index = CHANP_GET_ID(chan);
      sched->nb_free_chans--;
#ifdef RUNTIME_STAT
      sched->nb_channel_acquire++;
#endif
    } else { // no free channel, need to create a new one
      
      chan = (Channel *) MEM_ALLOC_SINGLE(sizeof(Channel),"Channel","cubesched.c","new");
      if(chan==NULL)
	fatal_error("cubesched.c","new",__LINE__,"Cannot allocate new channel");
#ifdef RUNTIME_STAT
      sched->nb_channel_alloc++;
#endif
      // don't forget to grow the list of channels if needed
      if(sched->nb_chans==sched->max_chans) {
	sched_grow_chans(sched);
      }
      
      // then add the new channel
      index = seek_first_allocatable_chan(sched);
      sched->nb_chans++;
    }
    
    // channel initialisation
    chan = chan_init(chan,index);
    
    // place channel in scheduler (remember id = channel index)
    sched->chans[index] = chan;
  
    // place channel in process ownership
    // index in environment if first binding of DEF_NEW definition
    // it is a VALUE_BIND entry
    proc_register_channel(sched,proc,chan,def->binder_ref);
    
    // NOTE: number of children must be ONE for new prefixes
    proc->def = def->children[0];
    def=proc->def;
  
#ifdef PROC_TRACE
    char trace[128];
    snprintf(trace, 128,"Channel '%ld' created", chan->id);
    trace[127]=(char)0;
    PTRACE(proc->id, trace);
#endif

  }

  return proc;
}

/********************************************************
 * VALUES AND PRIMITIVES
 ********************************************************/

/** Register a value for garbage collector
 * @param[in,out] val : the value to register
 **/
static inline
void register_value(Scheduler * sched, Value val) {
  switch(VALUE_GET_TYPE(val)) {
  case VALUE_CHAN:
    CHANP_INCR_REFCOUNT(VALUE_AS_CHANP(val));
    break;
  case VALUE_STRING:
    STRREF(sched->string_pool, VALUE_AS_STRINGREF(val));
    break;
  case VALUE_TUPLE: {
    Tuple * tuple = tuple_clone(sched,VALUE_AS_TUPLE(val));
    VALUE_SET_TUPLE(val,tuple);
  }
    break;
  default:
    // do nothing
    break;
  }
}

/** Interpret a value expression
 *  @param[in] sched : scheduler of process
 *  @param[in,out] proc : process of expression
 *  @param[in] expr : expression
 *  @return value of expression 
 **/ 
Value interpret_value_expr(Scheduler *sched, Process *proc,Definition* expr) {
  Value args[CALL_PRIMITIVE_MAX_ARITY];
  
  // PRECONDITON : value expression is passed
  assert(expr->type==DEF_VALUE || expr->type==DEF_PRIM);
  
  switch(expr->type) {
  case DEF_VALUE: 
    // definition is directly a value, return it
    return expr->val;
  case DEF_PRIM:
    // PRECONDITION : call id is correct
    assert(expr->call_id >=0 && expr->call_id < GLOBAL_nb_primitives);
    // PRECONDITION : correct number of arguments
    assert(GLOBAL_primitive_arity[expr->call_id]<CALL_PRIMITIVE_MAX_ARITY);
    assert(expr->nb_children == GLOBAL_primitive_arity[expr->call_id]);

    int i;
    for(i=0;i<GLOBAL_primitive_arity[expr->call_id];i++) {
      args[i] = interpret_value_expr(sched,proc,expr->children[i]);
    }
    GLOBAL_primitives[expr->call_id](sched,proc,expr,args);
#ifdef PROC_TRACE
  char trace[512];
  char buffer[256];
  print_value(sched->string_pool, buffer, 255, proc->val);
  snprintf(trace, 512,"Value primitive '%s' called, returned '%s'", GLOBAL_primitive_names[expr->call_id], buffer);
  trace[511]=(char)0;
  PTRACE(proc->id, trace);
#endif
    return proc->val;
  default:
    runtime_error(expr, "Non-value expression in value position");
  }

  // Dead code
  Value val = VALUE_CREATE_NONE();
  return val;
}

/** Interpret primitive call (not a value returning primitive)
 *  @param[in] sched : scheduler of process
 *  @param[in,out] proc : process calling primitive
 *  @param[in] instr : primitive instruction
 *  @return updated process
 **/ 
Process * interpret_primitive(Scheduler *sched, Process *proc, Definition *instr, int pref) {
  Value args[CALL_PRIMITIVE_MAX_ARITY];

  // PRECONDITION : instruction is a primitive call
  assert(instr->type==DEF_PRIM);
  // PRECONDITION : call id is correct
  assert(instr->call_id >=0 && instr->call_id < GLOBAL_nb_primitives);
  // PRECONDITION : correct number of arguments
  assert(GLOBAL_primitive_arity[instr->call_id]<CALL_PRIMITIVE_MAX_ARITY);
  assert(instr->nb_children == GLOBAL_primitive_arity[instr->call_id] + 1);

  int i;
  for(i=0;i<GLOBAL_primitive_arity[instr->call_id];i++) {
    args[i] = interpret_value_expr(sched,proc,instr->children[i]);
  }
  GLOBAL_primitives[instr->call_id](sched,proc,instr,args);
  proc->def = instr->children[instr->nb_children-1];
#ifdef PROC_TRACE
  char trace[128];
  snprintf(trace, 128,"Top-level primitive '%s' called", GLOBAL_primitive_names[instr->call_id]);
  trace[127]=(char)0;
  PTRACE(proc->id, trace);
#endif
  return proc;
}

/** Interpret if constructs
 *  @param[in] sched : scheduler of process
 *  @param[in,out] proc : process calling if
 *  @param[in] condition : condition expression
 *  @param[in] thenpart : then prefix
 *  @param[in] elsepart : else prefix
 *  @return updated process
 **/ 
Process * interpret_if(Scheduler *sched, Process *proc, Definition *condition, Definition *thenpart, Definition *elsepart) {
  // PRECONDITION: correct if expression
  assert(proc->def->type==DEF_IF);
  assert(proc->def->nb_children==3);
  assert(proc->def->children[0]==condition);
  assert(proc->def->children[1]==thenpart);
  assert(proc->def->children[2]==elsepart);

  Value val = interpret_value_expr(sched,proc,condition);

  if(VALUE_IS_BIND(val))
    val = proc->env.entries[VALUE_AS_BIND(val)];

  if(!VALUE_IS_BOOL(val))
    runtime_error(proc->def, "Condition of if expression must be a boolean");
  if(VALUE_AS_BOOL(val)==TRUE) {
#ifdef PROC_TRACE
  PTRACE(proc->id, "If expression, branching to then part");
#endif
  proc->def = thenpart;
  } else {
#ifdef PROC_TRACE
  PTRACE(proc->id, "If expression, branching to else part");
#endif
    proc->def = elsepart;
  }
  return proc;
}
  
/********************************************************
 * COMMUNICATIONS
 ********************************************************/

/** Communication function
 *  @param[in] sched : scheduler of processes
 *  @param[in,out] outproc : process performing output
 *  @param[in,out] inproc : process performing input
 **/ 
void communicate(Scheduler *sched, Process * outproc, Process * inproc, Channel * chan) {
  int i=0;

  // PRECONDITIONS : communication is enabled
  assert(outproc!=NULL && outproc->def!=NULL && inproc!=NULL && inproc->def!=NULL);
  assert(outproc->def->type==DEF_OUTPUT);
  assert(inproc->def->type==DEF_INPUT);
  assert(outproc->state==WAIT_OUTPUT_SYNC || outproc->state==REACT || outproc->state==REACT_CHILD);
  assert(inproc->state==ACTIVE || inproc->state==ACTIVE_WAIT_INPUT || inproc->state==REACT_END || inproc->state==REACT_CHILD || inproc->state==ACTIVE_WAIT_CHOICE);
  assert(outproc->def->arity>=0);
  assert(inproc->def->arity>=0);

  /* arity check */
  if(outproc->def->arity!=inproc->def->arity) {
    runtime_warning(outproc->def,"Mismatched output arity = %d", outproc->def->arity);
    runtime_error(inproc->def,"Mismatched input arity = %d", inproc->def->arity);
  }

  // PRECONDITION : arity match the number of children
  assert(outproc->def->nb_children==outproc->def->arity+1);

#ifdef PROC_TRACE
    {
      char trace[128];
      if(outproc->def->arity>1)
	snprintf(trace, 64, "Output on channel '%ld' of %d values", CHANP_GET_ID(chan),outproc->def->arity);
      else if(outproc->def->arity==1)
	snprintf(trace, 64, "Output on channel '%ld' (monadic)", CHANP_GET_ID(chan));
      else
	snprintf(trace, 64, "Action on channel '%ld'",CHANP_GET_ID(chan));
      PTRACE(outproc->id, trace);
    }
#endif

  for(i=0;i<outproc->def->arity;i++) {
    /* evaluation of output value */
    Value val = interpret_value_expr(sched,outproc,outproc->def->children[i]);
    if(VALUE_IS_BIND(val)) { // dereference if necessary
      // PRECONDITION : binding is ok
      assert(VALUE_AS_BIND(val)>=0);
      assert(VALUE_AS_BIND(val)< outproc->env.nb_entries);

      val = outproc->env.entries[VALUE_AS_BIND(val)];
    }
    
    if(VALUE_IS_CHANP(val)) { // if it's a channel
      proc_register_channel(sched,inproc,VALUE_AS_CHANP(val),inproc->def->binder_ref);
    } else {	// else if not a channel
      if(VALUE_IS_STRINGREF(val)) { // increment the reference count for the string
	STRREF(sched->string_pool,val.val._int);
      } else if(VALUE_IS_TUPLE(val)) {
	Tuple * clone = tuple_clone(sched,VALUE_AS_TUPLE(val));
	VALUE_SET_TUPLE(val,clone);
      }

      // then put in environment
      // PRECONDITION : environment is ok
      assert(inproc->def->binder_ref>=0);
      assert(inproc->def->binder_ref<inproc->env.nb_entries);

      proc_update_env(sched,inproc,inproc->def->binder_ref,val);
    }
  
    /* go to the next input */
    /* PRECONDITION : input has only one child */
    assert(inproc->def->nb_children==1);

    inproc->def = inproc->def->children[0];

#ifdef PROC_TRACE
    {
      char trace[128];
      char valuestr[64];
      snprintf(trace, 64, "Input on channel '%ld' , received value (%d/%d) = ", CHANP_GET_ID(chan),i+1,outproc->def->arity);
      print_value(sched->string_pool,valuestr,64,val);
      strncat(trace,valuestr,64);
      PTRACE(inproc->id, trace);
    }
#endif
  }

  if(outproc->def->arity==0) {
    inproc->def = inproc->def->children[0];
#ifdef PROC_TRACE
    {
      char trace[128];
      snprintf(trace, 64, "CoAction on channel '%ld'", CHANP_GET_ID(chan));
      PTRACE(inproc->id, trace);
    }
#endif
  }

  outproc->def = outproc->def->children[outproc->def->nb_children-1];

  /* values have been consumed */
  CHANP_UNSET_FULL(chan);

  /* no owner after output (active channels only) */
  if(!CHANP_IS_REACT(chan))
    CHANP_SET_OWNER(chan,NULL);
}

/** Input prefix
 *  @param[in] sched : scheduler of process
 *  @param[in,out] proc : process performing input
 *  @param[in] in_def : input definition
 *  @param[in,out] channel : channel of input (as value)
 *  @param[in,out] advanced : tells if the input was performed or not
 *  @return updated process
 **/ 
Process * input(Scheduler *sched, Process *proc, Definition *in_def,Value channel, Bool * advanced) {
  // PRECONDITION : correct input expression
  assert(in_def->type==DEF_INPUT);
  assert(in_def->nb_children==1);
  // PRECONDITION : binder reference is correct
  assert(in_def->binder_ref==-1 || (in_def->binder_ref>=0 && in_def->binder_ref<proc->env.nb_entries));
  // PRECONDITION : process state
  assert(proc->state==REACT_CHILD || proc->state==ACTIVE || proc->state==ACTIVE_WAIT_INPUT || proc->state==ACTIVE_WAIT_CHOICE);

  // by default we advance
  *advanced = TRUE;

  // first resolve channel reference
  if (VALUE_IS_BIND(channel)) {
    channel = proc->env.entries[VALUE_AS_BIND(channel)];
  }

  // XXX FixMe : Is it necessary to input on NONE values ?
  if(VALUE_IS_NONE(channel)) {
    runtime_warning(in_def,"Input on NONE value (indefinitely blocking)");
    *advanced = FALSE;
    return proc;
  }

  if(!VALUE_IS_CHANP(channel))
    runtime_error(in_def,"Cannot input on non-channel value (or NONE)");

  Channel *chan = VALUE_AS_CHANP(channel);

  // PRECONDITION : the channel is referenced somewher
  assert(CHANP_GET_REFCOUNT(chan)>0);
  
  if(CHANP_IS_EMPTY(chan)) { // channel is empty
    // blocking input on UNIQUE reference should be reclaimed (GC heuristics)
    if(CHANP_REFCOUNT_ONE(chan) && CHANP_IS_VOLATILE(chan)) {
#ifdef PROC_TRACE
      {
	char trace[128];
	snprintf(trace, 64, "Input on uniquely owned volatile channel '%ld' : reclaim process", CHANP_GET_ID(chan));
	PTRACE(proc->id, trace);
      }
#endif    
      reclaim_process(sched,proc);
      *advanced = FALSE;
      return NULL;
    }
  
    if(CHANP_IS_REACT(chan)) {
      if(CHANP_GET_OWNER(chan)==proc) {
	// cannot directly input on an owned reactive channel
	*advanced = FALSE;
	return proc;
      } else
	runtime_error(proc->def, "Input process cannot wait on a reactive channel they do not own");
    }

    /* or we did not advance */
    *advanced = FALSE;
    return proc;
  } else { // channel *is* full from here

    if(CHANP_IS_REACT(chan))
      runtime_error(proc->def, "May not input directly on a reactive channel");

    Process * outproc = CHANP_GET_OWNER(chan);
    
    // PRECONDITION : the output process is known
    assert(outproc!=NULL);

    if(outproc->state==REACT_END) 
      communicate(sched,outproc->partner,proc,chan);
    else
      communicate(sched,outproc,proc,chan);

    /* if at the end of reaction (in sync mode) */
    if(outproc->state==REACT_END) {
      if(outproc->partner->def->type==DEF_END) {
#ifdef PROC_TRACE
	{
	  char trace[128];
	    snprintf(trace, 64, "After output, will reclaim end of reaction child");
	  PTRACE(outproc->partner->id,trace);
	}
#endif
	reclaim_process(sched,outproc->partner);
      }
      outproc->partner=NULL;
    }

    if(outproc->state==REACT_CHILD) {
      if(outproc->def->type==DEF_END) {
#ifdef PROC_TRACE
	{
	  char trace[128];
	  snprintf(trace, 64, "After output, will reclaim output reactive child");
	  PTRACE(outproc->id,trace);
	}
#endif
	reclaim_process(sched,outproc);
      }
    }

    proc->state = ACTIVE;
    proc->nb_wait = 0;

    if(outproc!=NULL && outproc->state!=REACT_CHILD) {
      outproc->state = ACTIVE;
      outproc->nb_wait = 0;
    }
    
    return proc;
  }
}

/** Internal step execution
 **/
static
Process * step(Scheduler *sched, Process * proc) {
  while(1) {
    switch(proc->def->type) { // code extracted from proc_run_once
      // **** New prefix
    case DEF_NEW:
      proc = new(sched,proc,proc->def);
      break;
      // **** Let prefix
    case DEF_LET:
      proc = let(sched,proc,proc->def);
      break;
      // **** CALL SUFFIX
    case DEF_CALL:
    case DEF_JOKER: // special case, direct recursion
      proc = call(sched,proc,proc->def);
      break;
      /* **** POWN PREFIX **** */
    case DEF_POWN:
      proc = pown(sched,proc);
      break;
      /* **** OWN PREFIX **** */
    case DEF_OWN:
      proc = own(sched,proc);
      break;
    case DEF_PRIM:
      proc = interpret_primitive(sched,proc,proc->def,proc->def->call_id);
      break;
    case DEF_IF:
      proc = interpret_if(sched,proc,proc->def->children[0],proc->def->children[1],proc->def->children[2]);
      break;
    default:
      return proc;
    }
  }

  // CONDITION : do not go past this line
  assert(FALSE);

  return NULL;

}

/** Output prefix
 *  @param[in] sched : scheduler of process
 *  @param[in,out] proc : process performing output
 *  @param[in] chan_ref : lexical reference of channel in environment
 *  @param[in] value : definition of value to output
 *  @return updated process
 **/ 
Process * output(Scheduler *sched, Process *proc) {
  int chan_ref = 0;
  Channel * chan = NULL;
  Process * old = NULL;
#ifdef PROC_TRACE
  char trace[128];
  unsigned long pid = 0;
  unsigned long cid = 0;
#endif

  // PRECONDITION : definition is well formed
  assert(proc!=NULL && proc->def!=NULL);
  assert(proc->def->type==DEF_OUTPUT);
  assert(proc->def->nb_children==proc->def->arity+1); // value expressions and continuation

  chan_ref = proc->def->binder_ref;

  // PRECONDITION : channel reference exists
  assert(chan_ref>=0 && chan_ref<proc->env.nb_entries);

  old = proc;

  // first resolve channel reference
  Value chanval= proc->env.entries[chan_ref];
  if(!VALUE_IS_CHANP(chanval))
    runtime_error(proc->def,"Output on a non-channel reference (entry %d in environment)",chan_ref);
  
  chan = VALUE_AS_CHANP(chanval);

  // PRECONDITION : the channel is referenced somewhere
  assert(CHANP_GET_REFCOUNT(chan)>0);

  // output on UNIQUE reference for volatile channels should be reclaimed (GC heuristics)
  if(CHANP_REFCOUNT_ONE(chan) && CHANP_IS_VOLATILE(chan)) {
#ifdef PROC_TRACE
    pid = proc->id;
    cid = CHANP_GET_ID(chan);
#endif
    reclaim_process(sched,proc);
#ifdef PROC_TRACE
    snprintf(trace, 64, "Output on uniquely owned channel '%ld' ", cid);
    PTRACE(pid, trace);
#endif
    return NULL; // the process does not exist anymore
  } 
  
  if(CHANP_IS_FULL(chan)) { // If channel is full (cannot OUTPUT)

    if(proc->state==REACT_CHILD && CHANP_IS_REACT(chan))
      runtime_error(proc->def,"Process cannot block on output on reactive channel");
    // Channel is full
    proc->state = ACTIVE_WAIT_OUTPUT;
    proc->nb_wait++;
    return proc; // does not progress
  }

  /* here the channel is not full */

  // if was in active wait output, then reset
  proc->nb_wait = 0;
  
  // the channel is full from now on
  CHANP_SET_FULL(chan);

  if(proc->state==REACT_CHILD) { /* output from a reactive child */
    if(CHANP_IS_REACT(chan)) { // a reactive child outputs on a reactive channel
      // if already in a reactive child, communiate and replace the current child
      Process * owner = CHANP_GET_OWNER(chan);
      if(owner==proc) { // if ourself then error
	runtime_error(proc->def, "Reactive channel %ld owned by output process %ld", CHANP_GET_ID(chan),proc->id);
      }

      if(owner==NULL) 
	runtime_error(proc->def, "Reactive channel %ld with no owner", CHANP_GET_ID(chan));
      
      // here we have an owner

      // CONDITION : owner is another reactive child
      if(owner->state!=REACT_CHILD)
	runtime_error(proc->def, "Owner '%ld' of reactive channel '%ld' is not reactive",CHANP_GET_ID(chan),owner->id);
	
      /* communication may take place */
      
      /* if input process not ready, execute any internal step */
      owner = step(sched,owner);
      communicate(sched,proc,owner,chan);
      proc = step(sched,proc);
      	
      // CONDITION : parent must be in REACT mode
      assert(sched->active_procs[sched->last_proc]!=NULL && sched->active_procs[sched->last_proc]->state==REACT);
	
      sched->active_procs[sched->last_proc]->partner = owner; // the parent has a new reactive child
      owner->partner = proc; // the partner of the new child is the current child
      old = proc; // the current child becomes old
      proc = owner; // replace by new child

    } else { // if output on a non-reactive channel
      // give back control to parent 
      Process * parent = sched->active_procs[sched->last_proc];
      // eventually reclaim the output (reactive) process
      proc = parent; // don't touch at the parent

      proc->state = REACT_END; // parent will wait for input (reactive version
                               // of WAIT_OUTPUT_SYNC)
      proc->nb_wait = 0;
      CHANP_SET_OWNER(chan,proc); // owner of the channel
    }
 } else { // not a reactive child
    if(CHANP_IS_REACT(chan)) { // if channel is reactive
      // switch to reactive mode
      proc->partner = CHANP_GET_OWNER(chan); // the new reactive child is channel owner      
      proc->partner->partner = NULL; // the partner of the child is NULL (starting reaction)
      proc->state = REACT; // we switch to basic reactive mode

      proc->partner = step(sched,proc->partner);
      communicate(sched,proc,proc->partner,chan);
      proc = proc->partner; // we replace the parent by its new reactive child
    } else {
      // normal channel
      proc->state = WAIT_OUTPUT_SYNC; // On a normal channel we wait for synchronization
      proc->nb_wait = 0; // reinit the wait counter (used for garbage collection)
      CHANP_SET_OWNER(chan,proc); // and the current process is the owner of the channel
    }
  }

  return proc;
}

/********************************************************
 * LET
 ********************************************************/

/** Let prefix
 *  @param[in] sched : scheduler of process
 *  @param[in,out] proc : process performing let
 *  @param[in] let_def : let definition
 *  @return updated process
 **/ 
Process * let(Scheduler *sched, Process *proc, Definition *let_def) {
  // PRECONDITION : correct input expression
  assert(let_def->type==DEF_LET);
  assert(let_def->nb_children==2); // let value and continuation
  // PRECONDITION : binder reference is correct
  assert(let_def->binder_ref==-1 || (let_def->binder_ref>=0 && let_def->binder_ref<proc->env.nb_entries));
  
  Value val = interpret_value_expr(sched, proc, let_def->children[0]);
  // put in environment with reference in binder_ref
  if(VALUE_IS_BIND(val))
    val = proc->env.entries[VALUE_AS_BIND(val)];

  if(VALUE_IS_CHANP(val)) { // if it's a channel
    proc_register_channel(sched,proc,VALUE_AS_CHANP(val),let_def->binder_ref);
  } else {	// if it's a value
    if(VALUE_IS_STRINGREF(val)) { // increment the reference count for the string
      STRREF(sched->string_pool,val.val._int);
    } else if(VALUE_IS_TUPLE(val)) {
      Tuple * clone = tuple_clone(sched,VALUE_AS_TUPLE(val));
      VALUE_SET_TUPLE(val,clone);
    }
    
    // then put in environment
    proc_update_env(sched,proc,let_def->binder_ref,val);

  }
#ifdef PROC_TRACE
  char trace[128];
  char valuestr[64];
  snprintf(trace, 64, "Let on binding '%d' , value stored = ", let_def->binder_ref);
  print_value(sched->string_pool,valuestr,64,val);
  strncat(trace,valuestr,64);
  PTRACE(proc->id, trace);
#endif
  
  // Continue with the continuation
  // NOTE: number of children must be TWO for let prefixes
  proc->def = let_def->children[1];
  
  return proc;
}

/********************************************************
 * CALLS
 ********************************************************/

/** Switch environments for calls
 *  @param[in] sched : scheduler for calling process
 *  @param[in,out] proc : the calling process
 *  @param[in] children : the parameter definitions
 *  @param[in] nb_children : the number of parameters
 *  @param[in] env_size : the needed environment size for call
 **/ 
static __inline__
void call_switch_env(Scheduler *sched, Process *proc, Definition **children, int nb_children, int env_size) {
  int i;

  // PRECONDITION : call is correct
  assert(nb_children<=env_size);

  // allocate the new environment
  Value * newenv = (Value *) MEM_ALLOC_ARRAY(sizeof(Value),env_size,"Value","cubesched.c","call_switch_env");
  if(newenv==NULL) 
    fatal_error("cubesched.c","call_switch_env",__LINE__,"Cannot switch environment in call, no room for new environment");

  // then register the parameter entries
  for(i=0;i<nb_children;i++) {
    // PRECONDITION : children is correctly formed
    assert(children[i]!=NULL && ((children[i]->type==DEF_VALUE) ||
				 (children[i]->type==DEF_PRIM)));

    Value val = interpret_value_expr(sched,proc,children[i]);
    if(VALUE_IS_BIND(val)) { // call by reference
      int bind_ref = VALUE_AS_BIND(val);
      val = proc->env.entries[bind_ref];
    }
    
    // register the value (reference counting)
    register_value(sched,val);
    newenv[i]=val;

  }


  Value val = VALUE_CREATE_NONE();
  // Initialize the remaining entries in the new environment
  for(i=nb_children;i<env_size;i++) {
    newenv[i] = val;
  }

  // Early collection scheme, we collect the entries in previous environment
  // also put to None the potential remaining entries (potentially collecting)
  for(i=0;i<proc->env.nb_entries;i++) {
    proc_update_env(sched,proc,i,val);
  }
  
  // reclaim the memory from previous environment
  MEM_FREE_ARRAY(proc->env.entries,sizeof(Value),proc->env.nb_entries,"Value","cubesched.c","call_switch_env");

  // now the environment may be updated
  proc->env.nb_entries = env_size;
  proc->env.entries = newenv;
  
}

/** Perform a call (terminal position)
 *  @param[in] sched : the scheduler of the process performing the call
 *  @param[in,out] proc : the process performing the call
 *  @param[in] call : the call definition
 *  @return : the continuation process
 **/
Process * call(Scheduler *sched, Process *proc, Definition *call) {
  // contents of call is
  // call_id = target definition
  // bindings[0..nb_bindings-1] = call arguments

  // PRECONDITION : check that we call something
  assert(call->type==DEF_CALL || call->type==DEF_JOKER);
  assert(call->call_id > 0 && call->call_id < GLOBAL_nb_definitions);
  
  // need clever allocation
  DefEntry *target = GLOBAL_definitions[call->call_id];

  // PRECONDITION : Normal or Joker call is possible
  //printf("Call type = %d\n",call->type);
  //printf("Target env size = %d\n",target->def->env_size);
  //printf("Proc env size = %d\n",proc->def->env_size);
  assert(call->type==DEF_CALL || (call->type==DEF_JOKER));

  /* XXX: CHECK WHY THE FOLLOWING CONDITIONS DOES NOT OLD
     && 
				  (target->def->env_size <= proc->def->env_size)));
  */

  // substitute arguments, if any
  if(call->type==DEF_CALL) { // regular call
    call_switch_env(sched,proc,call->children,call->nb_children,target->def->env_size);
  } // if DEF_JOKER : do nothing with environment !
  
  // Note : DEF_HEAD has process body as first child 
  proc->def = target->def->children[0];
#ifdef PROC_TRACE
  char trace[256];
  if(call->type==DEF_CALL)
    snprintf(trace, 256,"Call to definiton '%s'", target->name);
  else
    snprintf(trace, 256,"Joker call to definiton '%s'", target->name);
  trace[255]=(char)0;
  PTRACE(proc->id, trace);
#endif
  return proc;
}

/********************************************************
 * REACTIVE PREFIXES
 ********************************************************/

/** Change dynamically the ownership of a reactive channel
 *  @param[in,out] sched : the scheduler of the process changing ownership
 *  @param[in,out] proc : the process performing the change
 *  @return : the continuation process
 **/
Process * pown(Scheduler *sched, Process *proc) {
  Value val = VALUE_CREATE_NONE();
  unsigned long proc_ref = 0;
#ifdef PROC_TRACE
  Process * last_owner = NULL;
#endif

  /* PRECONDITION : correct def */
  assert(proc->def != NULL && proc->def->type==DEF_POWN);

  /* PRECONDITION : the channel reference is ok */
  assert(proc->def->binder_ref>=0 && proc->def->binder_ref<proc->env.nb_entries);
  
  /* Get the channel to own */
  val = proc->env.entries[proc->def->binder_ref];
  
  /* test if the value is a channel */
  if(!VALUE_IS_CHANP(val))
    runtime_error(proc->def, "First argument *not* a channel reference in own prefix");

  /* check if the channel is reactive */
  Channel * chan = VALUE_AS_CHANP(val);
  if(!CHANP_IS_REACT(chan)) {
    if(proc->state!=REACT_CHILD)
      runtime_error(proc->def, "Cannot own an active channel : not a reactive process");
    else
      CHANP_SET_REACT(chan);    
  }

  /* Check if the destination location is correct */
  if(!VALUE_IS_LOC(proc->def->val))
    runtime_error(proc->def, "Second argument *not* a process location in own prefix");

  proc_ref = VALUE_AS_LOC(proc->def->val);

  if(proc_ref>=sched->nb_rprocs)
    runtime_error(proc->def, "Unknown reactive process location : %ld", proc_ref);

#ifdef PROC_TRACE
  last_owner = CHANP_GET_OWNER(chan);
#endif

  /* Change ownership */
  CHANP_SET_OWNER(chan,sched->react_procs[proc_ref]);

  /* Continuation is first child */
  proc->def = proc->def->children[0];
#ifdef PROC_TRACE
  char trace[256];
  if(last_owner!=NULL)
    snprintf(trace, 256,"Change channel '%ld' ownership : owned by proc '%ld' (was '%ld')", chan->id, sched->react_procs[proc_ref]->id, last_owner->id);
  else 
    snprintf(trace, 256,"Change channel '%ld' ownership : owned by proc '%ld' (was 'None')", chan->id, sched->react_procs[proc_ref]->id);
  trace[255]=(char)0;
  PTRACE(proc->id, trace);
#endif
  return proc;
}

/** Change dynamically the ownership of a reactive channel to self
 *  @param[in,out] sched : the scheduler of the process changing ownership
 *  @param[in,out] proc : the process performing the change
 *  @return : the continuation process
 **/
Process * own(Scheduler *sched, Process *proc) {
  Value val = VALUE_CREATE_NONE();
#ifdef PROC_TRACE
  Process * last_owner = NULL;
#endif

  /* PRECONDITION : correct def */
  assert(proc->def != NULL && proc->def->type==DEF_OWN);

  /* PRECONDITION : the channel reference is ok */
  assert(proc->def->binder_ref>=0 && proc->def->binder_ref<proc->env.nb_entries);
  
  /* Get the channel to own */
  val = proc->env.entries[proc->def->binder_ref];
  
  /* test if the value is a channel */
  if(!VALUE_IS_CHANP(val))
    runtime_error(proc->def, "First argument *not* a channel reference in own prefix");

  /* check if the channel is reactive */
  Channel * chan = VALUE_AS_CHANP(val);
  if(!CHANP_IS_REACT(chan)) {
    if(proc->state!=REACT_CHILD)
      runtime_error(proc->def, "Cannot own an active channel : not a reactive process");
    else
      CHANP_SET_REACT(chan);    
  }

#ifdef PROC_TRACE
  last_owner = CHANP_GET_OWNER(chan);
#endif

  /* Change ownership to self */
  CHANP_SET_OWNER(chan,proc);

  /* Continuation is first child */
  proc->def = proc->def->children[0];
#ifdef PROC_TRACE
  char trace[256];
  if(last_owner!=NULL)
    snprintf(trace, 256,"Change channel '%ld' ownership to self (was '%ld')", chan->id, last_owner->id);
  else
    snprintf(trace, 256,"Change channel '%ld' ownership to self (was 'None')", chan->id);
  trace[255]=(char)0;
  PTRACE(proc->id, trace);
#endif
  return proc;
}

/********************************************************
 * PROCESS MANAGEMENT
 ********************************************************/

/** Destroy process environment
 * @param[in] sched : the scheduler of the finishing process
 * @param[in,out] proc : the finishing process
 **/
static __inline__
void proc_destroy_env(Scheduler *sched, Process *proc) {
  int i=0;

  // simply update with NONE to trigger GC
  Value val = VALUE_CREATE_NONE();
  for(i=0;i<proc->env.nb_entries;i++) {
    proc_update_env(sched,proc,i,val);
  }

  // then get the memory back
  MEM_FREE_ARRAY(proc->env.entries,sizeof(Value),proc->env.nb_entries,"Value","cubesched.c","proc_destroy_env");
  proc->env.entries=NULL;
  proc->env.nb_entries=0;
}

/** Destroy a process
 *  @param[in,out] sched : the scheduler to update
 *  @param[in,out] proc : the process to reclaim
 **/
static
void reclaim_process(Scheduler *sched, Process * proc) {

#ifdef RUNTIME_STAT
  sched->nb_proc_ended++;
#endif
  
  if(proc->state!=REACT_CHILD) {
    
    // PRECONDITION : ensure we remove from the active queue
    
    assert(proc->state==ACTIVE || proc->state==ACTIVE_WAIT_INPUT || proc->state==ACTIVE_WAIT_OUTPUT || proc->state==ACTIVE_WAIT_CHOICE || proc->state==WAIT_OUTPUT_SYNC);

      // put the one at the end instead (if not yet at the end)
    if(proc->sched_ref<sched->nb_active-1) {
      sched->active_procs[proc->sched_ref] = sched->active_procs[sched->nb_active-1];
      sched->active_procs[proc->sched_ref]->sched_ref = proc->sched_ref;
      sched->last_proc--; // don't forget to execute the promoted process
    } 
    
    // decrement the process counter
    sched->nb_procs--;
    sched->nb_active--;
      
    // destroy the environment
    if(proc->env.nb_entries>0)
      proc_destroy_env(sched,proc);
    // now reclaim the process
    MEM_FREE_SINGLE(proc,sizeof(Process),"Process","cubesched.c","reclaim_process");
    proc = NULL;
      
  } else { // if this is a child process
    // remove from the reactive processes list
    if(proc->sched_ref<sched->nb_rprocs-1) {
      sched->react_procs[proc->sched_ref] = sched->react_procs[sched->nb_rprocs-1];
      sched->react_procs[proc->sched_ref]->sched_ref = proc->sched_ref;
    }
    sched->nb_rprocs--;

    // destroy the environment
    proc_destroy_env(sched,proc);
    // now reclaim the process
    MEM_FREE_SINGLE(proc,sizeof(Process),"Process","cubesched.c","reclaim_process");
    proc = NULL;
  }
}

/** Add newly created processes in scheduler
 *  @param[in,out] sched : scheduler to update
 *  @param[in] procs : the processes to add in scheduler
 *  @param[in] new_active_procs : the number of active processes to add
 *  @param[in] new_reactive_procs : the number of reactive processes to add
 **/
static 
void scheduler_add_procs(Scheduler *sched, Process ** procs, unsigned long nb_new_active_procs, unsigned long nb_new_reactive_procs) {
  unsigned long i;
    
  // need to regrow the number of active processes ?
  if(sched->nb_procs+nb_new_active_procs>sched->max_procs) {
    unsigned long old_max_procs = sched->max_procs;
    while(sched->nb_procs+nb_new_active_procs>sched->max_procs) {
      sched->max_procs += GLOBAL_CFG_AS_ULONG(CFG_GROW_PROCS_FACTOR);
    }

    sched->active_procs = (Process **) MEM_REALLOC_ARRAY(sched->active_procs,sizeof(Process *),old_max_procs,sched->max_procs,"Process*","cubesched.c","scheduler_add_procs");
    if(sched->active_procs==NULL) {
      fatal_error("cubeproc.c","scheduler_add_procs",__LINE__,"Cannot grow active process queue");
    }
    
    sched->wait_procs = (Process **) MEM_REALLOC_ARRAY(sched->wait_procs,sizeof(Process *),old_max_procs,sched->max_procs,"Process*","cubesched.c","scheduler_add_procs");
    if(sched->wait_procs==NULL) {
      fatal_error("cubeproc.c","scheduler_add_procs",__LINE__,"Cannot grow wait process queue");
    }

    /** XXX : Not needed, we will overwrite the entries just below
    // don't forget to nullify the new process entries
    for(i=sched->nb_procs;i<sched->max_procs;i++) {
      sched->active_procs[i] = NULL;
      sched->wait_procs[i] = NULL;
    }
    **/
  }

  // PRECONDITION : check that we have enough active/wait entries
  assert(sched->nb_procs+nb_new_active_procs<=sched->max_procs);

  // need to regrow the number of reactive processes ?
  if(sched->nb_rprocs+nb_new_reactive_procs>sched->max_rprocs) {
    unsigned long old_max_rprocs = sched->max_rprocs;
    while(sched->nb_rprocs+nb_new_reactive_procs>sched->max_rprocs) {
      sched->max_rprocs += GLOBAL_CFG_AS_ULONG(CFG_GROW_PROCS_FACTOR);
    }

    sched->react_procs = (Process **) MEM_REALLOC_ARRAY(sched->react_procs,sizeof(Process *),old_max_rprocs,sched->max_rprocs,"Process*","cubesched.c","scheduler_add_procs");
    if(sched->react_procs==NULL) {
      fatal_error("cubeproc.c","scheduler_add_procs",__LINE__,"Cannot grow reactive process list");
    }

    /** XXX : Not needed
    // don't forget to nullify the new process entries
    for(i=sched->nb_rprocs;i<sched->max_rprocs;i++) {
      sched->react_procs[i] = NULL;
    }
    **/
  }

  // PRECONDITION : check that we have enough reactive entries
  assert(sched->nb_rprocs+nb_new_reactive_procs<=sched->max_rprocs);

  // now add the new processes (Note: we can reuse i)
  // and activate them
  unsigned int old_nb_rprocs = sched->nb_rprocs;
  for(i=0;i<nb_new_active_procs+nb_new_reactive_procs;i++) {
    if(procs[i]->state!=REACT_CHILD) {
      // not a reactive process
      sched->active_procs[sched->nb_active] = procs[i];
      procs[i]->sched_ref = sched->nb_active;
      sched->nb_active++;
    } else {
      // reactive process
      sched->react_procs[sched->nb_rprocs] = procs[i];
      procs[i]->sched_ref = sched->nb_rprocs;
      sched->nb_rprocs++;
    }
  }

  sched->nb_procs += nb_new_active_procs; // active or wait processes (not reactive)
}

/** Add a single process in scheduler
 *  @param[in,out] sched : scheduler to update
 *  @param[in] proc : the process to add in scheduler
 **/
void scheduler_add_proc(Scheduler *sched, Process * newproc) {
  Process * newprocs[1];
  newprocs[0] = newproc;
  if(newproc->def->type!=DEF_REACT)
    scheduler_add_procs(sched, (Process **) newprocs , 1, 0);
  else
    scheduler_add_procs(sched, (Process **) newprocs, 0 , 1);
}

/** Initialize a reactive process
 *  @param[in,out] proc : the reactive process to initialize
 *  @param[in] def : the process body
 **/
static
Process * spawn_react_init(Process * proc, Definition *def) {

  // PRECONDITION : definition is correct reactive definition
  assert(def->type==DEF_REACT);
  assert(def->nb_children>=1); // at least a continuation

  int i;
  Value val = VALUE_CREATE_NONE();
  int index;
  if(def->nb_children>1) { // if more than just a continuation 
    for(i=0;i<def->nb_children-1;i++) {
      val = def->children[i]->val;
      if(!VALUE_IS_BIND(val))
	runtime_error(def->children[i],"Reactive channel reference expected");
      index = VALUE_AS_BIND(val);

      // PRECONDITION : correct index
      assert(index>=0 && index<proc->env.nb_entries);

      val = proc->env.entries[index];
      if(!VALUE_IS_CHANP(val))
	runtime_error(def->children[i],"Reference is not a reactive channel");

      // if all goes well, select the channel as a reactive one
      Channel * chan = VALUE_AS_CHANP(val);
      CHANP_SET_REACT(chan);
      // and put the owner field to the reactive process
      CHANP_SET_OWNER(chan,proc);
    }
  }

  // update the definition of process (last child)
  proc->def = def->children[def->nb_children-1];
  
  return proc;
}

/** Inherit process environment
 *  @param[in,out] sched : the scheduler to update (e.g. string refcount in pool)
 *  @param[in] parent : the parent process
 *  @param[in,out] child : the child process
 *  @return the updated child process
 **/
static inline
Process * proc_inherit_env(Scheduler * sched, Process * parent, Process *child) {
  
  // create enough room for new environment (max of parent and self continuation size)
  int env_size = child->def->env_size;
  int parent_nb_entries = parent->env.nb_entries;
  int nb_entries = (env_size>parent_nb_entries)?env_size:parent_nb_entries;
  child->env.entries = (Value *) MEM_ALLOC_ARRAY(sizeof(Value),nb_entries,"Value","cubesched.c","proc_inherit_env");
  if(child->env.entries==NULL)
    fatal_error("cubesched.c","proc_inherit_env",__LINE__,"Cannot allocate memory for inheriting environment");
  child->env.nb_entries = nb_entries;
  int i;

  // Copy from parent (and update GC)
  for(i=0;i<parent_nb_entries;i++) {
    Value val = parent->env.entries[i];
    child->env.entries[i] = val;
    register_value(sched,val);
  }

  // Create supplementary entries, if needed
  for(i=parent_nb_entries;i<nb_entries;i++) {
    Value val = VALUE_CREATE_NONE();
    child->env.entries[i] = val;
  }

  return child;
}

/** Create empty process environment
 *  @param[in,out] proc : the process whose environment must be created
 *  @return the process with created environment
 **/
static inline
Process * proc_create_empty_env(Process * proc) {
  int i;
  int nb_entries = proc->def->env_size;
  proc->env.nb_entries = nb_entries;
  if(proc->env.nb_entries>0) {
    proc->env.entries = (Value *) MEM_ALLOC_ARRAY(sizeof(Value),nb_entries,"Value","cubesched.c","proc_create_empty_env");
    if(proc->env.entries==NULL)
      fatal_error("cubesched.c","proc_create_empty_env",__LINE__,"Cannot allocate memory for new environment");

    for(i=0;i<nb_entries;i++) {
      Value val = VALUE_CREATE_NONE();
      proc->env.entries[i] = val;
    }
  } else 
    proc->env.entries=NULL;

  return proc;
}

 
/** Spawn a new process
 *  @param[in,out] sched : the scheduler to update
 *  @param[in,out] parent : the parent process, creating the new process
 *  @param[in] def : the creation definition in parent
 *  @return the newly created process
 **/
static
Process * spawn(Scheduler *sched, Process *parent, Definition * def) {

  // allocate the new process
  Process * proc = (Process *) MEM_ALLOC_SINGLE(sizeof(Process),"Process","cubesched.c","spawn");
  if(proc==NULL)
    fatal_error("cubesched.c","spawn",__LINE__,"Cannot spawn, fail to allocate process");
  proc->def = def;
  proc->last_choice = -1;
  if(def->type!=DEF_REACT)
    proc->state = ACTIVE;
  else
    proc->state = REACT_CHILD;
  proc->nb_wait = 0;
  proc->sched_ref = 0;  // will be set by scheduler_add_proc(s)
  proc->id = sched->process_counter;
  (sched->process_counter)++;
  proc->partner = NULL;

  if(parent!=NULL) { // inherit from parent
    proc = proc_inherit_env(sched,parent,proc);    
  } else { // no entry to inherit
    proc = proc_create_empty_env(proc); 
  }

  if(def->type==DEF_REACT) {
    proc = spawn_react_init(proc,def);
#ifdef PROC_TRACE
    char trace[128];
    snprintf(trace, 128,"Spawning reactive process '%ld'", proc->id);
    trace[127]=(char)0;
    if(parent!=NULL)
      PTRACE(parent->id, trace);
    else
      PTRACE(proc->id, "New root process");
#endif
    
  } else {
#ifdef PROC_TRACE
    char trace[128];
    snprintf(trace, 128,"Spawning process '%ld'", proc->id);
    trace[127]=(char)0;
    if(parent!=NULL)
      PTRACE(parent->id, trace);
    else
      PTRACE(proc->id, "New root process");
#endif
    
#ifdef RUNTIME_STAT
    sched->nb_proc_spawned++;
#endif
  }  

  return proc;
}

/** Spawn a root process
 *  @param[in,out] sched : the scheduler to update
 *  @param[in] def : the definition for the root process
 *  @return the newly created root process
 **/
Process *spawn_root(Scheduler *sched, Definition *def) {
  Process * proc = spawn(sched,NULL,def);
  return proc;
}

/** Parallel operator
 *  @param[in,out] sched : the scheduler to update
 *  @param[in,out] parent : the process performing the parallel
 *  @param[in] children : the children definitions
 *  @param[in] nb_children : the number of children
 *  @return : the continuation for the parent process
 **/
static 
Process * par(Scheduler *sched, Process *parent, Definition **children, unsigned int nb_children) {
  
  // PRECONDITION : there are more than one child
  assert(nb_children>=2);

  unsigned int i;

  Process ** parprocs = NULL;
  unsigned long nb_new_active = 0;
  unsigned long nb_new_reactive = 0;

  // allocate return processes
  parprocs = (Process **) MEM_ALLOC_ARRAY(sizeof(Process *),nb_children-1,"Process*","cubesched.c","par");
  if(parprocs==NULL)
    fatal_error("cubesched.c","par",__LINE__,"Cannot allocate parallel processes, not enough memory");

  // first process is parent
  parent->def = children[0];
  
  // create one process per child
  for(i=1;i<nb_children;i++) {
    parprocs[i-1] = spawn(sched,parent,children[i]);
    if(parprocs[i-1]->state!=REACT_CHILD) 
      nb_new_active++;
    else
      nb_new_reactive++;
  }

  // add the new processes in the scheduler
  scheduler_add_procs(sched,parprocs,nb_new_active,nb_new_reactive);
  MEM_FREE_ARRAY(parprocs,sizeof(Process *),nb_children-1,"Process*","cubesched.c","par");
  parprocs = NULL; // just to check
  
  return parent;
}

/********************************************************
 * SCHEDULING
 ********************************************************/

/** Test if may execute input prefix
 *  @param[in] proc : the current process
 *  @param[in] def : the input prefix
 *  @return TRUE is may execute, FALSE either
 **/
static inline
Bool input_may_execute(Process *proc, Definition *def) {

  // PRECONDITION : correct input prefix
  assert(def->type==DEF_INPUT);
  // PRECONDITION : correct channel reference
  assert((def->arity>0 && VALUE_IS_BIND(def->val)
                       && (VALUE_AS_BIND(def->val)>=0)
                       && (VALUE_AS_BIND(def->val)<proc->env.nb_entries))
         || (def->arity==0));
       
  // Get the channel to input on
  Value val = proc->env.entries[VALUE_AS_BIND(def->val)];
  Channel * chan;

  if(VALUE_IS_CHANP(val)) {	  
    chan = VALUE_AS_CHANP(val);

    // PRECONDITION : NON-NULL CHANNEL
    assert(chan!=NULL);
    
    if(CHANP_IS_FULL(chan) // if full channel
       || CHANP_GET_REFCOUNT(chan)<=1) { // or GCable, may execute
      return TRUE;
    } else {
      return FALSE;
    }
  } else if(VALUE_IS_NONE(val)) { // XXX FixMe : is this necessary ?
    runtime_warning(def,"Input on NONE value (indefinitely blocking)");
    return FALSE;
  } else 
    runtime_error(def,"Cannot input on non-channel value (or NONE)");
  
  // XXX : dead code
  return FALSE;
}

/** Test if may execute output prefix
 *  @param[in] proc : the current process
 *  @param[in] def : the output prefix
 *  @return TRUE is may execute, FALSE either
 **/
static inline
Bool output_may_execute(Process *proc, Definition *def) {

  // PRECONDITION : correct definition
  assert(def->type==DEF_OUTPUT);

  Value val = VALUE_CREATE_NONE();
  Channel * chan;

  if(proc->state==WAIT_OUTPUT_SYNC) { // output was effected (without sync)
	  // never go in the wait queue because the output is half-performed
	  // since the channel as awoken the readers, this should unlock quickly
	  // however, we cannot continue with such process
    return FALSE;
  } else { // If not in WAIT_OUTPUT_SYNC mode

    // PRECONDITION : channel reference is ok
    assert(def->binder_ref>=0 && def->binder_ref<proc->env.nb_entries);

    val = proc->env.entries[def->binder_ref];
    if(!VALUE_IS_CHANP(val))
      runtime_error(def,"Cannot output on non-channel value");
    chan = VALUE_AS_CHANP(val);
    // PRECONDITION : channel is not NULL
    assert(chan!=NULL);

    if(CHANP_IS_EMPTY(chan) // if the channel is empty
       || CHANP_GET_REFCOUNT(chan)<=1)  { // or may GC, then may execute
      return TRUE;
    } else {
      return FALSE;
    }
  }
  
  // XXX : dead code
  return FALSE;
}

/** Test if may execute choice branch
 *  @param[in] proc : the current process
 *  @param[in] def : the output prefix
 *  @return TRUE is may execute, FALSE either
 **/
static inline
Bool choice_may_execute(Process *proc, Definition *def) {

  // PRECONDITION : there could be a nested choice here,
  //                but these should be removed from the syntax
  //                so we assert this
  assert(def->type!=DEF_CHOICE);

  switch(def->type) {
  case DEF_INPUT:
    return input_may_execute(proc,def);

  case DEF_OUTPUT:
    return output_may_execute(proc,def);


 default:
      return TRUE; // anything else may be executed
  }
}

/** Select a choice branch to execute
 *  @param[in] sched : scheduler of process
 *  @param[in,out] parent : the process performing the choice
 *  @param[in] children : choice branches
 *  @param[in] nb_children : number of branches
 *  @param[in] last_choice : the last chosen branch (equitable choice)
 *  @return the process with continuation branch
 **/
static inline
Process * choice(Scheduler *sched, Process *parent, Definition **children, int nb_children, int last_choice) {

  // PRECONDITION : correct choice construct
  assert(parent->def->type==DEF_CHOICE);
  assert(parent->def->nb_children == nb_children);
  assert(parent->def->children == children);
  assert(nb_children>=2); // at least two branches

  int i;

  if(last_choice<0 || last_choice>=nb_children)
    last_choice = -1;

  last_choice++;
  for(i=last_choice; i<nb_children;i++) {
    Definition *child = children[i];
    if(choice_may_execute(parent,child)) {
      parent->def = child;
      parent->last_choice = i;
#ifdef PROC_TRACE
      char trace[128];
      snprintf(trace, 128,"Choice of '%d'th branch", i);
      trace[127]=(char)0;
      PTRACE(parent->id, trace);
#endif
      return parent;
    }
  }
  
  // if we are here, then maybe we can execute before the last choice
  // Note : we can reuse counter 'i'
  for(i=0;i<last_choice;i++) {
    Definition *child = children[i];
    if(choice_may_execute(parent,child)) {
      parent->def = child;
      parent->last_choice = i;
#ifdef PROC_TRACE
      char trace[128];
      snprintf(trace, 128,"[Wrapped] Choice of '%d'th branch", i);
      trace[127]=(char)0;
      PTRACE(parent->id, trace);
#endif
      return parent;
    } // or go to the next one
  }
  
  // we cannot execute anything, then return NULL
#ifdef PROC_TRACE
      char trace[128];
      snprintf(trace, 128,"Cannot choose");
      trace[127]=(char)0;
      PTRACE(parent->id, trace);
#endif
      return NULL;
}

/** Run a process until fuel is over
 *  @param[in,out] sched : the scheduler of the process  (fuel in sched->fuel)
 *  @param[in,out] proc : the running process
 *  @param[in] def : the definition for the process
 *  @return the number of executions (<=fuel)
 **/
int proc_run_once(Scheduler *sched, Process *proc, Definition *def) {

  Bool advanced = FALSE; // flag for process advancement in input

  int turn = 0;  // turn counter
  Process * next = NULL;  // the next process to execute (esp. reactive child)
  Process * parent = NULL; // the parent process

  while(sched->fuel>0) { // we repeat until fuel has been consumed

    // CONDITION : process is active
    //fprintf(stderr, "proc->state==%d\n",proc->state);
    //fprintf(stderr, "sched->turn==%ld\n",sched->turn);
    //assert(proc->state==ACTIVE);

    /* initialization for reaction chain */
    if(proc->state==REACT) { // if in reactive mode
      parent = proc;  // the parent is the running process in scheduler
      proc = proc->partner; // the truely running process is the reactive child
      proc->partner = NULL; // starting a reaction
      def = proc->def;  // the definition must be replaced
    }
    
    /* dispatch depending on definition type */
    switch(def->type) {
      // **** End of process
    case DEF_END:
#ifdef PROC_TRACE
      PTRACE(proc->id,"End of process");
#endif
      turn++;
      reclaim_process(sched,proc);
      if(parent!=NULL && parent->state==REACT) { // at the end of the reaction, go back to parent
	sched->fuel--;
	proc = parent;
	proc->state = ACTIVE;
	proc->partner = NULL;
	parent = NULL;
	def = proc->def;
	next = proc;
      } else { // if not a reactive process
	sched->fuel=0; // force no fuel
	continue; // exit directly
      }
      break;
      // **** Channel creation
    case DEF_NEW:
      next = new(sched,proc,def);
      next->nb_wait = 0;
      sched->fuel--;
      turn++;
      break;
      // **** Input prefix
    case DEF_INPUT:
      next = input(sched,proc,def,def->val,&advanced);
      if(!advanced) { // input not performed
	if(parent!=NULL && parent->state==REACT) {
	  // if reactive, go back to the (active) parent
	  proc = parent;
	  proc->state = ACTIVE;
	  proc->partner = NULL;
	  parent = NULL;
	  def = proc->def;
	  next = proc;
	} else { // for active processes
	  if(next!=NULL) { // if the process still exists (may be GC on input)
	    next->state = ACTIVE_WAIT_INPUT;
	    next->nb_wait++;
	  } else { // we reclaimed the process
	    turn++;
	  }
	  sched->fuel=0;
	  continue; // exit directly
	}
      } else { // ok, the input has been performed
	next->nb_wait = 0;
	sched->fuel--;
	turn++;
      }
      break;
      // **** Output prefix
    case DEF_OUTPUT:
      next = output(sched,proc);
      if(next==NULL) { // we reclaimed the process
	sched->fuel=0;
	turn++;
	continue; //  exit directly
      }
      // ouput is either ok or blocking
      switch(proc->state) {
      case WAIT_OUTPUT_SYNC:
	turn++;
      case ACTIVE_WAIT_OUTPUT:
	proc->nb_wait++;
	sched->fuel=0;
	continue;
	break;
      case REACT_CHILD:
	if(next->state==REACT_END) {
	  sched->fuel=0;
	  continue;
	}       
	sched->fuel--;
	turn++;
	break;
      case REACT:
	parent = proc;
      default:
	next->nb_wait=0;
	sched->fuel--;
	turn++;
      }
      break;
      // **** Let prefix
    case DEF_LET:
      next = let(sched,proc,def);
      next->nb_wait = 0;
      sched->fuel--;
      turn++;
      break;
      // **** CALL SUFFIX
    case DEF_CALL:
    case DEF_JOKER: // special case, direct recursion
      next = call(sched,proc,def);
      next->nb_wait = 0;
      sched->fuel--;
      turn++;
      break;
      /* **** POWN PREFIX **** */
    case DEF_POWN:
      next = pown(sched,proc);
      next->nb_wait = 0;
      sched->fuel--;
      turn++;
      break;
      /* **** OWN PREFIX **** */
    case DEF_OWN:
      next = own(sched,proc);
      next->nb_wait = 0;
      sched->fuel--;
      turn++;
      break;
      // **** PARALLEL OPERATOR
    case DEF_PAR:
      next = par(sched,proc,def->children,def->nb_children);
      next->nb_wait = 0;
      sched->fuel--;
      turn++;
      break;
      // **** PRIMITIVE PREFIX
    case DEF_PRIM:
      next = interpret_primitive(sched,proc,def,def->call_id);
      next->nb_wait = 0;
      sched->fuel--;
      turn++;
      break;
    case DEF_IF:
      next = interpret_if(sched,proc,def->children[0],def->children[1],def->children[2]);
      next->nb_wait = 0;
      turn++;
      sched->fuel--;
      break;
    case DEF_CHOICE:
      next = choice(sched,proc,def->children,def->nb_children,proc->last_choice);
      if(next==NULL) { // cannot realize the choice
	if(parent!=NULL && parent->state==REACT) {
	  // if reactive, go back to the (active) parent
	  proc = parent;
	  proc->state = ACTIVE;
	  proc->partner = NULL;
	  parent = NULL;
	  def = proc->def;
	  next = proc;
	  continue;
	} else { // for active processes
	  proc->state = ACTIVE_WAIT_CHOICE;
	  proc->nb_wait++;
	  // exit directly
	  sched->fuel=0;
	  continue;
	}
      } else {
	sched->fuel--;
	turn++;
	next->nb_wait=0;
      }
      break;
  default:
    fatal_error("cubesched.c","proc_run_once",__LINE__,"Cannot run once : instruction type '%d' not supported (please report, scheduler turn=%ld)", def->type,sched->turn);
    sched->fuel=0;
    }

    // if here, then next has next process to run
    proc = next;
    def = next->def;
    
  }

  return turn;
}

/** Activate a waiting process
 *  @param[in,out] sched : the scheduler of the process to activate
 *  @param[in,out] proc : the process to awake
 **/
static inline
void sched_activate_proc(Scheduler *sched, Process *proc) {

  // PRECONDITION : correct process reference
  assert((proc->sched_ref>=0) && (proc->sched_ref < sched->nb_wait));
  // PRECONDITION : process is in some wait mode
  assert((proc->state==WAIT_INPUT) || (proc->state==WAIT_OUTPUT) ||
	 (proc->state==WAIT_CHOICE));

  // remove the waiting process, replace by the last one
  if(proc->sched_ref<sched->nb_wait-1) {
    sched->wait_procs[proc->sched_ref] = sched->wait_procs[sched->nb_wait-1];
    sched->wait_procs[proc->sched_ref]->sched_ref = proc->sched_ref;
  }
  sched->nb_wait--;
  // put in the active queue
  sched->active_procs[sched->nb_active] = proc;
  proc->sched_ref = sched->nb_active;
  proc->state = ACTIVE;
  proc->nb_wait = 0;
  sched->nb_active++;
}

/** Put a process in wait for input mode, possibly putting it in wait queue
 *  @param[in,out] sched : the scheduler of the process to activate
 *  @param[in,out] proc : the process to awake
 *  @return TRUE if put in wait queue, FALSE if remain in active queue
 **/
static inline
Bool sched_wait_proc_input(Scheduler *sched, Process *proc) {

  // special case when we try to awake a waiting process
  if(proc->state==WAIT_INPUT)
    return FALSE;  // simulate the process stays active

  // PRECONDITION: process is active or already waits for input in active queue
  assert(proc->state==ACTIVE || proc->state==ACTIVE_WAIT_INPUT);

  proc->state = ACTIVE_WAIT_INPUT;
  proc->nb_wait++;
  // if we waited too much */
  if(proc->nb_wait>=GLOBAL_CFG_AS_INT(CFG_PASSIVATE_INPUT_TRESHOLD)) {
    // try to reclaim the process
    
    Value val = proc->def->val;
    if(VALUE_IS_BIND(val))
      val = proc->env.entries[VALUE_AS_BIND(val)];
    if(!VALUE_IS_CHANP(val))
      runtime_error(proc->def,"Cannot input on non-channel value");
    Channel * chan = VALUE_AS_CHANP(val);
    // PRECONDITION : channel is not NULL
      assert(chan!=NULL);
      proc = proc_test_reclaim(sched,proc,chan);
      if(proc==NULL)
	return TRUE; // no more in the active queue
      // standard case    // remove from the active queue
      if(proc->sched_ref<sched->nb_active-1) {
	sched->active_procs[proc->sched_ref] = sched->active_procs[sched->nb_active-1];
	sched->active_procs[proc->sched_ref]->sched_ref = proc->sched_ref;
      }
      sched->nb_active--;
      // put in the wait queue
      sched->wait_procs[sched->nb_wait] = proc;
      proc->sched_ref = sched->nb_wait;
      proc->nb_wait=0;
      proc->state=WAIT_INPUT;
      sched->nb_wait++;
      return TRUE; // in the wait queue
  } else {
    return FALSE; // remains active
  }
}

/** Put a process in wait for output mode, possibly putting it in wait queue
 *  @param[in,out] sched : the scheduler of the process to activate
 *  @param[in,out] proc : the process to awake
 *  @return TRUE if put in wait queue, FALSE if remain in active queue
 **/
static inline
Bool sched_wait_proc_output(Scheduler *sched, Process *proc) {

  // special case when we try to awake a waiting process
  if(proc->state==WAIT_OUTPUT)
    return FALSE;  // simulate the process stays active

  // Does not count as a wait if WAIT_OUTPUT_SYNC or already in WAIT_OUTPUT
  if(proc->state==WAIT_OUTPUT_SYNC) {
    // XXX TODO: here, handle the case when we did not read the value
    //           for some time, maybe we should try to reclaim
    //           the process
    proc->nb_wait++;
    if(proc->nb_wait>=GLOBAL_CFG_AS_INT(CFG_PASSIVATE_SYNC_TRESHOLD)) {
      // try to reclaim the process
      // PRECONDITION : channel reference is ok
      assert(proc->def->binder_ref>=0 && proc->def->binder_ref<proc->env.nb_entries);

      Value val = proc->env.entries[proc->def->binder_ref];
      if(!VALUE_IS_CHANP(val))
	runtime_error(proc->def,"Cannot output on non-channel value");
      Channel * chan = VALUE_AS_CHANP(val);
      // PRECONDITION : channel is not NULL
      assert(chan!=NULL);
      proc = proc_test_reclaim(sched,proc,chan);
      if(proc==NULL)
	return TRUE; // no more in the active queue
    }
    // standard case
    return FALSE;
  }

  // PRECONDITION: process is active or already waits for output in active queue
  assert(proc->state==ACTIVE || proc->state==ACTIVE_WAIT_OUTPUT);

  proc->state = ACTIVE_WAIT_OUTPUT;
  proc->nb_wait++;
  // if we waited too much */
  if(proc->nb_wait>=GLOBAL_CFG_AS_INT(CFG_PASSIVATE_OUTPUT_TRESHOLD)) {
    // try to reclaim the process
    // PRECONDITION : channel reference is ok
    assert(proc->def->binder_ref>=0 && proc->def->binder_ref<proc->env.nb_entries);

    Value val = proc->env.entries[proc->def->binder_ref];
    if(!VALUE_IS_CHANP(val))
      runtime_error(proc->def,"Cannot output on non-channel value");
    Channel * chan = VALUE_AS_CHANP(val);
    // PRECONDITION : channel is not NULL
    assert(chan!=NULL);
    proc = proc_test_reclaim(sched,proc,chan);
    if(proc==NULL)
      return TRUE; // no more in the active queue

    // here, we did not reclaim the process

    // remove from the active queue
    if(proc->sched_ref<sched->nb_active-1) {
      sched->active_procs[proc->sched_ref] = sched->active_procs[sched->nb_active-1];
      sched->active_procs[proc->sched_ref]->sched_ref = proc->sched_ref;
    }
    sched->nb_active--;
    // put in the wait queue
    sched->wait_procs[sched->nb_wait] = proc;
    proc->sched_ref = sched->nb_wait;
    proc->nb_wait=0;
    proc->state=WAIT_OUTPUT;
    sched->nb_wait++;
    return TRUE; // in the wait queue
  } else {
    return FALSE; // remains active
  }
}

/** Put a process in wait for choice mode, possibly putting it in wait queue
 *  @param[in,out] sched : the scheduler of the process to activate
 *  @param[in,out] proc : the process to awake
 *  @return TRUE if put in wait queue, FALSE if remain in active queue
 **/
static inline
Bool sched_wait_proc_choice(Scheduler *sched, Process *proc) {

  // special case when we try to awake a waiting process
  if(proc->state==WAIT_CHOICE)
    return FALSE;  // simulate the process stays active

  // PRECONDITION: process is active or already waits for choice in active queue
  assert(proc->state==ACTIVE || proc->state==ACTIVE_WAIT_CHOICE);

  proc->state = ACTIVE_WAIT_CHOICE;
  proc->nb_wait++;
  // if we waited too much */
  if(proc->nb_wait>=GLOBAL_CFG_AS_INT(CFG_PASSIVATE_CHOICE_TRESHOLD)) {
    // remove from the active queue
    if(proc->sched_ref<sched->nb_active-1) {
      sched->active_procs[proc->sched_ref] = sched->active_procs[sched->nb_active-1];
      sched->active_procs[proc->sched_ref]->sched_ref = proc->sched_ref;
    }
    sched->nb_active--;
    // put in the wait queue
    sched->wait_procs[sched->nb_wait] = proc;
    proc->sched_ref = sched->nb_wait;
    proc->nb_wait=0;
    proc->state = WAIT_CHOICE;
    sched->nb_wait++;
    return TRUE; // in the wait queue
  } else {
    return FALSE; // remains active
  }
}

#define MAY_NOT_EXECUTE_ACTIVE -2
#define MAY_NOT_EXECUTE_PASSIVE -1
#define MAY_EXECUTE 0

/** May a process execute ?
 *  @param[in] sched : the scheduler running the process
 *  @param[in] proc : the process that may execute
 *  @param[in] def : the definition to execute in the process
 *  @return MAY_NOT_EXECUTE_ACTIVE (-2) if the process may not execute but remain actives
 *  @return MAY_NOT_EXECUTE_PASSIVE (-1) if the process may not execute and has been put in the wait queue
 *  @return MAY_EXECUTE (0) if the process may execute
 **/
static inline
int proc_may_execute(Scheduler *sched, Process * proc, Definition * def) {
  Bool flag;
  Process *next;

  if(proc->state==REACT)
    return MAY_EXECUTE; // a reactive process never blocks until the reaction ends

  switch(def->type) {
  case DEF_INPUT:
    if(input_may_execute(proc,def)==TRUE)
      return MAY_EXECUTE;
    else {
      flag = sched_wait_proc_input(sched,proc);
      if(flag) 
	return MAY_NOT_EXECUTE_PASSIVE; // has been removed from active queue 
      else
	return MAY_NOT_EXECUTE_ACTIVE; // stays active
    }
    break;
  case DEF_OUTPUT:
    if(output_may_execute(proc,def)==TRUE) {
      return MAY_EXECUTE;
    } else {
	  flag = sched_wait_proc_output(sched,proc);
	  if(flag) 
	    return MAY_NOT_EXECUTE_PASSIVE; // has been removed from active queue
	  else
	    return MAY_NOT_EXECUTE_ACTIVE; // stays active
	}
    break;
  case DEF_CHOICE:
    next = choice(sched,proc,def->children,def->nb_children,proc->last_choice);
    if(next!=NULL) { // ok, may execute
      return MAY_EXECUTE;
    } else { // cannot execute (which==-1)
      flag = sched_wait_proc_choice(sched,proc);
      if(flag) 
	return MAY_NOT_EXECUTE_PASSIVE; // has been removed from active queue
      else
	return MAY_NOT_EXECUTE_ACTIVE; // stays active
	}
    break;
  default: // in any other case, we may execute
    return MAY_EXECUTE;
  }

  // Dead code
}

/** The scheduler algorithm
 * @param[in,out] sched : the scheduler structure
 **/
void schedule_run(Scheduler *sched) {

  Process *proc;
  Process *next;
  Definition *def;
  unsigned long starti, i;
  Bool inner_exit;
  int decide;
  Bool tried_awake;

  do {

    starti = sched->last_proc+1;
    if(starti>=sched->nb_active)
      starti=0;

    tried_awake=FALSE;

    next = NULL;
    inner_exit=FALSE;
    for(i=starti;(i<sched->nb_active) && (inner_exit==FALSE);i++) {
      proc = sched->active_procs[i];
      def = proc->def;

      decide = proc_may_execute(sched,proc,def);
      switch(decide) {
      case MAY_EXECUTE:
	sched->last_proc = i;
	proc->nb_wait=0;
	next = proc;
	inner_exit=TRUE;
	break;
      case MAY_NOT_EXECUTE_PASSIVE:
	i--; // next try is promotted process
      case MAY_NOT_EXECUTE_ACTIVE:
	break;
      default: 
	runtime_error(def,"Wrong decision %d in schedule_run (please report)\n",decide);
      }
    }

    // Here, next is the next process to run, or NULL    
    if(next!=NULL || starti==0) // if already found executable process or if we cannot cycle
      goto SKIP_CYCLE_LOOP; // skip the cycle loop
    
    // now the cycle loop (from first process to the last previous start)
    inner_exit=FALSE;
    for(i=0;(i<starti) && (inner_exit==FALSE);i++) {
      proc = sched->active_procs[i];
      def = proc->def;

      decide = proc_may_execute(sched,proc,def);
      switch(decide) {
      case MAY_EXECUTE:
	sched->last_proc = i;
	proc->nb_wait=0;
	next = proc;
	inner_exit=TRUE;
	break;
      case MAY_NOT_EXECUTE_PASSIVE:
	i--; // next try is promotted process
      case MAY_NOT_EXECUTE_ACTIVE:
	break;
      default:
	runtime_error(def,"Wrong decision %d in schedule_run - cyle loop (please report)\n",decide);	
      }
    }
    
  SKIP_CYCLE_LOOP: 

    if(next!=NULL) { // execute the process

      sched->fuel = GLOBAL_CFG_AS_INT(CFG_REFERENCE_FUEL); // charge the process fuel
      int advance = proc_run_once(sched,next,next->def);
      if(advance==0) {
	warning("cubesched.c","schedule_run",__LINE__,"Executable process did not advance at scheduler turn %ld (please report)",sched->turn);
      }
      sched->turn++;
      /* For debugging purpose :
         1) find the scheduler turn of the problem
         2) go back a few iteration before
         3) uncomment this for the given turn
         4) launch the debugger
         5) good luck ! */
      /*if(sched->turn==97366) {
	printf("I want to debug from now\n");
	}*/

    } else { // here we did not find any runnable process, try to awake the waiting thread
      int nb_awake = 0;
      sched->last_proc = sched->nb_active; // first awaken process or out of bounds
      for(i=0;i<sched->nb_wait;i++) {
	proc = sched->wait_procs[i];
	def = proc->def;
	proc->nb_wait=0;
	decide = proc_may_execute(sched,proc,def);
	if(decide==MAY_EXECUTE) {
	  sched_activate_proc(sched,proc);
	  nb_awake++;
	  i--; // don't forget the "promoted" process
	}
      }
      tried_awake = TRUE;

      /* now if no one has been awaken */
      /* check if all active process are active outputs */
      /* if it is the case, remove them all */
      if(nb_awake==0 && sched->nb_active>0) {
	int nb_outsync = 0;
	for(i=0;i<sched->nb_active;i++) {
	  proc = sched->active_procs[i];
	  if(proc->state==WAIT_OUTPUT_SYNC)
	    nb_outsync++;
	}
	if(nb_outsync==sched->nb_active) {
	  for(i=0;i<sched->nb_active;i++) {
	    proc = sched->active_procs[i];
	    runtime_warning(proc->def, "Deadlocked output");
#ifdef PROC_TRACE
	    {
	      char trace[128];
	      snprintf(trace, 128,"Process '%ld' is dead output sync, reclaim", proc->id);
	      PTRACE(proc->id, trace);
	    }
#endif
	    proc->state = ACTIVE;  /* activate to destroy ;-) */
	    reclaim_process(sched,proc);
	  }
	}
      }
    }
  } while(tried_awake==FALSE || sched->nb_active>0);
}

/** Create a scheduler structure
 *  @param[in] sp : a string pool for the scheduler 
 *  @return the newly allocated scheduler
 **/
Scheduler * scheduler_create(StringPool * sp, DefEntry** defs, int defs_size, int nb_defs) {
  Scheduler * sched = (Scheduler *) MEM_ALLOC_SINGLE(sizeof(Scheduler),"Scheduler","cubesched.c","scheduler_create");
  if(sched==NULL)
    fatal_error("cubesched.c","scheduler_create",__LINE__,"Cannot create scheduler : not enough memory");

  sched->defs = defs;
  sched->defs_size = defs_size;
  sched->defs = defs;
  sched->defs_size = defs_size;
  sched->nb_defs = nb_defs;
  sched->nb_procs = 0;
  sched->nb_active = 0;
  sched->nb_wait = 0;
  sched->nb_rprocs = 0;
  sched->string_pool = sp;
  sched->turn = 0;
  sched->max_procs = GLOBAL_CFG_AS_ULONG(CFG_START_MAX_PROCS);
  sched->max_rprocs = sched->max_procs; // XXX FixMe : separate configuration value ?

  sched->active_procs = (Process **) MEM_ALLOC_ARRAY_RESET(sizeof(Process *),sched->max_procs,"Process*","cubesched.c","scheduler_create");
  if(sched->active_procs==NULL)
    fatal_error("cubesched.c","scheduler_create",__LINE__,"Cannot allocate active process queue in scheduler");

  sched->wait_procs = (Process **) MEM_ALLOC_ARRAY_RESET(sizeof(Process *),sched->max_procs,"Process*","cubesched.c","scheduler_create");
  if(sched->wait_procs==NULL)
    fatal_error("cubesched.c","scheduler_create",__LINE__,"Cannot allocate waiting process queue in scheduler");

  sched->react_procs = (Process **) MEM_ALLOC_ARRAY_RESET(sizeof(Process *),sched->max_rprocs,"Process*","cubesched.c","scheduler_create");
  if(sched->react_procs==NULL)
    fatal_error("cubesched.c","scheduler_create",__LINE__,"Cannot allocate reactive process list in scheduler");

  sched->max_chans = GLOBAL_CFG_AS_ULONG(CFG_START_MAX_CHANS);
  sched->chans = (Channel **) MEM_ALLOC_ARRAY_RESET(sizeof(Channel *),sched->max_chans,"Channel*","cubesched.c","scheduler_create");
  if(sched->chans==NULL)
    fatal_error("cubesched.c","scheduler_create",__LINE__,"Cannot allocate channels in scheduler");
  
  sched->free_chans = (Channel **) MEM_ALLOC_ARRAY_RESET(sizeof(Channel *),GLOBAL_CFG_AS_ULONG(CFG_MAX_FREE_CHANS),"Channel*","cubesched.c","scheduler_create");
  if(sched->free_chans==NULL)
    fatal_error("cubesched.c","scheduler_create",__LINE__,"Cannot allocate cache for channels in scheduler");
  
  sched->nb_chans = 0;
  sched->nb_free_chans = 0;
  sched->process_counter = 1;  /* 0 is reserved and id==0 means "self" */
  sched->last_proc = 0;
  sched->last_chan_index = -1;

#ifdef RUNTIME_STAT
  sched->nb_proc_spawned = 0;
  sched->nb_proc_ended = 0;
  sched->nb_channel_alloc = 0;
  sched->nb_channel_acquire = 0;
  sched->nb_channel_free = 0;
  sched->nb_channel_reclaim = 0;
  sched->nb_ref_per_channel = 0;
#endif
  
  return sched;
}

/** Destroy the scheduler structure and reclaim memory
 *  Note: does not reclaim in the string pool (maybe shared by schedulers)
 *  @param[in,out] sched : the scheduler to destroy
 *  @return the NULLified scheduler
 **/
Scheduler * scheduler_free(Scheduler *sched) {

  /* 1) free all active processes */
  unsigned long i;
  for(i=0;i<sched->nb_active;i++) {
    Process * proc = sched->active_procs[i];
    // free environment
    // XXX FixMe: what if environment is of size O ???
    // XXX FixMe: Update the reference count for strings
    MEM_FREE_ARRAY(proc->env.entries, sizeof(Value),proc->env.nb_entries,"Value","cubesched.c","scheduler_free");
    // free process
    MEM_FREE_SINGLE(proc,sizeof(Process),"Process","cubesched.c","scheduler_free");
    sched->active_procs[i]=NULL;
  }

  MEM_FREE_ARRAY(sched->active_procs,sizeof(Process*),sched->max_procs,"Process*","cubesched.c","scheduler_free");
  sched->active_procs=NULL;

  /* 2) free all waiting processes */
  for(i=0;i<sched->nb_wait;i++) {
    Process * proc = sched->wait_procs[i];
    // free environment
    // XXX FixME: what if environment is of size O ???
    // XXX FixMe: Update the reference count for strings
    MEM_FREE_ARRAY(proc->env.entries, sizeof(Value),proc->env.nb_entries,"Value","cubesched.c","scheduler_free");
    // free process
    MEM_FREE_SINGLE(proc,sizeof(Process),"Process","cubesched.c","scheduler_free");
    sched->wait_procs[i]=NULL;
  }

  MEM_FREE_ARRAY(sched->wait_procs,sizeof(Process*),sched->max_procs,"Process*","cubesched.c","scheduler_free");
  sched->wait_procs=NULL;

  /* 3) free all reactive processes */
  for(i=0;i<sched->nb_rprocs;i++) {
    Process * proc = sched->react_procs[i];
    // free environment
    // XXX FixME: what if environment is of size O ???
    // XXX FixMe: Update the reference count for strings
    MEM_FREE_ARRAY(proc->env.entries, sizeof(Value),proc->env.nb_entries,"Value","cubesched.c","scheduler_free");
    // free process
    MEM_FREE_SINGLE(proc,sizeof(Process),"Process","cubesched.c","scheduler_free");
    sched->react_procs[i]=NULL;
  }

  MEM_FREE_ARRAY(sched->react_procs,sizeof(Process*),sched->max_rprocs,"Process*","cubesched.c","scheduler_free");
  sched->react_procs=NULL;

  /* 4) free all channels */
  for(i=0;i<sched->max_chans;i++) {
    Channel * chan = sched->chans[i];
    if(chan!=NULL) {
      MEM_FREE_SINGLE(chan,sizeof(Channel),"Channel","cubesched.c","scheduler_free");
      sched->chans[i]=NULL;
    }
  }

  MEM_FREE_ARRAY(sched->chans,sizeof(Channel*),sched->max_chans,"Channel*","cubesched.c","scheduler_free");
  sched->chans=NULL;

  /* 5) free all free channels */
  MEM_FREE_ARRAY(sched->free_chans,sizeof(Channel*),GLOBAL_CFG_AS_ULONG(CFG_MAX_FREE_CHANS),"Channel*","cubesched.c","scheduler_free");
  sched->free_chans=NULL;

  /* 6) free the scheduler */
  MEM_FREE_SINGLE(sched,sizeof(Scheduler),"Scheduler","cubesched.c","scheduler_free");
  sched = NULL;
  return sched;

}
