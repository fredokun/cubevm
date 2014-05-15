
/*****
* eXdbmTypes.h : eXdbm private data types
*
* Copyright (C) 1997-2004 Fred Pesch 
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

#ifndef EXDBM_TYPES_H
#define EXDBM_TYPES_H

/* defines */

#define HASH_LENGTH 8   /* the number of bits of the hash values */
#define HASH_MAX_ENTRIES (1 << HASH_LENGTH) /* number of elements in a hash tabl
e */

#define MAX_ENTRY_LENGTH 64  /* maximum length for a token */

#define MIN_ORDER_SIZE 256 /* minimum size of the order array */

#define TRUE 1
#define FALSE 0

/* enumerations */

enum {
  DBM_RETURN_FAIL=-1,
  DBM_ENTRY_VAR_INT =  0,
  DBM_ENTRY_VAR_REAL = 1,
  DBM_ENTRY_VAR_BOOL = 2,
  DBM_ENTRY_VAR_STRING = 3,
  DBM_ENTRY_VAR_IDENT = 4,
  DBM_ENTRY_LIST = 5,
  DBM_ENTRY_ROOT = 6
};

/* List entry typedefs */

typedef struct {
  double real_val;
  char *str_val;
  long int_val; 
 } TDbmEntryValue;

typedef struct DbmListEntry {
  char *key;                    /* key name */
  char *comment;                /* entry comment */
  int entry_type;               /* entry type */
  TDbmEntryValue value;          /* entry value */
  struct DbmListEntry* next;    /* next entry sharing the same key value */
  struct DbmListEntry** child;  /* children list */
  int current_order;            /* number of elemens in order array */
  int size_order;               /* size of the order array */
  struct DbmListEntry** order;   /* order array */
} TDbmListEntry;

typedef TDbmListEntry *DB_LIST;

/* Database typedefs */

typedef struct {
  char *filename; /* database filename */
  TDbmListEntry *root; /* the root entry */
} TDbmDatabase;

typedef struct {
  int nb_db;            /* number of loaded databases */
  int array_size;       /* the size of the array of databases */
  TDbmDatabase *dblist; /* list of databases */
} TDbmDbList;


typedef int DB_ID;  /* database identifier */

#endif
