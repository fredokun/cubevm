
#include "cubefreeze.h"

long freeze_string(char *str, FILE * f) {
  int size = strlen(str);
  size_t res = fwrite(f,&size,sizeof(int),1,f);
  if(res!=1) {
    fatal_error("");
  }
  res = fwrite(f,str,sizeof(char),size+1,f);
  if(res<size+1) {
    fatal_error("");
  }
}

char * melt_string(FILE * f, long &feedback) {
  int size;
  size_t res = fread(&size,sizeof(int),1,f);
  if(res!=1) {
    fatal_error("");
  }
  char * str = MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubefreeze.c","melt_string");
  if(str=NULL) {
    fatal_error("");
  }
  res = fread(str,sizeof(char),size+1,f);
  if(res!=size+1) {
    fatal_error("");
  }
  // CONDITION : TRAILING ZERO EXISTS
  assert(str[size+1]==(char) 0);

  return str;
}

int freeze_uint32(uint32 val, FILE * f) {
  val = FREEZE_UINT32(val);
  int res = fwrite(&val,sizeof(uint32),1,f);
  return res;
}

long freeze_definition(Definition *def, FILE *f) {
  int res = freeze_uint32(def->id,f);
  if(res!=1) {
    fatal_error("cubefreeze.c","freeze_definition",__LINE__,"Unable to freeze definition id");
  }
  int res = freeze_uint32(def->call_id,f);
  if(res!=1) {
    fatal_error("cubefreeze.c","freeze_definition",__LINE__,"Unable to freeze definition call id");
  }
}

long freeze_definitions(DefEntry *defs, int defs_size, int nb_defs, FILE * f) {

}

Scheduler * melt_definitions(FILE *f, long &feedback) {

}

long freeze_string_pool(StringPool* sp, FILE *f) {

}

StringPool * melt_string_pool(FILE *f, long &feedback) {

}

long freeze_scheduler(Scheduler * sched, FILE * f) {

}

Scheduler * melt_scheduler(FILE *f, long &feedback) {

}
