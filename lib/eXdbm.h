
/*****
* eXdbm.h : eXdbm main header
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

#ifndef EXDBM_H
#define EXDBM_H

#include <eXdbmTypes.h>
#include <eXdbmErrors.h>

/* error handling */

int eXdbmGetLastError(void);
const char *eXdbmGetErrorString(int errorcode);
int eXdbmLastLineParsed(void);

/* database functions */

int eXdbmInit(void);
int eXdbmOpenDatabase(char *filename, DB_ID *dbid);
int eXdbmNewDatabase(char *filename, DB_ID *dbid);
int eXdbmUpdateDatabase(DB_ID dbid);
int eXdbmBackupDatabase(DB_ID dbid, char *filename);
int eXdbmCloseDatabase(DB_ID dbid, int update);
int eXdbmReloadDatabase(DB_ID *dbid, int update);
char *eXdbmGetDatabaseFileName(DB_ID dbid);

/* get entry values functions */

int eXdbmGetEntryType(DB_ID dbid, DB_LIST list, char *entryname);
char *eXdbmGetEntryComment(DB_ID dbid, DB_LIST list, char *entryname);

int eXdbmGetVarInt(DB_ID dbid, DB_LIST list, char *entryname, long *value);
int eXdbmGetVarReal(DB_ID dbid, DB_LIST list, char *entryname, double *value);
int eXdbmGetVarString(DB_ID dbid, DB_LIST list, char *entryname, char **value);
int eXdbmGetVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char **value);
int eXdbmGetVarBool(DB_ID dbid, DB_LIST list, char *entryname, long *value);

DB_LIST eXdbmGetList(DB_ID dbid, DB_LIST list, char *listname);
DB_LIST eXdbmSearchList(DB_ID dbid, DB_LIST list, char *listname);
DB_LIST eXdbmPathList(DB_ID dbid, char *path);

/* change entry functions */

int eXdbmChangeEntryComment(DB_ID dbid, DB_LIST list, char *entryname, char *comment);
int eXdbmChangeVarInt(DB_ID dbid, DB_LIST list, char *entryname, long newvalue);
int eXdbmChangeVarReal(DB_ID dbid, DB_LIST list, char *entryname, double newvalue);
int eXdbmChangeVarString(DB_ID dbid, DB_LIST list, char *entryname, char *newvalue);
int eXdbmChangeVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char *newvalue);
int eXdbmChangeVarBool(DB_ID dbid, DB_LIST list, char *entryname, long newvalue);

/* create entry functions */

int eXdbmCreateList(DB_ID dbid, DB_LIST list, char *entryname, char *comment);
int eXdbmCreateVarInt(DB_ID dbid, DB_LIST list, char *entryname, char *comment, long value);
int eXdbmCreateVarReal(DB_ID dbid, DB_LIST list, char *entryname, char *comment, double value);
int eXdbmCreateVarBool(DB_ID dbid, DB_LIST list, char *entryname, char *comment, long value);
int eXdbmCreateVarString(DB_ID dbid, DB_LIST list, char *entryname, char *comment, char *value);
int eXdbmCreateVarIdent(DB_ID dbid, DB_LIST list, char *entryname, char *comment, char *value);

/* delete entry function */

int eXdbmDeleteEntry(DB_ID dbid, DB_LIST list, char *entryname);

#endif /* end of eXdbm.h */

