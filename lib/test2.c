
/* 	$Id: test2.c,v 1.1 2004/11/28 11:18:16 pesch Exp $	 */

/*****
* test2.c : eXdbm example
*
* This file Version	$Revision: 1.1 $
*
* Last modification: 	$Date: 2004/11/28 11:18:16 $
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
static char vcid[] = "$Id: test2.c,v 1.1 2004/11/28 11:18:16 pesch Exp $";
#endif /* lint */

#include <stdio.h>

#include "eXdbm.h"

extern TDbmDbList *DbmDbList;

int main(void)
{
  DB_ID dbid;
  int ret;

  printf(" \n*************************************************************");
  printf(" \n***** Test2.c : read the file test1.cfg, and rewrite it *****");
  printf(" \n*************************************************************\n\n");
  printf("**** Database manager initialization *****\n\n");

  ret = eXdbmInit();
  if(ret==-1) {
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

    printf("**** Opening database in file test1.cfg ****\n\n");

  ret = eXdbmOpenDatabase("test1.cfg", &dbid);
  if(ret==-1) {
    fprintf(stderr, "\nParsing databases aborted line %d", eXdbmLastLineParsed());
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

  fprintf(stderr, "%d lines parsed \n\n", eXdbmLastLineParsed()); 

  printf("**** Updating database file ****\n\n");

  ret = eXdbmUpdateDatabase(dbid);
  if(ret == -1) {
    fprintf(stderr, "\nUpdating databases aborted");
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

  printf("**** Make a backup copy of the database in test1.cfg.backup ****\n\n");

  ret = eXdbmBackupDatabase(dbid, "test1.cfg.backup");
  if(ret == -1) {
    fprintf(stderr, "\nDatabase Backup aborted");
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

  printf("**** Closing database (no update) ****\n\n");

  ret = eXdbmCloseDatabase(dbid, 0);

  printf("**** Try to reload the database ****\n\n");

  ret = eXdbmOpenDatabase("test1.cfg", &dbid);
  if(ret==-1) {
    fprintf(stderr, "\nParsing databases aborted line %d", eXdbmLastLineParsed());
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

  fprintf(stderr, "%d lines parsed \n\n", eXdbmLastLineParsed()); 

  printf("**** Reloading the database (no update) ****\n\n");

  ret = eXdbmReloadDatabase(&dbid, 0);
  if(ret == -1) {
    fprintf(stderr, "\nReloading database aborted");
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

  printf("**** Reloading the database (with update) ****");

  ret = eXdbmReloadDatabase(&dbid, 1);
  if(ret == -1) {
    fprintf(stderr, "\nReloading database aborted");
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }


  printf("\n\n**** Closing the database (with update) ****\n\n");
  
  ret = eXdbmCloseDatabase(dbid, 1);
  if(ret == -1) {
    fprintf(stderr, "\nClosing database aborted");
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

  printf("****** eXDbm exit without error *****\n\n");

  return(0);
}

