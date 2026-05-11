/**
 *
 * @file UART.h
 * This file defines types and constants to be used with receiving data
 * over UART
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -------------------------------------------------------
 * 1.0   BDR  1/23/2023  Initial revision
 * 2.0   BDR  9/26/2023  Changed to generic PEBB
 *
 * ***************************************************************************
 */

#ifndef UART_h_
#define UART_h_

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define UART_DEVICE_ID              XPAR_XUARTPS_0_DEVICE_ID

/*
 * The following constant controls the length of the buffers to be sent
 * and received with the device, this constant must be 32 bytes or less since
 * only as much as FIFO size data can be sent or received in polled mode.
 */
#define TEST_BUFFER_SIZE 32

//--------------------------------------------------------------------------------
// Function Prototypes
//--------------------------------------------------------------------------------

int setupUART(void);
void readUART(void);
void dump(void);


#endif                                 /* UART_h_ */
