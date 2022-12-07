// Default variant of CompilerSpecificSettings.h for GCC on Linux.
// For a detailed description of the semantics of the settings, please
// consult LinuxIntel.h which is the most elaborate documentation.

//#define UseTestSpecificCompilerSettings
//#define CompilerCLANG
//#define CompilerICC
#define CompilerHasProcStat
#define CompilerHasUTSName
#define CompilerHasTimespec
#define CompilerHasSysinfo
//#define CompilerDefinesMPIMaxNameString
#if !defined(noMPISupportsSingleSidedCommunication) and !defined(MPISupportsSingleSidedCommunication)
#define MPISupportsSingleSidedCommunication
#endif


#if !defined(UseManualInlining) &&  !defined(noUseManualInlining)
#endif


#define LittleEndian

#if !defined(AlignmentOnHeap)
#define AlignmentOnHeap 16
#endif


/**
 * Clang requires us to put a static keyword in front of header file implementations,
 * so we avoid duplicate symbols. This is not C++ conformal, and therefore GCC doesn't
 * eat stuff. Therefore we do not define the static keyword here.
 */
#define KeywordToAvoidDuplicateSymbolsForInlinedFunctions inline

#define PRAGMA_PUSH _Pragma("diag_push")
#define SILENCE_UNKNOWN_ATTRIBUTE _Pragma("diag_suppress unrecognized_attribute")
#define PRAGMA_POP _Pragma ("diag_pop")
