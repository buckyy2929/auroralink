/******************************************************************************
* Copyright (C) 2013 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file xllfifo_interrupt_example.c
 * This file has been modified to serve as the main for the PEBB_Chil project
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 3.00a adk 08/10/2013 initial release CR:727787
 * 5.1   ms  01/23/17   Modified xil_printf statement in main function to
 *                      ensure that "Successfully ran" and "Failed" strings
 *                      are available in all examples. This is a fix for
 *                      CR-965028.
 *       ms  04/05/17   Added tabspace for return statements in functions for
 *                      proper documentation and Modified Comment lines
 *                      to consider it as a documentation block while
 *                      generating doxygen.
 * 5.3  rsp 11/08/18    Modified TxSend to fill SourceBuffer with non-zero
 *                      data otherwise the test can return a false positive
 *                      because DestinationBuffer is initialized with zeros.
 *                      In fact, fixing this exposed a bug in FifoRecvHandler
 *                      and caused the test to start failing. According to the
 *                      product guide (pg080) for the AXI4-Stream FIFO, the
 *                      RDFO should be read before reading RLR. Reading RLR
 *                      first will result in the RDFO being reset to zero and
 *                      no data being received.
 * 1.0 ps&bdr 3/7/2023  Modified to use as main for MFESM_Chil project using
 * 						interrupt structure
 * 2.0 bdr 4/24/2023    Modifications for AC system on a chip (SOC, single Kria) from DC.
 * 						Renamed file to main.c
 * 3.0 BDR 9/26/2023    Changed to generic PEBB
 * </pre>
 *
 * ***************************************************************************
 */

/***************************** Include Files *********************************/
#include <stdio.h>
#include "xparameters.h"
#include "xil_exception.h"
#include "xstreamer.h"
#include "xil_cache.h"
#include "xllfifo.h"
#include "xstatus.h"
#include "xuartps.h"
#include "xil_printf.h"

#include "Axi_IO.h"
#include "UART.h"

#include "codegen.h"

#ifdef XPAR_UARTNS550_0_BASEADDR
#include "xuartns550_l.h"       /* to use uartns550 */
#endif

#ifdef XPAR_INTC_0_DEVICE_ID
 #include "xintc.h"
#else
 #include "xscugic.h"
#endif

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

#define FIFO_DEV_ID	   	XPAR_AXI_FIFO_0_DEVICE_ID

#ifdef XPAR_INTC_0_DEVICE_ID
#define INTC_DEVICE_ID          XPAR_INTC_0_DEVICE_ID
#define FIFO_INTR_ID		XPAR_INTC_0_LLFIFO_0_VEC_ID
#else
#define INTC_DEVICE_ID          XPAR_SCUGIC_SINGLE_DEVICE_ID
#define FIFO_INTR_ID            XPAR_FABRIC_LLFIFO_0_VEC_ID
#endif

#ifdef XPAR_INTC_0_DEVICE_ID
 #define INTC           XIntc
 #define INTC_HANDLER   XIntc_InterruptHandler
#else
 #define INTC           XScuGic
 #define INTC_HANDLER   XScuGic_InterruptHandler
#endif

/************************** Constant Definitions *****************************/

// constants for coordinating timing of different controller frame rates
#define CTR_MAX 8 // max value of ctrlCtr
#define PEBB_RATE 1 // run PEBB controller every PEBB_RATE interrupts (8 kHz)



#undef DEBUG

/************************** Function Prototypes ******************************/
#ifdef XPAR_UARTNS550_0_BASEADDR
static void Uart550_Setup(void);
#endif

int XLlFifoInterruptExample(XLlFifo *InstancePtr, u16 DeviceId);
int TxSend(XLlFifo *InstancePtr);
static void FifoHandler(XLlFifo *Fifo);
static void FifoRecvHandler(XLlFifo *Fifo);
static void FifoSendHandler(XLlFifo *Fifo);
static void FifoErrorHandler(XLlFifo *InstancePtr, u32 Pending);
int SetupInterruptSystem(INTC *IntcInstancePtr, XLlFifo *InstancePtr,
				u16 FifoIntrId);
static void DisableIntrSystem(INTC *IntcInstancePtr, u16 FifoIntrId);

/*
 * Flags interrupt handlers use to notify the application context the events.
 */
volatile int Rx_Done;
volatile int Tx_Done;
volatile int Error;
volatile int controllerStarted = 0; // set to 1 once controller starts running
volatile int controllerStopped = 0; // set to 1 if controller receives stop command after starting

/************************** Variable Definitions *****************************/

/*
 * Device instance definitions
 */
XLlFifo FifoInstance;

/*
 * Instance of the Interrupt Controller
 */
static INTC Intc;

double initTime = 0; // initial time

int ctrlCtr = 0; // counter for timing the controllers
volatile int runPrev = 0; // previous state of run

volatile int intOverrunFlag = 0; // set to 1 if one interrupt cycle isn't finished before the next comes in
int printedOverrunFlag = 0; // set to 1 if printed the overrun error already (avoids spamming the UART)
int printedStopFlag = 0;
volatile int dumpFlag = 0; // set 1 if a core dump is pending

extern volatile u32 ReceiveLength;  // length of received packet in words

volatile int CHIL_error = 0; // use to send out an error flag. Kept separate from the example Error
// because that one is used internally and reset if the error is handled.


/************************** User Inputs *****************************/

int ctrlOn = 1; // use to enable/disable the controller manually
volatile int run = 0; // sent over Aurora to tell controller to run
int dumpOnStop = 0; // set 1 to dump debug vars when run switched to 0

/*****************************************************************************/
/**
*
* Main function
*
* This function is the main entry of the AXI FIFO interrupt test.
*
* @param	None
*
* @return
*		- XST_SUCCESS if tests pass
* 		- XST_FAILURE if fails.
*
* @note		None
*
******************************************************************************/
int main()
{
	int Status;

	// setup UART (must be done before any prints)
	if (setupUART() == XST_FAILURE) {
		return XST_FAILURE;
	}

	xil_printf("--- Entering main() ---\n\r");

	while (1) {

	Status = XLlFifoInterruptExample(&FifoInstance, FIFO_DEV_ID);

	if (Status != XST_SUCCESS) break;
	}

	{
		xil_printf("Axi Streaming FIFO Interrupt Example Test Failed");
		xil_printf("--- Exiting main() ---\n\r");
		return XST_FAILURE;
	}

	xil_printf("Successfully ran Axi Streaming FIFO Interrupt Example\n\r");
	xil_printf("--- Exiting main() ---\n\r");

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function demonstrates the usage of AXI FIFO
* It does the following:
*       - Set up the output terminal if UART16550 is in the hardware build
*       - Initialize the Axi FIFO Device.
*	- Set up the interrupt handler for fifo
*	- Transmit the data
*	- Compare the data
*	- Return the result
*
* @param	InstancePtr is a pointer to the instance of the
*		XLlFifo instance.
* @param	DeviceId is Device ID of the Axi Fifo Device instance,
*		typically XPAR_<AXI_FIFO_instance>_DEVICE_ID value from
*		xparameters.h.
*
* @return
*		-XST_SUCCESS to indicate success
*		-XST_FAILURE to indicate failure
*
******************************************************************************/
int XLlFifoInterruptExample(XLlFifo *InstancePtr, u16 DeviceId)
{
	XLlFifo_Config *Config;
	int Status;
	Status = XST_SUCCESS;

	/* Initial setup for Uart16550 */
#ifdef XPAR_UARTNS550_0_BASEADDR

	Uart550_Setup();

#endif

	/* Initialize the Device Configuration Interface driver */
	Config = XLlFfio_LookupConfig(DeviceId);
	if (!Config) {
		xil_printf("No config found for %d\r\n", DeviceId);
		return XST_FAILURE;
	}

	/*
	 * This is where the virtual address would be used, this example
	 * uses physical address.
	 */
	Status = XLlFifo_CfgInitialize(InstancePtr, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed\n\r");
		return Status;
	}

	/* Check for the Reset value */
	Status = XLlFifo_Status(InstancePtr);
	XLlFifo_IntClear(InstancePtr,0xffffffff);
	Status = XLlFifo_Status(InstancePtr);
	if(Status != 0x0) {
		xil_printf("\n ERROR : Reset value of ISR0 : 0x%x\t"
			    "Expected : 0x0\n\r",
			    XLlFifo_Status(InstancePtr));
		return XST_FAILURE;
	}

	/*
	 * Connect the Axi Streaming FIFO to the interrupt subsystem such
	 * that interrupts can occur. This function is application specific.
	 */
	Status = SetupInterruptSystem(&Intc, InstancePtr, FIFO_INTR_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed intr setup\r\n");
		return XST_FAILURE;
	}

	XLlFifo_IntEnable(InstancePtr, XLLF_INT_ALL_MASK);

	Rx_Done = Tx_Done = Error = 0;
	setPEBBLRU(); // initialize the LRU count to the default

	PEBB_Control_initialize(initTime);
	initPEBB();

	xil_printf("Type \"help\" for help with console options.\n\r");

	xil_printf("Paused. Please Start RTDS model now. \n\r");

	int printedStartupMsg = 0; // used to only print a message once after the loop starts
	while(1)  {
		// put any code here that runs in background while waiting for RX

		if (controllerStarted && !printedStartupMsg) {
			xil_printf("Running. Please restart controller by typing 'reset' in the terminal before restarting RTDS. \n\r");
			printedStartupMsg = 1;
		}
		if (Error && Rx_Done) {
			/* Check for errors */
			if(Error) {
				xil_printf("Errors in the FIFO\n\r");
				CHIL_error |= 1;
				return XST_FAILURE;
			}
		}

		if (intOverrunFlag) { // && !printedOverrunFlag)
			xil_printf("Error: receive interrupt occurred while previous transmit cycle not finished.\n\r");
			CHIL_error |=1;
			printedOverrunFlag = 1; // keep a permanent flag that an error occured
			intOverrunFlag = 0; // clear the overrun flag
		}

		if (controllerStopped && !printedStopFlag) {
			xil_printf("Received stop command. Restart RTDS model to restart. \n\r");
			printedStopFlag = 1;
		}

		readUART();

		// NOTE: moved control code into a function called by RX interrupt routine
		// This is to ensure controller runs in real time instead of waiting for UART read to finish

		if (dumpFlag) {
			dump();
			dumpFlag = 0;
		}

	}

	DisableIntrSystem(&Intc, FIFO_INTR_ID);
	return Status;
}

/*****************************************************************************/
/**
*
* Control routine for the PEBB
* Also initiates the transit routine when control cycle finished
*
* @param	InstancePtr is a pointer to the instance of the
*		XLlFifo component.
*
* @return
*		-XST_SUCCESS to indicate success
*		-XST_FAILURE to indicate failure
*
* @note		Must be called at 8 kHz control frame rate
*
******************************************************************************/
int runControlCycle(XLlFifo *InstancePtr)
{
	if (!run && controllerStarted) { // the stop command has been sent after starting
		// TO DO: wire in PEBB reset logic to reset the controllers if start after
		// stopping. Until that is done the controllerStopped signal prevents the controllers from restarting
		// without being reset.

		controllerStopped = 1;

		// put controllers in fault state to use freewheeling diode path
		PEBB_Control_U.ForceFault = 1;
	}
	if ((ctrlOn && run && ReceiveLength == RECEIVE_LENGTH) || controllerStopped) { // only run controllers if valid comms received
		controllerStarted = 1; // set the status flag that the control interrupt has started running
		if (ctrlCtr % PEBB_RATE == 0) {
			PEBB_Control_step();
		}
		ctrlCtr++;
		if (ctrlCtr == CTR_MAX) {
			ctrlCtr = 0;
		}
	}

	if (TxSend(InstancePtr) != XST_SUCCESS){
		xil_printf("Transmission of Data failed\n\r");
		return XST_FAILURE;
	}
	if (run == 0 && runPrev == 1 && dumpOnStop == 1) {
		dumpFlag = 1;
	}
	runPrev = run;
	Rx_Done = Tx_Done = 0;
	return XST_SUCCESS;
}


/*****************************************************************************/
/**
*
* TxSend routine, It will send the requested amount of data at the
* specified addr.
*
* @param	InstancePtr is a pointer to the instance of the
*		XLlFifo component.
*
* @param	SourceAddr is the address of the memory
*
* @return
*		-XST_SUCCESS to indicate success
*		-XST_FAILURE to indicate failure
*
* @note		None
*
******************************************************************************/
int TxSend(XLlFifo *InstancePtr)
{
	axiSend(InstancePtr);
	/* Transmission Complete */
	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This is the interrupt handler for the fifo it checks for the type of interrupt
* and proceeds according to it.
*
* @param	InstancePtr is a reference to the Fifo device instance.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void FifoHandler(XLlFifo *InstancePtr)
{
	u32 Pending;

	Pending = XLlFifo_IntPending(InstancePtr);
	while (Pending) {
		if (Pending & XLLF_INT_RC_MASK) {
			XLlFifo_IntClear(InstancePtr, XLLF_INT_RC_MASK);
			if (Rx_Done) {
				intOverrunFlag = 1; // flag is set to 1 when Rx_Done has not been not cleared yet
			}
			FifoRecvHandler(InstancePtr);
		}
		else if (Pending & XLLF_INT_TC_MASK) {
			XLlFifo_IntClear(InstancePtr, XLLF_INT_TC_MASK);
			if (Tx_Done) {
				intOverrunFlag = 1; // flag is set to 1 when Tx_Done has not been cleared yet
			}
			FifoSendHandler(InstancePtr);
		}
		else if (Pending & XLLF_INT_ERROR_MASK){
			FifoErrorHandler(InstancePtr, Pending);
			XLlFifo_IntClear(InstancePtr, XLLF_INT_ERROR_MASK);
		} else {
			XLlFifo_IntClear(InstancePtr, Pending);
		}
		Pending = XLlFifo_IntPending(InstancePtr);
	}
}

/*****************************************************************************/
/**
*
* This is the Receive handler callback function.
*
* @param	InstancePtr is a reference to the Fifo device instance.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void FifoRecvHandler(XLlFifo *InstancePtr)
{
	//	xil_printf("Receiving Data... \n\r");
	axiReceive(InstancePtr); // receive data from AXI
	Rx_Done = 1;
	setPEBBLRU(); // update the number of LRUs based on info received over AXI
	// run control code after setting Rx_Done flag
	if (runControlCycle(InstancePtr)==XST_FAILURE) { // run control and check for errors
		CHIL_error|=1; // set an error flag
	}
}


/*****************************************************************************/
/*
*
* This is the transfer Complete Interrupt handler function.
*
* This clears the transmit complete interrupt and set the done flag.
*
* @param	InstancePtr is a pointer to Instance of AXI FIFO device.
*
* @return	None
*
* @note		None
*
******************************************************************************/
static void FifoSendHandler(XLlFifo *InstancePtr)
{
	Tx_Done = 1;
}

/*****************************************************************************/
/**
*
* This is the Error handler callback function and this function increments the
* the error counter so that the main thread knows the number of errors.
*
* @param	InstancePtr is a pointer to Instance of AXI FIFO device.
*
* @param	Pending is a bitmask of the pending interrupts.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void FifoErrorHandler(XLlFifo *InstancePtr, u32 Pending)
{
	if (Pending & XLLF_INT_RPURE_MASK) {
		XLlFifo_RxReset(InstancePtr);
	} else if (Pending & XLLF_INT_RPORE_MASK) {
		XLlFifo_RxReset(InstancePtr);
	} else if(Pending & XLLF_INT_RPUE_MASK) {
		XLlFifo_RxReset(InstancePtr);
	} else if (Pending & XLLF_INT_TPOE_MASK) {
		XLlFifo_TxReset(InstancePtr);
	} else if (Pending & XLLF_INT_TSE_MASK) {
	}
	xil_printf("Error code with value Pending 0x%08x\r\n", Pending);
	Error++;
}

/****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur
* for the FIFO device. This function is application specific since the
* actual system may or may not have an interrupt controller. The FIFO
* could be directly connected to a processor without an interrupt controller.
* The user should modify this function to fit the application.
*
* @param    InstancePtr contains a pointer to the instance of the FIFO
*           component which is going to be connected to the interrupt
*           controller.
*
* @return   XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note     None.
*
****************************************************************************/
int SetupInterruptSystem(INTC *IntcInstancePtr, XLlFifo *InstancePtr,
				u16 FifoIntrId)
{

	int Status;

#ifdef XPAR_INTC_0_DEVICE_ID
	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	Status = XIntc_Initialize(IntcInstancePtr, INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Connect a device driver handler that will be called when an interrupt
	 * for the device occurs, the device driver handler performs the
	 * specific interrupt processing for the device.
	 */
	Status = XIntc_Connect(IntcInstancePtr, FifoIntrId,
			   (XInterruptHandler)FifoHandler,
			   (void *)InstancePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the interrupt controller such that interrupts are enabled for
	 * all devices that cause interrupts, specific real mode so that
	 * the FIFO can cause interrupts through the interrupt controller.
	 */
	Status = XIntc_Start(IntcInstancePtr, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable the interrupt for the AXI FIFO device.
	 */
	XIntc_Enable(IntcInstancePtr, FifoIntrId);
#else
	XScuGic_Config *IntcConfig;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
				IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, FifoIntrId, 0xA0, 0x3);

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, FifoIntrId,
				(Xil_InterruptHandler)FifoHandler,
				InstancePtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	XScuGic_Enable(IntcInstancePtr, FifoIntrId);
#endif

	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
		(Xil_ExceptionHandler)INTC_HANDLER,
		(void *)IntcInstancePtr);;

	/*
	 * Enable exceptions.
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}
/*****************************************************************************/
/**
*
* This function disables the interrupts for the AXI FIFO device.
*
* @param	IntcInstancePtr is the pointer to the INTC component instance
* @param	FifoIntrId is interrupt ID associated for the FIFO component
*
* @return	None
*
* @note		None
*
******************************************************************************/
static void DisableIntrSystem(INTC *IntcInstancePtr, u16 FifoIntrId)
{
#ifdef XPAR_INTC_0_DEVICE_ID
	/* Disconnect the interrupts */
	XIntc_Disconnect(IntcInstancePtr, FifoIntrId);
#else
	XScuGic_Disconnect(IntcInstancePtr, FifoIntrId);
#endif
}

#ifdef XPAR_UARTNS550_0_BASEADDR
/*****************************************************************************/
/*
*
* Uart16550 setup routine, need to set baudrate to 9600 and data bits to 8
*
* @param	None
*
* @return	None
*
* @note		None
*
******************************************************************************/
static void Uart550_Setup(void)
{

	XUartNs550_SetBaud(XPAR_UARTNS550_0_BASEADDR,
			XPAR_XUARTNS550_CLOCK_HZ, 9600);

	XUartNs550_SetLineControlReg(XPAR_UARTNS550_0_BASEADDR,
			XUN_LCR_8_DATA_BITS);
}
#endif
