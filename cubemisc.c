
#include <stdlib.h>
#include <stdio.h>

#include <stdarg.h>

#include "cubemisc.h"

/********************************************************
 * MISC. UTILITIES
 ********************************************************/

#ifdef PROC_TRACE
void proc_trace(unsigned long id, char *message,int line) {
  fprintf(stderr, "[Proc #%ld] : %s [%d]\n", id,message,line);
}
void pseudo_proc_trace(unsigned long id, unsigned long pseudo_id, char *message, int line) {
  fprintf(stderr, "[PProc #%ld->%ld] : %s [%d]\n", id,pseudo_id,message,line);
}
#endif

void fatal_error(char *module, char *fun, int line_no, char * message, ...) {
  fprintf(stderr, "** Fatal Error (%s:%s:%d) **\n",module,fun,line_no);
  va_list args;

  fprintf(stderr,"==> ");
  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void warning(char *module, char *fun, int line_no, char * message, ...) {
  fprintf(stderr, "** Warning (%s:%s:%d) **\n==> ",module,fun,line_no);
  va_list args;

  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");
}

void runtime_error(Definition *def, char * message, ...) {
  va_list args;

  fprintf(stderr,"Runtime error in '%s' at line %d \n ==> ",
	  def_print_name(def),
	  def->line_no);
  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void runtime_warning(Definition *def, char * message, ...) {
  va_list args;

  fprintf(stderr,"Runtime warning in '%s' at line %d \n ==> ",
	  def_print_name(def),
	  def->line_no);
  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");
}
