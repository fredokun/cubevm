
/* 	$Id: test1.c,v 1.2 2005/01/04 17:12:36 pesch Exp $	 */

/*****
* test1.c : eXdbm example
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

#include <stdio.h>

#include <eXdbm.h>

extern TDbmDbList *DbmDbList;

int main(void)
{
  DB_ID dbid;
  int ret;

  printf(" \n*********************************************");
  printf(" \n***** Test1.c : read the file test1.cfg *****");
  printf(" \n*********************************************\n\n");

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

  printf("**** Checking database contents ****\n");

  WriteDatabase(stdout, DbmDbList->dblist[dbid].root, 0);

  printf("\n\n<<<<END OF DATABASE>>>>");
  printf("\n\n**** Updating database file ****\n\n");

  ret = eXdbmUpdateDatabase(dbid);
  if(ret == -1) {
    fprintf(stderr, "\nUpdating databases aborted");
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

  printf("**** Closing database (no update) ****\n\n");

  ret = eXdbmCloseDatabase(dbid, 0);
  if(ret == -1) {
    fprintf(stderr, "\nClosing databases aborted");
    fprintf(stderr, "\n%s\n", eXdbmGetErrorString(eXdbmGetLastError()));
    return(-1);
  }

  printf("****** eXDbm exit without error *****\n\n");

  return(0);
}

