// Default variant of CompilerSpecificSettings.h for Intel ICPX on Linux.
// For a detailed description of the semantics of the settings, please
// consult LinuxIntel.h which is the most elaborate documentation.

//#define UseTestSpecificCompilerSettings
#define CompilerCLANG
//#define CompilerICC
#define CompilerHasProcStat
#define CompilerHasUTSName
#define CompilerHasTimespec
#define CompilerHasSysinfo
//#define CompilerDefinesMPIMaxNameString
#if !defined(noMPISupportsSingleSidedCommunication) and !defined(MPISupportsSingleSidedCommunication)
#define MPISupportsSingleSidedCommunication
#endif

#if !defined(UseManualInlining) && !defined(noUseManualInlining)
#define UseManualInlining
#endif

#if !defined(AlignmentOnHeap)
#define AlignmentOnHeap 64
#endif

#define LittleEndian

#if !defined(UseManualInlining) &&  !defined(noUseManualInlining)
#define UseManualInlining
#endif

#if PeanoDebug == 0
#undef assert
#define assert(...)
#endif

#define KeywordToAvoidDuplicateSymbolsForInlinedFunctions static inline

#define OpenMPTaskGroup

#define OpenMP6Syntax
