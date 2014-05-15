
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cubeval.h"

Value GLOBAL_VALUE_NONE = { 0 , { 0 } };

void print_value(StringPool *sp, char *buffer, int max_size, Value val) {
  switch(VALUE_GET_TYPE(val)) {
  case VALUE_BOOL:
    if(val.val._int==TRUE)
      strncpy(buffer,"bool:true",max_size);
    else
      strncpy(buffer,"bool:false",max_size);
    break;
  case VALUE_INT:
    snprintf(buffer,max_size,"int:%d", val.val._int);
    break;
  case VALUE_REAL:
    snprintf(buffer,max_size,"real:%e", val.val._real);
    break;
  case VALUE_STRING:
    { char * str = NULL;
    str = (STRGET(sp,val.val._int)).str;
    snprintf(buffer,max_size,"str:\"%s\"", str);
    break; }
  case VALUE_CHAN:
    snprintf(buffer,max_size,"chan:%ld", (val.val._chan)->id);
    break;
  case VALUE_NONE:
    strncpy(buffer,"<NONE>",max_size);
    break;
  case VALUE_BIND:
    snprintf(buffer,max_size,"<BIND=%d>", val.val._int);
    break;
  default:
    strncpy(buffer,"<UNKNOWN>",max_size);
  }
  buffer[max_size-1]=(char)0;
}

