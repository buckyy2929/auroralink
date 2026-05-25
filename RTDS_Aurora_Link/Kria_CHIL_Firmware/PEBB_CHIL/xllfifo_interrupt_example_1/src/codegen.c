/**
 *
 * @file codegen.c
 * This defines functions to be used with the PLECS code gen controllers
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.0   BDR  2/6/2023  Initial revision
 * 2.0   BDR  9/26/2023  Changed to generic PEBB
 *
 * ***************************************************************************
 */

#include <stdio.h>
#include "xil_printf.h"
#include "codegen.h"
#include "MVDC.CLI\control_functions.h"
#include "MVDC.CLI\globals.h"

extern double initTime;
extern int ctrlCtr;
extern s32 seq;
extern s32 seq_prev;
extern s32 ctr;
extern struct GLOBALVARS vars;

// initialize or set the PEBB N_LRU and N_active inputs
// The number of LRUs is editable by the console, and initialized to a default based on the
// state of ACModeEnable received over the AXI bus.
// if set to DC mode and N_LRU wasn't manually set by user then it will be set to the DC default
// if set to AC mode and N_LRU wasn't manually set by user then it will be set to the AC default
// NOTE: this function won't allow the total number of LRUs to be set to 0 (which shouldn't be simulated anyway)
void setPEBBLRU(void) {
		// set to the default number of AC LRUs total and active
		PEBB_Control_U.N_LRU[0] = AC_LRU_DEFA;
		PEBB_Control_U.N_LRU[1] = AC_LRU_DEFB;
		PEBB_Control_U.N_LRU[2] = AC_LRU_DEFC;
		PEBB_Control_U.N_active[0] = AC_LRU_DEFA;
		PEBB_Control_U.N_active[1] = AC_LRU_DEFB;
		PEBB_Control_U.N_active[2] = AC_LRU_DEFC;
}


// resets each control component and the global variables
// NOTE: this may not be fully functional?
// recommended to use the HW regs instead

// PS reset via regs:
//uint32_t* const rst_addr = (uint32_t*)XRESETPS_PMU_GLB_RST_CTRL;
//*rst_addr =  PS_ONLY_RESET_MASK;
void resetSys() {

	resetPEBB();

	// reset globals
	ctrlCtr = 0;
	seq = 0;
	seq_prev = 0;
	ctr = 0;
}

// reset the PEBB state, force fault, and outputs
// NOTE: this function can only be ran asynchronously (while interrupts are not sent)
void resetPEBB() {
	PEBB_Control_initialize(initTime); // Note: this does not reset all globals in vars
	vars.current_state = 0; // Note: changing current_state in middle of an interrupt may introduce unpredictable behavior
	vars.next_state = 0;
	PEBB_Control_U.ForceFault = 0;
	PEBB_Control_Y.switch_1[0] = 0;
	PEBB_Control_Y.switch_1[1] = 0;
	PEBB_Control_Y.switch_1[2] = 0;
	PEBB_Control_Y.enable = 0;
	PEBB_Control_Y.highside_contact = 0;
	PEBB_Control_Y.lowside_contact = 0;
	PEBB_Control_Y.not_fault = 0; // inits in not_fault state // TBD
}

// initialize the PEBB commands
void initPEBB() {
	PEBB_NEXT_STATE_CMD = 64;
	V_CMD = 850;
	P_CMD = 0;
	Q_CMD = 0;
	LOWSIDE_CONTACTOR_CMD = 1;
	NEG_SEQ_STRAT = 1;
	PEBB_POWER_LOWER_LIMIT = -1.5e6;
	PEBB_POWER_UPPER_LIMIT = 1.5e6;
	P_DROOP_GAIN = 12;
	Q_DROOP_GAIN = 4;
	EN_GRID_SUPPORT = 0;
	Q_LIMIT = -0.6e6;
}
