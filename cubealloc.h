/*****
* cubealloc.h : memory allocation wrappers
/*****
/*****
* Copyright (C) 2004-2005 Frederic Peschanski
* All Rights Reserved
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

#ifndef D_ALLOC_H
#define D_ALLOC_H

#include <stdlib.h>
#include <stdio.h>

#include "cubeglobals.h"

#ifdef DEBUG_MEM

typedef enum { ALLOC_HEAD, ALLOC_SINGLE, ALLOC_ARRAY, REALLOC_ARRAY } AllocType;

typedef struct _AllocEntry {
  unsigned long alloc_id;
  char * type_name; // C type name
  AllocType type; // the type of allocation
  size_t unit_size;
  size_t nb_elem;
  char * module_name; // name of module making the allocation
  char * fun_name;  // function name
  unsigned int line_no; // line number of allocation
  void * buff; // the real buffer
  Bool correct_free; // correcty freed ?
  char * free_module_name; // module name of free call
  char * free_fun_name; // function name of free call
  int free_line_no; // line number of free call
  Bool free_realloc; // implicit free throuh realloc ?
  struct _AllocEntry *next; // the next entry
} AllocEntry;

extern void debug_mem_init();
extern void debug_mem_check(FILE *out);
extern void debug_mem_log(FILE *out);
extern void debug_mem_exit();
extern void * debug_mem_alloc_single(size_t unit_size, char * tname, char *module, char *fun, int line_no);
extern void * debug_mem_alloc_array(size_t unit_size, size_t nb_elem, char * tname, char *module, char* fun, int line_no);
extern void * debug_mem_alloc_array_reset(size_t unit_size, size_t nb_elem, char * tname, char *module, char* fun, int line_no);
extern void * debug_mem_alloc_array_reset(size_t unit_size, size_t nb_elem, char * tname, char *module, char *fun, int line_no);
extern void * debug_mem_realloc_array(void* buff, size_t unit_size, size_t old_nb_elem, size_t new_nb_elem, char * tname, char *module, char *fun, int line_no);
extern void debug_mem_free_array(void* buff, size_t unit_size, size_t old_nb_elem, char * tname, char *module, char *fun, int line_no);
extern void debug_mem_free_single(void* buff, size_t unit_size, char * tname, char *module, char *fun, int line_no);

#define MEM_INIT() debug_mem_init()
#define MEM_CHECK(out) debug_mem_check(out)
#define MEM_LOG(out) debug_mem_log(out)
#define MEM_EXIT() debug_mem_exit()
#define MEM_ALLOC_SINGLE(unit_size,tname,module,fun) debug_mem_alloc_single(unit_size,tname,module,fun,__LINE__)
#define MEM_ALLOC_ARRAY(unit_size,nb_elem,tname,module,fun) debug_mem_alloc_array(unit_size,nb_elem,tname,module,fun,__LINE__)
#define MEM_ALLOC_ARRAY_RESET(unit_size,nb_elem,tname,module,fun) debug_mem_alloc_array_reset(unit_size,nb_elem,tname,module,fun,__LINE__)
#define MEM_REALLOC_ARRAY(buff,unit_size,old_nb,new_nb,tname,module,fun) debug_mem_realloc_array(buff,unit_size,old_nb,new_nb,tname,module,fun,__LINE__)
#define MEM_FREE_SINGLE(buff,unit_size,tname,module,fun) debug_mem_free_single(buff,unit_size,tname,module,fun,__LINE__)
#define MEM_FREE_ARRAY(buff,unit_size,nb_elem,tname,module,fun) debug_mem_free_array(buff,unit_size,nb_elem,tname,module,fun,__LINE__)
#else
#define MEM_INIT() 
#define MEM_CHECK(out) 
#define MEM_LOG(out)
#define MEM_EXIT() 
#define MEM_ALLOC_SINGLE(unit_size,tname,module,fun) malloc(unit_size)
#define MEM_ALLOC_ARRAY(unit_size,nb_elem,tname,module,fun) malloc((unit_size)*(nb_elem))
#define MEM_ALLOC_ARRAY_RESET(unit_size,nb_elem,tname,module,fun) calloc((nb_elem),(unit_size))
#define MEM_REALLOC_ARRAY(buff,unit_size,old_nb,new_nb,tname,module,fun) realloc(buff,(unit_size)*(new_nb))
#define MEM_FREE_SINGLE(buff,unit_size,tname,module,fun) free(buff)
#define MEM_FREE_ARRAY(buff,unit_size,nb_elem,tname,module,fun) free(buff)

#endif

#endif
