
#ifndef CUBE_STR_H
#define CUBE_STR_H

typedef struct _String {
  char *str;
  int size;
  int ref_count; // ref_count = -1 for permanent strings
} String;

typedef int StringRef;

// XXX: FixMe put in configuration
#define START_MAX_STRING_POOL_STRINGS 4096
#define GROW_FACTOR_MAX_STRING_POOL_STRINGS 1024

typedef struct _StringPool {
  String * pool;
  int size;
  int max_size;
} StringPool;

#define STRGET(SP,STREF) (SP->pool[STREF])
#define STRREF(SP,STREF) if(SP->pool[STREF].ref_count>=0) SP->pool[STREF].ref_count++
#define STRUNREF(SP,STREF) if(SP->pool[STREF].ref_count>0) SP->pool[STREF].ref_count--
#define STRREFCOUNT(SP,STREF) (SP->pool[STREF].ref_count)

extern void init_string_pool(StringPool * sp);
extern StringPool* reclaim_string_pool(StringPool* sp);
extern StringRef str_make(StringPool * sp, char *str, int size, int init_ref_count);
extern void str_reclaim(StringPool * sp, StringRef s);
extern StringRef str_add(StringPool * sp, StringRef s1, StringRef s2);
extern StringRef str_add_cstr_1(StringPool * sp, char * sr1, StringRef s2);
extern StringRef str_add_cstr_2(StringPool * sp, StringRef s1, char* sr2);
extern char *cstr_from_int(int val);
extern int int_from_cstr(char *str);
extern StringRef str_from_int(StringPool * sp, int val);
extern char* cstr_from_real(double val);
extern double real_from_cstr(char *str);
extern StringRef str_from_real(StringPool * sp, double val);

#endif
