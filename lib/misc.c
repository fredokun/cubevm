
/* 	$Id: misc.c,v 1.2 2005/01/04 17:12:36 pesch Exp $	 */

/*****
* misc.c : eXdbm misc. functions
*
* This file Version	$Revision: 1.2 $
*
* Last modification: 	$Date: 2005/01/04 17:12:36 $
* By:					$Author: pesch $
* Current State:		$State: Exp $
*
* Copyright (C) 1997 Fred Pesch 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eXdbmTypes.h"
#include "eXdbmErrors.h"

#include "hash.h"
#include "misc.h"

/* externals */

extern int DbmLastErrorCode;
extern TDbmDbList *DbmDbList;

/* error handling */

void RaiseError(int errorcode)
{
  DbmLastErrorCode = errorcode;
}

/* is the database initialized ? */ 

int DbmIsInit(void)
{

  if(DbmDbList==NULL) {
    RaiseError(DBM_INIT_NEEDED);
    return(-1);
  }

  return(1);
}

/* check a database identifier */

int CheckDbIdent(DB_ID dbid)
{
  if(dbid>=DbmDbList->array_size) return(-1);

  if(DbmDbList->dblist[dbid].root == NULL) return(-1);

  return(1);
}

/* write a database to a file */

int WriteDatabase(FILE *f, TDbmListEntry *list, int level)
{
  int i;
  int j;
  TDbmListEntry *node;
  int ret;

  for(i=0; i< list->current_order; i++) { 

      node = list->order[i];

      switch(node->entry_type) {

      case DBM_ENTRY_VAR_INT :
	
	if(node->comment!=NULL) {
	  fprintf(f, "\n");
	  for(j=0;j<level;j++) fprintf(f,"  ");
	  fprintf(f,"%s", node->comment);
	}
	    
	fprintf(f,"\n");
	for(j=0;j<level;j++) fprintf(f,"  ");
	fprintf(f,"%s = %ld\n", node->key, node->value.int_val);
	  
	break;
	  
      case DBM_ENTRY_VAR_REAL :
	
	if(node->comment!=NULL) {
	  fprintf(f, "\n");
	  for(j=0;j<level;j++) fprintf(f,"  ");
	  fprintf(f,"%s", node->comment);
	}
		  
	fprintf(f,"\n");
	for(j=0;j<level;j++) fprintf(f,"  ");
	fprintf(f,"%s = %f\n", node->key, node->value.real_val);

	break;


      case DBM_ENTRY_VAR_STRING :

	if(node->comment!=NULL) {
	  fprintf(f, "\n");
	  for(j=0;j<level;j++) fprintf(f,"  ");
	  fprintf(f,"%s", node->comment);
	}

	fprintf(f,"\n");
	for(j=0;j<level;j++) fprintf(f,"  ");
	fprintf(f,"%s = \"%s\"\n", node->key, node->value.str_val);

	break;

      case DBM_ENTRY_VAR_IDENT :

	if(node->comment!=NULL) {
	  fprintf(f, "\n");
	  for(j=0;j<level;j++) fprintf(f,"  ");
	  fprintf(f,"%s", node->comment);
	}

	fprintf(f,"\n");
	for(j=0;j<level;j++) fprintf(f,"  ");
	fprintf(f,"%s = %s\n", node->key, node->value.str_val);

	break;

      case DBM_ENTRY_VAR_BOOL :
	  

	if(node->comment!=NULL) {
	  fprintf(f, "\n");
	  for(j=0;j<level;j++) fprintf(f,"  ");
	  fprintf(f,"%s", node->comment);
	}

	fprintf(f,"\n");
	for(j=0;j<level;j++) fprintf(f,"  ");
	if(node->value.int_val==1)
	  fprintf(f,"%s = TRUE\n", node->key);
	else 
	  fprintf(f,"%s = FALSE\n", node->key);
	
	break;
  
      case DBM_ENTRY_LIST :

	if(node->comment!=NULL) {
	  fprintf(f, "\n");
	  for(j=0;j<level;j++) fprintf(f,"  ");
	  fprintf(f,"%s", node->comment);
	}
  
	fprintf(f,"\n");
	for(j=0;j<level;j++) fprintf(f,"  ");
	fprintf(f,"%s {\n", node->key);
	
	ret = WriteDatabase(f, node, level+1);
	if(ret==-1) {
	  RaiseError(DBM_UPDATE_WRITE_ERROR);
	  return(-1);
	}

	fprintf(f,"\n");
	for(j=0;j<level;j++) fprintf(f,"  ");
	fprintf(f,"}\n");
	
	break;

      default : 
	RaiseError(DBM_UPDATE_WRITE_ERROR);
	return(-1);
      }

  }
  
  return(0);

}

/* destroy the full database */

int DestroyDatabase(TDbmListEntry *list)
{
  int i;
  TDbmListEntry *node;
  int ret;

  for(i=0; i< list->current_order; i++) { 
    node = list->order[i];
	
    switch(node->entry_type) {

    case DBM_ENTRY_VAR_INT :
      
      if(node->comment!=NULL) free(node->comment);

      free(node->key);
	  
      break;
	  
    case DBM_ENTRY_VAR_REAL :
	
      if(node->comment!=NULL) free(node->comment);

      free(node->key);
		  
      break;


    case DBM_ENTRY_VAR_STRING :

      if(node->comment!=NULL) free(node->comment);

      free(node->key);
      free(node->value.str_val);
	  
      break;

    case DBM_ENTRY_VAR_IDENT :

      if(node->comment!=NULL) free(node->comment);

      free(node->key);
      free(node->value.str_val);
	  
      break;

    case DBM_ENTRY_VAR_BOOL :
	  

      if(node->comment!=NULL) free(node->comment);
	  
      free(node->key);

      break;
  
    case DBM_ENTRY_LIST :

      if(node->comment!=NULL) free(node->comment);

      free(node->key);
  
      ret = DestroyDatabase(node);
      if(ret==-1) {
	RaiseError(DBM_DESTROY);
	return(-1);
      }

      free(node->child);

      free(node->order);

      break;

    default : 
      RaiseError(DBM_DESTROY);
      return(-1);
    }

  }
  
  return(0);

}

/* search an entry in a list */

TDbmListEntry * SearchListEntry(TDbmListEntry *list, char *entryname)
{
  int hash_value;
  TDbmListEntry *node;

  if(list==NULL || list->child==NULL || entryname==NULL) return(NULL);

  hash_value = HashValueGenerator(entryname);

  node = list->child[hash_value];

  while(node!=NULL) {

    if(strcmp(node->key, entryname)==0) { /* we've found the entry */

      return(node);

    }

    node=node->next;

  }

  return(NULL);

}

/* search an entry in a list and all sublists recursively */

TDbmListEntry * SearchListEntryRec(TDbmListEntry *list, char *entryname)
{
  TDbmListEntry *node;
  int i;

  node = SearchListEntry(list, entryname);

  if(node!=NULL) return(node);

  /* search in sublists */

  for(i=0;i< list->current_order ; i++) {

    if(list->order[i]->entry_type==DBM_ENTRY_LIST) {

      node = SearchListEntryRec(list->order[i], entryname);
      if(node!=NULL) return(node);

    }

  }

  return(NULL);

}

/* create a new entry */

TDbmListEntry * CreateListEntry(TDbmListEntry *list, char *entryname, char *comment, int entrytype)
{
  int hash_value;
  TDbmListEntry *node;
  int i;
  int ret;

  node = SearchListEntry(list, entryname);
  if(node!=NULL) {
    RaiseError(DBM_DUPLICATE_ENTRY);
    return(NULL);
  }

  hash_value = HashValueGenerator(entryname);

  node = list->child[hash_value];

  if(node!=NULL) {
    while(node->next!=NULL) node=node->next;

    /* create the new entry */

    node->next = (TDbmListEntry *) malloc(sizeof(TDbmListEntry));
    if(node->next==NULL) {
      RaiseError(DBM_ALLOC);
      return(NULL);
    }

    node=node->next;
  } else {

    node = (TDbmListEntry *) malloc(sizeof(TDbmListEntry));
    if(node==NULL) {
      RaiseError(DBM_ALLOC);
      return(NULL);
    }
    
    list->child[hash_value]=node;

  }

  /* fill the new entry */

  node->key = (char *) malloc(sizeof(char) * (strlen(entryname)+1));
  if(node->key==NULL) {
    RaiseError(DBM_ALLOC);
    return(NULL);
  }

  strcpy(node->key, entryname);

  if(comment!=NULL) {

    node->comment = (char *) malloc(sizeof(char) * (strlen(comment)+1));
    if(node->comment==NULL) {
      RaiseError(DBM_ALLOC);
      return(NULL);
    }
    strcpy(node->comment, comment);

  } else node->comment = NULL;

  node->entry_type = entrytype;

  node->value.str_val = NULL;
  node->value.int_val = -1;
  node->value.real_val = -1;

  node->child = NULL;

  if(node->entry_type == DBM_ENTRY_LIST) {

    node->child = (TDbmListEntry **) malloc( sizeof(TDbmListEntry *) * HASH_MAX_ENTRIES);
    if(node->child == NULL) {
      RaiseError(DBM_ALLOC);
      return(NULL);
    }

    for (i=0; i < HASH_MAX_ENTRIES ; i++) 
      node->child[i] = NULL;

    node->order = (TDbmListEntry **) malloc( sizeof(TDbmListEntry *) * MIN_ORDER_SIZE);
    if(node->order == NULL) {
      RaiseError(DBM_ALLOC);
      return(NULL);
    }

    for(i=0; i < MIN_ORDER_SIZE ; i++)
      node->order[i] = NULL;

    node->size_order = MIN_ORDER_SIZE;
    node->current_order = 0;

  } else {
    
    node->size_order = 0;
    node->current_order = 0;
    node->order = NULL;
  }

  node->next = NULL;

  /* add the list to the order array */

  list->current_order++;
  ret = AddOrderEntry( list, node);
  if(ret == -1) return(NULL);
  
  return(node);
}

/* Delete an entry in a list */

int DeleteListEntry(TDbmListEntry *list, char *entryname)
{
  int hash_value;
  TDbmListEntry *node;
  int found;
  TDbmListEntry *before;
  TDbmListEntry *after;
  int i,j;

  if(list==NULL || list->child==NULL || entryname==NULL) return(-1);

  hash_value = HashValueGenerator(entryname);

  node = list->child[hash_value];

  found = 0;
  before = NULL;
  after = node->next;

  while(!found && node!=NULL) {

    if(strcmp(node->key, entryname)==0) /* we've found the entry */
      found = 1;

    if(!found) { 
      before = node;
      node=node->next;
    }

    after = node->next;

  }

  if(node==NULL) return(-1);

  /* remove the list order entry */

  i=0;
  while(node!=list->order[i]) i++;

  for(j=i; j< list->current_order-1; j++) 
    list->order[j] = list->order[j+1];

  list->order[list->current_order-1] = NULL;

  list->current_order--;
  
  /* delete the entry */

  free(node->key);
  if(node->comment!=NULL) free(node->comment);
  
  switch(node->entry_type) {

  case DBM_ENTRY_VAR_STRING :
  case DBM_ENTRY_VAR_IDENT :
    if(node->value.str_val!=NULL ) free(node->value.str_val);
    break;

  case DBM_ENTRY_LIST :
    DestroyDatabase(node);
    free(node->child);
    free(node->order);
    break;

  default:
    break;
  }

  /* update the chained list */

  if(before!=NULL)
    before->next = after;
  else list->child[hash_value] = after;

  return(1);

}

/* add element in order array */

int AddOrderEntry(TDbmListEntry *list, TDbmListEntry *element)
{

  /* need new elements in array ? */

  if(list->current_order > list->size_order) {

    list->size_order *= 2;
    list->order = (TDbmListEntry **) realloc( list->order, sizeof(TDbmListEntry *) * (list->size_order));
    if(list->order == NULL) {
      RaiseError(DBM_ALLOC);
      return(-1);
    }

  }

  /* fill the element */

  (list->order)[list->current_order-1] = element;

  return(1);
}



