/**
 *
 * @file Axi_IO.h
 * This file defines types and constants to be used with receiving/transmitting
 * data over AXI from PL using AXI FIFO.
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.0   BDR  1/23/2023  Initial revision
 * 2.0   BDR  9/26/2023  Changed to generic PEBB
 *
 * ***************************************************************************
 */

#ifndef Axi_IO_h_
#define Axi_IO_h_

#include <stdbool.h>
#include "xllfifo.h"

#define WORD_SIZE 4			/* Size of words in bytes */
#define TRANSMIT_LENGTH 23 // length of transmission in words (Must have seq as last, this is 1+RTDS due to seq)
#define RECEIVE_LENGTH 37 // length of received message in words (NOTE: this is 1 + RTDS due to Seq #)
#define NUM_PACKED_BITS 8 // number of bits packed into the packed tx int

#define UNIT_SCALE 1000 // factor to convert RTDS units (kV or kA) to (V or A)
#define SEQ_MAX 1000 // maximum RTDS sequence number (1-1000)

// comment out SEQ_INCREMENT to not run the sequence increment check
//#define SEQ_INCREMENT 36 // sequence number increment per valid packet, equal to substeps per main step

//--------------------------------------------------------------------------------
// Define types of data
//--------------------------------------------------------------------------------

// list of type options for storing data locally on processor (32 or 64 bits)
// for tx, all are ultimately passed in as u32*, but data is represented in different ways
typedef enum Type_ {double_, float_, u32_, u64_, s32_, int64_} type_;

typedef struct {
	void* ptr; // point to local address to write to
	type_ localType;
	type_ axiType;
	float scaleFactor;
} axiData;

typedef struct {
	bool* boolPtr; // use either the bool pointer or double pointer
	double* doublePtr;
	u32 bitPos; // the bit in this position is controlled by boolPtr or doublePtr
} bitData;

//--------------------------------------------------------------------------------
// Function Prototypes
//--------------------------------------------------------------------------------

void axiReceive(XLlFifo *InstancePtr);
void axiSend(XLlFifo *InstancePtr);
u32 packBits();

#endif                                 /* Axi_IO_h_ */
