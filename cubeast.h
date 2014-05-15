
#ifndef CUBE_AST_H
#define CUBE_AST_H

#include "cubesched.h"

extern void ast_add_child(Definition *parent, Definition *child);
extern void ast_add_child_no_arity_check(Definition *parent, Definition *child);
extern void ast_add_children(Definition *def, Definition **child, int num_child);
extern Definition * ast_create_def(char *defname, int arity);
extern void ast_update_head(Definition *def, Definition *head);
extern void ast_update_root(Definition *root);
extern Definition * ast_def_add_expr(Definition *def, Definition *body);
extern Definition * ast_create_end_expr();
extern Definition * ast_create_call_expr(char *call_name, Definition ** args, int num_args);
extern Definition * ast_create_call_joker_expr(char *call_name);
extern Definition * ast_create_if_expr();
extern Definition * ast_create_choice_expr();
extern Definition * ast_create_par_expr();
extern Definition * ast_create_new_prefix(int binder_ref, int arity, Bool expand);
extern Definition * ast_create_action_prefix(int chan_ref);
extern Definition * ast_create_input_prefix(int binder_ref, int chan_ref, int arity, Bool expand);
extern Definition * ast_create_output_prefix(int chan_ref, int arity);
extern Definition * ast_create_let_prefix(int binder_ref, Bool expand);
extern Definition * ast_create_pown_prefix(int pid_ref, int chan_ref);
extern Definition * ast_create_own_prefix(int chan_ref);

extern Definition * ast_create_prim_prefix(int pref, Definition ** args, int num_args);
extern Definition * ast_create_react_prefix(Definition ** args, int num_args);
extern Definition * ast_create_int_value(int ival);
extern Definition * ast_create_real_value(double rval);
extern Definition * ast_create_string_value(int sval);
extern Definition * ast_create_none_value();
extern Definition * ast_create_true_value();
extern Definition * ast_create_false_value();
extern Definition * ast_create_binding_value(int ref);
extern Definition * ast_create_prim_value(int pref);
extern void debug_parse_tree(FILE * f, Definition *def, int depth);

#endif
