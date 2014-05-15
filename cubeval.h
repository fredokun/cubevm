
#ifndef CUBE_VAL_H
#define CUBE_VAL_H

#include "cubechan.h"
#include "cubestr.h"

typedef enum {
  VALUE_NONE = 0,
  VALUE_BIND = 1,  // to recognize bounded variables
                   // entry in environment is val_int
  VALUE_BOOL = 2,
  VALUE_INT = 3,
  VALUE_REAL = 4,
  VALUE_CHAR = 5,
  VALUE_STRING = 6,
  VALUE_PSTRING = 7,  // Parser string, only for parser
  VALUE_CHAN = 8,
  VALUE_SYMBOL = 9, // XX: symbol type :hello :hello:world:fr
  VALUE_ARRAY = 10,  // XX: primitive array
  VALUE_LOC = 11,
  VALUE_TUPLE = 12,
  VALUE_EXT = 254, // XX: external type
  VALUE_MASK = 255  // Value kind in 8 bits
} ValueType;

struct _Tuple;

typedef union _ValueVal {
  int _int;
  char _char;
  float _real;
  unsigned long _loc;
  char * _string; // now strings are in a pool, use _int instead
                  // but still need _string for parser
  struct _Tuple * _tuple;
  void * user; // for user defined types
  // XXX : must add array type (and hashtable type ?)
  struct _Channel *_chan;
} ValueVal;

typedef struct _Value {
  uint16 ctl;
  ValueVal val;
} Value;

extern Value GLOBAL_VALUE_NONE;

////////////////
// Interface  //
////////////////

#define VALUE_CREATE_NONE() (GLOBAL_VALUE_NONE)

#define VALUE_TYPE_MASK 0x00FFU
#define VALUE_CTL_MASK 0xFF00U
#define VALUE_GET_TYPE(v) /*(*/((v).ctl) /*)& VALUE_TYPE_MASK) */
#define VALUE_SET_TYPE(v,t) (((v).ctl)=/*(((v).ctl)&VALUE_CTL_MASK)+(*/(t)/*&VALUE_TYPE_MASK)*/)

/* XXX: FixMe propagate flag not needed ? other flags ?
#define VALUE_PROPAGATE_MASK 0x8FFFU
#define VALUE_PROPAGATE_COMPLEMENT 0x7FFFU
#define VALUE_PROPAGATE(v) (((v).ctl)&VALUE_PROPAGATE_MASK)
#define VALUE_SET_PROPAGATE(v) (((v).ctl)&=VALUE_PROPAGATE_MASK)
#define VALUE_UNSET_PROPAGATE(v) (((v).ctl)&=VALUE_PROPAGATE_COMPLEMENT)
*/

#define VALUE_IS_NONE(v) (VALUE_GET_TYPE(v)==VALUE_NONE)

#define VALUE_IS_BOOL(v) (VALUE_GET_TYPE(v)==VALUE_BOOL)
#define VALUE_AS_BOOL(v) ((v).val._int)
#define VALUE_SET_BOOL(v,l) (((v).val._int) = (l))
#define VALUE_SET_TRUE(v) (((v).val._int) = TRUE)
#define VALUE_SET_FALSE(v) (((v).val._int) = FALSE)

#define VALUE_IS_CHAR(v) (VALUE_GET_TYPE(v)==VALUE_CHAR)
#define VALUE_AS_CHAR(v) ((v).val._char)
#define VALUE_SET_CHAR(v,l) (((v).val._char) = (l))

#define VALUE_IS_INT(v) (VALUE_GET_TYPE(v)==VALUE_INT)
#define VALUE_AS_INT(v) ((v).val._int)
#define VALUE_SET_INT(v,l) (((v).val._int) = (l))

#define VALUE_IS_BIND(v) (VALUE_GET_TYPE(v)==VALUE_BIND)
#define VALUE_AS_BIND(v) ((v).val._int)
#define VALUE_SET_BIND(v,l) (((v).val._int) = (l))

#define VALUE_IS_REAL(v) (VALUE_GET_TYPE(v)==VALUE_REAL)
#define VALUE_AS_REAL(v) ((v).val._real)
#define VALUE_SET_REAL(v,l) (((v).val._real) = (l))

#define VALUE_IS_STRINGREF(v) (VALUE_GET_TYPE(v)==VALUE_STRING)
#define VALUE_AS_STRINGREF(v) ((v).val._int)
#define VALUE_SET_STRINGREF(v,l) (((v).val._int) = (l))

#define VALUE_IS_CHANP(v) (VALUE_GET_TYPE(v)==VALUE_CHAN)
#define VALUE_AS_CHANP(v) ((v).val._chan)
#define VALUE_SET_CHANP(v,l) (((v).val._chan) = (l))

#define VALUE_IS_LOC(v) (VALUE_GET_TYPE(v)==VALUE_LOC)
#define VALUE_AS_LOC(v) ((v).val._loc)
#define VALUE_SET_LOC(v,l) (((v).val._loc) = (l))

#define VALUE_IS_TUPLE(v) (VALUE_GET_TYPE(v)==VALUE_TUPLE)
#define VALUE_AS_TUPLE(v) ((v).val._tuple)
#define VALUE_SET_TUPLE(v,t) (((v).val._tuple) = (t))

extern void print_value(StringPool *sp, char *buffer, int max_size, Value val);

#endif
