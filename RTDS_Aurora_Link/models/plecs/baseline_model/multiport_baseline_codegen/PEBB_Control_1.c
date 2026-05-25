/*
 * C-Script file for: PEBB_Control/"RTDS_CTRL"
 * Generated with   : PLECS 4.7.5
 * Generated on     : 22 Oct 2023 21:15:06
 */
typedef double real_t;
#define REAL_MAX DBL_MAX
#define REAL_MIN DBL_MIN
#define REAL_EPSILON DBL_EPSILON
#define PLECS
//#define CHIL // only uncomment this include on the Kria CHIL

//This defines the path to the MVDC.CLI folder. If MVDC.CLI folder is in the same folder
//as this file then just use \MVDC.CLI (without a trailing \). Note that path cannot contain
//space characters.. e.g. C:\My_Dir\MVDC.CLI is ok, C:\My Dir\MVDC.CLI is not!
#ifdef CHIL
#define PATH_TO_MVDC_CLI MVDC.CLI
#else
#define PATH_TO_MVDC_CLI ..\..\..\multiport_p80i_libs\MVDC.CLI
#endif // CHIL



// Credit to https://stackoverflow.com/questions/32066204/construct-path-for-include-directive-with-macro
// For how to use defines to setup path!
#define XSTR(x) #x
#define STR(x) XSTR(x)

#include STR(PATH_TO_MVDC_CLI\control_functions.h)
#include STR(PATH_TO_MVDC_CLI\globals.h)

#include STR(PATH_TO_MVDC_CLI\control_functions.c)
#include STR(PATH_TO_MVDC_CLI\control_blocks.c)
#include STR(PATH_TO_MVDC_CLI\general_states.c)
#include STR(PATH_TO_MVDC_CLI\RTDS_CTRL.c)
#include STR(PATH_TO_MVDC_CLI\acpebb_states.c)
struct CScriptStruct{ int numInputTerminals; int numOutputTerminals; int* numInputSignals; int* numOutputSignals; int numContStates; int numDiscStates; int numZCSignals; int numSampleTimes; int numParameters; int isMajorTimeStep; double time; const double ***inputs; double ***outputs; double *contStates; double *contDerivs; double *discStates; double *zCSignals; const int *paramNumDims; const int **paramDims; const int *paramNumElements; const double **paramRealData; const char **paramStringData; const char * const *sampleHits; const double *sampleTimePeriods; const double *sampleTimeOffsets; double *nextSampleHit; const char** errorStatus; const char** warningStatus; const char** rangeErrorMsg; int* rangeErrorLine;  void (*writeCustomStateDouble)(void*, double); double (*readCustomStateDouble)(void*); void (*writeCustomStateInt)(void*, int);  void (*writeCustomStateData)(void*, const void*, int); void (*readCustomStateData)(void*, void*, int);};
#define NumInputTerminals cScriptStruct->numInputTerminals
#define NumOutputTerminals cScriptStruct->numOutputTerminals
#define NumInputSignals(terminalIdx) cScriptStruct->numInputSignals[terminalIdx]
#define NumOutputSignals(terminalIdx) cScriptStruct->numOutputSignals[terminalIdx]
#define InputSignal(terminalIdx, signalIdx) (*cScriptStruct->inputs[terminalIdx][signalIdx])
#define OutputSignal(terminalIdx, signalIdx) (*cScriptStruct->outputs[terminalIdx][signalIdx])
#define NumContStates cScriptStruct->numContStates
#define NumDiscStates cScriptStruct->numDiscStates
#define NumZCSignals cScriptStruct->numZCSignals
#define NumParameters cScriptStruct->numParameters
#define NumSampleTimes cScriptStruct->numSampleTimes
#define IsMajorStep cScriptStruct->isMajorTimeStep
#define CurrentTime cScriptStruct->time
#define NextSampleHit (*cScriptStruct->nextSampleHit)
#define SetErrorMessage(string) { *cScriptStruct->errorStatus=(string);}
#define SetWarningMessage(string)
#define ContState(idx) cScriptStruct->contStates[idx]
#define ContDeriv(idx) cScriptStruct->contDerivs[idx]
#define DiscState(idx) cScriptStruct->discStates[idx]
#define ZCSignal(idx) cScriptStruct->zCSignals[idx]
#define IsSampleHit(idx) (*cScriptStruct->sampleHits[idx])
#define SampleTimePeriod(idx) cScriptStruct->sampleTimePeriods[idx]
#define SampleTimeOffset(idx) cScriptStruct->sampleTimeOffsets[idx]
#define ParamNumDims(idx) cScriptStruct->paramNumDims[idx]
#define ParamDim(pIdx, dIdx) cScriptStruct->paramDims[pIdx][dIdx]
#define ParamRealData(pIdx, dIdx) cScriptStruct->paramRealData[pIdx][dIdx]
#define ParamStringData(pIdx) cScriptStruct->paramStringData[pIdx]

void PEBB_Control_1_cScriptStart(const struct CScriptStruct *cScriptStruct)
{
CtrlInit();
}

void PEBB_Control_1_cScriptOutput(const struct CScriptStruct *cScriptStruct)
{
#include STR(PATH_TO_MVDC_CLI\plecs_interface.c)
}

void PEBB_Control_1_cScriptUpdate(const struct CScriptStruct *cScriptStruct)
{
}

void PEBB_Control_1_cScriptDerivative(const struct CScriptStruct *cScriptStruct)
{
}

void PEBB_Control_1_cScriptTerminate(const struct CScriptStruct *cScriptStruct)
{
}
