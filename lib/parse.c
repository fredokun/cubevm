
/* 	$Id: parse.c,v 1.4 2005/01/04 17:12:36 pesch Exp $	 */

/*****
* parse.c : eXdbm parser
*
* This file Version	$Revision: 1.4 $
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

#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "eXdbmErrors.h"
#include "eXdbmTypes.h"
#include "parse.h"
#include "misc.h"
#include "hash.h"

/* externals */

extern int DbmParseLineNumber;

/* enumerations */

enum {
  DBM_TOKEN_LIST_ID = 0,
  DBM_TOKEN_VAR_ID = 1,
  DBM_TOKEN_INT = 2,
  DBM_TOKEN_REAL = 3,
  DBM_TOKEN_BOOL = 4,
  DBM_TOKEN_STRING = 5
};

/***** parse a comment *****/

int ParseComment(FILE *f, char *comment)
{
  char current;
  int pos_in_comment=0;

  do {
    
    current=fgetc(f);

    if(current==EOF) {
      comment[pos_in_comment]=(char)0;
      return(EOF);
    }

    if(current!='\n') {
      comment[pos_in_comment]=current;

      pos_in_comment++;
      if(pos_in_comment+1>=MAX_ENTRY_LENGTH) return(-1);
    }

  } while(current!='\n');

  DbmParseLineNumber++;

  comment[pos_in_comment]= (char) 0;
  
  return(1);
}

/***** parse an identifier *****/

int ParseIdentifier(FILE *f, char *token)
{
  char current;
  int pos_in_token=0;

  /* the first char is available */
  token[pos_in_token++] = fgetc(f);

  /* read the identifier name */
  do {

    current = fgetc(f);
    if(current==EOF) return(-1);
    if(isalnum(current) || current=='_') token[pos_in_token++]=current;
    else if(!isspace(current)) return(-1);

    if(pos_in_token+1>=MAX_ENTRY_LENGTH) return(-1);

  } while(!isspace(current));
  
  token[pos_in_token] = (char) 0;

  while(isspace(current)) {
    if(current=='\n') return(-1);
    current=fgetc(f);
    if(current==EOF) return(-1);
  }
  
  /* check if it's a list or a variable */

  switch(current) {
  case '{':

    /* this is a list */

    current=fgetc(f);

    while(current!='\n') {
      if(!isspace(current)) return(-1);
      current=fgetc(f);
      if(current==EOF) return(-1);
    }

    while(current=='\n') {
      DbmParseLineNumber++;
      current=fgetc(f);
    }
    
    return(DBM_TOKEN_LIST_ID);
    break;

  case '=' :

    /* this is a variable */

    do {
      current=fgetc(f);
      if(current==EOF || current=='\n') return(-1);
    } while(isspace(current));

    ungetc((int) current, f);
   
    return(DBM_TOKEN_VAR_ID);
 
    break;

  default :
    return(-1);
  }
  
  return(-1);
  
}

/***** parse an entry value *****/

int ParseEntryValue(FILE *f, TDbmEntryValue *value)
{
  char current;
  int state;
  char value_string[MAX_ENTRY_LENGTH];
  int pos_in_string = 0;
  int type;

  type = -1;
  state=0;
  while(1) {
    
    current = fgetc(f);
    if(current==EOF) return(-1); /* unexpected end of file */

    switch(state) {

    case 0 : /* initial state */ 
      if(isdigit(current)) {
	value_string[pos_in_string++] = current;
	state=2;
      }
      else if(current=='+' || current=='-') {
	value_string[pos_in_string++] = current;
	state=1;
      }
      else if (isalpha(current)) {
	value_string[pos_in_string++] = current;
	state=12;
      }
      else if (current == '"') state=14;
      else return(-1);
      break;

    case 1 : /* a '+' or a '-' starts the string */

      while(isspace(current)) {
	if(current == '\n' || current == EOF) return(-1);
	current = fgetc(f);
      }

      ungetc(current, f);
      state=2;

      break;
  
    case 2 : /* read numbers */

      while(isdigit(current)) {
	value_string[pos_in_string++] = current;
	if(pos_in_string+1>=MAX_ENTRY_LENGTH) return(-1);
	current = fgetc(f);
      }

      if(isspace(current) || current==EOF) {

	while(isspace(current)) {
	  if(current=='\n') DbmParseLineNumber++;
	  current=fgetc(f);
	}
	
	if(current!=EOF) ungetc(current, f);
	
	value_string[pos_in_string] = (char) 0;

	char * ret_str;
	value->int_val = atol(value_string);
	value->real_val = strtod(value_string,&ret_str);
	value->str_val = NULL;

	return(DBM_ENTRY_VAR_INT);
      }

      if(current=='e' || current=='E') {
	value_string[pos_in_string++] = current;
	state=4;
      } 
      else if(current=='.') {
	value_string[pos_in_string++] = current;
	state=7;
      }
      else return(-1);

      break;

    case 4 : /* an integer with exposant */

      if(current=='+' || current=='-') {
	value_string[pos_in_string++] = current;
	current=fgetc(f);
	if(current==EOF || isspace(current)) return(-1);
      }

      if(!isdigit(current)) return(-1);

      while(isdigit(current)) {
	value_string[pos_in_string++] = current;
	if(pos_in_string+1>=MAX_ENTRY_LENGTH) return(-1);
	current=fgetc(f);
      }

      if(!isspace(current) && current!=EOF) return(-1);

      while(isspace(current)) {
	if(current=='\n') DbmParseLineNumber++;
	current=fgetc(f);
      }

      ungetc(current, f);

      value->int_val = atol(value_string);
      //      value->real_val = atod(value_string);
      char * ret_str;
      value->real_val = strtod(value_string,&ret_str);
      value->str_val = NULL;

      return(DBM_ENTRY_VAR_INT);

      break;

    case 7: /* a real number */

      if(!isdigit(current)) return(-1);

      while(isdigit(current)) {
	value_string[pos_in_string++] = current;
	if(pos_in_string+1>=MAX_ENTRY_LENGTH) return -1;
	current = fgetc(f);
      }

      if(current=='e' || current=='E') {
	value_string[pos_in_string++] = current;
	current=fgetc(f);

	if(current=='+' || current=='-') {
	  value_string[pos_in_string++] = current;
	  current=fgetc(f);
	  if(current==EOF || isspace(current)) return(-1);
	}

	if(!isdigit(current)) return(-1);

	while(isdigit(current)) {
	  value_string[pos_in_string++] = current;
	  if(pos_in_string+1>=MAX_ENTRY_LENGTH) return(-1);
	  current=fgetc(f);
	}
      }

      if(isspace(current) || current==EOF) {

	while(isspace(current)) {
	  if(current=='\n') DbmParseLineNumber++;
	  current=fgetc(f);
	}
	
	if(current!=EOF) ungetc(current, f);
	
	value_string[pos_in_string] = (char) 0;

	//value->real_val = atod(value_string);
	char * ret_str;
	value->real_val = strtod(value_string,&ret_str);	
	value->int_val = (long) ceil(value->real_val);
	value->str_val = NULL;

	return(DBM_ENTRY_VAR_REAL);

      } else return(-1);

      break;
      
    case 12 : /* an identifier string */

      while(isalnum(current) || current=='_') {
	value_string[pos_in_string++] = current;
	if(pos_in_string+1>=MAX_ENTRY_LENGTH) return(-1);
	current=fgetc(f);
      }
     
      if(isspace(current) || current == EOF) {

	while(isspace(current)) {
	  if(current=='\n') DbmParseLineNumber++;
	  current=fgetc(f);
	}
	
	if(current!=EOF) ungetc(current, f);

	value_string[pos_in_string] = (char) 0;

	if(strcmp(value_string,"TRUE")==0) {
	  
	  /* a boolean value = TRUE */

	  value->real_val = 1;
	  value->int_val = 1;
	  value->str_val = NULL;
	  
	  return(DBM_ENTRY_VAR_BOOL);

	}

	if(strcmp(value_string,"FALSE")==0) {

	  /* a boolean value = FALSE */
	  
	  value->real_val = 0;
	  value->int_val = 0;
	  value->str_val = NULL;
	  
	  return(DBM_ENTRY_VAR_BOOL);
	}

	
	/* an identifier */
	
	value->real_val = -1;
	value->int_val = -1;
	value->str_val = (char *) malloc(sizeof(char) * (strlen(value_string)+1));
	
	if(value->str_val==NULL) return(-1);
	
	strcpy(value->str_val,value_string);
	
	return(DBM_ENTRY_VAR_IDENT);

      } else return(-1);
      
      break;

    case 14: /* a complete string */

      while(current!='"') {
	if(current=='\n') DbmParseLineNumber++;
	if(current==EOF) return(-1);
	value_string[pos_in_string++] = current;
	if(pos_in_string+1>=MAX_ENTRY_LENGTH) return(-1);
	current = fgetc(f);
      }

      current = fgetc(f);

      if(isspace(current) || current == EOF) {
	
	while(isspace(current)) {
	  if(current=='\n') DbmParseLineNumber++;
	  current=fgetc(f);
	}
	
	if(current!=EOF) ungetc(current, f);

	value_string[pos_in_string] = (char) 0;

	value->real_val = -1;
	value->int_val = -1;
	value->str_val = (char *) malloc(sizeof(char) * (strlen(value_string)+1));
					
	if(value->str_val==NULL) return(-1);

	strcpy(value->str_val,value_string);

	return(DBM_ENTRY_VAR_STRING);

      } else return(-1);

      break;
      
    default : return(-1);
    }

    if(pos_in_string+1>=MAX_ENTRY_LENGTH) return(-1);
  }

  return(-1);

}

/***** main parsing function *****/

int ParseFile(FILE *f, TDbmListEntry *list, int level)
{
  static char current_token[MAX_ENTRY_LENGTH];
  int token_type;
  static char last_comment[MAX_ENTRY_LENGTH];
  static int last_comment_available = 0;
  char current;
  int hash_value;
  TDbmListEntry *newnode;
  int ret;
  int i;

  do {

    /* skip the preceeding spaces & empty lines (counted) */
    
    current=fgetc(f);
    while(isspace(current)) {
      if(current=='\n') DbmParseLineNumber++;
      current=fgetc(f);
    }

    if(current == EOF || current=='}') continue;

    if(current=='#') { 

      ungetc(current, f);

      /* parse a comment */

      last_comment_available = ParseComment(f, last_comment);
  
      if(last_comment_available==-1) {
	RaiseError(DBM_PARSE_COMMENT);
	return(-1);
      } 

      if(last_comment_available==EOF) {
	current=EOF;
	continue;
      }

    } else if(isalpha(current)) {
      
      ungetc(current, f);

      /* parse a variable or list identifier */

      token_type = ParseIdentifier(f, current_token);
     
      switch(token_type) {
      case DBM_TOKEN_LIST_ID :

	/* new list entry */

	hash_value = HashValueGenerator ( current_token );
	
	if ( list->child[hash_value] == NULL ) {
	  list->child[hash_value] = ( TDbmListEntry *) malloc( sizeof(TDbmListEntry) );

	  if(list->child[hash_value] == NULL) {
	    RaiseError(DBM_ALLOC);
	    return(-1);
	  }
	    
	  newnode = list->child[hash_value];
	} else { /* there's already an entry at this place */
	  
	  newnode = list->child[hash_value];
	  while(newnode->next!=NULL) newnode = newnode->next; 
	  
	  newnode->next = (TDbmListEntry *) malloc( sizeof(TDbmListEntry) );
	  
	  if(newnode->next == NULL) {
	    RaiseError(DBM_ALLOC);
	    return(-1);
	  }

	  newnode=newnode->next;
	  newnode->next = NULL;
	}

	/* fill the new entry */

	/* key name */

	newnode->key = (char *) malloc (sizeof(char) * (strlen(current_token) + 1));
	if(newnode->key == NULL) {
	  RaiseError(DBM_ALLOC);
	  return(-1);
	}

	strcpy(newnode->key, current_token);

	/* comment */

	if(last_comment_available) {

	  newnode->comment = (char *) malloc (sizeof(char) * (strlen(last_comment) + 1));
	  if(newnode->comment == NULL) {
	    RaiseError(DBM_ALLOC);
	    return(-1);
	  }

	  strcpy(newnode->comment, last_comment);
	  last_comment_available = 0;
	} else newnode->comment = NULL;

	/* entry type */

	newnode->entry_type = DBM_ENTRY_LIST;

	/* child list */

	newnode->child = (TDbmListEntry **) malloc( sizeof(TDbmListEntry *) * HASH_MAX_ENTRIES);
	if(newnode->child == NULL) {
	  RaiseError(DBM_ALLOC);
	  return(-1);
	}

	for (i=0; i < HASH_MAX_ENTRIES ; i++) 
	  newnode->child[i] = NULL;

	/* order array */

	newnode->order = (TDbmListEntry **) malloc( sizeof(TDbmListEntry *) * MIN_ORDER_SIZE);
	if(newnode->order == NULL) {
	  RaiseError(DBM_ALLOC);
	  return(-1);
	}

	newnode->size_order = MIN_ORDER_SIZE;
	newnode->current_order = 0;

	/* add the list to the order array */

	list->current_order++;
	ret = AddOrderEntry( list, newnode);

	if(ret==-1) return(-1);

	/* continue the parsing recursively */

	ret = ParseFile(f, newnode, level+1);
	if(ret == -1) return(-1);

	break;

      case DBM_TOKEN_VAR_ID :

	/* new variable entry */

	hash_value = HashValueGenerator ( current_token );
	
	if ( list->child[hash_value] == NULL ) {
	  list->child[hash_value] = ( TDbmListEntry *) malloc( sizeof(TDbmListEntry) );

	  if(list->child[hash_value] == NULL) {
	    RaiseError(DBM_ALLOC);
	    return(-1);
	  }
	    
	  newnode = list->child[hash_value];
	} else { /* there's already an entry at this place */
	  
	  newnode = list->child[hash_value];
	  while(newnode->next!=NULL) newnode = newnode->next; 
	  
	  newnode->next = (TDbmListEntry *) malloc( sizeof(TDbmListEntry) );
	  
	  if(newnode->next == NULL) {
	    RaiseError(DBM_ALLOC);
	    return(-1);
	  }

	  newnode=newnode->next;
	  newnode->next = NULL;
	}

	/* fill the new entry */

	/* key name */

	newnode->key = (char *) malloc (sizeof(char) * (strlen(current_token) + 1));
	if(newnode->key == NULL) {
	  RaiseError(DBM_ALLOC);
	  return(-1);
	}

	strcpy(newnode->key, current_token);

	/* comment */

	if(last_comment_available) {

	  newnode->comment = (char *) malloc (sizeof(char) * (strlen(last_comment) + 1));
	  if(newnode->comment == NULL) {
	    RaiseError(DBM_ALLOC);
	    return(-1);
	  }

	  strcpy(newnode->comment, last_comment);
	  last_comment_available = 0;
	} else newnode->comment = NULL;

	/* entry type */

	newnode->entry_type = ParseEntryValue(f, &newnode->value);

	if(newnode->entry_type == -1) {
	  RaiseError(DBM_PARSE_VALUE);
	  return(-1);
	}

	/* no child list for variable entries */

	newnode->child = NULL;

	/* no order array */

	newnode->order = NULL;
	newnode->size_order = 0;
	newnode->current_order = 0;

	/* add the variable to the order array */

	list->current_order++;
	ret = AddOrderEntry( list, newnode);
	if(ret==-1) return(-1);
	break;

      default : 
	/* unknown entry */
	RaiseError(DBM_PARSE_ID);
	return(-1);
      }

    } else return(-1);
     
  } while(current!=EOF && current!='}');

  if(current==EOF && level>0) {
    RaiseError(DBM_PARSE_UNEXP_END);
    return(-1);
  }

  return(1);

}
