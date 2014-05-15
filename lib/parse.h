
/* 	$Id: parse.h,v 1.1 2004/11/28 11:18:16 pesch Exp $	 */

/*****
* parse.h : eXdbm parser header
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

#ifndef PARSE_H
#define PARSE_H

#include <stdio.h>
#include "eXdbmTypes.h"

int ParseFile(FILE *f, TDbmListEntry *list, int level);

#endif
