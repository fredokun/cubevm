
#include <eXdbm.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cubeglobals.h"
#include "cubecfg.h"
#include "cubemisc.h"

char* GLOBAL_cfg_keys[] = { "GrowChansFactor", 
			    "MaxFreeChans",
                            "GrowProcsFactor",
			    "PassivateInputTreshold",
			    "PassivateOutputTreshold",
			    "PassivateChoiceTreshold",
			    "ReferenceFuel",
			    "StartMaxProcs",
			    "StartMaxChans",
			    "GrowDefsFactor",
			    "StartMaxDefs"
 };

unsigned long GLOBAL_cfg_values[] = { 65536UL, // Default grow chans factor
				      65536UL, // Default max free chans 
				      512UL, // Default grow procs factor
				      5UL, // Default passivate input treshold
				      5UL, // Default passivate output treshold
				      5UL, // Default passivate choice treshold
				      15UL, // Default reference fuel
				      16384UL, // Default start max procs
				      65536UL, // Default start max chans
				      64UL, // Default grow defs factor
				      256UL, // Default start max defs
				      1024UL, // Default start max strings
				      128UL // Default grow strings factor
};

static Bool LOCAL_cfg_init = FALSE;

void cfg_init() {
  assert(LOCAL_cfg_init==FALSE);

  int status = eXdbmInit();
  if(status==-1) 
    fatal_error("cubecfg.c","cfg_init",__LINE__,"Cannot initialize eXdbm module : %s",eXdbmGetErrorString(eXdbmGetLastError()));

  LOCAL_cfg_init = TRUE;
}

void cfg_update(char *filename) {
  assert(LOCAL_cfg_init==TRUE);

  // If no configuration
  if(strcmp(filename,"")==0)
    return;

  // Open the database
  
  DB_ID dbid;
  int status = eXdbmOpenDatabase(filename,&dbid);
  if(status==-1) 
    fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot open configuration file : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());

  // Get the grow chan factor entry
  unsigned long gcf_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"GrowChansFactor",&gcf_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'GrowChansFactor' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_GROW_CHANS_FACTOR] = gcf_value;

  // Get the max free chans entry
  unsigned long mfc_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"MaxFreeChans",&mfc_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'MaxFreeChans' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_MAX_FREE_CHANS] = mfc_value;

  // Get the grow procs factor entry
  unsigned long gpf_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"GrowProcsFactor",&gpf_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'GrowProcsFactor' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_GROW_PROCS_FACTOR] = gcf_value;

  // Get the passivate input treshold
  long pit_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"PassivateInputTreshold",&pit_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'PassivateInputTreshold' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_PASSIVATE_INPUT_TRESHOLD] = pit_value;

  // Get the passivate output treshold
  long pot_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"PassivateOutputTreshold",&pot_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'PassivateOutputTreshold' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_PASSIVATE_OUTPUT_TRESHOLD] =  pot_value;

  // Get the passivate choice treshold
  long pct_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"PassivateChoiceTreshold",&pct_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'PassivateChoiceTreshold' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_PASSIVATE_CHOICE_TRESHOLD] =  pct_value;

  // Get the reference fuel
  long rf_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"ReferenceFuel",&rf_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'ReferenceFuel' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_REFERENCE_FUEL] =  rf_value;

  // Get the start max procs
  unsigned long smp_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"StartMaxProcs",&smp_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'StartMaxProcs' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_START_MAX_PROCS] =  smp_value;

  // Get the start max chans
  unsigned long smc_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"StartMaxChans",&smc_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'StartMaxChans' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_START_MAX_CHANS] =  smc_value;

  // Get the grow defs factor
  unsigned long gdf_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"GrowDefsFactor",&gdf_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'GrowDefsFactor' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_GROW_DEFS_FACTOR] =  gdf_value;

  // Get the start max defs
  unsigned long smd_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"StartMaxDefs",&smd_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'StartMaxDefs' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_START_MAX_DEFS] =  smd_value;

  // Get the start max strings
  unsigned long sms_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"StartMaxStrings",&sms_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'StartMaxStrings' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_START_MAX_STRINGS] =  sms_value;

  // Get the grow strings factor
  unsigned long gsf_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"GrowStringsFactor",&sms_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'GrowStringsFactor' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_GROW_STRINGS_FACTOR] =  gsf_value;

  // Get the passivate sync treshold (for garbage collection)
  long pst_value = 0;
  status = eXdbmGetVarInt(dbid,NULL,"PassivateSyncTreshold",&pst_value);
  if(status==-1) {
    if(eXdbmGetLastError()!=DBM_ENTRY_NOT_FOUND) 
      fatal_error("cubecfg.c","cfg_update",__LINE__,"Cannot configure 'PassivateSyncTreshold' : %s (%s:%d)",eXdbmGetErrorString(eXdbmGetLastError()),filename,eXdbmLastLineParsed());
  } else
    GLOBAL_cfg_values[CFG_PASSIVATE_SYNC_TRESHOLD] =  pst_value;

  // at the end close the configuration file
  eXdbmCloseDatabase(dbid,0);

}
      

