
#ifndef CUBE_MISC_H
#define CUBE_MISC_H

#include "cubedef.h"

#ifdef PROC_TRACE
#define PTRACE(p,m) proc_trace(p,m,__LINE__)
#else
#define PTRACE(p,m)
#endif

#ifdef PROC_TRACE
#define PPTRACE(p,pp,m) pseudo_proc_trace(p,pp,m,__LINE__)
#else
#define PPTRACE(p,pp,m)
#endif

extern void fatal_error(char * module, char * fun, int line_no, char * message, ...);
extern void warning(char * module, char * fun, int line_no, char * message, ...);
extern void proc_trace(unsigned long id, char *message,int line);
extern void pseudo_proc_trace(unsigned long id, unsigned long pseudo_id, char *message, int line);
extern void runtime_error(Definition *def, char * message, ...);
extern void runtime_warning(Definition *def, char * message, ...);
#endif


