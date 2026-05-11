/**
 *
 * @file Axi_IO.c
 * This defines data to be sent/received from PL through the AXI FIFO
 * Xilinx drivers are used to send/receive data, and the data is mapped to addresses
 * contained in this file
 *
 * Steps to use this file
 * 1. Include any header files required to access pointers the necessary memory addresses
 * 2. Create/modify axiData structs in this file for all RX words and TX words
 * 3. Put all TX words in txData and all rxWords in rxData
 * 4. In Axi_IO.h, update RECEIVE_LENGTH to match length(rxData),
 *    and update TRANSMIT_LENGTH to match length(txData)
 *
 * (optional):
 * If any bits are to be packed into a single word, use the packBits function
 * 5. Create/modify bitData structs in this file for all the bits to pack
 * 6. Put all bitData struct pointers in bits[]
 * 7. In Axi_IO.h, update NUM_PACKED_BITS to match the length of bits[]
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.0   BDR  1/23/2023  Initial revision
 * 2.0   BDR  9/26/2023  Changed to generic PEBB
 *
 * ***************************************************************************
 */

#include <stdbool.h>
#include <stdio.h>
#include "Axi_IO.h"
#include "xllfifo.h"

// for getting addresses of variables
#include "codegen.h"
#include "MVDC.CLI\control_functions.h"
#include "MVDC.CLI\globals.h"

extern struct GLOBALVARS vars;
extern volatile int run;
extern volatile int CHIL_error;

// vars for data/debugging
s32 key; // the read/ignore key received over Aurora (first packet)
s32 ctr; // the substep counter integer received over Aurora
s32 seq; // the sequence number received over Aurora
s32 seq_prev; // previous sequence number received (0 if previous packet invalid)
s32 RXI_chil; // set to 1 over Aurora to close loop on RXi

u32 packedInt; // firing words packed into one int

volatile u32 ReceiveLength;  // length of received packet in words

volatile int printNaNCtr; // a counter to slow down prints on NaN RX values
volatile int printNaNTxCtr; // a counter to slow down prints on NaN TX values
#define PRINT_NAN_FREQ 8000 // print error with this frequency

volatile int commGlitchCtr; // a counter for the number of comm glitches
#define COMM_GLITCH_LIMIT 10 // raise the error if a certain number of comm glitches occur


volatile int NanTxCtr;
volatile int NanRxCtr;
// set to 5 or more because a few bad packets at startup occasionally happens

// dummy variables for sending/receiving data from unused indices in Aurora AXI buffer
// NOTE: these were added to retain compatibility with MFESM PEBB code & RTDS Aurora I/O list. 9/27/2023
static volatile int dummyIntTx;
static volatile double dummyDoubleTx;
static volatile bool dummyBoolTx;
static volatile int dummyIntRx;
static volatile double dummyDoubleRx;


//--------------------------------------------------------------------------------
// Define received packets
//--------------------------------------------------------------------------------


axiData rxData1 = {
	.ptr = (void*) &key,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

axiData rxData2 = { // i_high
	.ptr = (void*) &PEBB_Control_U.feedback[0],
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData3 = { // vhigh (B phase AC)
	.ptr = (void*) &PEBB_Control_U.feedback[4],
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData4 = { // vhigh (A phase AC)
	.ptr = (void*) &PEBB_Control_U.feedback[3],
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData5 = { // v_dcc
	.ptr = (void*) &PEBB_Control_U.feedback[7],
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData6 = { // i_low
	.ptr = (void*) &PEBB_Control_U.feedback[6],
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData7 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData8 = {
	.ptr = (void*) &PEBB_NEXT_STATE_CMD,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData rxData9 = {
	.ptr = (void*) &P_CMD,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE // Note: MW is RTDS, kW in PLECS
};

axiData rxData10 = {
	.ptr = (void*) &LOWSIDE_CONTACTOR_CMD,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData rxData11 = {
	.ptr = (void*) &PEBB_POWER_LOWER_LIMIT,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE*UNIT_SCALE // Note: MW in RTDS, W in PLECS
};

axiData rxData12 = {
	.ptr = (void*) &PEBB_POWER_UPPER_LIMIT,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE*UNIT_SCALE // Note: MW in RTDS, W in PLECS
};

axiData rxData13 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData14 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData15 = {
	.ptr = (void*) &ctr,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

axiData rxData16 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData17 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};


axiData rxData18 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

// NOTE: this was PEBB curr_ff, now unused
axiData rxData19 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData20 = {
	.ptr = (void*) &V_CMD,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData21 = {
	.ptr = (void*) &PEBB_Control_U.ForceFault,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData rxData22 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData rxData23 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData rxData24 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData rxData25 = {
	.ptr = (void*) &dummyDoubleRx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData rxData26 = {
	.ptr = (void*) &EN_GRID_SUPPORT,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData rxData27 = {
	.ptr = (void*) &Q_CMD,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE // NOTE: this command is in kVA in code/PLECS and MVA in RTDS
};

axiData rxData28 = {
	.ptr = (void*) &Q_LIMIT,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE*UNIT_SCALE
};

axiData rxData29 = {
	.ptr = (void*) &PEBB_Control_U.feedback[1], // i_high_B
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData30 = {
	.ptr = (void*) &PEBB_Control_U.feedback[2], // i_high_C
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData31 = {
	.ptr = (void*) &PEBB_Control_U.feedback[5], // v_high_c
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData32 = {
	.ptr = (void*) &PEBB_Control_U.feedback[8], // v_stator_A
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData33 = {
	.ptr = (void*) &PEBB_Control_U.feedback[9], // v_stator_B
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData34 = {
	.ptr = (void*) &PEBB_Control_U.feedback[10], // v_stator_C
	.axiType = float_,
	.localType = double_,
	.scaleFactor = UNIT_SCALE
};

axiData rxData35 = {
	.ptr = (void*) &run,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

axiData rxData36 = {
	.ptr = (void*) &RXI_chil,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};


axiData rxDataSeq = {
	.ptr = (void*) &seq,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

// NOTE: remember to add new rxData to array *rxData below


//--------------------------------------------------------------------------------
// Define transmit packets
//--------------------------------------------------------------------------------


axiData txData1 = {
	.ptr = (void*) &PEBB_Control_Y.switch_1[0], // AC Phase A duty
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData2 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData3 = {
	.ptr = (void*) &packedInt,
	.axiType = u32_,
	.localType = u32_,
	.scaleFactor = 1
};

axiData txData4 = {
	.ptr = (void*) &ctr,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

axiData txData5 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData6 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData7 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData8 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData9 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData10 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData11 = {
	.ptr = (void*) &vars.next_state,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

axiData txData12 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData13 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData14 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData15 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData16 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData17 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData18 = {
	.ptr = (void*) &PEBB_Control_Y.switch_1[1], // AC phase B
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData19 = {
	.ptr = (void*) &PEBB_Control_Y.switch_1[2], // AC phase C
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData20 = {
	.ptr = (void*) &vars.current_state,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

axiData txData21 = {
	.ptr = (void*) &dummyDoubleTx,
	.axiType = float_,
	.localType = double_,
	.scaleFactor = 1
};

axiData txData22 = {
	.ptr = (void*) &CHIL_error,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

axiData txDataSeq = {
	.ptr = (void*) &seq,
	.axiType = s32_,
	.localType = s32_,
	.scaleFactor = 1
};

// NOTE: remember to add additional txData to array *txData below


//--------------------------------------------------------------------------------
// Define bits to pack into a single tx word
//--------------------------------------------------------------------------------


bitData bit0 = {
	.boolPtr = NULL,
	.doublePtr = &PEBB_Control_Y.lowside_contact,
	.bitPos = 0
};

bitData bit1 = {
	.boolPtr = NULL,
	.doublePtr = &PEBB_Control_Y.enable,
	.bitPos = 1
};

bitData bit2 = {
	.boolPtr = NULL,
	.doublePtr = &PEBB_Control_Y.highside_contact,
	.bitPos = 2
};

bitData bit3 = {
	.boolPtr = &PEBB_Control_Y.not_fault,
	.doublePtr = NULL,
	.bitPos = 3
};

bitData bit4 = {
	.boolPtr = &dummyBoolTx,
	.doublePtr = NULL,
	.bitPos = 4
};

bitData bit5 = {
	.boolPtr = &dummyBoolTx,
	.doublePtr = NULL,
	.bitPos = 5
};

bitData bit6 = {
	.boolPtr = &dummyBoolTx,
	.doublePtr = NULL,
	.bitPos = 6
};

bitData bit7 = {
	.boolPtr = NULL,
	.doublePtr = &PEBB_Control_Y.precharge_contact, // precharge contact
	.bitPos = 7
};

// NOTE: remember to add new bits to *bits below

//--------------------------------------------------------------------------------
// Create arrays of pointers to all data structs
//--------------------------------------------------------------------------------

volatile axiData *rxData[RECEIVE_LENGTH] = {&rxData1, &rxData2, &rxData3, &rxData4, &rxData5,
		&rxData6, &rxData7, &rxData8, &rxData9, &rxData10, &rxData11, &rxData12, &rxData13,
		&rxData14, &rxData15, &rxData16, &rxData17, &rxData18, &rxData19, &rxData20,
		&rxData21, &rxData22, &rxData23, &rxData24, &rxData25, &rxData26, &rxData27,
		&rxData28, &rxData29, &rxData30, &rxData31, &rxData32, &rxData33, &rxData34, &rxData35, &rxData36,
		&rxDataSeq};

volatile axiData *txData[TRANSMIT_LENGTH] = {&txData1, &txData2, &txData3, &txData4, &txData5,
		&txData6, &txData7, &txData8, &txData9, &txData10, &txData11, &txData12, &txData13,
		&txData14, &txData15, &txData16, &txData17, &txData18, &txData19, &txData20, &txData21, & txData22, &txDataSeq};

volatile bitData *bits[NUM_PACKED_BITS] = {&bit7, &bit6, &bit5, &bit4, &bit3, &bit2, &bit1, &bit0};

//--------------------------------------------------------------------------------
// Define functions
//--------------------------------------------------------------------------------

/*****************************************************************************/
/**
*
* axiReceive routine, it will  receive all data from axi bus to the locations
* described in *rxData
*
* @param	InstancePtr is a pointer to the instance of the
*		XLlFifo component.
*
* @return   void
*
* @note		None
*
******************************************************************************/
void axiReceive(XLlFifo *InstancePtr)
{
	int i;
	u32 RxWord;
	seq_prev = seq;

	if (XLlFifo_iRxOccupancy(InstancePtr)) {
		/* Read Receive Length */
		ReceiveLength = (XLlFifo_iRxGetLen(InstancePtr))/WORD_SIZE;

		if (ReceiveLength != RECEIVE_LENGTH) { // if incorrect packet size
			// empty the RX buffer, but do not load the invalid data to controller
			for (i=0; i < ReceiveLength; i++) {
				RxWord = XLlFifo_RxGetWord(InstancePtr);
			}
			//XLlFifo_RxReset(InstancePtr);
			seq = 0; // to set seq_prev to 0 at next cycle after invalid data
			xil_printf("Warning: receive Length mismatch. Received 0d%d words. Expected 0d%d.\r\n", ReceiveLength, RECEIVE_LENGTH);
			commGlitchCtr++;
			if (commGlitchCtr > COMM_GLITCH_LIMIT) {
				CHIL_error |= 1;
			}
		} else { // if correct packet size received
			type_ thisAxiType;
			type_ thisLocalType;
			float unitScale;
			float RxValue;
			int isNan;
			for (i=0; i < RECEIVE_LENGTH; i++) {
				RxWord = XLlFifo_RxGetWord(InstancePtr);
				thisAxiType = rxData[i]->axiType;
				thisLocalType = rxData[i]->localType;
				unitScale = rxData[i]->scaleFactor;
				isNan = 0;
				// check for NaN
				if (thisAxiType == float_) {
					RxValue = (*(float *) &RxWord );
					if (RxValue != RxValue) { // check for NaN values on floats
						isNan = 1;
					}
				}
				if (isNan) {
					printNaNCtr++;
					CHIL_error |= 1;
					NanRxCtr++;
					//if (printNaNCtr % PRINT_NAN_FREQ == 0) {
					if (printNaNCtr == 1) { // only print first time
						xil_printf("Error: NaN received for Aurora word 0d%d. Check NanRxCtr.\n\r", 1+i);
					}
				} else if ( (thisAxiType == u32_ || thisAxiType == s32_ || thisAxiType == float_) &&
						thisLocalType == thisAxiType && unitScale == 1) { // no scaling required
					u32* rxPtr = (u32 *) rxData[i]->ptr;
					*rxPtr = RxWord;
				} else if (thisAxiType == float_ && thisLocalType == float_){ // is a float that needs to be scaled
					// cast u32 ptr to float ptr, use float multiplication, and assign result with float pointer
					float* rxPtr = (float*) rxData[i]->ptr;
					RxValue = (*(float *) &RxWord );
					*rxPtr = RxValue * unitScale;
				} else if (thisAxiType == float_ && thisLocalType == double_){
					// cast u32 ptr to float ptr, use float multiplication, and cast result to double
					double* rxPtr = (double*) rxData[i]->ptr;
					RxValue = (*(float *) &RxWord );
					*rxPtr = (double) RxValue * unitScale;
				} else {
					xil_printf("Error: types not implemented in axiReceive for word 0d%d\n\r", 1+i);
					CHIL_error |= 1;
				}
			}
		}
	} else {
		xil_printf("Error: RX interrupt with no RX FIFO Occupancy");
		CHIL_error |= 1;
	}
#ifdef SEQ_INCREMENT
	if (seq && seq_prev && seq > 64 && seq%SEQ_MAX != (seq_prev+SEQ_INCREMENT)%SEQ_MAX ) {
		// if sequence number increment doesn't match the expected (packet missed)
		// have to exclude seq <= 64 as RTDS could reset at any time
		xil_printf("Warning: missing packet sequence number after 0d%d, received 0d%d\n\r", seq_prev, seq);
	}
#endif
}



/*****************************************************************************/
/**
*
* axiSend routine, it will send all data in *txData above to the axi bus
*
* @param	InstancePtr is a pointer to the instance of the
*		XLlFifo component.
*
* @return   void
*
* @note		axiSend function currently assumes transmitted data needs no
*           conversion or scaling
*
******************************************************************************/
void axiSend(XLlFifo *InstancePtr)
{
	packedInt = packBits(); // pack all the firing bits into one int

	int i;
	type_ thisAxiType;
	type_ thisLocalType;
	float unitScale;
	for(i=0; i < TRANSMIT_LENGTH; i++){
		/* Writing into the FIFO Transmit Port Buffer */
		if( XLlFifo_iTxVacancy(InstancePtr) ){
			thisAxiType = txData[i]->axiType;
			thisLocalType = txData[i]->localType;
			unitScale = txData[i]->scaleFactor;
			// check for NaN on outputs, send anyway (must send something)
			if ( (thisAxiType == u32_ || thisAxiType == s32_ ||
					(thisAxiType == float_ && unitScale ==1)) && (thisLocalType == thisAxiType) ) {
				if (thisLocalType == float_ && *(float *) txData[i]->ptr != *(float *) txData[i]->ptr) {
					printNaNTxCtr++;
					CHIL_error |= 1;
					//if (printNaNTxCtr % PRINT_NAN_FREQ == 0) {
					NanTxCtr++;
					if (printNaNTxCtr == 1) { // print first time
						xil_printf("Error: NaN output for Aurora TX word 0d%d. Check NanTxCtr.\n\r", 1+i);
					}
				}
				XLlFifo_TxPutWord(InstancePtr, *(u32 *) txData[i]->ptr);
			} else if (thisAxiType == float_ && thisLocalType == double_) {
				if (*(double *) txData[i]->ptr != *(double *) txData[i]->ptr) {
					printNaNTxCtr++;
					CHIL_error |= 1;
					NanTxCtr++;
					//if (printNaNTxCtr % PRINT_NAN_FREQ == 0) {
					if (printNaNTxCtr == 1) { // print first time
						xil_printf("Error: NaN output for Aurora TX word 0d%d. Check NanTxCtr.\n\r", 1+i);
					}
				}
				// dereference double pointer, cast to float, scale
				// then cast float pointer to u32 pointer, dereference, and give to driver
				float txWord = unitScale * (float) (*(double *) txData[i]->ptr);
				XLlFifo_TxPutWord(InstancePtr, *(u32 *) &txWord );
			} else {
				xil_printf("Error: types not implemented in axiSend for word 0d%d\n\r", 1+i);
				CHIL_error |= 1;
			}
		} else {
			xil_printf("Error: no vacancy in TX FIFO\n\r");
			CHIL_error |= 1;
		}
	}
	/* Start Transmission by writing transmission length into the TLR */
	XLlFifo_iTxSetLen(InstancePtr, (TRANSMIT_LENGTH * WORD_SIZE));
}

/*****************************************************************************/
/**
*
* packBits, packs a number of bits in bool/double format into a single u32.
* Used to pack a large number of firing bits in bool or double format into a
* single Aurora word
*
* @param	none (reads from global bitData*[] bits)
*
* @return   packBits. Bits are packed into this u32
*
* @note		none
*
******************************************************************************/
u32 packBits()
{
	u32 pack = 0;
	int i;
	for (i = 0; i < NUM_PACKED_BITS; i++) {
		if ((bits[i]->boolPtr != NULL && *(bits[i]->boolPtr) != 0) ||
				(bits[i]->doublePtr != NULL && *(bits[i]->doublePtr) != 0)) {
			//pack = ((1 << k) | pack); sets kth bit of pack
			pack = ((1 << bits[i]->bitPos) | pack);
		}
	}
	return pack;
}
