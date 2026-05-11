/*
 * Header file for: multiport_baseline/AC PEBB/PEBB_Control
 * Generated with : PLECS 4.7.5
 * Generated on   : 22 Oct 2023 21:15:06
 */
#ifndef PLECS_HEADER_PEBB_Control_h_
#define PLECS_HEADER_PEBB_Control_h_

#include <stdbool.h>
#include <stdint.h>

/* Model floating point type */
typedef double PEBB_Control_FloatType;

/* Model checksum */
extern const char * const PEBB_Control_checksum;

/* Model error status */
extern const char * PEBB_Control_errorStatus;


/* Model sample time */
extern const double PEBB_Control_sampleTime;


/* External inputs */
typedef struct
{
   double feedback[11];             /* PEBB_Control/feedback */
   double Cmds[12];                 /* PEBB_Control/Cmds */
   double ForceFault;               /* PEBB_Control/Force Fault */
   double N_LRU[3];                 /* PEBB_Control/N_LRU */
   double N_active[3];              /* PEBB_Control/N_active */
} PEBB_Control_ExternalInputs;
extern PEBB_Control_ExternalInputs PEBB_Control_U;


/* External outputs */
typedef struct
{
   double switch_1[3];              /* PEBB_Control/switch */
   double enable;                   /* PEBB_Control/enable */
   double highside_contact;         /* PEBB_Control/highside_contact */
   double lowside_contact;          /* PEBB_Control/lowside_contact */
   bool not_fault;                  /* PEBB_Control/not_fault */
   double precharge_contact;        /* PEBB_Control/precharge_contact */
   double curr_state;               /* PEBB_Control/curr_state */
   double curr_pqd_follow[6];       /* PEBB_Control/curr_pqd_follow */
   double curr_nqd_follow[6];       /* PEBB_Control/curr_nqd_follow */
   double v_follow[2];              /* PEBB_Control/v_follow */
   double lims[4];                  /* PEBB_Control/lims */
} PEBB_Control_ExternalOutputs;
extern PEBB_Control_ExternalOutputs PEBB_Control_Y;


/* Block outputs */
typedef struct
{
   double bitarray_to_uint32_1;     /* PEBB_Control/bitarray_to_uint32_1 */
   double x_RTDS_CTRL_[125];        /* PEBB_Control/"RTDS_CTRL" */
   double uint32_to_bitarray1[32];  /* PEBB_Control/uint32_to_bitarray1 */
   double uint32_to_bitarray2[32];  /* PEBB_Control/uint32_to_bitarray2 */
   double cmd4;                     /* PEBB_Control/cmd4 */
   double zero;                     /* PEBB_Control/zero */
   double FixFloating_intConversion; /* PEBB_Control/Fix floating->int conversion */
   double gains[20];                /* PEBB_Control/gains */
   double gains1[20];               /* PEBB_Control/gains1 */
   double flags_in[11];             /* PEBB_Control/flags_in */
   double flags_in1[21];            /* PEBB_Control/flags_in1 */
   bool CompareToConstant;          /* PEBB_Control/Compare to Constant */
} PEBB_Control_BlockOutputs;
extern PEBB_Control_BlockOutputs PEBB_Control_B;

/* Entry point functions */
void PEBB_Control_initialize(double time);
void PEBB_Control_step(void);
void PEBB_Control_terminate(void);

#endif /* PLECS_HEADER_PEBB_Control_h_ */
