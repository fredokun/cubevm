
%{
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "cubealloc.h"
  //#include "cubesched.h"
#include "cubeparse.h"  
#include "cubeast.h"
#include "cubeprim.h"
#include "cubemisc.h"
#include "cubedef.h"

Definition ***GLOBAL_seq_expr = NULL;
int GLOBAL_seq_expr_stack = -1;
int * GLOBAL_seq_expr_size = NULL;

unsigned int GLOBAL_line_no = 0;
char **GLOBAL_lex_env = NULL;
int * GLOBAL_lex_env_stack_ref = NULL;
int GLOBAL_lex_env_stack = -1; 
int GLOBAL_lex_env_size = 0;

Definition * GLOBAL_top_expr = NULL;

StringPool GLOBAL_string_pool;

/* Management of polyadic values */
char ** GLOBAL_POLY_PARAMS = NULL;
int GLOBAL_NB_POLY_PARAMS = 0;

static void reset_poly_params();
static void add_poly_params(char * param);

void debug_lex_env();
void parse_reset_lex_env();
void parse_dig_lex_env();
void parse_backtrack_lex_env();
int parse_add_lex_env(char *name, Bool * expand);
int parse_fetch_lex_env(char *name);
void parse_init_seq_expr();
void parse_add_seq_expr(Definition *expr);
void parse_reset_seq_expr();
void parse_error(char * message, ...);
void yyerror(char *message);
%}

/* The grammar is roughly as follows :

 <prog> ::= <defs> <expr>

 <defs> ::= <epsilon> | <def> <defs>
 <def> ::= "def" <IDENT> '(' <params> ')' '=' <expr>
 <params> ::= <epsilon> | <IDENT> | <IDENT> ',' <params>

 <expr> ::= '[' <expr> ']'
            | '(' <expr> ')'
            | <prefix> ',' <expr>
	    | <prefix>    // <=> <prefix>,0
            | <IDENT> '(' <args> ')'
            | '0'
	    | "if" <value> then <expr>
	    | "if" <value> then <expr> else <expr>
            | <expr> '+' <expr>   // lower priority
            | <expr> "||" <expr>   // higher priority

<prefix> ::= 'new' '(' <IDENT> ')'
             | <IDENT> '?' '(' <IDENT> ')'
	     | <IDENT> '!' <value>
	     | <PRIM> '(' <args> ')'

<value> ::= <BOOL> | <INT> | <STR> | <FLOAT> | <NONE>
            | <IDENT>
            | <value> '+' <value>
	    | '-' <value>
	    | <value> '=' <value>
	    | <value> '-' <value>
	    | <value> '*' <value>
	    | <value> '/' <value>
	    | <value> 'or' <value>
	    | <value> 'and' <value>
	    | 'not' <value>
	    | '[' <value> ']'
	    | '(' <value> ')'
	    | <PRIM> '(' <args> ')'

<args> ::= <epsilon>
           | <value>
           | <value> ',' <args>

*/


%union {
  char * string;
  int integer;
  double real;
  Definition *def;
}

%type <def> def expr prefix value elseexpr

%token TOK_DEF

%token TOK_NEW
%token TOK_LET
%token TOK_REACT
%token TOK_POWN
%token TOK_OWN
%token TOK_JUMP
%token TOK_RJUMP
%token TOK_IF
%token TOK_THEN
%token TOK_ELSE

%token TOK_TRUE
%token TOK_FALSE

%token TOK_NONE

%left TOK_PAR
%left TOK_SUM

%token <string> TOK_ID
%token <string> TOK_PRIM
%token <string> TOK_IN
%token <string> TOK_OUT
%token <integer> TOK_STR
%token <integer> TOK_INT
%token <integer> TOK_LOC
%token <real> TOK_REAL

%token ';'

%token '{'
%token '}'

%left ','

%right '='
%token TOK_DIFF

%token TOK_GT
%token TOK_GTE
%token TOK_LW
%token TOK_LWE
%left TOK_NOT
%left TOK_AND
%left TOK_OR
%left '%'
%left '*'
%left '/'
%left '+' '-'

%left NEG

%%

prog :  defs expr
  { ast_update_root($2); GLOBAL_top_expr = $2; }
;

defs :  /* empty */
       | defs def
;

def :  TOK_DEF TOK_ID { parse_reset_lex_env();  } '(' 
       params ')' { $<def>$ = ast_create_def($2, GLOBAL_lex_env_size); } 
       '=' { parse_dig_lex_env();} expr ';'
       { 
         //debug_lex_env();
	 Definition * def = ast_def_add_expr($<def>7,$10);
         parse_reset_lex_env();
         $<def>$ = def;
       }
;

params :/* empty */
        | TOK_ID
{ Bool expand; parse_add_lex_env($1,&expand); 
  if(expand==FALSE)
   parse_error("Duplicate (last) parameter '%s'",$1);
}
        | params ',' TOK_ID
{ Bool expand; parse_add_lex_env($3, &expand); 
  if(expand==FALSE)
   parse_error("Duplicate (inner) parameter '%s'",$3);   
}
;

expr :   '[' { parse_dig_lex_env(); } expr ']'
{ parse_backtrack_lex_env();
  $$ = $3; }
       | '(' { parse_dig_lex_env(); } expr ')'
{ parse_backtrack_lex_env();
  $$ = $3; }       
       | prefix ',' expr 
{ 
  //fprintf(stderr,"Prefix = \n");
  //fprint_def(stderr,$1,0);
  //fprintf(stderr,"Expr = \n");
  //fprint_def(stderr,$3,0);
  ast_add_child($1,$3); $$ = $1; }
       | prefix
{ 
  Definition * end = ast_create_end_expr();
  //fprintf(stderr,"Prefix = \n");
  //fprint_def(stderr,$1,0);
  //fprintf(stderr,"End = \n");
  //fprint_def(stderr,end,0);
  ast_add_child($1,end); 
  $$ = $1 ;
}       
       | TOK_ID '(' { parse_init_seq_expr(); } args ')'
{ 
Definition * def  = ast_create_call_expr($1,GLOBAL_seq_expr[GLOBAL_seq_expr_stack], GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]);
  parse_reset_seq_expr();
  $$ = def;
}
       | TOK_ID '(' '*' ')'
{
  $$ = ast_create_call_joker_expr($1);
}       
       | TOK_INT
{ if($1!=0)
    parse_error("Unexpected integer '%d' (hint: end of process is '0' only)",$1);
  $$ = ast_create_end_expr(); }
       | TOK_IF value TOK_THEN { parse_dig_lex_env(); } expr elseexpr 
{
  Definition * def = ast_create_if_expr();
  ast_add_child(def,$2);
  ast_add_child(def,$5);
  ast_add_child(def,$6);
  $$ = def;
}
      |  expr TOK_SUM { parse_backtrack_lex_env();
                        parse_dig_lex_env(); }
              expr 
{ Definition * def;
  if($1->type==DEF_CHOICE) {
    printf("$1 is a CHOICE\n");
    def = $1;
    ast_add_child(def, $4);
  } else {
    def = ast_create_choice_expr();
    ast_add_child(def,$1);
    ast_add_child(def,$4);
  }
  $$ = def;
}
       | expr TOK_PAR { parse_backtrack_lex_env();
                        parse_dig_lex_env(); }
              expr
{ Definition * def;
  if($1->type==DEF_PAR) {
    def = $1;
    ast_add_child(def, $4);
  } else {
    def = ast_create_par_expr();
    ast_add_child(def,$1);
    ast_add_child(def,$4);
  }
  $$ = def;
}
;

elseexpr : /* empty */ { $$ = ast_create_end_expr(); }
          | TOK_ELSE { parse_backtrack_lex_env(); } expr
{
  $$ = $3;
}

poly_params : /* empty */
              | TOK_ID { add_poly_params($1); }
              | TOK_ID { add_poly_params($1); } ',' poly_params

prefix :   TOK_NEW '(' { reset_poly_params(); } poly_params ')'
{ 
  int i=0;
  Bool expand = FALSE;
  int bind_ref = -1;
  Definition * first = NULL;
  Definition * current = NULL;
  Definition * last = NULL;

  parse_dig_lex_env();  // reset should remove everything at the end, no ?

  if(GLOBAL_NB_POLY_PARAMS<1) {
    parse_error("Missing channel variable for new prefix");
  }

  /* set up first new prefix */
  
  bind_ref = parse_add_lex_env(GLOBAL_POLY_PARAMS[0],&expand);
  first = ast_create_new_prefix(bind_ref,GLOBAL_NB_POLY_PARAMS,expand);
  last = first;
  //fprintf(stderr," Poly param [%d] = %s\n",0,GLOBAL_POLY_PARAMS[0]);

  /* then set up the other ones (in case of polyadic new) */

  for(i=1;i<GLOBAL_NB_POLY_PARAMS;i++) {
    expand = FALSE;
    bind_ref = parse_add_lex_env(GLOBAL_POLY_PARAMS[i], &expand);
    current = ast_create_new_prefix(bind_ref,GLOBAL_NB_POLY_PARAMS-i,expand);
    ast_add_child_no_arity_check(last,current);
    last = current;
    // fprintf(stderr," Poly param [%d] = %s\n",i,GLOBAL_POLY_PARAMS[i]);
  }

  reset_poly_params();

  $$ = first; /* we need to return the first prefix */
}
         | TOK_IN '(' { reset_poly_params(); } poly_params ')'
{ int i=0;
  Bool expand = FALSE;
  Definition * first = NULL;
  Definition * current = NULL;
  Definition * last = NULL;
  int bind_ref = -1; 
  int chan_ref = parse_fetch_lex_env($1);
  if(chan_ref==-1)
    parse_error("Unknown reference to channel '%s' in input prefix",$1);
  MEM_FREE_ARRAY($1,sizeof(char),strlen($1)+1,"char","cubeparse.y","[rule prefix: TOK_NEW...]"); // reclaim channel identifier memory

  parse_dig_lex_env();  // reset should remove everything at the end, no ?

  if(GLOBAL_NB_POLY_PARAMS==0) { // no binder = CCS-like action prefix
    first = ast_create_action_prefix(chan_ref);
  } else { // else if there are binders = PI-like input prefix
    
    /* set up first input prefix */
    bind_ref = parse_add_lex_env(GLOBAL_POLY_PARAMS[0],&expand);
    first = ast_create_input_prefix(bind_ref, chan_ref, GLOBAL_NB_POLY_PARAMS,expand);
    last = first;

    /* then set up the other ones (polyadic case) */
    for(i=1;i<GLOBAL_NB_POLY_PARAMS;i++) {
      expand = FALSE;
      bind_ref = parse_add_lex_env(GLOBAL_POLY_PARAMS[i],&expand);
      current = ast_create_input_prefix(bind_ref,chan_ref, GLOBAL_NB_POLY_PARAMS-i,expand);
      ast_add_child_no_arity_check(last,current);
      last = current;
    }
  }

  reset_poly_params();

  $$ = first; /* we need to return the first prefix */
}
         | TOK_OUT '(' { parse_init_seq_expr(); } args ')'
{ 
  Definition *def = NULL;
  int arity = 0;
  int chan_ref = parse_fetch_lex_env($1);
  if(chan_ref==-1)
    parse_error("Unknown reference to channel '%s' in output prefix",$1);
  MEM_FREE_ARRAY($1,sizeof(char),strlen($1)+1,"char","cubeparse.y","[rule prefix: TOK_OUT ...]");

  arity = GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack];

  def = ast_create_output_prefix(chan_ref, arity);
  if(arity>0) {
    int i=0;
    for(i=0;i<arity;i++) {
      ast_add_child_no_arity_check(def, (GLOBAL_seq_expr[GLOBAL_seq_expr_stack])[i]);
    }
  }
  parse_reset_seq_expr();
  //fprint_def(stderr,def,0);
  $$ = def;
}	 
         | TOK_LET '(' TOK_ID '=' value ')'
{
  Bool expand = FALSE;
  parse_dig_lex_env();  // reset should remove everything at the end, no ?
  int bind_ref = parse_add_lex_env($3, &expand);
  Definition *def = ast_create_let_prefix(bind_ref,expand);
  ast_add_child(def,$5);
  $$ = def;
}
         | TOK_POWN '(' TOK_ID ',' TOK_ID ')'
{
  int pid_ref = -1;
  int chan_ref = -1;

  pid_ref = parse_fetch_lex_env($3);
  if(pid_ref==-1)
    parse_error("Unknown reference to process id '%s' in own prefix",$3);

  MEM_FREE_ARRAY($3,sizeof(char),strlen($3)+1,"char","cubeparse.y","[rule prefix: TOK_OWN ...]");

  chan_ref = parse_fetch_lex_env($5);
  if(chan_ref==-1)
    parse_error("Unknown reference to channel '%s' in own prefix",$5);

  MEM_FREE_ARRAY($5,sizeof(char),strlen($5)+1,"char","cubeparse.y","[rule prefix: TOK_OWN ...]");

  Definition *def = ast_create_pown_prefix(pid_ref,chan_ref);
  $$ = def;
}
         | TOK_OWN '(' TOK_ID ')'
{
  int chan_ref = -1;

  chan_ref = parse_fetch_lex_env($3);
  if(chan_ref==-1)
    parse_error("Unknown reference to channel '%s' in own prefix",$3);

  MEM_FREE_ARRAY($3,sizeof(char),strlen($3)+1,"char","cubeparse.y","[rule prefix: TOK_SOWN ...]");

  Definition *def = ast_create_own_prefix(chan_ref);
  $$ = def;
}
         | TOK_PRIM { parse_init_seq_expr(); } '(' args ')'
{
  int pref = GLOBAL_get_primitive_ref($1);
  if(pref==-1)
    parse_error("Unknown call to primitive '%s'", $1);
  if(GLOBAL_primitive_value[pref]==TRUE)
    parse_error("Primitive '%s' with return value is called as prefix", $1);
  if(GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]!=GLOBAL_primitive_arity[pref])
    parse_error("Wrong arity for primitive '%s', called with %d arguments, %d expected", $1, GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack], GLOBAL_primitive_arity[pref]);
  Definition *def = ast_create_prim_prefix(pref, GLOBAL_seq_expr[GLOBAL_seq_expr_stack], GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]);
  parse_reset_seq_expr();
  /* reclaim memory for primitive name */
  MEM_FREE_ARRAY($1,sizeof(char),strlen($1)+1,"char","cubeparse.y","[TOK_PRIM/prefix]");
  $$ = def;
}
         | TOK_REACT { parse_init_seq_expr(); } '(' args ')'
{
  Definition *def = ast_create_react_prefix(GLOBAL_seq_expr[GLOBAL_seq_expr_stack], GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]);
  parse_reset_seq_expr();
  $$ = def;
}
;

value : TOK_TRUE { $$ = ast_create_true_value(); } 
        | TOK_FALSE { $$ = ast_create_false_value(); }
        | TOK_INT { $$ = ast_create_int_value($1); }
        | TOK_REAL { $$ = ast_create_real_value($1); }
        | TOK_STR  { $$ = ast_create_string_value($1); }
        | TOK_NONE { $$ = ast_create_none_value(); }
        | TOK_ID
{ 
  int varref = parse_fetch_lex_env($1);
  if(varref==-1)
    parse_error("Unknown variable '%s' while parsing value",$1);
  MEM_FREE_ARRAY($1,sizeof(char),strlen($1)+1,"char","cubeparse.y","[rule value: TOK_TRUE...]"); // this is not needed anymore
  $$ = ast_create_binding_value(varref);
}
        | value '=' value
{
  int pref = GLOBAL_get_primitive_ref("#eq");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value TOK_DIFF value
{
  int pref = GLOBAL_get_primitive_ref("#ineq");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}        | value TOK_GT value
{
  int pref = GLOBAL_get_primitive_ref("#gt");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value TOK_GTE value
{
  int pref = GLOBAL_get_primitive_ref("#gte");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value TOK_LW value
{
  int pref = GLOBAL_get_primitive_ref("#lw");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value TOK_LWE value
{
  int pref = GLOBAL_get_primitive_ref("#lwe");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value '+' value
{
  int pref = GLOBAL_get_primitive_ref("#add");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | '-' value %prec NEG
{
  if($2->type==DEF_VALUE) {
    if(VALUE_IS_INT($2->val)) {
      VALUE_SET_INT($2->val, - VALUE_AS_INT($2->val));
      $$ = $2;
    } else if(VALUE_IS_REAL($2->val)) {
      VALUE_SET_REAL($2->val, - VALUE_AS_REAL($2->val));
      $$ = $2;
    } else
      parse_error("Unary minus only works on numbers");
  } else {
    int pref = GLOBAL_get_primitive_ref("#umin");
    Definition *def = ast_create_prim_value(pref);
    ast_add_child(def,$2);
    $$ = def;
  }
}
        | value '-' value
{
  int pref = GLOBAL_get_primitive_ref("#sub");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value '*' value
{
  int pref = GLOBAL_get_primitive_ref("#mul");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value '/' value
{
  int pref = GLOBAL_get_primitive_ref("#div");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value '%' value
{
  int pref = GLOBAL_get_primitive_ref("#mod");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | TOK_NOT value
{
  int pref = GLOBAL_get_primitive_ref("#not");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$2);
  $$ = def;
}
        | value TOK_AND value
{
  int pref = GLOBAL_get_primitive_ref("#and");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | value TOK_OR value
{
  int pref = GLOBAL_get_primitive_ref("#or");
  Definition *def = ast_create_prim_value(pref);
  ast_add_child(def,$1);
  ast_add_child(def,$3);
  $$ = def;
}
        | '(' value ')' { $$ = $2; }
        | '[' value ']' { $$ = $2; }
        | TOK_PRIM { parse_init_seq_expr(); } '(' args ')'
{
  int pref = GLOBAL_get_primitive_ref($1);
  if(pref==-1)
    parse_error("Unknown call to primitive '%s'", $1);
  if(GLOBAL_primitive_value[pref]==FALSE)
    parse_error("Primitive '%s' without return value is called as value", $1);
  if(GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]!=GLOBAL_primitive_arity[pref])
    parse_error("Wrong arity for primitive '%s', called with %d arguments, %d expected", $1, GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack], GLOBAL_primitive_arity[pref]);
  Definition *def = ast_create_prim_value(pref);
  ast_add_children(def,GLOBAL_seq_expr[GLOBAL_seq_expr_stack], GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]);
  parse_reset_seq_expr();
  /* reclaim memory for primitive name */
  MEM_FREE_ARRAY($1,sizeof(char),strlen($1)+1,"char","cubeparse.y","[TOK_PRIM/value]");
  $$ = def;
} 
;

args : /* empty */
       | value { parse_add_seq_expr($1); }
       | args ',' value { parse_add_seq_expr($3); }
 ;
 
%%

/** Management of polyadic parameters */

static void reset_poly_params() {
  if(GLOBAL_POLY_PARAMS!=NULL) {
    MEM_FREE_ARRAY(GLOBAL_POLY_PARAMS,sizeof(char*),GLOBAL_NB_POLY_PARAMS,"char*","cubeparse.y","reset_poly_params");
    GLOBAL_POLY_PARAMS = NULL;
    GLOBAL_NB_POLY_PARAMS = 0;
  }

  assert(GLOBAL_POLY_PARAMS==NULL);
  assert(GLOBAL_NB_POLY_PARAMS==0);
}

static void add_poly_params(char * param) {
  if(GLOBAL_POLY_PARAMS==NULL) { /* if no parameter yet */
    assert(GLOBAL_NB_POLY_PARAMS==0);
    GLOBAL_POLY_PARAMS = MEM_ALLOC_ARRAY(sizeof(char*), 1, "char*", "cubeparse.y", "add_poly_params");
    if(GLOBAL_POLY_PARAMS==NULL) {
      parse_error("Cannot allocate memory for polyadic parameters (first param)");
    }
  } else { /* already a parameter, expand the array */
    assert(GLOBAL_NB_POLY_PARAMS>0);
    GLOBAL_POLY_PARAMS = MEM_REALLOC_ARRAY(GLOBAL_POLY_PARAMS,sizeof(char*), GLOBAL_NB_POLY_PARAMS, GLOBAL_NB_POLY_PARAMS+1, "char*", "cubeparse.y", "add_poly_params");
    if(GLOBAL_POLY_PARAMS==NULL) {
      parse_error("Cannot reallocate memory for polyadic parameters (next params)");
    }
  }

  /* now add the parameters */
  GLOBAL_POLY_PARAMS[GLOBAL_NB_POLY_PARAMS] = param;
  GLOBAL_NB_POLY_PARAMS++;
}
   

/** Reset the current lexical environment 
 **/
void parse_reset_lex_env() {
  //fprintf(stderr,"**** Reset lexical environment ***\n");

  // remove all stacked environments
  if(GLOBAL_lex_env_stack_ref!=NULL) {
    MEM_FREE_ARRAY(GLOBAL_lex_env_stack_ref,sizeof(int),GLOBAL_lex_env_stack+1,"int","cubeparse.y","parse_reset_lex_env");
    GLOBAL_lex_env_stack_ref=NULL;
    GLOBAL_lex_env_stack = -1;
  }

  // reset current lexical environment
  if(GLOBAL_lex_env!=NULL) {
    int i;
    for(i=0;i<GLOBAL_lex_env_size;i++)
      MEM_FREE_ARRAY(GLOBAL_lex_env[i],sizeof(char),strlen(GLOBAL_lex_env[i])+1,"char","cubeparse.y","parse_reset_lex_env");
    MEM_FREE_ARRAY(GLOBAL_lex_env,sizeof(char*),GLOBAL_lex_env_size,"char*","cubeparse.y","parse_reset_lex_env");
  }
  GLOBAL_lex_env = NULL;
  GLOBAL_lex_env_size = 0;
  // debug_lex_env();
}

/** Add an entry in the current lexical environment
 *  Note: does not add if the entry is already present
 *  @param[in,out] name : the identifier to add
 *  @param[in,out] expand : set to TRUE if the variable is added, FALSE if already present
 *  @return : the coordinate of the variable in the current environment
 *            (Note: environment is flat so an integer is enough)
 **/
int parse_add_lex_env(char *name, Bool * expand) {
  // printf("**** Add '%s' to lexical environment ***\n",name);
  // first look for the variable (linear search is ok)
  int i;
  for(i=0;i<GLOBAL_lex_env_size;i++)
    if(strcmp(GLOBAL_lex_env[i],name)==0) {
      MEM_FREE_ARRAY(name,sizeof(char),strlen(name)+1,"char","cubeparse.y","parse_add_lex_env");
      *expand = FALSE;
      // debug_lex_env();
      return i; // found
    }
  // or create a new entry for real
  if(GLOBAL_lex_env==NULL) {
    // PRECONDITION : the environment size is zero
    assert(GLOBAL_lex_env_size==0);
    GLOBAL_lex_env = (char **) MEM_ALLOC_ARRAY(sizeof(char *), GLOBAL_lex_env_size+1,"char*","cubeparse.y","parse_add_lex_env");
  } else { 
    // PRECONDITION : the environment size is strictly positive
    assert(GLOBAL_lex_env_size>0);
    GLOBAL_lex_env = (char **) MEM_REALLOC_ARRAY(GLOBAL_lex_env, sizeof(char *),GLOBAL_lex_env_size,GLOBAL_lex_env_size+1,"char*","cubeparse.y","parse_add_lex_env");
  }
  if(GLOBAL_lex_env==NULL)
    parse_error("Cannot allocate memory for new lexical variable");
  GLOBAL_lex_env[GLOBAL_lex_env_size] = name;
  GLOBAL_lex_env_size++;
  *expand = TRUE;
  // debug_lex_env();
  return GLOBAL_lex_env_size-1; // the location corresponds to the last entry
}

/** Create a nested environment to handle recursion in the syntax
 **/
void parse_dig_lex_env() {
  // fprintf(stderr,"**** Dig lexical environment ***\n");
  GLOBAL_lex_env_stack++;
  if(GLOBAL_lex_env_stack_ref==NULL) {
    // PRECONDITION: stack index is zero
    assert(GLOBAL_lex_env_stack==0);
    GLOBAL_lex_env_stack_ref = (int *) MEM_ALLOC_ARRAY(sizeof(int),GLOBAL_lex_env_stack+1,"int","cubeparse.y","parse_dig_lex_env");
  } else { // already has a value
    // PRECONDITION: stack index is strictly positive
    assert(GLOBAL_lex_env_stack>0);
    
    GLOBAL_lex_env_stack_ref = (int *) MEM_REALLOC_ARRAY(GLOBAL_lex_env_stack_ref,sizeof(int),GLOBAL_lex_env_stack,GLOBAL_lex_env_stack+1,"int","cubeparse.y","parse_dig_lex_env");
  }
  if(GLOBAL_lex_env_stack_ref==NULL)
    parse_error("Cannot reallocate memory for digging lexical environment");

  GLOBAL_lex_env_stack_ref[GLOBAL_lex_env_stack] = GLOBAL_lex_env_size;
  // debug_lex_env();  
}

/** Goes back to the parent of the current nested environment
 **/
void parse_backtrack_lex_env() {
  // fprintf(stderr, "**** Backtrack lexical environment ***\n");
  if(GLOBAL_lex_env_stack==-1) {
    // PRECONDITION : the stack reference is NULL
    assert(GLOBAL_lex_env_stack_ref==NULL);
    // remove the whole environment
    parse_reset_lex_env();
    // debug_lex_env();
    return;
  }

  // PRECONDITION: here we now the stack reference is not empty
  assert((GLOBAL_lex_env_stack >= 0) && (GLOBAL_lex_env_stack_ref!=NULL));

  int newsize = GLOBAL_lex_env_stack_ref[GLOBAL_lex_env_stack];
  int oldsize = GLOBAL_lex_env_size;

  // first (re)allocate the (new, backtracked) current environment
  // we want newsize elements, it had oldsize elements before
  if(newsize!=oldsize) { // if they have the same size then do nothing
    GLOBAL_lex_env_size = newsize;
    if(GLOBAL_lex_env==NULL) { // XXX FixMe: can we have this case ?
      GLOBAL_lex_env = (char **) MEM_ALLOC_ARRAY(sizeof(char *),newsize,"char*","cubeparse.y","parse_backtrack_lex_env");
    } else {
      if(newsize<oldsize) { /* need to remove the old entries */
        int i = newsize;
        for(i=newsize;i<oldsize;i++) {
          MEM_FREE_ARRAY(GLOBAL_lex_env[i],sizeof(char),strlen(GLOBAL_lex_env[i])+1,"char","cubeparse.y","parse_backtrack_lex_env");
          GLOBAL_lex_env[i] = NULL;
        }
      }
      GLOBAL_lex_env = (char **) MEM_REALLOC_ARRAY(GLOBAL_lex_env,sizeof(char *),oldsize,newsize,"char*","cubeparse.y","parse_backtrack_lex_env");
    }
    if(GLOBAL_lex_env==NULL)
      parse_error("Cannot reallocate lexical environment on backtrack");
  }
  GLOBAL_lex_env_stack--;
  if(GLOBAL_lex_env_stack==-1) { // no more stack references (was 1)
    MEM_FREE_ARRAY(GLOBAL_lex_env_stack_ref,sizeof(int),1,"int","cubeparse.y","parse_backtrack_lex_env");
    GLOBAL_lex_env_stack_ref = NULL;
  } else { // remove the last stack reference
    GLOBAL_lex_env_stack_ref = (int *) MEM_REALLOC_ARRAY(GLOBAL_lex_env_stack_ref,sizeof(int),GLOBAL_lex_env_stack+2,GLOBAL_lex_env_stack+1,"int","cubeparse.y","parse_backtrack_lex_env");
    if(GLOBAL_lex_env_stack_ref==NULL)
      parse_error("Cannot reallocate memory for backtracking lexical environment");
  }
  // debug_lex_env();
}
  
/** Fetch a name in the current lexical environment
 *  @param name: the variable name to look for
 *  @return : the (flat) location of the variable or -1 if not found
 **/
int parse_fetch_lex_env(char *name) {
  int i;
  for(i=0;i<GLOBAL_lex_env_size;i++)
    if(strcmp(GLOBAL_lex_env[i],name)==0) {
      return i; // found
    }
  // not found
  return -1;
}
 
/** Print the lexical environment contents (debugging)
 **/
void debug_lex_env() {
  printf("Lex env = [");
  if(GLOBAL_lex_env_size==0) {
    printf("]\n");
  } else {
    int i;
    for(i=0;i<GLOBAL_lex_env_size-1;i++) {
      printf(GLOBAL_lex_env[i]);
      printf(",");
    }
    printf(GLOBAL_lex_env[GLOBAL_lex_env_size-1]);
    printf("]\n");
  }
  printf("Env stack = [");
  if(GLOBAL_lex_env_stack==-1) {
    printf("]\n");
  } else {
    int i;
    for(i=0;i<GLOBAL_lex_env_stack;i++) {
      printf("%d", GLOBAL_lex_env_stack_ref[i]);
      printf(",");
    }
    printf("%d", GLOBAL_lex_env_stack_ref[GLOBAL_lex_env_stack]);
    printf("]\n");
  }
  fflush(stdout);
}

/** Initialize (possibly nested) expressions in parameter lists    
 **/
void parse_init_seq_expr() {
  GLOBAL_seq_expr_stack++;
  if(GLOBAL_seq_expr==NULL) {
    GLOBAL_seq_expr = (Definition ***) MEM_ALLOC_ARRAY_RESET(sizeof(Definition **),GLOBAL_seq_expr_stack+1,"Definition**","cubeparse.y","parse_init_seq_expr");  
  } else {
    GLOBAL_seq_expr = (Definition ***) MEM_REALLOC_ARRAY(GLOBAL_seq_expr, sizeof(Definition **),GLOBAL_seq_expr_stack,GLOBAL_seq_expr_stack+1,"Definition**","cubeparse.y","parse_init_seq_expr");
  }
  if(GLOBAL_seq_expr==NULL)
    parse_error("Cannot reallocate memory to init argument expressions");
  
  if(GLOBAL_seq_expr_size==NULL) {  
    GLOBAL_seq_expr_size = (int *) MEM_ALLOC_ARRAY(sizeof(int),GLOBAL_seq_expr_stack+1,"int","cubeparse.y","parse_init_seq_expr");
  } else {
    GLOBAL_seq_expr_size = (int *) MEM_REALLOC_ARRAY(GLOBAL_seq_expr_size,sizeof(int),GLOBAL_seq_expr_stack,GLOBAL_seq_expr_stack+1,"int","cubeparse.y","parse_init_seq_expr");
  }
  if(GLOBAL_seq_expr_size==NULL)
    parse_error("Cannot reallocate memory to init argument expression sizes");

  GLOBAL_seq_expr[GLOBAL_seq_expr_stack] = NULL;
  GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack] = 0;
}

/** Add an expression in parameter list
 **/
void parse_add_seq_expr(Definition *expr) {
  if(GLOBAL_seq_expr[GLOBAL_seq_expr_stack]==NULL) {
    GLOBAL_seq_expr[GLOBAL_seq_expr_stack] = (Definition **) MEM_ALLOC_ARRAY_RESET(sizeof(Definition *),GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]+1,"Definition*","cubeparse.y","parse_add_seq_expr");
  } else {
    GLOBAL_seq_expr[GLOBAL_seq_expr_stack] = (Definition **) MEM_REALLOC_ARRAY(GLOBAL_seq_expr[GLOBAL_seq_expr_stack], sizeof(Definition *),GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack],GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]+1,"Definition*","cubeparse.y","parse_add_seq_expr");
  }
  if(GLOBAL_seq_expr[GLOBAL_seq_expr_stack]==NULL)
    parse_error("Cannot allocate memory for new expression in sequence");
  GLOBAL_seq_expr[GLOBAL_seq_expr_stack][GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]] = expr;
  GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack]++;
}

/** Reset the parameter list
 **/
void parse_reset_seq_expr() { 
  // precondition : the stack index must be valid
  assert(GLOBAL_seq_expr_stack>=0);
  if(GLOBAL_seq_expr[GLOBAL_seq_expr_stack] != NULL) {
    MEM_FREE_ARRAY(GLOBAL_seq_expr[GLOBAL_seq_expr_stack],sizeof(Definition*),GLOBAL_seq_expr_size[GLOBAL_seq_expr_stack],"Definition*","cubeparse.y","parse_reset_seq_expr");
    GLOBAL_seq_expr[GLOBAL_seq_expr_stack] = NULL;
  }
  GLOBAL_seq_expr_stack--;
  if(GLOBAL_seq_expr_stack==-1) {
    MEM_FREE_ARRAY(GLOBAL_seq_expr,sizeof(Definition**),1,"Definition**","cubeparse.y","parse_reset_seq_expr");
    GLOBAL_seq_expr = NULL;
    MEM_FREE_ARRAY(GLOBAL_seq_expr_size,sizeof(Definition**),1,"int","cubeparse.y","parse_reset_seq_expr");
    GLOBAL_seq_expr_size = NULL;
  } else {
    if(GLOBAL_seq_expr == NULL) { // XXX FixMe : this case is necessary ?
      GLOBAL_seq_expr = (Definition ***) MEM_ALLOC_ARRAY(sizeof(Definition **),GLOBAL_seq_expr_stack+1,"Definition**","cubeparse.y","parse_reset_seq_expr");
    } else {
      GLOBAL_seq_expr = (Definition ***) MEM_REALLOC_ARRAY(GLOBAL_seq_expr, sizeof(Definition **),GLOBAL_seq_expr_stack+2,GLOBAL_seq_expr_stack+1,"Definition**","cubeparse.y","parse_reset_seq_expr");
    }
    if(GLOBAL_seq_expr==NULL)
      parse_error("Cannot reduce memory to init argument expressions");

    if(GLOBAL_seq_expr_size==NULL) { // XXX FixMe : this case is necessary ?
      GLOBAL_seq_expr_size = (int *) MEM_ALLOC_ARRAY(sizeof(int),GLOBAL_seq_expr_stack+1,"int","cubeparse.y","parse_reset_seq_expr");
    } else {
      GLOBAL_seq_expr_size = (int *) MEM_REALLOC_ARRAY(GLOBAL_seq_expr_size,sizeof(int),GLOBAL_seq_expr_stack+2,GLOBAL_seq_expr_stack+1,"int","cubeparse.y","parse_reset_seq_expr");
    }
    if(GLOBAL_seq_expr_size==NULL)
      parse_error("Cannot reduce memory to init argument expression sizes");
  }
}

void parse_error(char * message, ...) {
  va_list args;

  fprintf(stderr,"Parser error at line %d \n ==> ",
	  GLOBAL_line_no);
  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void yyerror(char *message) {
  parse_error(message);
}

extern FILE* yyin;

Definition * parse_file(FILE * f,Bool debug) {
  if(debug)
    yydebug=1;  
  
  GLOBAL_top_expr = NULL;
  GLOBAL_line_no = 1;
  GLOBAL_lex_env = NULL;
  GLOBAL_lex_env_size = 0;

  GLOBAL_definitions_init();

  init_string_pool(&GLOBAL_string_pool);
  
  yyin = f;
  int ret = yyparse();
  if(ret==1)
    fatal_error("cubeparse.y","parse_file",__LINE__,"Problem with parsing (please report)");
  
  // maybe something stays in the lexical environment
  parse_reset_lex_env();

  return GLOBAL_top_expr;
}

Definition * parse(Bool debug) {
  return parse_file(yyin,debug);
}

Definition * parse_filename(char *filename, Bool debug) {
  Definition *def = NULL;
  FILE *f = fopen(filename,"r");
  if(f==NULL)
    fatal_error("cubeparse.y","parse_file",__LINE__,"Cannot open file '%s'",filename);
  def = parse_file(f,debug);
  fclose(f);
  return def;
}
