
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include "cubeprim.h"
#include "cubestr.h"
#include "cubemisc.h"
#include "cubealloc.h"

char * GLOBAL_primitive_names[] = { "#print", "#println", "#add", "#sub", "#mul", "#div", "#eq", "#umin", "#gt", "#gte", "#lw", "#lwe", "#rclock", "#clock", "#not", "#mod", "#ineq", "#and", "#or","#arg", "#intfromstr" };
int GLOBAL_primitive_arity[] = { 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 0, 0, 1, 2, 2, 2 ,2,1,1 };
Bool GLOBAL_primitive_value[] = { FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE , TRUE, TRUE, TRUE, TRUE };
int GLOBAL_nb_primitives = 21;

PrimitivePtrType GLOBAL_primitives[] = {
  &prim_print, &prim_println, &prim_add, &prim_sub,
  &prim_mul, &prim_div, &prim_eq, &prim_umin, &prim_gt, &prim_gte,
  &prim_lw, &prim_lwe, &prim_rclock, &prim_clock, &prim_not,
  &prim_mod, &prim_ineq, &prim_and, &prim_or, &prim_arg, &prim_intfromstr
};

int GLOBAL_get_primitive_ref(char *pname) {
  int i;
  for(i=0;i<GLOBAL_nb_primitives;i++) {
    if(strcmp(GLOBAL_primitive_names[i],pname)==0)
      return i;
  }
  fatal_error("cubeprim.c", "GLOBAL_get_primitive_ref", __LINE__, "Unknown primitive : %s",pname);

  return -1; // not found
}

// this is not really called but it shows how to call a primitive
Process * exec_primitive(Scheduler *sched, Process *proc, Definition *expr, int pref, Value args[]) {
  assert(pref>=0 && pref<= GLOBAL_nb_primitives);

  GLOBAL_primitives[pref](sched,proc,expr,args);
  // continuation is last prefix
  proc->def = expr->children[expr->nb_children-1];
  return proc;
}
  
void prim_print(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {

  Value val = args[0];
  if(VALUE_IS_BIND(val))
    val = proc->env.entries[VALUE_AS_BIND(val)];

  switch(VALUE_GET_TYPE(val)) {
  case VALUE_NONE:
    printf("NONE");
    break;
  case VALUE_INT:
    printf("%d", val.val._int);
    break;
  case VALUE_REAL:
    printf("%lf", val.val._real);
    break;
  case VALUE_STRING:
    printf("%s", STRGET(sched->string_pool,val.val._int).str);
    break;
  case VALUE_CHAN:
    printf("Channel[ID=%ld]", val.val._chan->id);
    break;
  case VALUE_BOOL:
    if(val.val._int==TRUE)
      printf("true");
    else
      printf("false");
    break;
  case VALUE_CHAR:
    printf("%c", val.val._char);
    break;
  default:
    warning("cubeprim.c","prim_print",__LINE__,"Cannot print value");
  }
}

void prim_println(Scheduler *sched, Process *proc,Definition *expr, Value args[]) {
  prim_print(sched, proc, expr,args);
  printf("\n");
}

void prim_not(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  //printf("debug : ");
  //prim_print(sched,proc,expr,args);
  //fflush(stdout);

  Value arg = args[0];
  if(VALUE_IS_BIND(arg))
    arg = proc->env.entries[VALUE_AS_BIND(arg)];
  int val=0;

  switch(VALUE_GET_TYPE(arg)) {
  case VALUE_BOOL:
    val = VALUE_AS_BOOL(arg);
    VALUE_SET_TYPE(proc->val,VALUE_BOOL);
    if(val==FALSE) 
      VALUE_SET_TRUE(proc->val);
    else
      VALUE_SET_FALSE(proc->val);
    break;
  default:
      runtime_error(expr,"Bad operand type, #not primitive only accepts boolean arguments");
  } 
}  

void prim_and(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {

  Value arg1 = args[0];
  if(VALUE_IS_BIND(arg1))
    arg1 = proc->env.entries[VALUE_AS_BIND(arg1)];
  Value arg2 = args[1];
  if(VALUE_IS_BIND(arg2))
    arg2 = proc->env.entries[VALUE_AS_BIND(arg2)];

  if(VALUE_GET_TYPE(arg1)!=VALUE_BOOL)
    runtime_error(expr,"Bad left operand type #and expects boolean operands");

  if(VALUE_GET_TYPE(arg2)!=VALUE_BOOL)
    runtime_error(expr,"Bad right operand type #and expects boolean operands");

  VALUE_SET_TYPE(proc->val, VALUE_BOOL);
  VALUE_SET_BOOL(proc->val, VALUE_AS_BOOL(arg1) && VALUE_AS_BOOL(arg2));

}

void prim_or(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {

  Value arg1 = args[0];
  if(VALUE_IS_BIND(arg1))
    arg1 = proc->env.entries[VALUE_AS_BIND(arg1)];
  Value arg2 = args[1];
  if(VALUE_IS_BIND(arg2))
    arg2 = proc->env.entries[VALUE_AS_BIND(arg2)];

  if(VALUE_GET_TYPE(arg1)!=VALUE_BOOL)
    runtime_error(expr,"Bad left operand type #or expects boolean operands");

  if(VALUE_GET_TYPE(arg2)!=VALUE_BOOL)
    runtime_error(expr,"Bad right operand type #or expects boolean operands");

  VALUE_SET_TYPE(proc->val, VALUE_BOOL);
  VALUE_SET_BOOL(proc->val, VALUE_AS_BOOL(arg1) || VALUE_AS_BOOL(arg2));
}

void prim_add(Scheduler *sched, Process *proc,Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);

  //  printf("debug left : ");
  //  prim_print(sched,proc,expr,args);
  //  fflush(stdout);

  //  printf("debug right : ");
  //  prim_print(sched,proc,expr,args+1);
  //  fflush(stdout);


  // XXX : dynamic type checking could be
  //       sometimes removed with static analysis (soft typing)
  if(ltype==VALUE_INT) {
    // prim_add_int_int
    if(rtype==VALUE_INT) {
      VALUE_SET_TYPE(proc->val,VALUE_INT);
      VALUE_SET_INT(proc->val,VALUE_AS_INT(left) + VALUE_AS_INT(right));
      return;
    }
    // prim_add_int_real
    else if(rtype==VALUE_REAL) {
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,(double) VALUE_AS_INT(left) + VALUE_AS_REAL(right));
      return;
    }
    // prim_add_int_string
    else if(rtype==VALUE_STRING) {
      VALUE_SET_TYPE(proc->val,VALUE_STRING);
      char * intstr = cstr_from_int(VALUE_AS_INT(left));
      VALUE_SET_STRINGREF(proc->val,str_add_cstr_1(sched->string_pool,intstr,VALUE_AS_STRINGREF(right)));
      MEM_FREE_ARRAY(intstr,sizeof(char),strlen(intstr)+1,"char","cubedef.c","prim_add");
      // reclaim the right string if needed (it may be temporary with ref_count==0)
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(right));
      return;
    }      
    // or error
    else
      runtime_error(expr,"Bad operand type (right), may only add int to int, real or string");      
  } else if(ltype==VALUE_REAL) {
    // prim_add_real_real
    if(rtype==VALUE_REAL) { 
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val, VALUE_AS_REAL(left) + VALUE_AS_REAL(right));
      return;
    }
    // prim_add_real_int
    else if(rtype==VALUE_INT) {
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val, VALUE_AS_REAL(left) + (double) VALUE_AS_INT(right));
      return;
    }
    // prim_add_real_str
    else if(rtype==VALUE_STRING) {
      VALUE_SET_TYPE(proc->val,VALUE_STRING);
      char * realstr = cstr_from_real(VALUE_AS_REAL(left));
      VALUE_SET_STRINGREF(proc->val,str_add_cstr_1(sched->string_pool,realstr,VALUE_AS_STRINGREF(right)));
      MEM_FREE_ARRAY(realstr,sizeof(char),strlen(realstr)+1,"char","cubeprim.c","prim_add");
      // reclaim the right string if needed
      str_reclaim(sched->string_pool, right.val._int);
      return;
    }
    // or error
    else
      runtime_error(expr,"Bad operand type (right), may only add real to int, real or string");
  } else if(ltype==VALUE_STRING) {
    // prim_add_string_string
    if(rtype==VALUE_STRING) {
      VALUE_SET_TYPE(proc->val,VALUE_STRING);
     VALUE_SET_STRINGREF(proc->val,str_add(sched->string_pool, VALUE_AS_STRINGREF(left),VALUE_AS_STRINGREF(right)));
      // reclaim the source strings if needed
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(right));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(right));
      return;
    }
    // prim_add_string_int
    else if(rtype==VALUE_INT) {
      VALUE_SET_TYPE(proc->val,VALUE_STRING);
      char * intstr = cstr_from_int(VALUE_AS_INT(right));
      VALUE_SET_STRINGREF(proc->val,str_add_cstr_2(sched->string_pool, VALUE_AS_STRINGREF(left),intstr));
      MEM_FREE_ARRAY(intstr,sizeof(char),strlen(intstr)+1,"char","cubeprim.c","prim_add");
      // reclaim the left string if needed
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool, VALUE_AS_STRINGREF(left));
      return;
    }
    // prim_add_string_int
    else if(rtype==VALUE_REAL) {
      VALUE_SET_TYPE(proc->val,VALUE_STRING);
      char * realstr = cstr_from_real(VALUE_AS_REAL(right));
      VALUE_SET_STRINGREF(proc->val,str_add_cstr_2(sched->string_pool, VALUE_AS_STRINGREF(left),realstr));
      MEM_FREE_ARRAY(realstr,sizeof(char),strlen(realstr)+1,"char","cubeprim.c","prim_add");
      // reclaim the left string if needed
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool, VALUE_AS_STRINGREF(left));
      return;
    } 
    // or error
    else
      runtime_error(expr,"Bad operand type (right), may only add string to int, real or string");
  }
  // or error
    else
      runtime_error(expr,"Bad operand type (right), may only add string to int and real");
}

void prim_sub(Scheduler *sched, Process *proc,Definition *expr, Value args[]) { 
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);
  // XXX : dynamic type checking could be
  //       sometimes removed with static analysis (soft typing)
  if(ltype==VALUE_INT) {
    // prim_sub_int_int
    if(rtype==VALUE_INT) {
      VALUE_SET_TYPE(proc->val,VALUE_INT);
      VALUE_SET_INT(proc->val,VALUE_AS_INT(left) - VALUE_AS_INT(right));
      return;
    }
    // prim_sub_int_real
    else if(rtype==VALUE_REAL) {
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,(double) VALUE_AS_INT(left) - VALUE_AS_REAL(right));
      return;
    }
    else
      runtime_error(expr,"Bad operand type (right), may only #sub int with int or real");      
  } else if(ltype==VALUE_REAL) {
    // prim_sub_real_real
    if(rtype==VALUE_REAL) { 
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,VALUE_AS_REAL(left) - VALUE_AS_REAL(right));
      return;
    }
    // prim_sub_real_int
    else if(rtype==VALUE_INT) {
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,VALUE_AS_REAL(left) - (double) VALUE_AS_INT(right));
      return;
    }
    // or error
    else
      runtime_error(expr,"Bad operand type (right), may only #sub real with int or real");
  }
  // or error
  else
    runtime_error(expr,"Bad operand type (right), may only #sub numbers");
}

void prim_mul(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);
  // XXX : dynamic type checking could be
  //       sometimes removed with static analysis (soft typing)
  if(ltype==VALUE_INT) {
    // prim_mul_int_int
    if(rtype==VALUE_INT) {
      VALUE_SET_TYPE(proc->val,VALUE_INT);
      VALUE_SET_INT(proc->val,VALUE_AS_INT(left) * VALUE_AS_INT(right));
      return;
    }
    // prim_mul_int_real
    else if(rtype==VALUE_REAL) {
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,(double) VALUE_AS_INT(left) * VALUE_AS_REAL(right));
      return;
    }
    else
      runtime_error(expr,"Bad operand type (right), may only #mul int with int or real");      
  } else if(ltype==VALUE_REAL) {
    // prim_mul_real_real
    if(rtype==VALUE_REAL) { 
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,VALUE_AS_REAL(left) * VALUE_AS_REAL(right));
      return;
    }
    // prim_mul_real_int
    else if(rtype==VALUE_INT) {
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,VALUE_AS_REAL(left) * (double) VALUE_AS_INT(right));
      return;
    }
    // or error
    else
      runtime_error(expr,"Bad operand type (right), may only #mul real with int or real");
  }
  // or error
  else
    runtime_error(expr,"Bad operand type (right), may only #mul numbers");
}

void prim_div(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);
  // XXX : dynamic type checking could be
  //       sometimes removed with static analysis (soft typing)
  if(ltype==VALUE_INT) {
    // prim_div_int_int
    if(rtype==VALUE_INT) {
      if(VALUE_AS_INT(right)==0)
	runtime_error(expr,"Division by (integer) zero");
      VALUE_SET_TYPE(proc->val,VALUE_INT);
      VALUE_SET_INT(proc->val,VALUE_AS_INT(left) / VALUE_AS_INT(right));
      return;
    }
    // prim_div_int_real
    else if(rtype==VALUE_REAL) {
      if(VALUE_AS_REAL(right)==0.0)
	runtime_error(expr,"Division by (real) zero");
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,(double) VALUE_AS_INT(left) / VALUE_AS_REAL(right));
      return;
    }
    else
      runtime_error(expr,"Bad operand type (right), may only #div int with int or real");      
  } else if(ltype==VALUE_REAL) {
    // prim_div_real_real
    if(rtype==VALUE_REAL) { 
      if(VALUE_AS_REAL(right)==0.0)
	runtime_error(expr,"Division by (real) zero");
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,VALUE_AS_REAL(left) / VALUE_AS_REAL(right));
      return;
    }
    // prim_div_real_int
    else if(rtype==VALUE_INT) {
      if(VALUE_AS_INT(right)==0)
	runtime_error(expr,"Division by (integer) zero");
      VALUE_SET_TYPE(proc->val,VALUE_REAL);
      VALUE_SET_REAL(proc->val,VALUE_AS_REAL(left) / (double) VALUE_AS_INT(right));
      return;
    }
    // or error
    else
      runtime_error(expr,"Bad operand type (right), may only #div real with int or real");
  }
  // or error
  else
    runtime_error(expr,"Bad operand type (right), may only #div numbers");
}

void prim_mod(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);
  // XXX : dynamic type checking could be
  //       sometimes removed with static analysis (soft typing)
  if(ltype==VALUE_INT) {
    // prim_mul_int_int
    if(rtype==VALUE_INT) {
      VALUE_SET_TYPE(proc->val,VALUE_INT);
      VALUE_SET_INT(proc->val,VALUE_AS_INT(left) % VALUE_AS_INT(right));
      return;
    }
    // prim_mul_int_real
    else if(rtype==VALUE_REAL) {
      // beware : cast to an int
      runtime_warning(expr,"Right argument of #mod is real, cast to int");
      VALUE_SET_TYPE(proc->val,VALUE_INT);
      VALUE_SET_INT(proc->val,VALUE_AS_INT(left) % (int) VALUE_AS_REAL(right));
      return;
    }
    else
      runtime_error(expr,"Bad operand type (right), may only #mod int with int or real");      
  } else if(ltype==VALUE_REAL) {
    // prim_mul_real_real
    if(rtype==VALUE_REAL) { 
      // beware : cast to an int
      runtime_warning(expr,"Left and Right arguments of #mod are real, cast to int");
      VALUE_SET_TYPE(proc->val,VALUE_INT);
      VALUE_SET_REAL(proc->val,(int) VALUE_AS_REAL(left) % (int) VALUE_AS_REAL(right));
      return;
    }
    // prim_mul_real_int
    else if(rtype==VALUE_INT) {
      // beware : cast to an int
      runtime_warning(expr,"Left argument of #mod is real, cast to int");
      VALUE_SET_TYPE(proc->val,VALUE_INT);
      VALUE_SET_INT(proc->val,(int) VALUE_AS_REAL(left) % VALUE_AS_INT(right));
      return;
    }
    // or error
    else
      runtime_error(expr,"Bad operand type (right), may only #mod real with int or real");
  }
  // or error
  else
    runtime_error(expr,"Bad operand type (right), may only #mod numbers");
}

// XXXX FixMe: should be able to compare ints and reals ...
void prim_eq(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);

  // by default, returned value is FALSE
  VALUE_SET_TYPE(proc->val,VALUE_BOOL);
  VALUE_SET_BOOL(proc->val,FALSE);

  if(ltype!=rtype) {
    return; // FALSE if distinct types
  }

  if(ltype==VALUE_NONE && rtype==VALUE_NONE) {
    VALUE_SET_BOOL(proc->val,TRUE);
    return;
  }
    
  // XXX : FixMe : need to garbage collect strings ???

  switch(ltype) {
  case VALUE_STRING: {
    String strleft = STRGET(sched->string_pool,VALUE_AS_STRINGREF(left));
    String strright = STRGET(sched->string_pool,VALUE_AS_STRINGREF(right));
    if(strcmp(strleft.str, strright.str)==0) {
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(right));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(right));
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      STRUNREF(sched->string_pool,left.val._int);
      STRUNREF(sched->string_pool,right.val._int);
      str_reclaim(sched->string_pool,left.val._int);
      str_reclaim(sched->string_pool,right.val._int);
      return; // FALSE if distinct strings
    } }
    break;

  case VALUE_BOOL:
    if(VALUE_AS_BOOL(left)==VALUE_AS_BOOL(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_CHAR:
    if(VALUE_AS_CHAR(left)==VALUE_AS_CHAR(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_INT:
    if(VALUE_AS_INT(left)==VALUE_AS_INT(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_REAL:
    if(VALUE_AS_REAL(left)==VALUE_AS_REAL(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_CHAN:
    if(CHANP_GET_ID(VALUE_AS_CHANP(left))==CHANP_GET_ID(VALUE_AS_CHANP(right))) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  default:
    runtime_error(expr,"Unknown type for equality");
  }
      
}

void prim_ineq(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);

  // by default, returned value is TRUE
  VALUE_SET_TYPE(proc->val,VALUE_BOOL);
  VALUE_SET_BOOL(proc->val,TRUE);

  if(ltype!=rtype) {
    return; // TRUE if distinct types
  }

  if(ltype==VALUE_NONE && rtype==VALUE_NONE) {
    VALUE_SET_BOOL(proc->val,FALSE);
    return;
  }
    
  // XXX : FixMe : need to garbage collect strings ???

  switch(ltype) {
  case VALUE_STRING: {
    String strleft = STRGET(sched->string_pool,VALUE_AS_STRINGREF(left));
    String strright = STRGET(sched->string_pool,VALUE_AS_STRINGREF(right));
    if(strcmp(strleft.str, strright.str)==0) {
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(right));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(right));
      VALUE_SET_BOOL(proc->val,FALSE);
      return;
    } else {
      STRUNREF(sched->string_pool,left.val._int);
      STRUNREF(sched->string_pool,right.val._int);
      str_reclaim(sched->string_pool,left.val._int);
      str_reclaim(sched->string_pool,right.val._int);
      // TRUE if inequal strings
      return; 
    } }
    break;

  case VALUE_BOOL:
    if(VALUE_AS_BOOL(left)==VALUE_AS_BOOL(right)) {
      VALUE_SET_BOOL(proc->val,FALSE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_CHAR:
    if(VALUE_AS_CHAR(left)==VALUE_AS_CHAR(right)) {
      VALUE_SET_BOOL(proc->val,FALSE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_INT:
    if(VALUE_AS_INT(left)==VALUE_AS_INT(right)) {
      VALUE_SET_BOOL(proc->val,FALSE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_REAL:
    if(VALUE_AS_REAL(left)==VALUE_AS_REAL(right)) {
      VALUE_SET_BOOL(proc->val,FALSE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_CHAN:
    if(CHANP_GET_ID(VALUE_AS_CHANP(left))==CHANP_GET_ID(VALUE_AS_CHANP(right))) {
      VALUE_SET_BOOL(proc->val,FALSE);
      return;
    } else {
      return;
    }
    break;

  default:
    runtime_error(expr,"Unknown type for inequality");
  }
      
}

void prim_gt(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);

  // by default, returned value is FALSE
  VALUE_SET_TYPE(proc->val,VALUE_BOOL);
  VALUE_SET_BOOL(proc->val,FALSE);

  if(ltype!=rtype) {
    return; // FALSE if distinct types
  }

  if(ltype==VALUE_NONE && rtype==VALUE_NONE) {
    VALUE_SET_BOOL(proc->val,TRUE);
    return;
  }
    
  // XXX : FixMe : need to garbage collect strings ???

  switch(ltype) {
  case VALUE_STRING: {
    String strleft = STRGET(sched->string_pool,VALUE_AS_STRINGREF(left));
    String strright = STRGET(sched->string_pool,VALUE_AS_STRINGREF(right));
    if(strcmp(strleft.str, strright.str)>0) {
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(right));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(right));
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      STRUNREF(sched->string_pool,left.val._int);
      STRUNREF(sched->string_pool,right.val._int);
      str_reclaim(sched->string_pool,left.val._int);
      str_reclaim(sched->string_pool,right.val._int);
      return; // FALSE if distinct strings
    } }
    break;

  case VALUE_BOOL:
    runtime_error(expr, "No order relation on booleans (greater)");
    break;

  case VALUE_CHAR:
    if(VALUE_AS_CHAR(left)>VALUE_AS_CHAR(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_INT:
    if(VALUE_AS_INT(left)>VALUE_AS_INT(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_REAL:
    if(VALUE_AS_REAL(left)>VALUE_AS_REAL(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_CHAN:
    runtime_error(expr, "No order relation on channels (greater)");
    break;

  default:
    runtime_error(expr,"Unknown type for equality");
  }      
}

void prim_gte(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);

  // by default, returned value is FALSE
  VALUE_SET_TYPE(proc->val,VALUE_BOOL);
  VALUE_SET_BOOL(proc->val,FALSE);

  if(ltype!=rtype) {
    return; // FALSE if distinct types
  }

  if(ltype==VALUE_NONE && rtype==VALUE_NONE) {
    VALUE_SET_BOOL(proc->val,TRUE);
    return;
  }
    
  // XXX : FixMe : need to garbage collect strings ???

  switch(ltype) {
  case VALUE_STRING: {
    String strleft = STRGET(sched->string_pool,VALUE_AS_STRINGREF(left));
    String strright = STRGET(sched->string_pool,VALUE_AS_STRINGREF(right));
    if(strcmp(strleft.str, strright.str)>=0) {
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(right));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(right));
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      STRUNREF(sched->string_pool,left.val._int);
      STRUNREF(sched->string_pool,right.val._int);
      str_reclaim(sched->string_pool,left.val._int);
      str_reclaim(sched->string_pool,right.val._int);
      return; // FALSE if distinct strings
    } }
    break;

  case VALUE_BOOL:
    runtime_error(expr, "No order relation on booleans (greater or equal)");
    break;

  case VALUE_CHAR:
    if(VALUE_AS_CHAR(left)>=VALUE_AS_CHAR(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_INT:
    if(VALUE_AS_INT(left)>=VALUE_AS_INT(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_REAL:
    if(VALUE_AS_REAL(left)>=VALUE_AS_REAL(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_CHAN:
    runtime_error(expr, "No order relation on channels (greater or equal)");
    break;

  default:
    runtime_error(expr,"Unknown type for equality");
  }            
}

void prim_lw(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);

  // by default, returned value is FALSE
  VALUE_SET_TYPE(proc->val,VALUE_BOOL);
  VALUE_SET_BOOL(proc->val,FALSE);

  if(ltype!=rtype) {
    return; // FALSE if distinct types
  }

  if(ltype==VALUE_NONE && rtype==VALUE_NONE) {
    VALUE_SET_BOOL(proc->val,TRUE);
    return;
  }
    
  // XXX : FixMe : need to garbage collect strings ???

  switch(ltype) {
  case VALUE_STRING: {
    String strleft = STRGET(sched->string_pool,VALUE_AS_STRINGREF(left));
    String strright = STRGET(sched->string_pool,VALUE_AS_STRINGREF(right));
    if(strcmp(strleft.str, strright.str)<0) {
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(right));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(right));
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      STRUNREF(sched->string_pool,left.val._int);
      STRUNREF(sched->string_pool,right.val._int);
      str_reclaim(sched->string_pool,left.val._int);
      str_reclaim(sched->string_pool,right.val._int);
      return; // FALSE if distinct strings
    } }
    break;

  case VALUE_BOOL:
    runtime_error(expr, "No order relation on booleans (lower)");
    break;

  case VALUE_CHAR:
    if(VALUE_AS_CHAR(left)<VALUE_AS_CHAR(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_INT:
    if(VALUE_AS_INT(left)<VALUE_AS_INT(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_REAL:
    if(VALUE_AS_REAL(left)<VALUE_AS_REAL(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_CHAN:
    runtime_error(expr, "No order relation on channels (lower)");
    break;

  default:
    runtime_error(expr,"Unknown type for equality");
  }            
}

void prim_lwe(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  Value left = args[0];
  if(VALUE_IS_BIND(left))
    left = proc->env.entries[VALUE_AS_BIND(left)];
  int ltype = VALUE_GET_TYPE(left);
  Value right = args[1];
  if(VALUE_IS_BIND(right))
    right = proc->env.entries[VALUE_AS_BIND(right)];
  int rtype = VALUE_GET_TYPE(right);

  // by default, returned value is FALSE
  VALUE_SET_TYPE(proc->val,VALUE_BOOL);
  VALUE_SET_BOOL(proc->val,FALSE);

  if(ltype!=rtype) {
    return; // FALSE if distinct types
  }

  if(ltype==VALUE_NONE && rtype==VALUE_NONE) {
    VALUE_SET_BOOL(proc->val,TRUE);
    return;
  }
    
  // XXX : FixMe : need to garbage collect strings ???

  switch(ltype) {
  case VALUE_STRING: {
    String strleft = STRGET(sched->string_pool,VALUE_AS_STRINGREF(left));
    String strright = STRGET(sched->string_pool,VALUE_AS_STRINGREF(right));
    if(strcmp(strleft.str, strright.str)<=0) {
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(left));
      STRUNREF(sched->string_pool,VALUE_AS_STRINGREF(right));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(left));
      str_reclaim(sched->string_pool,VALUE_AS_STRINGREF(right));
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      STRUNREF(sched->string_pool,left.val._int);
      STRUNREF(sched->string_pool,right.val._int);
      str_reclaim(sched->string_pool,left.val._int);
      str_reclaim(sched->string_pool,right.val._int);
      return; // FALSE if distinct strings
    } }
    break;

  case VALUE_BOOL:
    runtime_error(expr, "No order relation on booleans (lower or equal)");
    break;

  case VALUE_CHAR:
    if(VALUE_AS_CHAR(left)<=VALUE_AS_CHAR(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_INT:
    if(VALUE_AS_INT(left)<=VALUE_AS_INT(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_REAL:
    if(VALUE_AS_REAL(left)<=VALUE_AS_REAL(right)) {
      VALUE_SET_BOOL(proc->val,TRUE);
      return;
    } else {
      return;
    }
    break;

  case VALUE_CHAN:
    runtime_error(expr, "No order relation on channels (lower or equal)");
    break;

  default:
    runtime_error(expr,"Unknown type for equality");
  }            
      }

void prim_umin(Scheduler *sched, Process *proc, Definition *expr, Value args[]) {
  fatal_error("cubeprim.c","prim_umin",__LINE__,"Primitive #umin not yet implemented");
}

clock_t GLOBAL_current_clock;

void prim_rclock(Scheduler *sched, Process *proc, Definition *expr, Value args[])
{
  GLOBAL_current_clock = clock();
}

void prim_clock(Scheduler *sched, Process *proc, Definition *expr, Value args[])
{
  clock_t now = clock();
  
  VALUE_SET_TYPE(proc->val,VALUE_REAL);
  VALUE_SET_REAL(proc->val,(double) (now - GLOBAL_current_clock) / (CLOCKS_PER_SEC / 1000.0));
}

void prim_arg(Scheduler *sched, Process *proc,Definition *expr, Value args[]) {
  Value narg = args[0];
  if (VALUE_GET_TYPE(narg) != VALUE_INT)
    runtime_error(expr,"argument of #arg must be an integer");

  int n = VALUE_AS_INT(narg);

  if(n<0 || n>=sched->argc)
    runtime_error(expr,"no such command line argument");

  VALUE_SET_TYPE(proc->val,VALUE_STRING);

  /*
  char * cstr = MEM_ALLOC_ARRAY(sizeof(char),strlen(sched->argv[n])+1,"char*","cubeprim.c","prim_arg");
  if(cstr==NULL)
    fatal_error("cubeprim.c","prim_arg",__LINE__,"Cannot allocate string");

  cstr = strncat(cstr,sched->argv[n],strlen(sched->argv[n]));
  cstr[strlen(cstr)]=(char)0;

  */
  
  StringRef argstr = str_make(sched->string_pool,sched->argv[n],strlen(sched->argv[n])+1,-1); // permanent string
  
  VALUE_SET_STRINGREF(proc->val,argstr);
}

void prim_intfromstr(Scheduler *sched, Process *proc,Definition *expr, Value args[]) {
  Value sarg = args[0];
  if (VALUE_GET_TYPE(sarg) != VALUE_STRING)
    runtime_error(expr,"argument of #intfromstr must be a string");

  String str = STRGET(sched->string_pool,VALUE_AS_STRINGREF(sarg));

  int val = int_from_cstr(str.str);
  Value vval;

  VALUE_SET_INT(vval,val);

  VALUE_SET_TYPE(proc->val,VALUE_INT);
  VALUE_SET_INT(proc->val,VALUE_AS_INT(vval));
}
