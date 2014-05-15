
/* 	$Id: test3.c,v 1.3 2005/01/06 10:55:02 pesch Exp $	 */

/*****
* test3.c : eXdbm database tool
*
* This file Version	$Revision: 1.3 $
*
* Last modification: 	$Date: 2005/01/06 10:55:02 $
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

#ifndef lint
static char vcid[] = "$Id: test3.c,v 1.3 2005/01/06 10:55:02 pesch Exp $";
#endif /* lint */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "eXdbm.h"

#define MAX_DB 16

int DbCount=0;
DB_ID Databases[MAX_DB];

void ErrorMessage(void)
{
  printf("\n*******************\n");
  printf("Error received : \n"); 
  printf("   ===> %s\n", eXdbmGetErrorString(eXdbmGetLastError()));
  printf("*******************\n");
}

void HitKey(void)
{
  char toto;

  printf("\nHit <Return> to continue");
  scanf("%c",&toto);
  printf("\n");
}

void OpenDatabase(void)
{
  char fname[256];
  int ret;
  int dbid;

  printf("\n\n");
  
  printf("Opening a new database");
  printf("----------------------\n");

  if(DbCount>MAX_DB) {
    printf("Error ==> already %d databases in memory\n", DbCount);
    HitKey();
    return;
  }

  printf("\nDatabase filename : ");
  scanf("%s", fname);

  ret = eXdbmOpenDatabase(fname, &dbid);
  if(ret==-1) {
    ErrorMessage();
    HitKey();
    return;
  }

  Databases[DbCount++] = dbid;

  printf("\nDatabase opened with identifier : %d\n\n", dbid);

  HitKey();

}

void NewDatabase(void)
{
  char fname[256];
  int ret;
  int dbid;

  printf("\n\n");
  
  printf("Creating a new database");
  printf("-----------------------\n");

  if(DbCount>MAX_DB) {
    printf("Error ==> already %d databases in memory\n", DbCount);
    HitKey();
    return;
  }

  printf("\nDatabase filename : ");
  scanf("%s", fname);

  ret = eXdbmNewDatabase(fname, &dbid);
  if(ret==-1) {
    ErrorMessage();
    HitKey();
    return;
  }

  Databases[DbCount++] = dbid;

  printf("\nDatabase created with identifier : %d\n", dbid);

  HitKey();

}

void PrintAvailableDatabases(void)
{
  int i;
  
  printf("\nAvailable databases: \n\n");
  
  if(DbCount==0) {
    printf("No available database\n");
    HitKey();
    return;
  }

  printf("Identifier\t\tFile name\n");
  for(i=0;i<80;i++) printf("-");

  for(i=0; i<DbCount; i++)
    printf("\n          %d\t\t%s", Databases[i], eXdbmGetDatabaseFileName(Databases[i]));

  printf("\n");
}

void CloseDatabase(void)
{
  DB_ID dbid;
  int i;
  int found;
  int ret;

  printf("\n\n");
  
  printf("Closing a database");
  printf("------------------");

  if(DbCount==0) {
    printf("\n\nError ==> no database in memory\n");
    HitKey();
    return;
  }

  PrintAvailableDatabases();

  printf("\n Choose a database ==> ");
  scanf("%d", &dbid);
  
  found = -1;
  for(i=0;i<DbCount;i++)
    if(Databases[i]==dbid) { found=i; break; }

  if(found == -1) {
    printf("\n\nError ==> Wrong database ID\n");
    HitKey();
    return;
  }

  ret = eXdbmCloseDatabase(dbid, 0);
  if(ret==-1) {
    ErrorMessage();
    return;
  }

  for(i=found; i < DbCount-1 ; i++)
    Databases[i] = Databases[i+1];

  DbCount--;

  printf("\nDatabase %d removed\n", dbid);

  HitKey();
}

void UpdateDatabase(void)
{
  DB_ID dbid;
  int i;
  int found;
  int ret;

  printf("\n\n");
  
  printf("Update a database file");
  printf("----------------------");

  if(DbCount==0) {
    printf("\n\nError ==> no database in memory\n");
    HitKey();
    return;
  }

  PrintAvailableDatabases();

  printf("\n Choose a database ==> ");
  scanf("%d", &dbid);
  
  found = -1;
  for(i=0;i<DbCount;i++)
    if(Databases[i]==dbid) { found=i; break; }

  if(found == -1) {
    printf("\n\nError ==> Wrong database ID\n");
    HitKey();
    return;
  }

  ret = eXdbmUpdateDatabase(dbid);
  if(ret==-1) {
    ErrorMessage();
    return;
  }

  printf("\nDatabase %d updated\n", dbid);

  HitKey();
}

void BackupDatabase(void)
{
  DB_ID dbid;
  int i;
  int found;
  int ret;
  char fname[256];

  printf("\n\n");
  
  printf("Backup a database");
  printf("-----------------");

  if(DbCount==0) {
    printf("\n\nError ==> no database in memory\n");
    HitKey();
    return;
  }

  PrintAvailableDatabases();

  printf("\n Choose a database ==> ");
  scanf("%d", &dbid);
  
  found = -1;
  for(i=0;i<DbCount;i++)
    if(Databases[i]==dbid) { found=i; break; }

  if(found == -1) {
    printf("\n\nError ==> Wrong database ID\n");
    HitKey();
    return;
  }

  printf("\nChoose a backup file name ==> ");
  scanf("%s", fname);

  ret = eXdbmBackupDatabase(dbid, fname);
  if(ret==-1) {
    ErrorMessage();
    HitKey();
    return;
  }

  printf("\nDatabase %d backup successfull\n", dbid);

  HitKey();
}

void ReloadDatabase(void)
{
  DB_ID dbid;
  int i;
  int found;
  int ret;

  printf("\n\n");
  
  printf("Reload a database file");
  printf("----------------------");

  if(DbCount==0) {
    printf("\n\nError ==> no database in memory\n");
    HitKey();
    return;
  }

  PrintAvailableDatabases();

  printf("\n Choose a database ==> ");
  scanf("%d", &dbid);
  
  found = -1;
  for(i=0;i<DbCount;i++)
    if(Databases[i]==dbid) { found=i; break; }

  if(found == -1) {
    printf("\n\nError ==> Wrong database ID\n");
    HitKey();
    return;
  }

  ret = eXdbmReloadDatabase(&Databases[found],0);
  if(ret==-1) {
    ErrorMessage();
    return;
  }

  printf("\nDatabase %d realoaded\n", dbid);

  HitKey();
}


void PrintDatabase(void)
{
  DB_ID dbid;
  int i;
  int found;
  int ret;

  printf("\n\n");
  
  printf("Printing a database contents");
  printf("----------------------------");

  if(DbCount==0) {
    printf("\n\nError ==> no database in memory\n");
    HitKey();
    return;
  }

  PrintAvailableDatabases();

  printf("\n Choose a database ==> ");
  scanf("%d", &dbid);
  
  found = -1;
  for(i=0;i<DbCount;i++)
    if(Databases[i]==dbid) { found=i; break; }

  if(found == -1) {
    printf("\n\nError ==> Wrong database ID\n");
    HitKey();
    return;
  }

  ret = eXdbmBackupDatabase(dbid, "test.database.tmp");
  if(ret==-1) {
    ErrorMessage();
    HitKey();
    return;
  }

  system("less test.database.tmp");

  printf("\n====================\n");

  system("rm test.database.tmp");

}

DB_ID ChooseDatabase(void)
{
  DB_ID dbid;
  int i;
  int found;

  if(DbCount==0) {
    printf("\n\nError ==> no database in memory\n");
    HitKey();
    return(-1);
  }

  PrintAvailableDatabases();

  printf("\n Choose a database ==> ");
  scanf("%d", &dbid);
  
  found = -1;
  for(i=0;i<DbCount;i++)
    if(Databases[i]==dbid) { found=i; break; }

  if(found == -1) {
    printf("\n\nError ==> Wrong database ID\n");
    HitKey();
    return(-1);
  }

  return(dbid);
}

DB_LIST ChooseParentList(DB_ID dbid)
{
  int choice = 0;
  char cbuf;
  char name[256];
  char path[16384];
  char searchpath[16384];
  DB_LIST current = NULL;
  DB_LIST newlist;

  strcpy(path, "Root:");

  while(1) {

    while (choice<1 || choice>5) {
    
      printf("\n");
      printf("Choose the parent's list of the entry : \n\n");
    
      printf("\nCurrent list = %s\n\n", path);

      printf("1) Root list\n");
      printf("2) Get sublist\n");
      printf("3) Search sublist recursively\n");
      printf("4) Enter full path of list\n");
      printf("\n5) Use the current list\n");
      printf("\n  Your choice ==> ");
      choice=0;
      scanf("%d", &choice);
      if(choice==0) scanf("%c", &cbuf);
    }

    switch(choice) {
    
    case 1 :
      strcpy(path,"Root:");
      current = NULL;
      break;

    case 2:
      printf("\nEnter sublist name ==> ");
      scanf("%s", name);
      newlist = eXdbmGetList(dbid, current, name);
      if(newlist==NULL) {
	ErrorMessage();
      } else {
	current = newlist;
	strcat(path,name);
	strcat(path,":");
      }
      break;
  
    case 3:
      printf("\nEnter sublist name ==> ");
      scanf("%s", name);
      newlist = eXdbmSearchList(dbid, current, name);
      if(newlist==NULL) {
	ErrorMessage();
      } else {
	current = newlist;
	strcat(path, "...:");
	strcat(path, name);
	strcat(path,":");
      }
      break;
 
    case 4:
      printf("\nEnter list path (except Root:) ==> ");
      scanf("%s", searchpath);
      newlist = eXdbmPathList(dbid, searchpath);
      if(newlist==NULL) {
	ErrorMessage();
      } else {
	current = newlist;
	strcpy(path, "Root:");
	strcat(path, searchpath);
	strcat(path, ":");
      }
      break;

    case 5:
      return(current);
    }
    choice=0;
  }
  
  return(NULL);
  
}

void PrintValues(void)
{
  int dbid;
  DB_LIST parent;
  char name[256];
  int etype;
  char *comment;
  DB_LIST list;
  long ival;
  double rval;
  char *sval;

  printf("\n\n");
  
  printf("Values of an entry\n");
  printf("------------------\n");

  dbid = ChooseDatabase();
  if(dbid==-1) return;
 
  parent = ChooseParentList(dbid);

  printf("\nChoose entry name ==> ");
  scanf("%s", name);

  etype = eXdbmGetEntryType(dbid, parent, name);
  if(etype==-1) {
    ErrorMessage();
    HitKey();
    return;
  }

  printf("\nEntry values :\n");
  printf(  "------------\n\n");

  printf("[NAME] = %s\n\n", name);

  comment = eXdbmGetEntryComment(dbid, parent, name);
  if(comment!=NULL)
    printf("[COMMENT] = %s\n\n", comment);


  switch(etype) {
  case DBM_ENTRY_LIST :
    printf("[TYPE] = List\n\n");
    list = eXdbmGetList(dbid, parent, name);
    printf("Info : This list contains %d entries\n\n", list->current_order);
    break;
    
  case DBM_ENTRY_VAR_INT :
    printf("[TYPE] = integer variable\n\n");
    eXdbmGetVarInt(dbid, parent, name, &ival);
    printf("[VALUE] = %ld\n\n", ival);
    break;

  case DBM_ENTRY_VAR_REAL :
    printf("[TYPE] = real number variable\n\n");
    eXdbmGetVarReal(dbid, parent, name, &rval);
    printf("[VALUE] = %f\n\n", rval);
    break;

  case DBM_ENTRY_VAR_BOOL :
    printf("[TYPE] = boolean variable\n\n");
    eXdbmGetVarBool(dbid, parent, name, &ival);
    if(ival==0)
      printf("[VALUE] = FALSE\n\n");
    else 
      printf("[VALUE] = TRUE\n\n");
    break;

  case DBM_ENTRY_VAR_STRING :
    printf("[TYPE] = string variable\n\n");
    eXdbmGetVarString(dbid, parent, name, &sval);
    printf("[VALUE] = %s\n\n", sval);
    free(sval);
    break;

  case DBM_ENTRY_VAR_IDENT :
    printf("[TYPE] = identifier variable\n\n");
    eXdbmGetVarIdent(dbid, parent, name, &sval);
    printf("[VALUE] = %s\n\n", sval);
    free(sval);
    break;

  }

  HitKey();

}

void AddEntry(void)
{
  int dbid;
  DB_LIST parent;
  char name[256];
  int etype;
  char *comment;
  char comval[256];
  int ival;
  double rval;
  char sval[256];
  char choice[10];
  int ret;

  printf("\n\n");
  
  printf("Create an entry\n");
  printf("---------------\n");

  dbid = ChooseDatabase();
  if(dbid==-1) return;
 
  parent = ChooseParentList(dbid);

  printf("\nChoose entry name ==> ");
  scanf("%s", name);

  etype = eXdbmGetEntryType(dbid, parent, name);
  if(etype!=-1) {
    printf("\nerror ==> entry already defined\n");
    HitKey();
    return;
  }

  printf("\nEntry values :\n");
  printf(  "------------\n\n");

  printf("[NAME] = %s\n\n", name);

  printf("Do you want to specify a comment (y/n) ? ");
  scanf("%s", choice);

  comment = NULL;

  if(toupper(choice[0])=='Y') {
    printf("[COMMENT] = ");
    fgets(comval,256,stdin);
    comment = comval;
  }
  
  etype = 0;

  while(etype<1 || etype > 6) {
    printf("\nChoose the type of the entry :\n\n");
    printf("1 => integer\n");
    printf("2 => real\n");
    printf("3 => bool\n");
    printf("4 => string\n");
    printf("5 => idenfifier\n");
    printf("6 => list\n");
    
    printf("\n  Your choice ==> ");
    etype=0;
    scanf("%d", &etype);
    if(etype==0) scanf("%s", choice);
  }

  etype--;

  switch(etype) {
  case DBM_ENTRY_LIST :
    printf("[TYPE] = List\n\n");
    ret = eXdbmCreateList(dbid, parent, name, comment);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;
    
  case DBM_ENTRY_VAR_INT :
    printf("[TYPE] = integer variable\n\n");
    printf("[VALUE] = ");
    scanf("%d", &ival);
    ret = eXdbmCreateVarInt(dbid, parent, name, comment, ival);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;

  case DBM_ENTRY_VAR_REAL :
    printf("[TYPE] = real number variable\n\n");
    printf("[VALUE] = ");
    scanf("%lf", &rval);
    ret = eXdbmCreateVarReal(dbid, parent, name, comment, rval);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;

  case DBM_ENTRY_VAR_BOOL :
    printf("[TYPE] = boolean variable\n\n");
    printf("[VALUE] = ");
    scanf("%s", sval);
    if(strcmp(sval, "FALSE")==0)
      ret = eXdbmCreateVarBool(dbid, parent, name, comment, 0);
    else
      ret = eXdbmCreateVarBool(dbid, parent, name, comment, 1);

    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;


  case DBM_ENTRY_VAR_STRING :
    printf("[TYPE] = string variable\n\n");
    printf("[VALUE] = ");
    scanf("%s", sval);
    ret = eXdbmCreateVarString(dbid, parent, name, comment, sval);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;


  case DBM_ENTRY_VAR_IDENT :
    printf("[TYPE] = identifier variable\n\n");
    printf("[VALUE] = ");
    scanf("%s", sval);
    ret = eXdbmCreateVarIdent(dbid, parent, name, comment, sval);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;

  }

  HitKey();

}

void ChangeEntry(void)
{
  int dbid;
  DB_LIST parent;
  char name[256];
  int etype;
  char *comment;
  char comval[256];
  int ival;
  double rval;
  char sval[256];
  char choice[10];
  int ret;

  printf("\n\n");
  
  printf("Change an entry\n");
  printf("---------------\n");

  dbid = ChooseDatabase();
  if(dbid==-1) return;
 
  parent = ChooseParentList(dbid);

  printf("\nChoose entry name ==> ");
  scanf("%s", name);

  etype = eXdbmGetEntryType(dbid, parent, name);
  if(etype==-1) {
    printf("\nerror ==> entry not defined\n");
    HitKey();
    return;
  }

  printf("\nEntry values :\n");
  printf(  "------------\n\n");

  printf("[NAME] = %s\n\n", name);

  comment = eXdbmGetEntryComment(dbid, parent, name);

  if(comment!=NULL) 
    printf("[COMMENT] = %s\n\n", comment);
  
  printf("Do you want to specify a new comment (y/n) ? ");
  scanf("%s", choice);

  comment = NULL;

  if(toupper(choice[0])=='Y') {
    printf("[COMMENT] = ");
    fgets(comval,256,stdin);
    comment = comval;
  }

  if(comment!=NULL)
    eXdbmChangeEntryComment(dbid, parent, name, comment);
 
  switch(etype) {
  case DBM_ENTRY_LIST :
    printf("[TYPE] = List\n\n");
    printf("Cannot change a list entry\n");
    break;
    
  case DBM_ENTRY_VAR_INT :
    printf("[TYPE] = integer variable\n\n");
    printf("[VALUE] = ");
    scanf("%d", &ival);
    ret = eXdbmChangeVarInt(dbid, parent, name, ival);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;

  case DBM_ENTRY_VAR_REAL :
    printf("[TYPE] = real number variable\n\n");
    printf("[VALUE] = ");
    scanf("%lf", &rval);
    ret = eXdbmChangeVarReal(dbid, parent, name, rval);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;

  case DBM_ENTRY_VAR_BOOL :
    printf("[TYPE] = boolean variable\n\n");
    printf("[VALUE] = ");
    scanf("%s", sval);
    if(strcmp(sval, "FALSE")==0)
      ret = eXdbmChangeVarBool(dbid, parent, name, 0);
    else
      ret = eXdbmChangeVarBool(dbid, parent, name, 1);

    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;


  case DBM_ENTRY_VAR_STRING :
    printf("[TYPE] = string variable\n\n");
    printf("[VALUE] = ");
    fgets(sval,256,stdin);
    ret = eXdbmChangeVarString(dbid, parent, name, sval);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;


  case DBM_ENTRY_VAR_IDENT :
    printf("[TYPE] = identifier variable\n\n");
    printf("[VALUE] = ");
    fgets(sval,256,stdin);
    ret = eXdbmChangeVarIdent(dbid, parent, name, sval);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    break;

  }

  HitKey();

}

void DeleteEntry(void)
{
  int dbid;
  DB_LIST parent;
  char name[256];
  int etype;
  char *comment = NULL;
  char choice[10];
  int ret;

  printf("\n\n");
  
  printf("Delete an entry\n");
  printf("---------------\n");

  dbid = ChooseDatabase();
  if(dbid==-1) return;
 
  parent = ChooseParentList(dbid);

  printf("\nChoose entry name ==> ");
  scanf("%s", name);

  etype = eXdbmGetEntryType(dbid, parent, name);
  if(etype==-1) {
    printf("\nerror ==> entry not defined\n");
    HitKey();
    return;
  }

  printf("\nEntry values :\n");
  printf(  "------------\n\n");

  printf("[NAME] = %s\n\n", name);

  if(comment!=NULL)
    eXdbmChangeEntryComment(dbid, parent, name, comment);
 
  switch(etype) {
  case DBM_ENTRY_LIST :
    printf("[TYPE] = List\n\n");
    printf("Cannot change a list entry\n");
    break;
    
  case DBM_ENTRY_VAR_INT :
    printf("[TYPE] = integer variable\n\n");
    break;

  case DBM_ENTRY_VAR_REAL :
    printf("[TYPE] = real number variable\n\n");
    break;

  case DBM_ENTRY_VAR_BOOL :
    printf("[TYPE] = boolean variable\n\n");
    break;


  case DBM_ENTRY_VAR_STRING :
    printf("[TYPE] = string variable\n\n");
    break;


  case DBM_ENTRY_VAR_IDENT :
    printf("[TYPE] = identifier variable\n\n");
    break;

  }


  printf("Do you want to erase this variable (y/n) ? ");
  scanf("%s", choice);

  if(toupper(choice[0])=='Y') {
    ret = eXdbmDeleteEntry(dbid, parent, name);
    if(ret==-1) {
      ErrorMessage();
      HitKey();
      return;
    }
    printf("\nEntry deleted successfully\n");
  } else printf("\nEntry not deleted\n");
  
  HitKey();

}


int MainMenu(void) 
{
  int choice=0;

  while(choice<1 || choice>12) {

    printf("\n\n");

    printf("eXdbm test application main menu\n");
    printf("================================\n");
    printf("\n");

    printf("%d database(s) in memory\n\n", DbCount);
    printf("Database management : \n");
    printf("-------------------   \n");
    printf("1)  Open a database\n");
    printf("2)  New database\n");
    printf("3)  Close a database\n");
    printf("4)  Update database\n");
    printf("5)  Backup database\n");
    printf("6)  Reload a database\n");
    printf("7)  Print database contents\n");
    printf("Entry management :\n");
    printf("----------------\n");
    printf("8)  Print entry values\n");
    printf("9)  Add an entry\n");
    printf("10) Change entry values\n");
    printf("11) Delete an entry\n");
  
    printf("12) Quit\n");

    printf("\n  Make your choice  ===> ");
    choice=0;
    scanf ("%d", &choice);
    if(choice==0) HitKey();
  }

  return(choice);
}

int main(void) 
{
  int choice;
  int ret;

  ret = eXdbmInit();
  if(ret==-1) {
    ErrorMessage();
    return(EXIT_FAILURE);
  }

  choice=-1;

  while(choice!=12) {
    choice = MainMenu();
    switch(choice) {
    case 1 : 
      OpenDatabase();
      break;

    case 2 :
      NewDatabase();
      break;

    case 3 : 
      CloseDatabase();
      break;

    case 4 :
      UpdateDatabase();
      break;

    case 5 : 
      BackupDatabase();
      break;

    case 6 :
      ReloadDatabase();
      break;

    case 7 :
      PrintDatabase();
      break;

    case 8 :
      PrintValues();
      break;

    case 9 :
      AddEntry();
      break;

    case 10:
      ChangeEntry();
      break;

    case 11 :
      DeleteEntry();
      break;
      
    }
  }

  printf("\n\nBye bye ...\n");

  return(1);
}
