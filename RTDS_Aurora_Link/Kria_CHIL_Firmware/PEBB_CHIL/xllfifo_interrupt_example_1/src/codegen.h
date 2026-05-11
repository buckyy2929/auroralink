/**
 *
 * @file codegen.h
 * This file has includes and macros for interfacing to the PEBB code gen files
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.0   BDR  2/3/2023  Initial revision
 * 2.0   BDR  9/26/2023  Changed to generic PEBB
 *
 * ***************************************************************************
 */

#ifndef CODEGEN_h_
#define CODEGEN_h_

#include "PEBB_Control.h"

#ifndef PI
#define PI 3.1415926535
#endif

// define size of inputs
#define PEBB_CMD_SIZE 12 // number of signals in PEBB input array

#define PEBB_FEEDBACKS 11 // number of feedback signals (muxed together)

// the default LRU settings
#define DC_LRU_DEFA 12
#define DC_LRU_DEFB 0
#define DC_LRU_DEFC 0

#define AC_LRU_DEFA 4
#define AC_LRU_DEFB 4
#define AC_LRU_DEFC 4

// input mapping
#define PEBB_NEXT_STATE_CMD PEBB_Control_U.Cmds[0]
#define V_CMD PEBB_Control_U.Cmds[1]
#define P_CMD PEBB_Control_U.Cmds[2]
#define Q_CMD PEBB_Control_U.Cmds[3]
#define LOWSIDE_CONTACTOR_CMD PEBB_Control_U.Cmds[4]
#define PEBB_POWER_LOWER_LIMIT PEBB_Control_U.Cmds[5]
#define PEBB_POWER_UPPER_LIMIT PEBB_Control_U.Cmds[6]
#define NEG_SEQ_STRAT PEBB_Control_U.Cmds[7]
#define P_DROOP_GAIN PEBB_Control_U.Cmds[8]
#define Q_DROOP_GAIN PEBB_Control_U.Cmds[9]
#define EN_GRID_SUPPORT PEBB_Control_U.Cmds[10]
#define Q_LIMIT PEBB_Control_U.Cmds[11]

void resetPEBB(void);
void resetSys(void);
void setPEBBLRU(void);
void initPEBB(void);

#endif                                 /* CODEGEN_h_ */
