
#include <stdlib.h>
#include <string.h>

#include "cubedef.h"
#include "cubealloc.h"
#include "cubecfg.h"
#include "cubemisc.h"

DefEntry ** GLOBAL_definitions = NULL;
int GLOBAL_nb_definitions = 0;
int GLOBAL_definitions_size = 0;

char * GLOBAL_def_type_name[] = { "None", "End prefix", "Input prefix", "Output prefix",
				  "New prefix", "Parallel operator", "Choice operator",
				  "Call expression", "Definition Header", "Primitive call",
				  "Value expression", "If expression",
				  "Reactive process", "Joker call", "Let prefix", 
                                  "Own prefix", "Self own prefix", "Jump prefix", 
                                  "Relative jump prefix" };

int GLOBAL_nb_def_types = 19;

char * def_print_name(Definition *def) {
  if(def->type<0 || def->type>=GLOBAL_nb_def_types)
    return "Unknown type";
  else
    return GLOBAL_def_type_name[def->type];
}

void GLOBAL_definitions_init() {
  GLOBAL_definitions = (DefEntry **) MEM_ALLOC_ARRAY(sizeof(DefEntry *),GLOBAL_CFG_AS_ULONG(CFG_START_MAX_DEFS),"DefEntry*","cubedef.c","GLOBAL_definitions_init");
  if(GLOBAL_definitions==NULL)
    fatal_error("cubedef.c","GLOBAL_definitions_init",__LINE__,"Cannot allocate room for global definitions");
  GLOBAL_definitions_size = GLOBAL_CFG_AS_ULONG(CFG_START_MAX_DEFS);
  // first definition (index 0) is reserved   (and NULL)
  GLOBAL_definitions[0] = NULL;
  GLOBAL_nb_definitions = 1;
}

unsigned long GLOBAL_definitions_add(Definition *def, char *name) {
  if(GLOBAL_nb_definitions==GLOBAL_definitions_size) {
    // add new space if necessary
    GLOBAL_definitions = (DefEntry **) MEM_REALLOC_ARRAY(GLOBAL_definitions, sizeof(DefEntry *),GLOBAL_nb_definitions,GLOBAL_nb_definitions+GLOBAL_CFG_AS_ULONG(CFG_GROW_DEFS_FACTOR),"DefEntry*","cubedef.c","GLOBAL_definitions_add");
    if(GLOBAL_definitions==NULL)
      fatal_error("cubedef.c","GLOBAL_definitions_add",__LINE__,"Cannot allocate for growing global definitions");
    GLOBAL_definitions_size += GLOBAL_CFG_AS_ULONG(CFG_GROW_DEFS_FACTOR);
  }

  // add the definition
  DefEntry * newdef = (DefEntry *) MEM_ALLOC_SINGLE(sizeof(DefEntry),"DefEntry","cubedef.c","GLOBAL_definitions_add");

  newdef->id = GLOBAL_nb_definitions;
  newdef->def = def;
  newdef->name = name;
  def->id = newdef->id;
  
  GLOBAL_definitions[GLOBAL_nb_definitions] =  newdef;
  GLOBAL_nb_definitions++;

  return newdef->id;
}

unsigned long GLOBAL_definitions_fetch(char *name) {
  unsigned long i;
  for(i=1;i<GLOBAL_nb_definitions;i++)
    if(strcmp(GLOBAL_definitions[i]->name,name)==0)
      return i;
  return 0;
}

void GLOBAL_definition_cleanup(Definition *def) {
  // first cleanup the children
  if(def->children!=NULL) {
    unsigned int i;
    for(i=0;i<def->nb_children;i++) {
      GLOBAL_definition_cleanup(def->children[i]);
      def->children[i]=NULL;
    }
    MEM_FREE_ARRAY(def->children,sizeof(Definition*),def->nb_children,"Definition*","cubedef.c","definition_cleanup");
    def->children=NULL;
    def->nb_children=0;
  }

  // then cleanup the definition itself
  MEM_FREE_SINGLE(def,sizeof(Definition),"Definition","cubedef.c","definitions_cleanup");
  def=NULL;
}

void GLOBAL_all_definitions_cleanup() {
  int i;
  // Note: definition 0 is reserved and NULL (so no need to cleanup)
  for(i=1;i<GLOBAL_nb_definitions;i++) {
    DefEntry * def = GLOBAL_definitions[i];
    GLOBAL_definition_cleanup(def->def);
    MEM_FREE_ARRAY(def->name,sizeof(char),strlen(def->name)+1,"char","cubedef.c","GLOBAL_definitions_cleanup");
    MEM_FREE_SINGLE(def,sizeof(DefEntry),"DefEntry","cubedef.c","GLOBAL_definitions_cleanup");
    def = NULL;
  }
  MEM_FREE_ARRAY(GLOBAL_definitions,sizeof(DefEntry*),GLOBAL_definitions_size,"DefEntry*","cubedef.c","GLOBAL_definitions_cleanup");
  GLOBAL_definitions = NULL;
  GLOBAL_nb_definitions = 0;
  GLOBAL_definitions_size = 0;
}
