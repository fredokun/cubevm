
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "cubeparse.h"
#include "cubeast.h"
#include "cubesched.h"
#include "cubealloc.h"
#include "cubemisc.h"

#ifdef RUNTIME_STAT
#include "cubestat.h"
#endif

void print_presentation(void) {
  printf("---    ______                                                      \n");
  printf("      /\\     \\           ___                     _           _    \n");
  printf("     /  \\_____\\         (  _`\\                _ (_ )        ( )_  \n");
  printf("   __\\  /  ___/__       | ( (_)   _    _ _   (_) | |    _   | ,_) \n");    
  printf("  /\\  \\/__/\\     \\      | |  _  /'_`\\ ( '_`\\ | | | |  /'_`\\ | |   \n");
  printf(" /  \\_____\\ \\_____\\     | (_( )( (_) )| (_) )| | | | ( (_) )| |_  \n"); 
  printf(" \\  /     / /     /     (____/'`\\___/'| ,__/'(_)(___)`\\___/'`\\__) \n"); 
  printf("  \\/_____/\\/_____/jiri                | |                         \n");
  printf("                                      (_)   Version 0.51        \n\n");  
  printf("        -- powered by I.S. + CubeVM engine                         \n");
  printf("        (C) 2004 - Frederic Peschanski                             \n");
  printf("---\n");
}

void print_usage(void) {
  printf("Usage : copilot [--ast --dparse] <file>.pi\n");
  printf("  Options:\n");
  printf("      --ast    : generate abstract syntax tree on stderr\n");
  printf("      --dparse : generate parsing trace\n");
  printf("      --stat   : generate statistics on stdout\n");
  printf("\n\n");
}

void command_error(char * message, ...) {
  fprintf(stderr, "** Error **\n");
  va_list args;

  fprintf(stderr,"==> ");
  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n\n");
  print_usage();
  exit(EXIT_FAILURE);
}

void print_statistics(Scheduler *sched) {
  printf("Basic memory usage : \n");
  printf("  Nominal size of channel = %d bytes\n", sizeof(Channel));
  printf("  Reference size of channel = %d bytes\n", sizeof(Channel *));
  printf("  Nominal size of process = %d bytes\n", sizeof(Process));
  printf("  Reference size of process = %d bytes\n\n", sizeof(Process *));

#ifdef RUNTIME_STAT
  print_runtime_stats(stdout,sched);
#endif
}

int main(int argc, char*args[]) {

  Bool debug_ast = FALSE;
  Bool debug_parse = FALSE;
  Bool show_stats = FALSE;
  char *filename = NULL;

  // initialize the memory allocation wrapper
  MEM_INIT();

  print_presentation();
  
  int i;
  for(i=1;i<argc;i++) {
    //printf("args[%d]=%s\n",i,args[i]);
    if(strcmp(args[i],"--ast")==0)
      debug_ast = TRUE;
    else if(strcmp(args[i],"--dparse")==0)
      debug_parse = TRUE;
    else if(strcmp(args[i],"--stat")==0)
      show_stats = TRUE;
    else if(args[i][0]=='-')
      command_error("Unknown option '%s'",args[i]);
    else if(filename==NULL)
      filename = args[i];
  }

  if(strcmp(filename,"")==0)
    command_error("Missing file name");
  else
    printf("*** Compiling and run file '%s'\n", filename);
  
  if(debug_ast)
    printf("*** Generate Abstract Syntax Tree on stderr\n");

  if(debug_parse) {
    printf("*** Generate parse traces on stderr\n");
  }
  
  Definition *expr = parse_filename(filename,debug_parse);  
  if(expr!=NULL)
    fprintf(stderr, "==> Compilation successful\n");
  else
    fatal_error("copilot.c","main",__LINE__,"Missing top-level expression");

  if(debug_ast==TRUE) {
  
  int i;
  for(i=1;i<GLOBAL_nb_definitions;i++) {
    fprintf(stderr, "================ DEFINITION '%s' ==============\n\n",GLOBAL_definitions[i]->name);
    
    debug_parse_tree(stderr,GLOBAL_definitions[i]->def,0);
    
    fprintf(stderr, "====================================================\n\n");
  }
  
  fprintf(stderr, "================ TOP LEVEL DEFINITION ==============\n\n");
  
  debug_parse_tree(stderr,expr,0);

  fprintf(stderr, "====================================================\n\n");

  }
  
  // now run the program

  printf("*** Initializing CubeVM engine\n");
  
  Scheduler *sched = scheduler_create(&GLOBAL_string_pool,GLOBAL_definitions,GLOBAL_definitions_size,GLOBAL_nb_definitions);

  printf("*** Create root process\n");

  Process * root = spawn_root(sched, expr);

  sched->argc = argc;
  sched->argv = args;
  
  scheduler_add_proc(sched, root);

  printf("*** Start scheduling\n");

  schedule_run(sched);

  printf("==> End of scheduling\n\n");
  
  if(show_stats)
    print_statistics(sched);

  GLOBAL_definition_cleanup(expr);
  GLOBAL_all_definitions_cleanup();

  StringPool* sp = sched->string_pool;
  sched = scheduler_free(sched);
  sp = reclaim_string_pool(sp);

  MEM_CHECK(stderr);
  
  MEM_EXIT();

  return 0;
}
