
/*****
* cubealloc.c : memory allocation wrappers
*
* Copyright (C) 2004-2005 Frederic Peschanski
* All Rights Reserved
*
* This file is part of the eXdbm Library.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

#ifdef DEBUG_MEM

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "cubealloc.h"
#include "cubemisc.h"

static AllocEntry * AllocEntriesHead = NULL;
static AllocEntry * AllocEntriesLast = NULL;
static Bool DebugMemInit = FALSE;
static int AllocIDGenerator = 1;  // 0 is a reserved ID

static
void debug_mem_print_entry(FILE * f, AllocEntry* entry) {

  assert(entry!=NULL);

  fprintf(f,"Alloc ID = %ld\n", entry->alloc_id);
  switch(entry->type) {
  case ALLOC_SINGLE:
    fprintf(f,"Type = Single allocation entry\n");
    break;
  case ALLOC_ARRAY:
    fprintf(f,"Type = Array allocation entry\n");
    break;
  case REALLOC_ARRAY:
    fprintf(f,"Type = Array reallocation entry\n");
    break;
  default:
    fprintf(f,"Type = Unknown entry type (please report)\n");
  }
  fprintf(f,"Type name of allocation = %s\n", entry->type_name);
  fprintf(f,"Unit size = %ld\n",(long) entry->unit_size);
  if(entry->type!=ALLOC_SINGLE) 
    fprintf(f,"Number of elements = %ld\n", (long) entry->nb_elem);
  if(entry->type==ALLOC_ARRAY && entry->correct_free==FALSE && strcmp(entry->type_name,"char")==0) {
    int i=0;
    fprintf(f,"Char array contents = \"");
    for(i=0;i<entry->nb_elem;i++) {
      fprintf(f,"%c",((char *) entry->buff)[i]);
    }
    fprintf(f,"\"\n");
  }
  fprintf(f,"Source module = %s\n",entry->module_name);
  fprintf(f,"Source function = %s()\n",entry->fun_name);
  fprintf(f,"Source line number = %d\n",entry->line_no);
  fprintf(f,"Address of wrapped buffer = %p\n",entry->buff);
  if(entry->correct_free==TRUE) {
    fprintf(f,"Correctly freed at '%s:%s:%d'",entry->free_module_name, entry->free_fun_name,entry->free_line_no);
    if(entry->free_realloc==TRUE) {
      fprintf(f," (reallocation)\n");
    } else
      fprintf(f,"\n");
  }
  fprintf(f,"\n");
}

void debug_mem_check(FILE *out) {
  AllocEntry * cur = AllocEntriesHead->next;

  unsigned long nb_leaks=0;

  fprintf(out,"**** MEMORY CHECK START ****\n");

  while(cur!=NULL) {
    if(cur->correct_free==FALSE) {
      fprintf(out,"[%ld]=> Memory leak:\n",nb_leaks);
      debug_mem_print_entry(out,cur);
      nb_leaks++;
      fprintf(out,"----------------------------\n");
    }
    if(cur->next==NULL && cur!=AllocEntriesLast)
      fprintf(out,"=> Alloc wrapper corrupted (please report)\n");
    cur=cur->next;
  }

  fprintf(out,"\n=> Total leaks = %ld\n",nb_leaks);

  fprintf(out,"**** MEMORY CHECK END ****\n");
}

void debug_mem_log(FILE *out) {
  AllocEntry * cur = AllocEntriesHead->next;

  fprintf(out,"**** MEMORY LOG ****\n");

  while(cur!=NULL) {
    debug_mem_print_entry(out,cur);
    if(cur->next==NULL && cur!=AllocEntriesLast)
      fprintf(out,"=> Alloc wrapper corrupted (please report)\n");
    cur=cur->next;
    if(cur!=NULL && cur->next!=NULL)
      fprintf(out,"----------------------------\n");
  }

  fprintf(out,"**** MEMORY LOG END ****\n");
}

void debug_mem_error_gen(char * module, char *fun, int line_no, char *message, ...) {
  fprintf(stderr, "** Memory check error at %s:%s():%d **\n",module,fun,line_no);
  va_list args;

  fprintf(stderr,"==> ");
  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void debug_mem_error_entry(AllocEntry* entry, char *module, char *fun, int line_no, char *message, ...) {
  fprintf(stderr, "** Memory check error at %s:%s():%d **\n",module,fun,line_no);
  va_list args;

  fprintf(stderr,"==> ");
  va_start(args,message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");

  fprintf(stderr, "Referenced entry is :\n");
  debug_mem_print_entry(stderr, entry);
  fprintf(stderr,"\n");
  exit(EXIT_FAILURE);
}


/** Memory allocation debug module initialization
 *  Must be called once before first allocation
 **/
void debug_mem_init() {
  AllocEntriesHead = (AllocEntry *) malloc(sizeof(AllocEntry));
  if(AllocEntriesHead==NULL)
    fatal_error("cubealloc.c","debug_mem_init",__LINE__,"Cannot initialize debug_mem module, not enough memory");
  AllocEntriesHead->next=NULL;
  AllocEntriesHead->alloc_id = AllocIDGenerator;
  AllocEntriesHead->type = ALLOC_HEAD;
  AllocIDGenerator++;
  AllocEntriesLast=AllocEntriesHead;
  DebugMemInit=TRUE;
}

void debug_mem_exit() {
  AllocEntry *cur,*next;
  
  assert(DebugMemInit==TRUE);

  cur=AllocEntriesHead;
  next=cur->next;
  while(cur!=NULL) {
    free(cur);
    cur=next;
    if(cur!=NULL)
      next=cur->next;
  }

  AllocEntriesHead=NULL;
  AllocEntriesLast=NULL;
  DebugMemInit=FALSE;
}

/** Find a debug_mem entry
 *  @param buff address of allocated block
 *  @return address of debugging wrapper
 **/
static AllocEntry* debug_mem_find_from_pointer(void * buff) {
  
  // Warning : search only from address is not enough
  //           since we may reuse the same address
  //           so we should return the last entry in the
  //           list of buffers

  assert(DebugMemInit==TRUE && AllocEntriesHead!=NULL);

  AllocEntry * cur = AllocEntriesHead->next; // skip head
  AllocEntry * found = NULL; // not found yet
  unsigned long found_id = 0;

  while(cur!=NULL) {
    if(cur->buff==buff && cur->alloc_id!=found_id) { 
      // replace the found entry only if id changed
      found = cur;
      found_id = cur->alloc_id;
    }	
    cur=cur->next;
  }

  return found;
}

/** Debugging wrapper for allocation of single datum
 *  @param[in] unit_size : size of datum in bytes (use sizeof)
 *  @param[in] tname : name of type to allocate (user choice)
 *  @param[in] module : name of module where allocation if asked (C file)
 *  @param[in] fun : name of function where allocation if asked (C file)
 *  @param[in] line_no : line number of allocation code in module 
 *
 *  @return : the wrapped allocated block (as standard malloc)
 **/
void * debug_mem_alloc_single(size_t unit_size, char * tname, char *module, char *fun, int line_no) {
  // Preconditions
  assert(DebugMemInit==TRUE && AllocEntriesLast!=NULL);

  // Get the memory for the debug wrapper
  AllocEntriesLast->next = (AllocEntry *) malloc(sizeof(AllocEntry));
  if(AllocEntriesLast->next == NULL) 
    fatal_error("cubealloc.c","debug_mem_alloc_single",__LINE__,"Cannot allocate single debug_mem entry, not enough memory");

  // Set up the wrapper
  AllocEntriesLast=AllocEntriesLast->next;
  AllocEntriesLast->alloc_id = AllocIDGenerator;
  AllocIDGenerator++;
  AllocEntriesLast->type_name=tname;
  AllocEntriesLast->type=ALLOC_SINGLE;
  AllocEntriesLast->unit_size=unit_size;
  AllocEntriesLast->nb_elem=1;
  AllocEntriesLast->module_name=module;
  AllocEntriesLast->fun_name=fun;
  AllocEntriesLast->line_no=line_no;
  AllocEntriesLast->buff = malloc(unit_size);
  if(AllocEntriesLast->buff==NULL)
    fatal_error("cubealloc.c","debug_mem_alloc_single",__LINE__,"Cannot allocate wrapped debug_mem single buffer, not enough memory");
  AllocEntriesLast->correct_free=FALSE;
  AllocEntriesLast->free_module_name="";
  AllocEntriesLast->free_fun_name="";
  AllocEntriesLast->free_line_no=0;
  AllocEntriesLast->next=NULL;

  // return the wrapped buffer
  return AllocEntriesLast->buff;
}

/** Debugging wrapper for allocation of array of data
 *  @param[in] unit_size : size of datum in bytes (use sizeof)
 *  @param[in] nb_elem : number of elements to allocate
 *  @param[in] tname : name of type to allocate (user choice)
 *  @param[in] module : name of module where allocation is asked (C file)
 *  @param[in] fun : name of function where allocation is asked (C file)
 *  @param[in] line_no : line number of allocation cod in module 
 *
 *  @return : the wrapped allocated block (as standard malloc)
 **/
void * debug_mem_alloc_array(size_t unit_size, size_t nb_elem, char * tname, char *module, char *fun,int line_no) {
  // Preconditions
  assert(DebugMemInit==TRUE && AllocEntriesLast!=NULL);

  if(unit_size<=0)
    debug_mem_error_gen(module,fun,line_no,"Array entry unit size must be strictly positive, passed 0");

  if(nb_elem<=0)
    debug_mem_error_gen(module,fun,line_no,"Array size must be strictly positive, passed 0");

  // Get the memory for the debug wrapper
  AllocEntriesLast->next = (AllocEntry *) malloc(sizeof(AllocEntry));
  if(AllocEntriesLast->next == NULL) 
    fatal_error("cubealloc.c","debug_mem_alloc_array",__LINE__,"Cannot allocate array debug_mem entry, not enough memory");

  // Set up the wrapper
  AllocEntriesLast=AllocEntriesLast->next;
  AllocEntriesLast->alloc_id = AllocIDGenerator;
  AllocIDGenerator++;
  AllocEntriesLast->type_name=tname;
  AllocEntriesLast->type=ALLOC_ARRAY;
  AllocEntriesLast->unit_size=unit_size;
  AllocEntriesLast->nb_elem=nb_elem;
  AllocEntriesLast->module_name=module;
  AllocEntriesLast->fun_name=fun;
  AllocEntriesLast->line_no=line_no;
  AllocEntriesLast->buff = malloc(unit_size*nb_elem);
  if(AllocEntriesLast->buff==NULL)
    fatal_error("cubealloc.c","debug_mem_alloc_array",__LINE__,"Cannot allocate wrapped debug_mem array buffer, not enough memory");
  AllocEntriesLast->correct_free=FALSE;
  AllocEntriesLast->free_module_name="";
  AllocEntriesLast->free_fun_name="";
  AllocEntriesLast->free_line_no=0;
  AllocEntriesLast->next=NULL;

  // return the wrapped buffer
  return AllocEntriesLast->buff;
}

/** Debugging wrapper for allocation of array of data with reset
 *  @param[in] unit_size : size of datum in bytes (use sizeof)
 *  @param[in] nb_elem : number of elements to allocate
 *  @param[in] tname : name of type to allocate (user choice)
 *  @param[in] module : name of module where allocation is asked (C file)
 *  @param[in] fun : name of function where allocation is asked (C file)
 *  @param[in] line_no : line number of allocation cod in module 
 *
 *  @return : the wrapped allocated block (as standard calloc)
 **/
void * debug_mem_alloc_array_reset(size_t unit_size, size_t nb_elem, char * tname, char *module, char *fun,int line_no) {
  // Preconditions
  assert(DebugMemInit==TRUE && AllocEntriesLast!=NULL);

  if(unit_size<=0)
    debug_mem_error_gen(module,fun,line_no,"Array entry unit size must be strictly positive, passed 0");

  if(nb_elem<=0)
    debug_mem_error_gen(module,fun,line_no,"Array size must be strictly positive, passed 0");
  // Get the memory for the debug wrapper
  AllocEntriesLast->next = (AllocEntry *) malloc(sizeof(AllocEntry));
  if(AllocEntriesLast->next == NULL) 
    fatal_error("cubealloc.c","debug_mem_alloc_array_reset",__LINE__,"Cannot allocate array debug_mem entry, not enough memory");

  // Set up the wrapper
  AllocEntriesLast=AllocEntriesLast->next;
  AllocEntriesLast->alloc_id = AllocIDGenerator;
  AllocIDGenerator++;
  AllocEntriesLast->type_name=tname;
  AllocEntriesLast->type=ALLOC_ARRAY;
  AllocEntriesLast->unit_size=unit_size;
  AllocEntriesLast->nb_elem=nb_elem;
  AllocEntriesLast->module_name=module;
  AllocEntriesLast->fun_name=fun;
  AllocEntriesLast->line_no=line_no;
  AllocEntriesLast->buff = calloc(nb_elem,unit_size);
  if(AllocEntriesLast->buff==NULL)
    fatal_error("cubealloc.c","debug_mem_alloc_array_reset",__LINE__,"Cannot allocate wrapped debug_mem array buffer, not enough memory");
  AllocEntriesLast->correct_free=FALSE;
  AllocEntriesLast->free_module_name="";
  AllocEntriesLast->free_fun_name="";
  AllocEntriesLast->free_line_no=0;
  AllocEntriesLast->next=NULL;

  // return the wrapped buffer
  return AllocEntriesLast->buff;
}

/** Debugging wrapper for reallocation of array of data
 *  @param[in] unit_size : size of datum in bytes (use sizeof)
 *  @param[in] old_nb_elem : number of elements previously allocated
 *  @param[in] new_nb_elem : number of elements to allocate
 *  @param[in] tname : name of type to allocate (user choice)
 *  @param[in] module : name of module where reallocation is asked (C file)
 *  @param[in] fun : name of function where reallocation is asked (C file)
 *  @param[in] line_no : line number of reallocation code in module 
 *
 *  @return : the wrapped reallocated block (as standard malloc)
 **/
void * debug_mem_realloc_array(void* buff, size_t unit_size, size_t old_nb_elem, size_t new_nb_elem, char * tname, char *module, char *fun, int line_no) {

  if(unit_size<=0)
    debug_mem_error_gen(module,fun,line_no,"Array entry unit size must be strictly positive, passed 0");

  if(new_nb_elem<=0)
    debug_mem_error_gen(module,fun,line_no,"Array size must be strictly positive, passed 0");

  // search the first entry for the buffer
  AllocEntry* bufentry = debug_mem_find_from_pointer(buff);
  if(bufentry==NULL)
    debug_mem_error_gen(module,fun,line_no, "Try to reallocate unregistered buffer");

  // then go to the last possible reallocation
  unsigned long id = bufentry->alloc_id;
  AllocEntry* next = bufentry->next;
  while(next!=NULL && next->alloc_id==id) {
    bufentry=next;
    next=next->next;
  }

  // check if buffer pointer changed
  if(bufentry->buff!=buff)
    debug_mem_error_entry(bufentry, module, fun,line_no, "Cannot reallocate : Does not match with previous allocation pointer  = '%p' (new is '%p')",buff,bufentry->buff);

  // check if not freed
  if(bufentry->correct_free==TRUE) 
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot reallocate : Already freed buffer");

  // check if matching entries
  if(bufentry->unit_size!=unit_size)
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot reallocate : Unit sizes do not match (given '%ld', expected '%ld')",unit_size,bufentry->unit_size);
  if(bufentry->nb_elem!=old_nb_elem) 
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot reallocate : Previous number of elements does not match (given '%ld', expected '%ld')",old_nb_elem,bufentry->nb_elem);
  if(bufentry->nb_elem==new_nb_elem) 
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot reallocate : New number of elements does not change (given '%ld', was '%ld')",new_nb_elem,bufentry->nb_elem);
  if(strcmp(bufentry->type_name,tname)!=0)
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot reallocate : Type name does not match (given '%s', expected '%s')",tname,bufentry->type_name);
  
  // create a new entry for the reallocation record
  AllocEntry * newentry = (AllocEntry *) malloc(sizeof(AllocEntry));
  if(newentry == NULL) 
    fatal_error("cubealloc.c","debug_mem_realloc_array",__LINE__,"Cannot reallocate array debug_mem entry, not enough memory");
  
  newentry->alloc_id=bufentry->alloc_id;
  newentry->type_name=tname;
  newentry->type=REALLOC_ARRAY;
  newentry->unit_size=unit_size;
  newentry->nb_elem=new_nb_elem;
  newentry->module_name=module;
  newentry->fun_name=fun;
  newentry->line_no=line_no;
  newentry->buff = realloc(buff,unit_size*new_nb_elem);
  if(newentry->buff==NULL)
    fatal_error("cubealloc.c","debug_mem_realloc_array",__LINE__,"Cannot reallocate wrapped debug_mem array buffer, not enough memory");
  newentry->correct_free=FALSE;
  newentry->free_module_name="";
  newentry->free_fun_name="";
  newentry->free_line_no=0;
  newentry->free_realloc=FALSE;
  newentry->next=bufentry->next;
  if(newentry->next==NULL)
    AllocEntriesLast=newentry;

  // update the parent entry
  bufentry->buff= NULL;
  bufentry->next = newentry;
  bufentry->correct_free=TRUE;
  bufentry->free_module_name=module;
  bufentry->free_fun_name=fun;
  bufentry->free_line_no=line_no;
  bufentry->free_realloc=TRUE;

  // return the wrapped buffer
  return newentry->buff;
}

/** Debugging wrapper for collection of array of data
 *  @param[in] unit_size : size of datum in bytes (use sizeof)
 *  @param[in] old_nb_elem : previous number of elements in the array
 *  @param[in] tname : name of type allocated (user choice)
 *  @param[in] module : name of module where collection is asked (C file)
 *  @param[in] fun : name of function where collection is asked (C file)
 *  @param[in] line_no : line number of collection code in module 
 **/
void debug_mem_free_array(void* buff, size_t unit_size, size_t old_nb_elem, char * tname, char *module, char * fun, int line_no) {

  // search the first entry for the buffer
  AllocEntry* bufentry = debug_mem_find_from_pointer(buff);
  if(bufentry==NULL)
    debug_mem_error_gen(module,fun, line_no, "Try to free unregistered buffer");

  // then go to the last possible reallocation
  unsigned long id = bufentry->alloc_id;
  AllocEntry* next = bufentry->next;
  while(next!=NULL && next->alloc_id==id) {
    bufentry=next;
    next=next->next;
  }

  // check if buffer pointer changed
  if(bufentry->buff!=buff)
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Does not match with previous allocation pointer  = '%p' (new is '%p')",buff,bufentry->buff);

  // check if array allocation
  if(bufentry->type==ALLOC_SINGLE) 
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free: array free of a single data");

  // check if not freed
  if(bufentry->correct_free==TRUE) 
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Already freed buffer");

  // check if matching entries
  if(bufentry->unit_size!=unit_size)
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Unit sizes do not match (given '%ld', expected '%ld')",unit_size,bufentry->unit_size);
  if(bufentry->nb_elem!=old_nb_elem) 
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Previous number of elements does not match (given '%ld', expected '%ld')",old_nb_elem,bufentry->nb_elem);
  if(strcmp(bufentry->type_name,tname)!=0)
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Type name does not match (given '%s', expected '%s')",tname,bufentry->type_name);
  
  free(buff);
  bufentry->buff=NULL;
  bufentry->correct_free=TRUE;
  bufentry->free_module_name=module;
  bufentry->free_fun_name=fun;
  bufentry->free_line_no=line_no;
  bufentry->free_realloc=FALSE;
}

/** Debugging wrapper for collection of single data
 *  @param[in] unit_size : size of datum in bytes (use sizeof)
 *  @param[in] tname : name of type allocated (user choice)
 *  @param[in] module : name of module where collection is asked (C file)
 *  @param[in] fun : name of function where collection is asked (C file)
 *  @param[in] line_no : line number of collection code in module 
 **/
void debug_mem_free_single(void* buff, size_t unit_size, char * tname, char *module, char * fun, int line_no) {

  // search the first entry for the buffer
  AllocEntry* bufentry = debug_mem_find_from_pointer(buff);
  if(bufentry==NULL)
    debug_mem_error_gen(module,fun,line_no, "Try to free unregistered buffer");

  // then go to the last possible reallocation
  unsigned long id = bufentry->alloc_id;
  AllocEntry* next = bufentry->next;
  while(next!=NULL && next->alloc_id==id) {
    bufentry=next;
    next=next->next;
  }

  // check if buffer pointer changed
  if(bufentry->buff!=buff)
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Does not match with previous allocation pointer  = '%p' (new is '%p')",buff,bufentry->buff);

  // check if single allocation
  if(bufentry->type!=ALLOC_SINGLE) 
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free: single free of an array");

  // check if not freed
  if(bufentry->correct_free==TRUE) 
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Already freed buffer");

  // check if matching entries
  if(bufentry->unit_size!=unit_size)
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Unit sizes do not match (given '%ld', expected '%ld')",unit_size,bufentry->unit_size);
  if(strcmp(bufentry->type_name,tname)!=0)
    debug_mem_error_entry(bufentry, module, fun, line_no, "Cannot free : Type name does not match (given '%s', expected '%s')",tname,bufentry->type_name);
  
  free(buff);
  bufentry->buff=NULL;
  bufentry->correct_free=TRUE;
  bufentry->free_module_name=module;
  bufentry->free_fun_name=module;
  bufentry->free_line_no=line_no;
  bufentry->free_realloc=FALSE;
}

#endif
