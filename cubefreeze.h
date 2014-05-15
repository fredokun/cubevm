
#ifndef CUBE_FREEZE_H
#define CUBE_FREEZE_H

#include "cubeglobals.h"

#define FREEZE_INT16(val) INT16_TO_BE(val)
#define MELT_INT16(val) INT16_TO_BE(val) /* symmetrical */
#define FREEZE_UINT16(val) UINT16_TO_BE(val)
#define MELT_UINT16(val) UINT16_TO_BE(val) /* symmetrical */
#define FREEZE_INT32(val) INT32_TO_BE(val)
#define MELT_INT32(val) INT32_TO_BE(val) /* symmetrical */
#define FREEZE_UINT32(val) UINT32_TO_BE(val)
#define MELT_UINT32(val) UINT32_TO_BE(val) /* symmetrical */
#define FREEZE_INT64(val) INT64_TO_BE(val)
#define MELT_INT64(val) INT64_TO_BE(val) /* symmetrical */
#define FREEZE_UINT64(val) UINT64_TO_BE(val)
#define MELT_UINT64(val) UINT64_TO_BE(val) /* symmetrical */

#define FREEZE_MAGIC ((uint32)20121977)
#define FREEZE_SEGMENT_START_MAGIC ((uint32)20030709)

#define FREEZE_SEGMENT_AGENT 1
#define FREEZE_SEGMENT_STRING_POOL 2
#define FREEZE_SEGMENT_DEFS 3
#define FREEZE_SEGMENT_ACTIVE_PROCS 4
#define FREEZE_SEGMENT_WAIT_PROCS 5
#define FREEZE_SEGMENT_REACT_PROCS 6
#define FREEZE_SEGMENT_CHANS 7
#define FREEZE_SEGMENT_PROC 8
#define FREEZE_SEGMENT_USER 255

#endif
