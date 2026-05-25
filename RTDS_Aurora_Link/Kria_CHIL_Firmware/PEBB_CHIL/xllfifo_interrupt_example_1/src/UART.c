/**
 *
 * @file UART.c
 * This defines functions to be used for receiving data over a UART terminal
 * Terminal is set for 115200 baud
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.0   BDR  1/23/2023  Initial revision
 * 2.0   BDR  9/26/2023  Changed to generic PEBB
 *
 * ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "xuartps.h"
#include "xil_printf.h"
#include "xresetps_hw.h"

#include "UART.h"

// for variable declarations
#include "codegen.h"
#include "MVDC.CLI\control_functions.h"
#include "MVDC.CLI\globals.h"

#define VERSION 2 // current version number for the code (int)
#define SUB_VERSION 3 // version VERSION.SUB_VERSION
#define DATE 20231101 // date of the current revision (int)
#define VARIANT "PEBB" // name of the controller variant

XUartPs Uart_PS;		/* Instance of the UART Device */

int recv_cnt = 0; // chars received during this UART read
int total_recv_cnt = 0; // chars received between carriage returns and put in str2
int total_recv_cnt2 = 0; // chars received and put in str1
int total_recv_cnt_index = 0; // chars received and put in strIndex
char buf[TEST_BUFFER_SIZE]; // buffer for this UART read
char str[32]; // buffer for all UART reads between carriage returns
char str2[32]; // buffer for all UART reads after '=' and before carriage return
char strIndex[8]; // buffer for the index of an array
int seenEquals; // 1 if seen a '=' sign
int seenIndex; // 1 if seen a '['
int commaCount; // count of commas seen

extern double initTime;
extern int dumpOnStop;

typedef enum Type_ {double_, float_, u32_, u64_, s32_, int_, bool_} type_;

extern struct GLOBALVARS vars;
extern volatile int run;
extern volatile int CHIL_error;

//--------------------------------------------------------------------------------
// List of available data
//--------------------------------------------------------------------------------

#define VARS_LENGTH 35

const char *vs[VARS_LENGTH]; // list of variable options
type_ ty[VARS_LENGTH]; // list of types for each
void *pt[VARS_LENGTH]; // list of pointers to each variable
s8 ls[VARS_LENGTH]; // list of lengths of each variable (1 if a single value, >1 if an array)
float scales[VARS_LENGTH]; // list of scale factors for each variable

static void initInputArrays(void);
static void printValue(int varNum);
static int getVarNum(char* stri);
static void setVar(int varNum, char* indexChar, char* val);
static void parseInput(char *stri1, char *strIndex, char *stri2, int lastFlag);


//--------------------------------------------------------------------------------
// Function Definitions
//--------------------------------------------------------------------------------

/*****************************************************************************/
/*
*
* Enter variable strings, types, and addresses here
* These are the strings which are editable/viewable from the UART console
*
* @param	None
*
* @return	None
*
* @note
******************************************************************************/
static void initInputArrays(void) {
	static int arrCtr = 0; // counter for curret index in initInputArrays() function

	extern int ctrlOn;
	vs[arrCtr] = "ctrlOn";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &ctrlOn;
	ls[arrCtr] = 1;
	arrCtr++;

	extern int intOverrunFlag;
	vs[arrCtr] = "intOverrunFlag";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &intOverrunFlag;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_U.ForceFault";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_Control_U.ForceFault;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_U.Cmds";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_Control_U.Cmds;
	ls[arrCtr] = PEBB_CMD_SIZE;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_U.feedback";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_Control_U.feedback;
	ls[arrCtr] = PEBB_FEEDBACKS;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_Y.not_fault";
	ty[arrCtr] = bool_;
	pt[arrCtr] = (void *) &PEBB_Control_Y.not_fault;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.next_state";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.next_state;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.current_state";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.next_state;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.faults.all";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.faults.all;
	ls[arrCtr] = 1;
	arrCtr++;

	extern s32 seq;
	vs[arrCtr] = "seq";
	ty[arrCtr] = s32_;
	pt[arrCtr] = (void *) &seq;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_U.N_LRU";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_Control_U.N_LRU;
	ls[arrCtr] = 3;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_U.N_active";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_Control_U.N_active;
	ls[arrCtr] = 3;
	arrCtr++;

	vs[arrCtr] = "PEBB_state_cmd";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_NEXT_STATE_CMD;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "V_low_cmd";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &V_CMD;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "P_cmd";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &P_CMD;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "Q_cmd";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &Q_CMD;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "lowside_contactor_cmd";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &LOWSIDE_CONTACTOR_CMD;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "Pdroopgain";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &P_DROOP_GAIN;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "Qdroopgain";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &Q_DROOP_GAIN;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "neg_seq_strat";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &NEG_SEQ_STRAT;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "enGridSupport";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &EN_GRID_SUPPORT;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "Q_limit";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &Q_LIMIT;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "P_ll";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_POWER_LOWER_LIMIT;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "P_ul";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_POWER_UPPER_LIMIT;
	ls[arrCtr] = 1;
	arrCtr++;


/*
	vs[arrCtr] = "vars.faults.bit.highside_overcurrent";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.faults.bit.highside_overcurrent;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.faults.bit.highside_overvoltage";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.faults.bit.highside_overvoltage;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.faults.bit.lowside_overvoltage";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.faults.bit.lowside_overvoltage;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.faults.bit.bus_overvoltage";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.faults.bit.bus_overvoltage;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.faults.bit.highside_lowside_overcurrent";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.faults.bit.lowside_overcurrent;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.faults.bit.highside_lowside_overcurrent";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.faults.bit.lowside_overcurrent;
	ls[arrCtr] = 1;
	arrCtr++;
*/
	vs[arrCtr] = "vars.flags.all";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &vars.flags.all;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "CHIL_error";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &CHIL_error;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "run";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &run;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "dumpOnStop";
	ty[arrCtr] = int_;
	pt[arrCtr] = (void *) &dumpOnStop;
	ls[arrCtr] = 1;
	arrCtr++;

	extern s32 RXI_chil;
	vs[arrCtr] = "RXI_chil";
	ty[arrCtr] = s32_;
	pt[arrCtr] = (void *) &RXI_chil;
	ls[arrCtr] = 1;
	arrCtr++;

	extern volatile int NanTxCtr;
	vs[arrCtr] = "NanTxCtr";
	ty[arrCtr] = s32_;
	pt[arrCtr] = (void *) &NanTxCtr;
	ls[arrCtr] = 1;
	arrCtr++;

	extern volatile int NanRxCtr;
	vs[arrCtr] = "NanRxCtr";
	ty[arrCtr] = s32_;
	pt[arrCtr] = (void *) &NanRxCtr;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_Y.switch_1";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_Control_Y.switch_1;
	ls[arrCtr] = 3;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_Y.highside_contact";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_Control_Y.highside_contact;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "PEBB_Control_Y.lowside_contact";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &PEBB_Control_Y.lowside_contact;
	ls[arrCtr] = 1;
	arrCtr++;

	vs[arrCtr] = "vars.v_dc_bus_filt";
	ty[arrCtr] = double_;
	pt[arrCtr] = (void *) &vars.v_dc_bus_filt;
	ls[arrCtr] = 1;
	arrCtr++;

	// NOTE: remember to update VARS_LENGTH at the top of UART.c when add or remove items from this list

	if (arrCtr != VARS_LENGTH) {
		xil_printf("Warning: counter arrCtr (%d) does not match VARS_LENGTH (%d). Check value of VARS_LENGTH for consistency.", arrCtr, VARS_LENGTH);
	}
}

/*****************************************************************************/
/*
*
* XUartPs read routine. Handles input based on user entry
*
* @param	char stri1: this string before the '=' and before '['.
* 			char striIndex: this string between '[' and ']'
* 			char stri2: this string after the '='.
* 			int lastFlag: set to 1 when this function is called on a '\n' or 'r', but not when parsing
* 			an intermediate aray input after a ',' is received
*
* @return	None
*
* @note
******************************************************************************/
static void parseInput(char *stri1, char *strIndex, char *stri2, int lastFlag) {
	// Note: can't use switch on strings
	if (stri1[0] == 0) {
		// do nothing (repeated semicolons or blank lines trigger this)
	} else if(!strcmp(stri1, "help")) {
		xil_printf("Software version: %s variant, %d.%d, date: %d \n",VARIANT, VERSION, SUB_VERSION, DATE);
		xil_printf("\"reset\": reset all controllers.\r\n");
		xil_printf("\"list\" list all available variables \r\n");
		xil_printf("\"var\" to view the value of list item \"var\" or type \"var=value\" to set its value.\r\n");
		xil_printf("     Arrays can be assigned by element e.g. (arr[1]=2) or in entirety e.g. (arr=[1,2,3]).\r\n");
		xil_printf("     Multiple assignments can be made in one line by separating them with semi-colons (;)\r\n");
		xil_printf("     Spaces are ignored, and up to 32 characters are allowed per input.\r\n");
		xil_printf("\"dump\": dump all available variables.\r\n");
		xil_printf("Additional debugging commands (not recommended for general use): \r\n");
		xil_printf("\"resetPEBB\": soft reset corresponding controller.\r\n");
		xil_printf("\"ssPEBB\": run a single step of the corresponding controller.\r\n");
		xil_printf("\n");
	} else if (!strcmp(stri1, "reset")) {
		xil_printf("Starting system reset...\n\r");

		// full system reset via regs
		volatile uint32_t* const rst_addr = (uint32_t*)XRESETPS_CRL_APB_RESET_CTRL;
	    *rst_addr = SOFT_RESET_MASK;

		/*
		 * // NOTE: don't reset PS only, crashes when re-initializing HW
		uint32_t* const rst_addr = (uint32_t*)XRESETPS_PMU_GLB_RST_CTRL;
	    *rst_addr =  PS_ONLY_RESET_MASK;
	    */

		//resetSys(); // not fully functional
		xil_printf("Issued system reset\n\r");
	} else if (!strcmp(stri1, "softReset")) {
		resetSys(); // not fully functional
		xil_printf("Issued system reset\n\r");
	} else if (!strcmp(stri1, "resetPEBB")) {
		resetPEBB();
		xil_printf("Issued PEBB reset\n\r");
	} else if (!strcmp(stri1, "ssPEBB")) {
		xil_printf("Starting PEBB control step(s)\n\r");
		int stepCount = atoi(stri2);
		if (stepCount == 0) { // do a single step when no number provided
			stepCount = 1;
		}
		for (int i = 0; i < stepCount; i++) {
			PEBB_Control_step();
		}
		xil_printf("Finished %d PEBB control step(s)\n\r", stepCount);
	} else if (!strcmp(stri1, "dump")) {
		dump();
	} else if (!strcmp(stri1, "list")) {
		int i;
		xil_printf("\nDebug variables: \r\n");
		for (i = 0; i < VARS_LENGTH; i++) {
			xil_printf("%s, ", vs[i]);
			if (i%5 == 0) {
				xil_printf("\r"); // add a new line every 5
			}
		}
		xil_printf("\n\r");
	} else if (stri2[0] != 0) { // is an assignment command
		int varNum = getVarNum(stri1);
		//xil_printf("write \"%s\", varNum = %d\n\r, stri2 = %s\n\r", stri1, varNum, stri2); // debug printf
		if (varNum < -1 || varNum > VARS_LENGTH) {
			xil_printf("Unrecognized input \"%s\"\n\r.", stri1);
		} else { // set a single value of array at this index
			setVar(varNum, strIndex, stri2);
			if (lastFlag) {
				printValue(varNum); // print to confirm it was set (only on the last index)
			}
		}
	} else { // is a read command
		int varNum = getVarNum(stri1);
		//xil_printf("read \"%s\", varNum = %d\n\r", stri1, varNum); // debug printf
		if (varNum != -1) {
			printValue(varNum);
		} else {
			xil_printf("Unrecognized input \"%s\"\n\r.", stri1);
		}
	}
}

/*****************************************************************************/
/*
*
* Set the value of the given variable number to the given string value
*
* @param	int varNum. number of variable in global vs[]
* 			s8 index. index of this array value.
* 			char* val. Value to set as a string.
*
* @return	None
*
* @note     If val is a 0 or null, the variable is set to 0.
******************************************************************************/
static void setVar(int varNum, char* indexChar, char* val) {
	int index = atoi(indexChar);
	//xil_printf("setVar index %d (string %s) out of %d for variable number %d\r\n", index,
	//		indexChar, ls[varNum], varNum); // debug printf
	if (varNum < 0 || varNum >= VARS_LENGTH) {
		xil_printf("Error: input not found exception\r\n");
	} else if (index < 0 || index >= ls[varNum]) {
		xil_printf("Error: invalid index %d out of 0-%d\r\n", index, ls[varNum]-1);
	} else {
		switch(ty[varNum]){

		case int_:
			((int*) pt[varNum])[index] = atoi(val);
			break;
		case s32_:
			((s32*) pt[varNum])[index] = (s32) atoi(val);
			break;
		case u32_:
			((u32*) pt[varNum])[index] = (u32) atoi(val);
			break;
		case double_:
			((double*) pt[varNum])[index] = atof(val);
			break;
		case float_:
			((float*) pt[varNum])[index] = (float) atof(val);
			break;
		case bool_:
			((bool*) pt[varNum])[index] = (bool) atoi(val);
			break;
		default:
			xil_printf("Error: Unimplemented type for input \"%s\"\r\n",vs[varNum]);
		}
	}
}

/*****************************************************************************/
/*
*
* Prints the value of the given variable number
*
* @param	int varNum. number of variable in global vs[]
*
* @return	None
*
* @note
******************************************************************************/
static void printValue(int varNum) {
	int i;
	if (varNum < 0 || varNum > VARS_LENGTH) {
		xil_printf("Error: input not found exception");
	}
	if (ty[varNum] == float_ || ty[varNum] == double_) {
		// Note: don't intermix printf and xil_printf, order is destroyed
		printf("%s = ", vs[varNum]);
		if (ls[varNum] > 1) {
			printf("[");
		}
		for (i = 0; i < ls[varNum]; i++) {
			if (ty[varNum] == float_) {
				printf("%f, ",((float*) pt[varNum])[i]);
			} else { // double
				printf("%f, ",((double*) pt[varNum])[i]);
			}
		}
		if (ls[varNum] > 1) {
			printf("]");
		}
		printf("\n");
	} else {
		xil_printf("%s = ", vs[varNum]);
		if (ty[varNum] == int_ || ty[varNum] == s32_ || ty[varNum] == u32_) {
			xil_printf("0d");
		}
		if (ls[varNum] > 1) {
			xil_printf("[");
		}
		for (i = 0; i < ls[varNum]; i++) {
			switch(ty[varNum]) {
			case int_:
				xil_printf("%d, ",((int*) pt[varNum])[i]);
				break;
			case s32_:
				xil_printf("%d, ",((s32*) pt[varNum])[i]);
				break;
			case u32_:
				xil_printf("%d, ",((u32*) pt[varNum])[i]);
				break;
			case bool_:
				xil_printf("%d, ",((bool*) pt[varNum])[i]);
				break;
			default:
				xil_printf("Error: Unimplemented type for input \"%s\"\r\n",vs[varNum]);
			}
		}
		if (ls[varNum] > 1) {
			xil_printf("]");
		}
		if (ty[varNum] == int_ || ty[varNum] == s32_ || ty[varNum] == u32_) {
			xil_printf(" = 0x"); // print hex also
			if (ls[varNum] > 1) {
				xil_printf("[");
			}
			for (i = 0; i < ls[varNum]; i++) {
				switch(ty[varNum]) {
				case int_:
					xil_printf("%x, ",((int*) pt[varNum])[i]);
					break;
				case s32_:
					xil_printf("%x, ",((s32*) pt[varNum])[i]);
					break;
				case u32_:
					xil_printf("%x, ",((u32*) pt[varNum])[i]);
					break;
				case bool_:
					xil_printf("%x, ",((bool*) pt[varNum])[i]);
					break;
				default:
					// do nothing
				}
			}
			if (ls[varNum] > 1) {
				xil_printf("]");
			}
		}
		xil_printf("\r\n");
	}
}

/*****************************************************************************/
/*
*
* Returns the index of the given variable string
*
* @param	char* stri
*
* @return	int index of the variable in vs.
*
* @note     Returns -1 if not found
******************************************************************************/
static int getVarNum(char* stri) {
	int i;
	for (i = 0; i< VARS_LENGTH; i++) {
		if(!strcmp(stri, vs[i])) {
			return i;
		}
	}
	return -1;
}

/*****************************************************************************/
/*
*
* XUartPs setup routine
*
* @param	None
*
* @return	int (error code)
*
* @note		Baud 115200 and data bits = 8
*
******************************************************************************/
int setupUART(void) {
	int Status;

	/*
	 * Initialize the UART driver so that it's ready to use.
	 * Look up the configuration in the config table, then initialize it.
	 */
	XUartPs_Config *Config;
	Config = XUartPs_LookupConfig(UART_DEVICE_ID);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	Status = XUartPs_CfgInitialize(&Uart_PS, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Check hardware build. */
	Status = XUartPs_SelfTest(&Uart_PS);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Use normal mode. */
	XUartPs_SetOperMode(&Uart_PS, XUARTPS_OPER_MODE_NORMAL);

	initInputArrays();
	return XST_SUCCESS;
}

/*****************************************************************************/
/*
*
* XUartPs read routine. Any input before a carriage return or equal sign is
* assigned to str. Any input after an equal sign is assigned to str2.
* Any input after a '[' but before '=' is assigned to strIndex.
*
* @param	None
*
* @return	None
*
* @note		Must first set up with setupUART()
******************************************************************************/
void readUART(void) {
	recv_cnt = XUartPs_Recv(&Uart_PS, (u8 *) buf, 100);
	for(int i = 0; i < recv_cnt; i++)
	{
		if(buf[i] == '\r' || buf[i] == ';') // end of input
		{
			str[total_recv_cnt] = 0; // add end of string characters
			str2[total_recv_cnt2] = 0;
			if (commaCount == 0) { // no commas in assignment (whole array read)
				strIndex[total_recv_cnt_index] = 0; // terminate the string
			} else { // commas were used in assignment
				itoa(commaCount, strIndex, 10); // convert comma count to ASCII in strIndex for parse input
			}
			parseInput(str, strIndex, str2, 1);
			total_recv_cnt = 0; // reset the count
			total_recv_cnt2 = 0; // reset the count
			total_recv_cnt_index = 0;
			recv_cnt = 0; // reset the count
			seenEquals = 0;
			seenIndex = 0;
			commaCount = 0;
		} else if (buf[i] == '[') {
			seenIndex = 1 ;
		} else if (buf[i] == '=') { // don't put = in either string
			seenEquals = 1;
			commaCount = 0; // reset comma count in case any accidental commas before =
		} else if (seenEquals && buf[i] == ',') { // set another index of variable
			str2[total_recv_cnt2] = 0; // add end of string
			str[total_recv_cnt] = 0; // add end of string characters
			itoa(commaCount, strIndex, 10); // convert comma count to ASCII in strIndex for parse input
			parseInput(str, strIndex, str2, 0);
			commaCount++;
			total_recv_cnt2 = 0; // reset the count for part after =
		} else if (buf[i] == ' ' || buf[i] == ']' || buf[i] == '{' || buf[i] == '}' || buf[i] == ',') {
			// ignore these characters
		} else if (seenEquals) { // put in str2
			str2[total_recv_cnt2] = buf[i];
			total_recv_cnt2++;
		} else if (seenIndex) { // put in strIndex
			strIndex[total_recv_cnt_index] = buf[i];
			total_recv_cnt_index++;
		} else { // put in str
			str[total_recv_cnt]=buf[i];
			total_recv_cnt++;
		}
	}
}

// dump information on all debug signals and some parameters
void dump() {
	printf("\n");
	printf("Debug variables: \n");
	int i;
	for (i = 0; i < VARS_LENGTH; i++) {
		printValue(i);
	}
}
