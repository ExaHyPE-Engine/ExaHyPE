// Default variant of CompilerSpecificSettings.h for the Intel compiler running on Windows.
// For a detailed description of the semantics of the settings, please
// consult LinuxIntel.h which is the most elaborate documentation.

#define UseTestSpecificCompilerSettings
//#define CompilerCLANG
#define CompilerICC
//#define CompilerHasProcStat
//#define CompilerHasUTSName
#define CompilerHasTimespec
//#define CompilerHasSysinfo
//#define CompilerDefinesMPIMaxNameString

#if !defined(noMPISupportsSingleSidedCommunication) and !defined(MPISupportsSingleSidedCommunication)
#define MPISupportsSingleSidedCommunication
#endif

#if !defined(UseManualInlining) &&  !defined(noUseManualInlining)
#define UseManualInlining
#endif

#define LittleEndian

#if !defined(AlignmentOnHeap)
#define AlignmentOnHeap 16
#endif

/**
 * Clang requires us to put a static keyword in front of header file implementations,
 * so we avoid duplicate symbols. This is not C++ conformal, and therefore GCC doesn't
 * eat stuff. icpc is GNU-based and therefore we do not define the static keyword here.
 */
#define KeywordToAvoidDuplicateSymbolsForInlinedFunctions

#define OpenMPTaskGroup

#define PragmaPush
#define SilenceUnknownAttribute
#define PragmaPop
