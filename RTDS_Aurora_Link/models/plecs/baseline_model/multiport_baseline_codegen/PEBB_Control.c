/*
 * Implementation file for: multiport_baseline/AC PEBB/PEBB_Control
 * Generated with         : PLECS 4.7.5
 * Generated on           : 22 Oct 2023 21:15:06
 */
#include "PEBB_Control.h"
#ifndef PLECS_HEADER_PEBB_Control_h_
#error The wrong header file "PEBB_Control.h" was included. Please check
#error your include path to see whether this file name conflicts with the
#error name of another header file.
#endif /* PLECS_HEADER_PEBB_Control_h_ */
#if defined(__GNUC__) && (__GNUC__ > 4)
#   define _ALIGNMENT 16
#   define _RESTRICT __restrict
#   define _ALIGN __attribute__((aligned(_ALIGNMENT)))
#   if defined(__clang__)
#      if __has_builtin(__builtin_assume_aligned)
#         define _ASSUME_ALIGNED(a) __builtin_assume_aligned(a, _ALIGNMENT)
#      else
#         define _ASSUME_ALIGNED(a) a
#      endif
#   else
#      define _ASSUME_ALIGNED(a) __builtin_assume_aligned(a, _ALIGNMENT)
#   endif
#else
#   ifndef _RESTRICT
#      define _RESTRICT
#   endif
#   ifndef _ALIGN
#      define _ALIGN
#   endif
#   ifndef _ASSUME_ALIGNED
#      define _ASSUME_ALIGNED(a) a
#   endif
#endif
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#define PLECSRunTimeError(msg) PEBB_Control_errorStatus = msg
struct CScriptStruct
{
   int numInputTerminals;
   int numOutputTerminals;
   int* numInputSignals;
   int* numOutputSignals;
   int numContStates;
   int numDiscStates;
   int numZCSignals;
   int numSampleTimes;
   int numParameters;
   int isMajorTimeStep;
   double time;
   const double ***inputs;
   double ***outputs;
   double *contStates;
   double *contDerivs;
   double *discStates;
   double *zCSignals;
   const int *paramNumDims;
   const int **paramDims;
   const int *paramNumElements;
   const double **paramRealData;
   const char **paramStringData;
   const char * const *sampleHits;
   const double *sampleTimePeriods;
   const double *sampleTimeOffsets;
   double *nextSampleHit;
   const char** errorStatus;
   const char** warningStatus;
   const char** rangeErrorMsg;
   int* rangeErrorLine;
   void (*writeCustomStateDouble)(void*, double);
   double (*readCustomStateDouble)(void*);
   void (*writeCustomStateInt)(void*, int);
   void (*writeCustomStateData)(void*, const void*, int);
   void (*readCustomStateData)(void*, void*, int);
};
static struct CScriptStruct PEBB_Control_cScriptStruct[4];
static char PEBB_Control_isMajorStep = '\001';
static const double PEBB_Control_UNCONNECTED = 0;
static double PEBB_Control_D_double[2];
void PEBB_Control_0_cScriptStart(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_0_cScriptOutput(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_0_cScriptUpdate(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_0_cScriptDerivative(
                                      const struct CScriptStruct *cScriptStruct);
void PEBB_Control_0_cScriptTerminate(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_1_cScriptStart(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_1_cScriptOutput(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_1_cScriptUpdate(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_1_cScriptDerivative(
                                      const struct CScriptStruct *cScriptStruct);
void PEBB_Control_1_cScriptTerminate(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_2_cScriptStart(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_2_cScriptOutput(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_2_cScriptUpdate(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_2_cScriptDerivative(
                                      const struct CScriptStruct *cScriptStruct);
void PEBB_Control_2_cScriptTerminate(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_3_cScriptStart(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_3_cScriptOutput(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_3_cScriptUpdate(const struct CScriptStruct *cScriptStruct);
void PEBB_Control_3_cScriptDerivative(
                                      const struct CScriptStruct *cScriptStruct);
void PEBB_Control_3_cScriptTerminate(const struct CScriptStruct *cScriptStruct);
static uint32_t PEBB_Control_tickLo;
static int32_t PEBB_Control_tickHi;
PEBB_Control_ExternalInputs PEBB_Control_U;
PEBB_Control_ExternalOutputs PEBB_Control_Y;
PEBB_Control_BlockOutputs PEBB_Control_B;
const char * PEBB_Control_errorStatus;
const double PEBB_Control_sampleTime = 0.000125000000000000003;
const char * const PEBB_Control_checksum =
   "fae45feb2a443b5c54431da2dccc8786ea7e00f2";
void PEBB_Control_initialize(double time)
{
   double remainder;
   PEBB_Control_errorStatus = NULL;
   PEBB_Control_tickHi = floor(time/(4294967296.0*PEBB_Control_sampleTime));
   remainder = time - PEBB_Control_tickHi*4294967296.0*
               PEBB_Control_sampleTime;
   PEBB_Control_tickLo = floor(remainder/PEBB_Control_sampleTime + .5);
   remainder -= PEBB_Control_tickLo*PEBB_Control_sampleTime;
   if (fabs(remainder) > 1e-6*fabs(time))
   {
      PEBB_Control_errorStatus =
         "Start time must be an integer multiple of the base sample time.";
   }

   /* Initialization for C-Script : 'PEBB_Control/bitarray_to_uint32_1' */
   {
      static int numInputSignals[] = {
         32
      };
      static const double* inputPtrs[] = {
         &PEBB_Control_B.flags_in[0], &PEBB_Control_B.flags_in[1],
         &PEBB_Control_B.flags_in[2], &PEBB_Control_B.flags_in[3],
         &PEBB_Control_B.flags_in[4], &PEBB_Control_B.flags_in[5],
         &PEBB_Control_B.flags_in[6], &PEBB_Control_B.flags_in[7],
         &PEBB_Control_B.flags_in[8], &PEBB_Control_B.flags_in[9],
         &PEBB_Control_B.flags_in[10], &PEBB_Control_B.flags_in1[0],
         &PEBB_Control_B.flags_in1[1], &PEBB_Control_B.flags_in1[2],
         &PEBB_Control_B.flags_in1[3], &PEBB_Control_B.flags_in1[4],
         &PEBB_Control_B.flags_in1[5], &PEBB_Control_B.flags_in1[6],
         &PEBB_Control_B.flags_in1[7], &PEBB_Control_B.flags_in1[8],
         &PEBB_Control_B.flags_in1[9], &PEBB_Control_B.flags_in1[10],
         &PEBB_Control_B.flags_in1[11], &PEBB_Control_B.flags_in1[12],
         &PEBB_Control_B.flags_in1[13], &PEBB_Control_B.flags_in1[14],
         &PEBB_Control_B.flags_in1[15], &PEBB_Control_B.flags_in1[16],
         &PEBB_Control_B.flags_in1[17], &PEBB_Control_B.flags_in1[18],
         &PEBB_Control_B.flags_in1[19], &PEBB_Control_B.flags_in1[20]
      };
      static const double** inputs[] = {
         &inputPtrs[0]
      };
      static int numOutputSignals[] = {
         1
      };
      static double* outputPtrs[] = {
         &PEBB_Control_B.bitarray_to_uint32_1
      };
      static double** outputs[] = {
         &outputPtrs[0]
      };
      static double nextSampleHit;
      static const char * sampleHits[] = {
         &PEBB_Control_isMajorStep
      };
      static double sampleTimePeriods[] = {
         0.000125
      };
      static double sampleTimeOffsets[] = {
         0
      };
      static const char* errorStatus;
      static const char* warningStatus;
      static const char* rangeErrorMsg;
      errorStatus = NULL;
      warningStatus = NULL;
      rangeErrorMsg = NULL;
      PEBB_Control_cScriptStruct[0].isMajorTimeStep = 1;
      PEBB_Control_cScriptStruct[0].numInputTerminals = 1;
      PEBB_Control_cScriptStruct[0].numInputSignals = numInputSignals;
      PEBB_Control_cScriptStruct[0].inputs = inputs;
      PEBB_Control_cScriptStruct[0].numOutputTerminals = 1;
      PEBB_Control_cScriptStruct[0].numOutputSignals = numOutputSignals;
      PEBB_Control_cScriptStruct[0].outputs = outputs;
      PEBB_Control_cScriptStruct[0].numContStates = 0;
      PEBB_Control_cScriptStruct[0].contStates = NULL;
      PEBB_Control_cScriptStruct[0].contDerivs = NULL;
      PEBB_Control_cScriptStruct[0].numDiscStates = 0;
      PEBB_Control_cScriptStruct[0].discStates = NULL;
      PEBB_Control_cScriptStruct[0].numZCSignals = 0;
      PEBB_Control_cScriptStruct[0].numParameters = 0;
      PEBB_Control_cScriptStruct[0].paramNumDims = NULL;
      PEBB_Control_cScriptStruct[0].paramDims = NULL;
      PEBB_Control_cScriptStruct[0].paramNumElements = NULL;
      PEBB_Control_cScriptStruct[0].paramRealData = NULL;
      PEBB_Control_cScriptStruct[0].paramStringData = NULL;
      PEBB_Control_cScriptStruct[0].numSampleTimes = 1;
      PEBB_Control_cScriptStruct[0].sampleTimePeriods = sampleTimePeriods;
      PEBB_Control_cScriptStruct[0].sampleTimeOffsets = sampleTimeOffsets;
      PEBB_Control_cScriptStruct[0].numSampleTimes = 1;
      PEBB_Control_cScriptStruct[0].sampleHits = sampleHits;
      PEBB_Control_cScriptStruct[0].nextSampleHit = &nextSampleHit;
      PEBB_Control_cScriptStruct[0].errorStatus = &errorStatus;
      PEBB_Control_cScriptStruct[0].warningStatus = &warningStatus;
      PEBB_Control_cScriptStruct[0].rangeErrorMsg = &rangeErrorMsg;
      PEBB_Control_0_cScriptStart(&PEBB_Control_cScriptStruct[0]);
      if (*PEBB_Control_cScriptStruct[0].errorStatus)
         PEBB_Control_errorStatus =
            *PEBB_Control_cScriptStruct[0].errorStatus;
   }

   /* Initialization for C-Script : 'PEBB_Control/"RTDS_CTRL"' */
   {
      static int numInputSignals[] = {
         4, 3, 3, 3, 3, 3, 3, 1, 1, 40, 1, 1, 3, 2
      };
      static const double* inputPtrs[] = {
         &PEBB_Control_U.Cmds[1], &PEBB_Control_B.cmd4,
         &PEBB_Control_U.Cmds[2], &PEBB_Control_U.Cmds[3],
         &PEBB_Control_U.feedback[0], &PEBB_Control_U.feedback[1],
         &PEBB_Control_U.feedback[2], &PEBB_Control_U.feedback[6],
         &PEBB_Control_B.zero, &PEBB_Control_B.zero,
         &PEBB_Control_U.feedback[3], &PEBB_Control_U.feedback[4],
         &PEBB_Control_U.feedback[5], &PEBB_Control_B.zero,
         &PEBB_Control_B.zero, &PEBB_Control_B.zero, &PEBB_Control_U.N_LRU[0],
         &PEBB_Control_U.N_LRU[1], &PEBB_Control_U.N_LRU[2],
         &PEBB_Control_U.N_active[0], &PEBB_Control_U.N_active[1],
         &PEBB_Control_U.N_active[2], &PEBB_Control_U.feedback[7],
         &PEBB_Control_B.FixFloating_intConversion, &PEBB_Control_B.gains[0],
         &PEBB_Control_B.gains[1], &PEBB_Control_B.gains[2],
         &PEBB_Control_B.gains[3], &PEBB_Control_B.gains[4],
         &PEBB_Control_B.gains[5], &PEBB_Control_B.gains[6],
         &PEBB_Control_B.gains[7], &PEBB_Control_B.gains[8],
         &PEBB_Control_B.gains[9], &PEBB_Control_B.gains[10],
         &PEBB_Control_B.gains[11], &PEBB_Control_B.gains[12],
         &PEBB_Control_B.gains[13], &PEBB_Control_B.gains[14],
         &PEBB_Control_B.gains[15], &PEBB_Control_B.gains[16],
         &PEBB_Control_B.gains[17], &PEBB_Control_B.gains[18],
         &PEBB_Control_B.gains[19], &PEBB_Control_B.gains1[0],
         &PEBB_Control_B.gains1[1], &PEBB_Control_B.gains1[2],
         &PEBB_Control_B.gains1[3], &PEBB_Control_B.gains1[4],
         &PEBB_Control_B.gains1[5], &PEBB_Control_B.gains1[6],
         &PEBB_Control_B.gains1[7], &PEBB_Control_B.gains1[8],
         &PEBB_Control_B.gains1[9], &PEBB_Control_B.gains1[10],
         &PEBB_Control_B.gains1[11], &PEBB_Control_B.gains1[12],
         &PEBB_Control_B.gains1[13], &PEBB_Control_B.gains1[14],
         &PEBB_Control_B.gains1[15], &PEBB_Control_B.gains1[16],
         &PEBB_Control_B.gains1[17], &PEBB_Control_B.gains1[18],
         &PEBB_Control_B.gains1[19], &PEBB_Control_D_double[0],
         &PEBB_Control_U.ForceFault, &PEBB_Control_U.feedback[8],
         &PEBB_Control_U.feedback[9], &PEBB_Control_U.feedback[10],
         &PEBB_Control_B.zero, &PEBB_Control_B.zero
      };
      static const double** inputs[] = {
         &inputPtrs[0], &inputPtrs[4], &inputPtrs[7], &inputPtrs[10],
         &inputPtrs[13], &inputPtrs[16], &inputPtrs[19], &inputPtrs[22],
         &inputPtrs[23], &inputPtrs[24], &inputPtrs[64], &inputPtrs[65],
         &inputPtrs[66], &inputPtrs[69]
      };
      static int numOutputSignals[] = {
         3, 3, 3, 1, 1, 1, 1, 1, 1, 10, 100
      };
      static double* outputPtrs[] = {
         &PEBB_Control_B.x_RTDS_CTRL_[0], &PEBB_Control_B.x_RTDS_CTRL_[1],
         &PEBB_Control_B.x_RTDS_CTRL_[2], &PEBB_Control_B.x_RTDS_CTRL_[3],
         &PEBB_Control_B.x_RTDS_CTRL_[4], &PEBB_Control_B.x_RTDS_CTRL_[5],
         &PEBB_Control_B.x_RTDS_CTRL_[6], &PEBB_Control_B.x_RTDS_CTRL_[7],
         &PEBB_Control_B.x_RTDS_CTRL_[8], &PEBB_Control_B.x_RTDS_CTRL_[9],
         &PEBB_Control_B.x_RTDS_CTRL_[10], &PEBB_Control_B.x_RTDS_CTRL_[11],
         &PEBB_Control_B.x_RTDS_CTRL_[12], &PEBB_Control_B.x_RTDS_CTRL_[13],
         &PEBB_Control_B.x_RTDS_CTRL_[14], &PEBB_Control_B.x_RTDS_CTRL_[15],
         &PEBB_Control_B.x_RTDS_CTRL_[16], &PEBB_Control_B.x_RTDS_CTRL_[17],
         &PEBB_Control_B.x_RTDS_CTRL_[18], &PEBB_Control_B.x_RTDS_CTRL_[19],
         &PEBB_Control_B.x_RTDS_CTRL_[20], &PEBB_Control_B.x_RTDS_CTRL_[21],
         &PEBB_Control_B.x_RTDS_CTRL_[22], &PEBB_Control_B.x_RTDS_CTRL_[23],
         &PEBB_Control_B.x_RTDS_CTRL_[24], &PEBB_Control_B.x_RTDS_CTRL_[25],
         &PEBB_Control_B.x_RTDS_CTRL_[26], &PEBB_Control_B.x_RTDS_CTRL_[27],
         &PEBB_Control_B.x_RTDS_CTRL_[28], &PEBB_Control_B.x_RTDS_CTRL_[29],
         &PEBB_Control_B.x_RTDS_CTRL_[30], &PEBB_Control_B.x_RTDS_CTRL_[31],
         &PEBB_Control_B.x_RTDS_CTRL_[32], &PEBB_Control_B.x_RTDS_CTRL_[33],
         &PEBB_Control_B.x_RTDS_CTRL_[34], &PEBB_Control_B.x_RTDS_CTRL_[35],
         &PEBB_Control_B.x_RTDS_CTRL_[36], &PEBB_Control_B.x_RTDS_CTRL_[37],
         &PEBB_Control_B.x_RTDS_CTRL_[38], &PEBB_Control_B.x_RTDS_CTRL_[39],
         &PEBB_Control_B.x_RTDS_CTRL_[40], &PEBB_Control_B.x_RTDS_CTRL_[41],
         &PEBB_Control_B.x_RTDS_CTRL_[42], &PEBB_Control_B.x_RTDS_CTRL_[43],
         &PEBB_Control_B.x_RTDS_CTRL_[44], &PEBB_Control_B.x_RTDS_CTRL_[45],
         &PEBB_Control_B.x_RTDS_CTRL_[46], &PEBB_Control_B.x_RTDS_CTRL_[47],
         &PEBB_Control_B.x_RTDS_CTRL_[48], &PEBB_Control_B.x_RTDS_CTRL_[49],
         &PEBB_Control_B.x_RTDS_CTRL_[50], &PEBB_Control_B.x_RTDS_CTRL_[51],
         &PEBB_Control_B.x_RTDS_CTRL_[52], &PEBB_Control_B.x_RTDS_CTRL_[53],
         &PEBB_Control_B.x_RTDS_CTRL_[54], &PEBB_Control_B.x_RTDS_CTRL_[55],
         &PEBB_Control_B.x_RTDS_CTRL_[56], &PEBB_Control_B.x_RTDS_CTRL_[57],
         &PEBB_Control_B.x_RTDS_CTRL_[58], &PEBB_Control_B.x_RTDS_CTRL_[59],
         &PEBB_Control_B.x_RTDS_CTRL_[60], &PEBB_Control_B.x_RTDS_CTRL_[61],
         &PEBB_Control_B.x_RTDS_CTRL_[62], &PEBB_Control_B.x_RTDS_CTRL_[63],
         &PEBB_Control_B.x_RTDS_CTRL_[64], &PEBB_Control_B.x_RTDS_CTRL_[65],
         &PEBB_Control_B.x_RTDS_CTRL_[66], &PEBB_Control_B.x_RTDS_CTRL_[67],
         &PEBB_Control_B.x_RTDS_CTRL_[68], &PEBB_Control_B.x_RTDS_CTRL_[69],
         &PEBB_Control_B.x_RTDS_CTRL_[70], &PEBB_Control_B.x_RTDS_CTRL_[71],
         &PEBB_Control_B.x_RTDS_CTRL_[72], &PEBB_Control_B.x_RTDS_CTRL_[73],
         &PEBB_Control_B.x_RTDS_CTRL_[74], &PEBB_Control_B.x_RTDS_CTRL_[75],
         &PEBB_Control_B.x_RTDS_CTRL_[76], &PEBB_Control_B.x_RTDS_CTRL_[77],
         &PEBB_Control_B.x_RTDS_CTRL_[78], &PEBB_Control_B.x_RTDS_CTRL_[79],
         &PEBB_Control_B.x_RTDS_CTRL_[80], &PEBB_Control_B.x_RTDS_CTRL_[81],
         &PEBB_Control_B.x_RTDS_CTRL_[82], &PEBB_Control_B.x_RTDS_CTRL_[83],
         &PEBB_Control_B.x_RTDS_CTRL_[84], &PEBB_Control_B.x_RTDS_CTRL_[85],
         &PEBB_Control_B.x_RTDS_CTRL_[86], &PEBB_Control_B.x_RTDS_CTRL_[87],
         &PEBB_Control_B.x_RTDS_CTRL_[88], &PEBB_Control_B.x_RTDS_CTRL_[89],
         &PEBB_Control_B.x_RTDS_CTRL_[90], &PEBB_Control_B.x_RTDS_CTRL_[91],
         &PEBB_Control_B.x_RTDS_CTRL_[92], &PEBB_Control_B.x_RTDS_CTRL_[93],
         &PEBB_Control_B.x_RTDS_CTRL_[94], &PEBB_Control_B.x_RTDS_CTRL_[95],
         &PEBB_Control_B.x_RTDS_CTRL_[96], &PEBB_Control_B.x_RTDS_CTRL_[97],
         &PEBB_Control_B.x_RTDS_CTRL_[98], &PEBB_Control_B.x_RTDS_CTRL_[99],
         &PEBB_Control_B.x_RTDS_CTRL_[100], &PEBB_Control_B.x_RTDS_CTRL_[101],
         &PEBB_Control_B.x_RTDS_CTRL_[102], &PEBB_Control_B.x_RTDS_CTRL_[103],
         &PEBB_Control_B.x_RTDS_CTRL_[104], &PEBB_Control_B.x_RTDS_CTRL_[105],
         &PEBB_Control_B.x_RTDS_CTRL_[106], &PEBB_Control_B.x_RTDS_CTRL_[107],
         &PEBB_Control_B.x_RTDS_CTRL_[108], &PEBB_Control_B.x_RTDS_CTRL_[109],
         &PEBB_Control_B.x_RTDS_CTRL_[110], &PEBB_Control_B.x_RTDS_CTRL_[111],
         &PEBB_Control_B.x_RTDS_CTRL_[112], &PEBB_Control_B.x_RTDS_CTRL_[113],
         &PEBB_Control_B.x_RTDS_CTRL_[114], &PEBB_Control_B.x_RTDS_CTRL_[115],
         &PEBB_Control_B.x_RTDS_CTRL_[116], &PEBB_Control_B.x_RTDS_CTRL_[117],
         &PEBB_Control_B.x_RTDS_CTRL_[118], &PEBB_Control_B.x_RTDS_CTRL_[119],
         &PEBB_Control_B.x_RTDS_CTRL_[120], &PEBB_Control_B.x_RTDS_CTRL_[121],
         &PEBB_Control_B.x_RTDS_CTRL_[122], &PEBB_Control_B.x_RTDS_CTRL_[123],
         &PEBB_Control_B.x_RTDS_CTRL_[124]
      };
      static double** outputs[] = {
         &outputPtrs[0], &outputPtrs[3], &outputPtrs[6], &outputPtrs[9],
         &outputPtrs[10], &outputPtrs[11], &outputPtrs[12], &outputPtrs[13],
         &outputPtrs[14], &outputPtrs[15], &outputPtrs[25]
      };
      static double nextSampleHit;
      static const char * sampleHits[] = {
         &PEBB_Control_isMajorStep
      };
      static double sampleTimePeriods[] = {
         0.000125
      };
      static double sampleTimeOffsets[] = {
         0
      };
      static const char* errorStatus;
      static const char* warningStatus;
      static const char* rangeErrorMsg;
      errorStatus = NULL;
      warningStatus = NULL;
      rangeErrorMsg = NULL;
      PEBB_Control_cScriptStruct[1].isMajorTimeStep = 1;
      PEBB_Control_cScriptStruct[1].numInputTerminals = 14;
      PEBB_Control_cScriptStruct[1].numInputSignals = numInputSignals;
      PEBB_Control_cScriptStruct[1].inputs = inputs;
      PEBB_Control_cScriptStruct[1].numOutputTerminals = 11;
      PEBB_Control_cScriptStruct[1].numOutputSignals = numOutputSignals;
      PEBB_Control_cScriptStruct[1].outputs = outputs;
      PEBB_Control_cScriptStruct[1].numContStates = 0;
      PEBB_Control_cScriptStruct[1].contStates = NULL;
      PEBB_Control_cScriptStruct[1].contDerivs = NULL;
      PEBB_Control_cScriptStruct[1].numDiscStates = 0;
      PEBB_Control_cScriptStruct[1].discStates = NULL;
      PEBB_Control_cScriptStruct[1].numZCSignals = 0;
      PEBB_Control_cScriptStruct[1].numParameters = 0;
      PEBB_Control_cScriptStruct[1].paramNumDims = NULL;
      PEBB_Control_cScriptStruct[1].paramDims = NULL;
      PEBB_Control_cScriptStruct[1].paramNumElements = NULL;
      PEBB_Control_cScriptStruct[1].paramRealData = NULL;
      PEBB_Control_cScriptStruct[1].paramStringData = NULL;
      PEBB_Control_cScriptStruct[1].numSampleTimes = 1;
      PEBB_Control_cScriptStruct[1].sampleTimePeriods = sampleTimePeriods;
      PEBB_Control_cScriptStruct[1].sampleTimeOffsets = sampleTimeOffsets;
      PEBB_Control_cScriptStruct[1].numSampleTimes = 1;
      PEBB_Control_cScriptStruct[1].sampleHits = sampleHits;
      PEBB_Control_cScriptStruct[1].nextSampleHit = &nextSampleHit;
      PEBB_Control_cScriptStruct[1].errorStatus = &errorStatus;
      PEBB_Control_cScriptStruct[1].warningStatus = &warningStatus;
      PEBB_Control_cScriptStruct[1].rangeErrorMsg = &rangeErrorMsg;
      PEBB_Control_1_cScriptStart(&PEBB_Control_cScriptStruct[1]);
      if (*PEBB_Control_cScriptStruct[1].errorStatus)
         PEBB_Control_errorStatus =
            *PEBB_Control_cScriptStruct[1].errorStatus;
   }

   /* Initialization for C-Script : 'PEBB_Control/uint32_to_bitarray1' */
   {
      static int numInputSignals[] = {
         1
      };
      static const double* inputPtrs[] = {
         &PEBB_Control_D_double[1]
      };
      static const double** inputs[] = {
         &inputPtrs[0]
      };
      static int numOutputSignals[] = {
         32
      };
      static double* outputPtrs[] = {
         &PEBB_Control_B.uint32_to_bitarray1[0],
         &PEBB_Control_B.uint32_to_bitarray1[1],
         &PEBB_Control_B.uint32_to_bitarray1[2],
         &PEBB_Control_B.uint32_to_bitarray1[3],
         &PEBB_Control_B.uint32_to_bitarray1[4],
         &PEBB_Control_B.uint32_to_bitarray1[5],
         &PEBB_Control_B.uint32_to_bitarray1[6],
         &PEBB_Control_B.uint32_to_bitarray1[7],
         &PEBB_Control_B.uint32_to_bitarray1[8],
         &PEBB_Control_B.uint32_to_bitarray1[9],
         &PEBB_Control_B.uint32_to_bitarray1[10],
         &PEBB_Control_B.uint32_to_bitarray1[11],
         &PEBB_Control_B.uint32_to_bitarray1[12],
         &PEBB_Control_B.uint32_to_bitarray1[13],
         &PEBB_Control_B.uint32_to_bitarray1[14],
         &PEBB_Control_B.uint32_to_bitarray1[15],
         &PEBB_Control_B.uint32_to_bitarray1[16],
         &PEBB_Control_B.uint32_to_bitarray1[17],
         &PEBB_Control_B.uint32_to_bitarray1[18],
         &PEBB_Control_B.uint32_to_bitarray1[19],
         &PEBB_Control_B.uint32_to_bitarray1[20],
         &PEBB_Control_B.uint32_to_bitarray1[21],
         &PEBB_Control_B.uint32_to_bitarray1[22],
         &PEBB_Control_B.uint32_to_bitarray1[23],
         &PEBB_Control_B.uint32_to_bitarray1[24],
         &PEBB_Control_B.uint32_to_bitarray1[25],
         &PEBB_Control_B.uint32_to_bitarray1[26],
         &PEBB_Control_B.uint32_to_bitarray1[27],
         &PEBB_Control_B.uint32_to_bitarray1[28],
         &PEBB_Control_B.uint32_to_bitarray1[29],
         &PEBB_Control_B.uint32_to_bitarray1[30],
         &PEBB_Control_B.uint32_to_bitarray1[31]
      };
      static double** outputs[] = {
         &outputPtrs[0]
      };
      static double nextSampleHit;
      static const char * sampleHits[] = {
         &PEBB_Control_isMajorStep
      };
      static double sampleTimePeriods[] = {
         0.000125
      };
      static double sampleTimeOffsets[] = {
         0
      };
      static const char* errorStatus;
      static const char* warningStatus;
      static const char* rangeErrorMsg;
      errorStatus = NULL;
      warningStatus = NULL;
      rangeErrorMsg = NULL;
      PEBB_Control_cScriptStruct[2].isMajorTimeStep = 1;
      PEBB_Control_cScriptStruct[2].numInputTerminals = 1;
      PEBB_Control_cScriptStruct[2].numInputSignals = numInputSignals;
      PEBB_Control_cScriptStruct[2].inputs = inputs;
      PEBB_Control_cScriptStruct[2].numOutputTerminals = 1;
      PEBB_Control_cScriptStruct[2].numOutputSignals = numOutputSignals;
      PEBB_Control_cScriptStruct[2].outputs = outputs;
      PEBB_Control_cScriptStruct[2].numContStates = 0;
      PEBB_Control_cScriptStruct[2].contStates = NULL;
      PEBB_Control_cScriptStruct[2].contDerivs = NULL;
      PEBB_Control_cScriptStruct[2].numDiscStates = 0;
      PEBB_Control_cScriptStruct[2].discStates = NULL;
      PEBB_Control_cScriptStruct[2].numZCSignals = 0;
      PEBB_Control_cScriptStruct[2].numParameters = 0;
      PEBB_Control_cScriptStruct[2].paramNumDims = NULL;
      PEBB_Control_cScriptStruct[2].paramDims = NULL;
      PEBB_Control_cScriptStruct[2].paramNumElements = NULL;
      PEBB_Control_cScriptStruct[2].paramRealData = NULL;
      PEBB_Control_cScriptStruct[2].paramStringData = NULL;
      PEBB_Control_cScriptStruct[2].numSampleTimes = 1;
      PEBB_Control_cScriptStruct[2].sampleTimePeriods = sampleTimePeriods;
      PEBB_Control_cScriptStruct[2].sampleTimeOffsets = sampleTimeOffsets;
      PEBB_Control_cScriptStruct[2].numSampleTimes = 1;
      PEBB_Control_cScriptStruct[2].sampleHits = sampleHits;
      PEBB_Control_cScriptStruct[2].nextSampleHit = &nextSampleHit;
      PEBB_Control_cScriptStruct[2].errorStatus = &errorStatus;
      PEBB_Control_cScriptStruct[2].warningStatus = &warningStatus;
      PEBB_Control_cScriptStruct[2].rangeErrorMsg = &rangeErrorMsg;
      PEBB_Control_2_cScriptStart(&PEBB_Control_cScriptStruct[2]);
      if (*PEBB_Control_cScriptStruct[2].errorStatus)
         PEBB_Control_errorStatus =
            *PEBB_Control_cScriptStruct[2].errorStatus;
   }

   /* Initialization for C-Script : 'PEBB_Control/uint32_to_bitarray2' */
   {
      static int numInputSignals[] = {
         1
      };
      static const double* inputPtrs[] = {
         &PEBB_Control_B.x_RTDS_CTRL_[10]
      };
      static const double** inputs[] = {
         &inputPtrs[0]
      };
      static int numOutputSignals[] = {
         32
      };
      static double* outputPtrs[] = {
         &PEBB_Control_B.uint32_to_bitarray2[0],
         &PEBB_Control_B.uint32_to_bitarray2[1],
         &PEBB_Control_B.uint32_to_bitarray2[2],
         &PEBB_Control_B.uint32_to_bitarray2[3],
         &PEBB_Control_B.uint32_to_bitarray2[4],
         &PEBB_Control_B.uint32_to_bitarray2[5],
         &PEBB_Control_B.uint32_to_bitarray2[6],
         &PEBB_Control_B.uint32_to_bitarray2[7],
         &PEBB_Control_B.uint32_to_bitarray2[8],
         &PEBB_Control_B.uint32_to_bitarray2[9],
         &PEBB_Control_B.uint32_to_bitarray2[10],
         &PEBB_Control_B.uint32_to_bitarray2[11],
         &PEBB_Control_B.uint32_to_bitarray2[12],
         &PEBB_Control_B.uint32_to_bitarray2[13],
         &PEBB_Control_B.uint32_to_bitarray2[14],
         &PEBB_Control_B.uint32_to_bitarray2[15],
         &PEBB_Control_B.uint32_to_bitarray2[16],
         &PEBB_Control_B.uint32_to_bitarray2[17],
         &PEBB_Control_B.uint32_to_bitarray2[18],
         &PEBB_Control_B.uint32_to_bitarray2[19],
         &PEBB_Control_B.uint32_to_bitarray2[20],
         &PEBB_Control_B.uint32_to_bitarray2[21],
         &PEBB_Control_B.uint32_to_bitarray2[22],
         &PEBB_Control_B.uint32_to_bitarray2[23],
         &PEBB_Control_B.uint32_to_bitarray2[24],
         &PEBB_Control_B.uint32_to_bitarray2[25],
         &PEBB_Control_B.uint32_to_bitarray2[26],
         &PEBB_Control_B.uint32_to_bitarray2[27],
         &PEBB_Control_B.uint32_to_bitarray2[28],
         &PEBB_Control_B.uint32_to_bitarray2[29],
         &PEBB_Control_B.uint32_to_bitarray2[30],
         &PEBB_Control_B.uint32_to_bitarray2[31]
      };
      static double** outputs[] = {
         &outputPtrs[0]
      };
      static double nextSampleHit;
      static const char * sampleHits[] = {
         &PEBB_Control_isMajorStep
      };
      static double sampleTimePeriods[] = {
         0.000125
      };
      static double sampleTimeOffsets[] = {
         0
      };
      static const char* errorStatus;
      static const char* warningStatus;
      static const char* rangeErrorMsg;
      errorStatus = NULL;
      warningStatus = NULL;
      rangeErrorMsg = NULL;
      PEBB_Control_cScriptStruct[3].isMajorTimeStep = 1;
      PEBB_Control_cScriptStruct[3].numInputTerminals = 1;
      PEBB_Control_cScriptStruct[3].numInputSignals = numInputSignals;
      PEBB_Control_cScriptStruct[3].inputs = inputs;
      PEBB_Control_cScriptStruct[3].numOutputTerminals = 1;
      PEBB_Control_cScriptStruct[3].numOutputSignals = numOutputSignals;
      PEBB_Control_cScriptStruct[3].outputs = outputs;
      PEBB_Control_cScriptStruct[3].numContStates = 0;
      PEBB_Control_cScriptStruct[3].contStates = NULL;
      PEBB_Control_cScriptStruct[3].contDerivs = NULL;
      PEBB_Control_cScriptStruct[3].numDiscStates = 0;
      PEBB_Control_cScriptStruct[3].discStates = NULL;
      PEBB_Control_cScriptStruct[3].numZCSignals = 0;
      PEBB_Control_cScriptStruct[3].numParameters = 0;
      PEBB_Control_cScriptStruct[3].paramNumDims = NULL;
      PEBB_Control_cScriptStruct[3].paramDims = NULL;
      PEBB_Control_cScriptStruct[3].paramNumElements = NULL;
      PEBB_Control_cScriptStruct[3].paramRealData = NULL;
      PEBB_Control_cScriptStruct[3].paramStringData = NULL;
      PEBB_Control_cScriptStruct[3].numSampleTimes = 1;
      PEBB_Control_cScriptStruct[3].sampleTimePeriods = sampleTimePeriods;
      PEBB_Control_cScriptStruct[3].sampleTimeOffsets = sampleTimeOffsets;
      PEBB_Control_cScriptStruct[3].numSampleTimes = 1;
      PEBB_Control_cScriptStruct[3].sampleHits = sampleHits;
      PEBB_Control_cScriptStruct[3].nextSampleHit = &nextSampleHit;
      PEBB_Control_cScriptStruct[3].errorStatus = &errorStatus;
      PEBB_Control_cScriptStruct[3].warningStatus = &warningStatus;
      PEBB_Control_cScriptStruct[3].rangeErrorMsg = &rangeErrorMsg;
      PEBB_Control_3_cScriptStart(&PEBB_Control_cScriptStruct[3]);
      if (*PEBB_Control_cScriptStruct[3].errorStatus)
         PEBB_Control_errorStatus =
            *PEBB_Control_cScriptStruct[3].errorStatus;
   }
}

void PEBB_Control_step(void)
{
   if (PEBB_Control_errorStatus)
   {
      return;
   }

   /* Constant : 'PEBB_Control/cmd4' */
   PEBB_Control_B.cmd4 = 8000.;

   /* Constant : 'PEBB_Control/zero' */
   PEBB_Control_B.zero = 0.;

   /* Rounding : 'PEBB_Control/Fix floating->int conversion'
    * incorporates
    *  Signal Inport : 'PEBB_Control/Cmds'
    */
   PEBB_Control_B.FixFloating_intConversion =
      (PEBB_Control_U.Cmds[0] >= 0) ? floor(PEBB_Control_U.Cmds[0]+
                                            0.5) : ceil(
                                                        PEBB_Control_U.Cmds[
                                                           0]-
                                                        0.5);

   /* Constant : 'PEBB_Control/gains' */
   PEBB_Control_B.gains[0] = 8000.;
   PEBB_Control_B.gains[1] = 8000.;
   PEBB_Control_B.gains[2] = 8000.;
   PEBB_Control_B.gains[3] = 8000.;
   PEBB_Control_B.gains[4] = 8000.;
   PEBB_Control_B.gains[5] = 1000.;
   PEBB_Control_B.gains[6] = 100.;
   PEBB_Control_B.gains[7] = 200.;
   PEBB_Control_B.gains[8] = 10.;
   PEBB_Control_B.gains[9] = 250.;
   PEBB_Control_B.gains[10] = 10.;
   PEBB_Control_B.gains[11] = 30.;
   PEBB_Control_B.gains[12] = 10.;
   PEBB_Control_B.gains[13] = 0.5;
   PEBB_Control_B.gains[14] = 1.;
   PEBB_Control_B.gains[15] = -1200000.;
   PEBB_Control_B.gains[16] = 1200000.;
   PEBB_Control_B.gains[17] = 20.;
   PEBB_Control_B.gains[18] = 20.;
   PEBB_Control_B.gains[19] = 1200000.;

   /* Constant : 'PEBB_Control/gains1' */
   PEBB_Control_B.gains1[0] = 0.;
   PEBB_Control_B.gains1[1] = 0.;
   PEBB_Control_B.gains1[2] = 0.;
   PEBB_Control_B.gains1[3] = 0.;
   PEBB_Control_B.gains1[4] = 0.;
   PEBB_Control_B.gains1[5] = 0.;
   PEBB_Control_B.gains1[6] = 0.;
   PEBB_Control_B.gains1[7] = 0.;
   PEBB_Control_B.gains1[8] = 0.;
   PEBB_Control_B.gains1[9] = 0.;
   PEBB_Control_B.gains1[10] = 0.;
   PEBB_Control_B.gains1[11] = 0.;
   PEBB_Control_B.gains1[12] = 0.;
   PEBB_Control_B.gains1[13] = 0.;
   PEBB_Control_B.gains1[14] = 0.;
   PEBB_Control_B.gains1[15] = 0.;
   PEBB_Control_B.gains1[16] = 0.;
   PEBB_Control_B.gains1[17] = 0.;
   PEBB_Control_B.gains1[18] = 0.;
   PEBB_Control_B.gains1[19] = 0.;

   /* Constant : 'PEBB_Control/flags_in' */
   PEBB_Control_B.flags_in[0] = 1.;
   PEBB_Control_B.flags_in[1] = 0.;
   PEBB_Control_B.flags_in[2] = 1.;
   PEBB_Control_B.flags_in[3] = 1.;
   PEBB_Control_B.flags_in[4] = 0.;
   PEBB_Control_B.flags_in[5] = 0.;
   PEBB_Control_B.flags_in[6] = 0.;
   PEBB_Control_B.flags_in[7] = 0.;
   PEBB_Control_B.flags_in[8] = 0.;
   PEBB_Control_B.flags_in[9] = 1.;
   PEBB_Control_B.flags_in[10] = 0.;

   /* Constant : 'PEBB_Control/flags_in1' */
   PEBB_Control_B.flags_in1[0] = 0.;
   PEBB_Control_B.flags_in1[1] = 0.;
   PEBB_Control_B.flags_in1[2] = 0.;
   PEBB_Control_B.flags_in1[3] = 0.;
   PEBB_Control_B.flags_in1[4] = 0.;
   PEBB_Control_B.flags_in1[5] = 0.;
   PEBB_Control_B.flags_in1[6] = 0.;
   PEBB_Control_B.flags_in1[7] = 0.;
   PEBB_Control_B.flags_in1[8] = 0.;
   PEBB_Control_B.flags_in1[9] = 0.;
   PEBB_Control_B.flags_in1[10] = 0.;
   PEBB_Control_B.flags_in1[11] = 0.;
   PEBB_Control_B.flags_in1[12] = 0.;
   PEBB_Control_B.flags_in1[13] = 0.;
   PEBB_Control_B.flags_in1[14] = 0.;
   PEBB_Control_B.flags_in1[15] = 0.;
   PEBB_Control_B.flags_in1[16] = 0.;
   PEBB_Control_B.flags_in1[17] = 0.;
   PEBB_Control_B.flags_in1[18] = 0.;
   PEBB_Control_B.flags_in1[19] = 0.;
   PEBB_Control_B.flags_in1[20] = 0.;

   /* C-Script : 'PEBB_Control/bitarray_to_uint32_1' */
   PEBB_Control_0_cScriptOutput(&PEBB_Control_cScriptStruct[0]);
   if (*PEBB_Control_cScriptStruct[0].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[0].errorStatus;

   /* C-Script : 'PEBB_Control/"RTDS_CTRL"' */
   PEBB_Control_D_double[0] = (uint32_t)PEBB_Control_B.bitarray_to_uint32_1;
   PEBB_Control_1_cScriptOutput(&PEBB_Control_cScriptStruct[1]);
   if (*PEBB_Control_cScriptStruct[1].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[1].errorStatus;

   /* C-Script : 'PEBB_Control/uint32_to_bitarray1' */
   PEBB_Control_D_double[1] = (uint32_t)PEBB_Control_B.x_RTDS_CTRL_[14];
   PEBB_Control_2_cScriptOutput(&PEBB_Control_cScriptStruct[2]);
   if (*PEBB_Control_cScriptStruct[2].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[2].errorStatus;

   /* Compare to Constant : 'PEBB_Control/Compare to\nConstant'
    * incorporates
    *  Data Type : 'PEBB_Control/Data Type'
    */
   PEBB_Control_B.CompareToConstant =
      ((uint32_t)PEBB_Control_B.x_RTDS_CTRL_[10]) == 0.;

   /* C-Script : 'PEBB_Control/uint32_to_bitarray2' */
   PEBB_Control_3_cScriptOutput(&PEBB_Control_cScriptStruct[3]);
   if (*PEBB_Control_cScriptStruct[3].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[3].errorStatus;

   /* Global output signals */
   PEBB_Control_Y.switch_1[0] = PEBB_Control_B.x_RTDS_CTRL_[0] -
                                PEBB_Control_B.x_RTDS_CTRL_[3];
   PEBB_Control_Y.switch_1[1] = PEBB_Control_B.x_RTDS_CTRL_[1] -
                                PEBB_Control_B.x_RTDS_CTRL_[4];
   PEBB_Control_Y.switch_1[2] = PEBB_Control_B.x_RTDS_CTRL_[2] -
                                PEBB_Control_B.x_RTDS_CTRL_[5];
   PEBB_Control_Y.enable = PEBB_Control_B.x_RTDS_CTRL_[9];
   PEBB_Control_Y.highside_contact = PEBB_Control_B.uint32_to_bitarray1[4] *
                                     PEBB_Control_B.CompareToConstant;
   PEBB_Control_Y.lowside_contact = PEBB_Control_U.Cmds[4] *
                                    PEBB_Control_B.CompareToConstant;
   PEBB_Control_Y.not_fault = PEBB_Control_B.CompareToConstant;
   PEBB_Control_Y.precharge_contact = PEBB_Control_B.uint32_to_bitarray1[6];
   PEBB_Control_Y.curr_state = 1.*PEBB_Control_B.x_RTDS_CTRL_[11];
   PEBB_Control_Y.curr_pqd_follow[0] = 1.*PEBB_Control_B.x_RTDS_CTRL_[65];
   PEBB_Control_Y.curr_pqd_follow[1] = 1.*PEBB_Control_B.x_RTDS_CTRL_[66];
   PEBB_Control_Y.curr_pqd_follow[2] = 1.*PEBB_Control_B.x_RTDS_CTRL_[28];
   PEBB_Control_Y.curr_pqd_follow[3] = 1.*PEBB_Control_B.x_RTDS_CTRL_[29];
   PEBB_Control_Y.curr_pqd_follow[4] = 1.*PEBB_Control_B.x_RTDS_CTRL_[79];
   PEBB_Control_Y.curr_pqd_follow[5] = 1.*PEBB_Control_B.x_RTDS_CTRL_[80];
   PEBB_Control_Y.curr_nqd_follow[0] = 1.*PEBB_Control_B.x_RTDS_CTRL_[87];
   PEBB_Control_Y.curr_nqd_follow[1] = 1.*PEBB_Control_B.x_RTDS_CTRL_[88];
   PEBB_Control_Y.curr_nqd_follow[2] = 1.*PEBB_Control_B.x_RTDS_CTRL_[89];
   PEBB_Control_Y.curr_nqd_follow[3] = 1.*PEBB_Control_B.x_RTDS_CTRL_[90];
   PEBB_Control_Y.curr_nqd_follow[4] = 1.*PEBB_Control_B.x_RTDS_CTRL_[85];
   PEBB_Control_Y.curr_nqd_follow[5] = 1.*PEBB_Control_B.x_RTDS_CTRL_[86];
   PEBB_Control_Y.v_follow[0] = 1.*PEBB_Control_B.x_RTDS_CTRL_[60];
   PEBB_Control_Y.v_follow[1] = 1.*PEBB_Control_B.x_RTDS_CTRL_[58];
   PEBB_Control_Y.lims[0] = PEBB_Control_B.x_RTDS_CTRL_[96];
   PEBB_Control_Y.lims[1] = PEBB_Control_B.x_RTDS_CTRL_[97];
   PEBB_Control_Y.lims[2] = PEBB_Control_B.x_RTDS_CTRL_[98];
   PEBB_Control_Y.lims[3] = PEBB_Control_B.x_RTDS_CTRL_[99];

   if (PEBB_Control_errorStatus)
   {
      return;
   }

   /* Update for C-Script : 'PEBB_Control/bitarray_to_uint32_1' */
   PEBB_Control_0_cScriptUpdate(&PEBB_Control_cScriptStruct[0]);
   if (*PEBB_Control_cScriptStruct[0].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[0].errorStatus;

   /* Update for C-Script : 'PEBB_Control/"RTDS_CTRL"' */
   PEBB_Control_1_cScriptUpdate(&PEBB_Control_cScriptStruct[1]);
   if (*PEBB_Control_cScriptStruct[1].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[1].errorStatus;

   /* Update for C-Script : 'PEBB_Control/uint32_to_bitarray1' */
   PEBB_Control_2_cScriptUpdate(&PEBB_Control_cScriptStruct[2]);
   if (*PEBB_Control_cScriptStruct[2].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[2].errorStatus;

   /* Update for C-Script : 'PEBB_Control/uint32_to_bitarray2' */
   PEBB_Control_3_cScriptUpdate(&PEBB_Control_cScriptStruct[3]);
   if (*PEBB_Control_cScriptStruct[3].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[3].errorStatus;
}

void PEBB_Control_terminate()
{
   /* Termination for C-Script : 'PEBB_Control/bitarray_to_uint32_1' */
   PEBB_Control_0_cScriptTerminate(&PEBB_Control_cScriptStruct[0]);
   if (*PEBB_Control_cScriptStruct[0].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[0].errorStatus;
   /* Termination for C-Script : 'PEBB_Control/"RTDS_CTRL"' */
   PEBB_Control_1_cScriptTerminate(&PEBB_Control_cScriptStruct[1]);
   if (*PEBB_Control_cScriptStruct[1].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[1].errorStatus;
   /* Termination for C-Script : 'PEBB_Control/uint32_to_bitarray1' */
   PEBB_Control_2_cScriptTerminate(&PEBB_Control_cScriptStruct[2]);
   if (*PEBB_Control_cScriptStruct[2].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[2].errorStatus;
   /* Termination for C-Script : 'PEBB_Control/uint32_to_bitarray2' */
   PEBB_Control_3_cScriptTerminate(&PEBB_Control_cScriptStruct[3]);
   if (*PEBB_Control_cScriptStruct[3].errorStatus)
      PEBB_Control_errorStatus = *PEBB_Control_cScriptStruct[3].errorStatus;
}
