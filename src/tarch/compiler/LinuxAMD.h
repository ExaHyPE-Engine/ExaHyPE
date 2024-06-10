// Default variant of CompilerSpecificSettings.h for GCC on Linux.
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

#if !defined(UseManualInlining) &&  !defined(noUseManualInlining)
#endif

#define LittleEndian

#if !defined(AlignmentOnHeap)
#define AlignmentOnHeap 16
#endif

#if PeanoDebug == 0
#undef assert
#define assert(...)
#endif

#define KeywordToAvoidDuplicateSymbolsForInlinedFunctions static inline

#define OpenMPDependenciesInTaskWait

#define OpenMPTaskGroup

#define PragmaPush _Pragma("clang diagnostic push")
#define SilenceUnknownAttribute _Pragma("clang diagnostic ignored \"-Wunknown-attributes\"")
#define PragmaPop _Pragma("clang diagnostic pop")

/**
* AMD has not annotated their memset with offloading statements.
* So we write one manually, if the following macro is defined
*/
#define OpenMPManuallyOffloadMemset
