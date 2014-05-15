
#ifndef CUBE_CFG_H
#define CUBE_CFG_H

enum { CFG_GROW_CHANS_FACTOR =0,
       CFG_MAX_FREE_CHANS=1,
       CFG_GROW_PROCS_FACTOR=2,
       CFG_PASSIVATE_INPUT_TRESHOLD=3,
       CFG_PASSIVATE_OUTPUT_TRESHOLD=4,
       CFG_PASSIVATE_CHOICE_TRESHOLD=5,
       CFG_REFERENCE_FUEL=6,
       CFG_START_MAX_PROCS=7,
       CFG_START_MAX_CHANS=8,
       CFG_GROW_DEFS_FACTOR=9,
       CFG_START_MAX_DEFS=10,
       CFG_START_MAX_STRINGS=11,
       CFG_GROW_STRINGS_FACTOR=12,
       CFG_PASSIVATE_SYNC_TRESHOLD=13
};

extern char* GLOBAL_cfg_keys[];

extern unsigned long GLOBAL_cfg_values[];

#define GLOBAL_CFG_AS_ULONG(key) ((unsigned long) GLOBAL_cfg_values[key])
#define GLOBAL_CFG_AS_INT(key) ((int) GLOBAL_cfg_values[key])

extern void cfg_init();
extern void cfg_update(char *cfg_filename);

#endif
