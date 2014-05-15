
/* 	$Id: eXdbmErrors.h,v 1.2 2004/11/28 17:01:15 pesch Exp $	 */

/*****
* eXdbmErrors.h : eXdbm error numbers
*
* This file Version	$Revision: 1.2 $
*
* Last modification: 	$Date: 2004/11/28 17:01:15 $
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
#ifndef EXDBM_ERRORS_H
#define EXDBM_ERRORS_H

enum { 
  DBM_NO_ERROR = 0,
  DBM_ALLOC = 1,
  DBM_INIT_NEEDED = 2,
  DBM_INIT_REINIT = 3,
  DBM_OPEN_FILE = 4,
  DBM_PARSE_COMMENT = 5,
  DBM_PARSE_ID = 6,
  DBM_PARSE_VALUE = 7,
  DBM_PARSE_UNEXP_END = 8,
  DBM_UPDATE_FILE = 9,
  DBM_WRONG_ID = 10,
  DBM_UPDATE_WRITE_ERROR = 11,
  DBM_DESTROY = 12,
  DBM_WRONG_TYPE = 13,
  DBM_ENTRY_NOT_FOUND = 14,
  DBM_BAD_PARAMETER = 15,
  DBM_DUPLICATE_ENTRY = 16,
  DBM_LIST_NOT_FOUND = 17,
  DBM_END_ERROR = 18
};  

#endif /* end of eXdbmErrors.h */






