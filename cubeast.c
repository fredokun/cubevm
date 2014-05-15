
#include <stdlib.h>
#include <string.h>

#include "cubeparse.h"
#include "cubeast.h"
#include "cubemisc.h"
#include "cubeprim.h"
#include "cubealloc.h"

Definition * allocate_definition() {
  Definition *def = (Definition *) MEM_ALLOC_SINGLE(sizeof(Definition),"Definition","cubeast.c","allocate_definition");
  if(def==NULL)
    fatal_error("cubeast.c","allocate_definition",__LINE__,"Cannot allocate definition node");
  def->val = VALUE_CREATE_NONE();
  def->id = 0;
  def->call_id = 0; // invalid call
  def->head = NULL;
  def->type = DEF_NONE;
  def->env_size = 0;
  def->arity = 0;
  def->binder_ref = -1;
  def->children = NULL;
  def->nb_children = 0;
  def->line_no = GLOBAL_line_no;
  def->extra_ptr = NULL;

  return def;
}

void ast_add_child_no_arity_check(Definition *parent, Definition *child) {

  if(parent->children==NULL) {
    parent->children = (Definition **) MEM_ALLOC_ARRAY_RESET(sizeof(Definition*),parent->nb_children+1,"Definition*","cubeast.c","ast_add_child");
  } else {
    parent->children = (Definition **) MEM_REALLOC_ARRAY(parent->children, sizeof(Definition*),parent->nb_children,parent->nb_children+1,"Definition*","cubeast.c","ast_add_child");
  }
  if(parent->children==NULL)
    fatal_error("cubeast.c","ast_add_child",__LINE__,"Cannot add child definition (not enough memory)");
  parent->children[parent->nb_children] = child;
  parent->nb_children++;
}

void ast_add_child(Definition *parent, Definition *child) {
  Definition * previous = parent;
  /* handle polyadic prefixes */
  if(parent->type!=DEF_OUTPUT) {
    while(previous->arity>1) {
      if(previous->nb_children==0) 
	fatal_error("cubeast.c","ast_add_child",__LINE__,"AST Node with arity>1 should have children (please report)");
      previous=previous->children[previous->nb_children-1];
    }
  }

  ast_add_child_no_arity_check(previous,child);
}

void ast_add_children(Definition *def, Definition **child, int num_child) {
  int i;
  for(i=0;i<num_child;i++)
    ast_add_child(def,child[i]);
}

Definition * ast_create_def(char *defname, int arity) {

  if(arity>CALL_PRIMITIVE_MAX_ARITY)
    fatal_error("cubeast.c","ast_create_def",__LINE__,"Definition '%s' has arity %d, max is %d\n",defname,arity,CALL_PRIMITIVE_MAX_ARITY);
  
  Definition * def = allocate_definition();
  int def_id = GLOBAL_definitions_add(def, defname);
  
  GLOBAL_definitions[def_id]->arity = arity;
  
  def->id = def_id;
  def->head = def;
  def->type = DEF_HEAD;
  def->env_size = arity;
  return def;
}

int ast_fetch_env_size(Definition *def) {
  int ret = def->env_size;
  unsigned int i;
  unsigned int j;
  int size;
  int max;
  Definition * child;

  for(i=0;i<def->nb_children;i++) {
    child = def->children[i];
    switch(child->type) {
    case DEF_CHOICE: // environment size is maximum of child size
    case DEF_PAR:
      for(j=0,max=0;j<child->nb_children;j++) {
	size = ast_fetch_env_size(child->children[j]);
	if(size>=max)
	  max = size;
      }
      ret += max;
      break;
    case DEF_IF: // environment size is size of condition + maximum size
      ret += ast_fetch_env_size(child->children[0]);
      size = ast_fetch_env_size(child->children[1]);
      max = ast_fetch_env_size(child->children[2]);
      if(max>size)
	ret+=max;
      else
	ret+=size;
      break;
    default:
      ret += ast_fetch_env_size(child);
    }
  }
  return ret;
}

void ast_update_set_head(Definition *def, Definition *head) {
  def->head = head;
  def->id = head->id;
  
  // do the children
  unsigned int i;
  for(i=0;i<def->nb_children;i++)
    if(def->children[i]!=NULL)
      ast_update_set_head(def->children[i], head);
}

void ast_update_head(Definition *def, Definition *head) {
  ast_update_set_head(def,head);
  head->env_size = ast_fetch_env_size(head);
  //printf("Head env size = %d\n",head->env_size);
}

void ast_update_unresolved_calls(Definition *def) {
  // resolve calls
  if(def->type==DEF_CALL && def->call_id==0) {
    unsigned long id = GLOBAL_definitions_fetch(def->call_name);
    if(id==0)
      runtime_error(def,"Cannot find definition '%s' in call", def->call_name);

    def->call_id = id;
    
    if(def->nb_children!=GLOBAL_definitions[id]->arity)
      runtime_error(def,"Wrong arity in call to definition '%s'",def->call_name);

    MEM_FREE_ARRAY(def->call_name,sizeof(char),strlen(def->call_name)+1,"char","cubeast.c","ast_update_unresolved_calls");
    def->call_name = NULL;
    
  }

  // do the children
  unsigned int i;
  for(i=0;i<def->nb_children;i++)
    if(def->children[i]!=NULL)
      ast_update_unresolved_calls(def->children[i]);  
}

void ast_update_root(Definition *root) {
  root->env_size = ast_fetch_env_size(root);
  int i;
  for(i=1;i<GLOBAL_nb_definitions;i++)
    ast_update_unresolved_calls(GLOBAL_definitions[i]->def);
}

Definition * ast_def_add_expr(Definition *def, Definition *body) {
  ast_add_child(def,body);
  ast_update_head(body, def);
  return def;
}

Definition * ast_create_end_expr() {
  Definition * def = allocate_definition();
  def->type = DEF_END;
  return def;
}

Definition * ast_create_call_expr(char *call_name, Definition ** args, int num_args) {
  Definition * def = allocate_definition();
  def->type = DEF_CALL;
  
  unsigned long id = GLOBAL_definitions_fetch(call_name);

  def->call_id = id; // 0 if not (yet) resolved

  if(id!=0) {
    if(num_args!=GLOBAL_definitions[id]->arity)
      parse_error("Wrong arity in call to definition '%s'",call_name);
    
    // we can get the memory back because identifier already exists
    MEM_FREE_ARRAY(call_name,sizeof(char),strlen(call_name)+1,"char","cubeast.c","ast_create_call_expr");
    def->call_name = NULL;
  } else { // unresolved reference, keep name
    // XXX FixMe : the memory should be reclaimed eventually ! (when reclaiming definition ?) 
    def->call_name = call_name;
  }
  
  unsigned int i;
  for(i=0;i<num_args;i++) {
    ast_add_child(def, args[i]);
  }

  return def;
}

Definition * ast_create_call_joker_expr(char *call_name) {
  Definition * def = allocate_definition();
  def->type = DEF_JOKER;
  
  unsigned long id = GLOBAL_definitions_fetch(call_name);

  def->call_id = id; // 0 if not (yet) resolved

  if(id!=0) {
    // we can get the memory back because identifier already exists
    MEM_FREE_ARRAY(call_name,sizeof(char),strlen(call_name)+1,"char","cubeast.c","ast_create_call_joker_expr");
    def->call_name = NULL;
  } else { // unresolved reference, keep name
    // XXX FixMe : the memory should be reclaimed eventually ! (when reclaiming definition ?) 
    def->call_name = call_name;
  }
  
  return def;
}

Definition * ast_create_if_expr() {
  Definition *def = allocate_definition();
  def->type = DEF_IF;
  return def;
}

Definition * ast_create_choice_expr() {
  Definition *def = allocate_definition();
  def->type = DEF_CHOICE;
  return def;
}

Definition * ast_create_par_expr()  {
  Definition *def = allocate_definition();
  def->type = DEF_PAR;
  return def;
}

Definition * ast_create_new_prefix(int binder_ref, int arity, Bool expand) {
  Definition * def = allocate_definition();
  def->type = DEF_NEW;
  def->arity = arity;
  def->binder_ref = binder_ref;
  // must expand environment ?
  if(expand==TRUE)
    def->env_size = 1;
  return def;
}

Definition * ast_create_action_prefix(int chan_ref) {
  Definition * def = allocate_definition();
  def->type = DEF_INPUT;
  def->arity = 0;
  VALUE_SET_TYPE(def->val,VALUE_BIND);
  VALUE_SET_BIND(def->val,chan_ref);
  return def;
}

Definition * ast_create_input_prefix(int binder_ref, int chan_ref, int arity, Bool expand) {
  Definition * def = allocate_definition();
  def->type = DEF_INPUT;
  def->binder_ref = binder_ref;
  def->arity = arity;
  // must expand environment ?
  if(expand==TRUE)
    def->env_size = 1;
  VALUE_SET_TYPE(def->val,VALUE_BIND);
  VALUE_SET_BIND(def->val,chan_ref);
  return def;
}

Definition * ast_create_output_prefix(int chan_ref, int arity) {
  Definition * def = allocate_definition();
  def->type = DEF_OUTPUT;
  def->binder_ref = chan_ref;
  def->arity = arity;
  return def;
}

Definition * ast_create_let_prefix(int binder_ref, Bool expand) {
  Definition * def = allocate_definition();
  def->type = DEF_LET;
  def->binder_ref = binder_ref;
  // must expand environment ?
  if(expand==TRUE)
    def->env_size = 1;
  return def;
}

Definition * ast_create_prim_prefix(int pref, Definition ** args, int num_args)
 {
  Definition * def = allocate_definition();
  def->type = DEF_PRIM;
  def->call_id = pref;
  int i;
  for(i=0;i<num_args;i++)
    ast_add_child(def,args[i]);
  return def;
}

Definition * ast_create_react_prefix(Definition ** args, int num_args) {
  Definition * def = allocate_definition();
  def->type = DEF_REACT;
  int i;
  for(i=0;i<num_args;i++) {
    ast_add_child(def,args[i]);
  }
  return def;
}

Definition * ast_create_pown_prefix(int pid_ref, int chan_ref) {
  Definition * def = allocate_definition();
  def->type = DEF_POWN;
  def->binder_ref = chan_ref;
  VALUE_SET_TYPE(def->val,VALUE_BIND);
  VALUE_SET_BIND(def->val,pid_ref);
  return def;
}

Definition * ast_create_own_prefix(int chan_ref) {
  Definition * def = allocate_definition();
  def->type = DEF_OWN;
  def->binder_ref = chan_ref;
  return def;
}

Definition * ast_create_int_value(int ival) {
  Definition * def = allocate_definition();
  def->type = DEF_VALUE;
  VALUE_SET_TYPE(def->val,VALUE_INT);
  VALUE_SET_INT(def->val,ival);
  return def;
}

Definition * ast_create_real_value(double rval) {
  Definition * def = allocate_definition();
  def->type = DEF_VALUE;
  VALUE_SET_TYPE(def->val,VALUE_REAL);
  VALUE_SET_REAL(def->val,rval);
  return def;
}

Definition * ast_create_string_value(int sval) {
  Definition * def = allocate_definition();
  def->type = DEF_VALUE;
  VALUE_SET_TYPE(def->val,VALUE_STRING);
  VALUE_SET_STRINGREF(def->val,sval);
  return def;
}

Definition * ast_create_none_value() {
  Definition * def = allocate_definition();
  def->type = DEF_VALUE;
  def->val = VALUE_CREATE_NONE();
  return def;
}

Definition * ast_create_true_value() {
  Definition * def = allocate_definition();
  def->type = DEF_VALUE;
  VALUE_SET_TYPE(def->val,VALUE_BOOL);
  VALUE_SET_BOOL(def->val,TRUE);
  return def;
}

Definition * ast_create_false_value() {
  Definition * def = allocate_definition();
  def->type = DEF_VALUE;
  VALUE_SET_TYPE(def->val,VALUE_BOOL);
  VALUE_SET_BOOL(def->val,FALSE);
  return def;
}

Definition * ast_create_binding_value(int ref) {
  Definition * def = allocate_definition();
  def->type = DEF_VALUE;
  VALUE_SET_TYPE(def->val,VALUE_BIND);
  VALUE_SET_BIND(def->val,ref);
  return def;
}

Definition * ast_create_prim_value(int pref) {
  Definition * def = allocate_definition();
  def->type = DEF_PRIM;
  def->call_id = pref;
  return def;
}

void fprint_depth(FILE *f, int depth) {
  int i;
  for(i=0;i<depth;i++)
    fprintf(f, " ");
}

void fprint_def(FILE *f, Definition *def, int depth) {
  if(def==NULL) {
    fprint_depth(f,depth); fatal_error("cubeast.c","fprint_def",__LINE__," NULL AST Node (Please report)");
  }
  fprint_depth(f,depth); fprintf(f,"-------------------------------------------------------------\n");
  fprint_depth(f,depth); fprintf(f,"| Node type = '%s' at line %d\n", def_print_name(def),def->line_no);
  if(def->id==0) {
    fprint_depth(f,depth); fprintf(f,"| At top-level\n");
  } else {
    fprint_depth(f,depth); fprintf(f,"| In definition '%s'\n", GLOBAL_definitions[def->id]->name);
  }
  if(def->type==DEF_CALL) {
    fprint_depth(f,depth); fprintf(f,"| Call to definition '%s'\n",GLOBAL_definitions[def->call_id]->name);
  }
  if(def->type==DEF_PRIM) {
    fprint_depth(f,depth); fprintf(f,"| Call to primitive '%s'\n",GLOBAL_primitive_names[def->call_id]);
  }
  if(def->type==DEF_VALUE) {
    char buffer[1000];
    print_value(&GLOBAL_string_pool,buffer, 999, def->val);
    fprint_depth(f,depth); fprintf(f,"| Value is '%s'\n",buffer);
  }
  if(def->env_size>0) {
    fprint_depth(f,depth); fprintf(f,"| Extend environment with %d entries\n", def->env_size);
  }
  if(def->type==DEF_INPUT) {
    fprint_depth(f,depth); fprintf(f,"| Input on %d'th entry in environment\n", def->val.val._int);
    fprint_depth(f,depth); fprintf(f,"| Bound variable is %d'th entry in environment\n", def->binder_ref);
    fprint_depth(f,depth); fprintf(f,"| Arity is %d\n", def->arity);
  }
  if(def->type==DEF_OUTPUT) {
    fprint_depth(f,depth); fprintf(f,"| Output on %d'th entry in environment\n", def->binder_ref);
    fprint_depth(f,depth); fprintf(f,"| Arity is %d\n", def->arity);
  }
  fprint_depth(f,depth); fprintf(f, "| Number of child nodes = %d\n", def->nb_children);
  fprint_depth(f,depth); fprintf(f,"-------------------------------------------------------------\n");
}

void debug_parse_tree(FILE * f, Definition *def, int depth) {
  fprint_def(f,def,depth);
  unsigned int i;
  for(i=0;i<def->nb_children;i++)
    debug_parse_tree(f,def->children[i],depth+4);
}
