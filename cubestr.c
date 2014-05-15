
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <assert.h>

#include "cubestr.h"
#include "cubemisc.h"
#include "cubealloc.h"
#include "cubecfg.h"

void init_string_pool(StringPool * sp) {
  sp->size = 0;
  sp->max_size = GLOBAL_CFG_AS_ULONG(CFG_START_MAX_STRINGS);
  sp->pool = (String *) MEM_ALLOC_ARRAY_RESET(sizeof(String),sp->max_size,"String","cubestr.c","init_string_spool");
  if(sp->pool==NULL)
    fatal_error("cubestr.c","init_string_pool",__LINE__,"Cannot allocate the string pool");
}

StringPool * reclaim_string_pool(StringPool *sp) {
  int i=0;
  for(i=0;i<sp->max_size;i++) {
    if(sp->pool[i].str!=NULL && sp->pool[i].ref_count>=0) {
      MEM_FREE_ARRAY(sp->pool[i].str,sizeof(char),sp->pool[i].size+1,"char","cubestr.c","reclaim_string_pool");
    }
  }
  MEM_FREE_ARRAY(sp->pool,sizeof(String),sp->max_size, "String", "cubestr.c","reclaim_string_pool");
  return NULL;
}

void grow_string_pool(StringPool * sp) {

  // PRECONDITION: number of strings is max
  assert(sp->size>=sp->max_size);

  unsigned long old_max_size = sp->max_size;
  sp->max_size+=GLOBAL_CFG_AS_ULONG(CFG_GROW_STRINGS_FACTOR);
  sp->pool = (String *) MEM_REALLOC_ARRAY(sp->pool, sizeof(String),old_max_size,sp->max_size,"String","cubestr.c","grow_string_spool");

  if(sp->pool==NULL)
    fatal_error("cubestr.c","grow_string_pool",__LINE__,"Cannot grow the string pool");
  int i;
  for(i=sp->size;i<sp->max_size;i++) {
    sp->pool[i].str = NULL;
    sp->pool[i].size = -1;
    sp->pool[i].ref_count = 0;
  }
}

StringRef str_make(StringPool * sp, char *str, int size, int ref_count) {
  String ret;
  ret.str = str;
  ret.size = size;
  ret.ref_count = ref_count;

  // first look for an empty entry (with size=-1)
  int i;
  for(i=0;i<sp->size;i++) {
    if(sp->pool[i].size==-1) {
      sp->pool[i] = ret;
      return i;
    }
  }
  
  // if none, add at the end of the string pool
  if(sp->size==sp->max_size)
    grow_string_pool(sp);
  
  sp->pool[sp->size] = ret;
  sp->size++;
  return sp->size-1;
  }

void str_reclaim(StringPool * sp, StringRef s) {
  if(STRREFCOUNT(sp,s)==0) { // no more reference to the string
    MEM_FREE_ARRAY(sp->pool[s].str,sizeof(char),strlen(sp->pool[s].str)+1,"char","cubestr.c","str_reclaim");
    sp->pool[s].str = NULL;
    sp->pool[s].size = -1; // marker for empty reference
  }
}
  
StringRef str_add(StringPool * sp, StringRef s1, StringRef s2) {
  String sr1 = STRGET(sp,s1);
  String sr2 = STRGET(sp,s2);
  
  char * str = (char *) MEM_ALLOC_ARRAY(sizeof(char),sr1.size+sr2.size+1,"char","cubestr.c","str_add");
  if(str==NULL)
    fatal_error("cubestr.c","str_add",__LINE__,"Cannot allocate result string for string addition");

  int size = sr1.size + sr2.size;
  strncpy(str,sr1.str,sr1.size+1);
  str = strncat(str,sr2.str,sr2.size);

  return str_make(sp,str,size,0); // by default, ref_count is null, may be reclaimed
}

StringRef str_add_cstr_1(StringPool * sp, char * sr1, StringRef s2) {
  String sr2 = STRGET(sp,s2);
  int sr1_size = strlen(sr1);
  
  char * str = (char *) MEM_ALLOC_ARRAY(sizeof(char),sr1_size+sr2.size+1,"char","cubestr.c","str_add_cstr_1");
  if(str==NULL)
    fatal_error("cubestr.c","str_add_cstr_1",__LINE__,"Cannot allocate result string for string addition (cstr 1)");

  int size = sr1_size + sr2.size;
  strncpy(str,sr1,sr1_size+1);
  strncat(str,sr2.str,sr2.size);

  return str_make(sp,str,size,0); // by default, ref_count is null, may be reclaimed
}

StringRef str_add_cstr_2(StringPool * sp, StringRef s1, char* sr2) {
  String sr1 = STRGET(sp,s1);
  int sr2_size = strlen(sr2);

  char * str = NULL;
  str = (char *) MEM_ALLOC_ARRAY(sizeof(char),sr1.size+sr2_size+1,"char","cubestr.c","str_add_cstr_2");
  if(str==NULL)
    fatal_error("cubestr.c","str_add_cstr_2",__LINE__,"Cannot allocate result string for string addition (cstr 2)");

  int size = sr1.size + sr2_size;
  str = strncpy(str,sr1.str,sr1.size+1);
  str = strncat(str,sr2,sr2_size);

  return str_make(sp,str,size,0); // by default, ref_count is null, may be reclaimed
}

char *cstr_from_int(int val) {
  char buffer[1000]; // long enough !
  snprintf(buffer,999,"%d",val);
  
  int size = strlen(buffer);
  char* str = (char *) MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubestr.c","cstr_from_int");
  if(str==NULL)
    fatal_error("cubestr.c","cstr_from_int",__LINE__,"Cannot allocate memory for return c-string while converting from int");
  strcpy(str,buffer);
  return str;
}

int int_from_cstr(char *str) {
  int ret = atoi(str);
  return ret;
}

StringRef str_from_int(StringPool * sp, int val) {
  char buffer[1000]; // long enough !
  snprintf(buffer,999,"%d",val);

  int size = strlen(buffer);
  char* str = (char *) MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubestr.c","str_from_int");
  if(str==NULL)
    fatal_error("cubestr.c","str_from_int",__LINE__,"Cannot allocate memory for return string while converting from int");
  return str_make(sp,str,size,0);
}

char* cstr_from_real(double val) {
  char buffer[1000]; // long enough !
  snprintf(buffer,999,"%lf",val);

  int size = strlen(buffer);
  char *str = (char *) MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubestr.c","cstr_from_real");
  if(str==NULL)
    fatal_error("cubestr.c","cstr_from_real",__LINE__,"Cannot allocate memory for return c-string while converting from real");
  strncpy(str,buffer,size+1);
  return str;
}

double real_from_cstr(char *str) {
  char *end_ptr;
  double ret;
  ret = strtod(str,&end_ptr);
  if(ret==HUGE_VAL || ret==-HUGE_VAL) {
    fatal_error("cubestr.c","real_from_cstr",__LINE__,"Cannot parse double from string '%s', overflow",str);
  } else if(ret==0 && errno==ERANGE) {
    fatal_error("cubestr.c","real_from_cstr",__LINE__,"Cannot parse double from string '%s', underflow",str);    
  } else if(end_ptr==str) {
    fatal_error("cubestr.c","real_from_cstr",__LINE__,"Cannot parse double from string '%s'", str);
  }

  return ret;
}

StringRef str_from_real(StringPool * sp, double val) {
  char buffer[1000]; // long enough !
  snprintf(buffer,999,"%lf",val);

  int size = strlen(buffer);
  char *str = (char *) MEM_ALLOC_ARRAY(sizeof(char),size+1,"char","cubestr.c","str_from_real");
  if(str==NULL)
    fatal_error("cubestr.c","str_from_real",__LINE__,"Cannot allocate memory for return string while converting from real");
  return str_make(sp,str,size,0);
}

