
#ifndef CUBE_PARSE_H
#define CUBE_PARSE_H

#include <stdio.h>

#include "cubestr.h"
#include "cubesched.h"
#include "cubeparse.tab.h"

// Reserved keywords
extern char *GLOBAL_reserved_ident[];
extern int GLOBAL_keyword[];
extern int GLOBAL_nb_reserved_ident;

extern int GLOBAL_get_keyword(char *str);

extern unsigned int GLOBAL_line_no;
extern char **GLOBAL_lex_env;
extern int GLOBAL_lex_env_size;

extern Definition * GLOBAL_top_expr;

extern StringPool GLOBAL_string_pool;

extern void parse_error(char * message, ...);
extern void lexer_error(char * message, ...);

extern Definition * parse_file(FILE * f, Bool debug);
extern Definition * parse_filename(char *filename, Bool debug);
extern Definition * parse(Bool debug);

#endif
