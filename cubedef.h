
#ifndef CUBE_DEF_H
#define CUBE_DEF_H

#include "cubeval.h"

typedef enum {
  DEF_NONE = 0,
  DEF_END = 1,    // explicit end of process : end(same as NULL)
  DEF_INPUT = 2,  // input prefix : chan?(binder){substitution pointers}
  DEF_OUTPUT = 3, // output prefix : chan!value
  DEF_NEW = 4,    // new channel prefix : new(chan[,...])
  DEF_PAR = 5,    // n-ary parallel operator :  P1 || P2 || ...
  DEF_CHOICE = 6, // n-ary choice operator :   P1 + P2 + ...
  DEF_CALL = 7,   // call of definition :    Def(arg1, arg2, ...){substitution pointers}
  DEF_HEAD = 8,   // head of definition
  DEF_PRIM = 9,   // primitive : #prim{arg1,arg2,...}
  DEF_VALUE = 10, // value node
  DEF_IF = 11,    // if construct
  DEF_REACT = 12, // reactive process header
  DEF_JOKER = 13, // joker call, no environment switching (optimization)
  DEF_LET = 14,   // lexical variable
  DEF_POWN = 15,   // dynamic reactive channel ownership
  DEF_OWN = 16,  // self if owner
  DEF_MAX_DEF = 17 // Not a definition type
} DefType;

typedef struct _Definition {
  unsigned long id; // a few definition may cohabit ;-)
                    // 0 is reserved for top-level definitions
  unsigned long call_id; // for CALL and PRIM, and also JOKER nodes
  char * call_name; // for CALL nodes
  struct _Definition * head; // the head of definition
  DefType type;
  Value val;    // for constants
  int arity; // arity for prefix (1=monadic)
  int env_size; // static environment size
  int binder_ref; // static reference for binder node
                  // also used for HEAD as arity counter
  int line_no;  // for error messages
  struct _Definition ** children; // children definitions
  unsigned int nb_children; //number of children
  void * extra_ptr;
} Definition;

#define CALL_PRIMITIVE_MAX_ARITY 32

typedef struct _DefEntry {
  char * name;
  unsigned long id;
  int arity;
  Definition *def;
} DefEntry;

extern char * GLOBAL_def_type_name[];
extern int GLOBAL_nb_def_types;

extern DefEntry ** GLOBAL_definitions;
extern int GLOBAL_nb_definitions;
extern int GLOBAL_definitions_size;

////////////////
// Interface //
///////////////

extern void GLOBAL_definitions_init();

extern unsigned long GLOBAL_definitions_add(Definition *def, char *name);
extern unsigned long GLOBAL_definitions_fetch(char *name);

extern void GLOBAL_definition_cleanup(Definition *def);
extern void GLOBAL_all_definitions_cleanup();

extern char* def_print_name(Definition *def);

#endif
