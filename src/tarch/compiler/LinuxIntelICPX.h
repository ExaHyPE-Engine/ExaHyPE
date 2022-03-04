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
#define UseManualInlining
#endif

#if !defined(ExaHyPEFusedKernelAlignment)
#define ExaHyPEFusedKernelAlignment 64
#endif

#define LittleEndian

#if !defined(UseManualInlining) &&  !defined(noUseManualInlining)
/**
 * I use this flag to switch on __attribute__((always_inline))
 * which I otherwise mask out.
 */
#define UseManualInlining
#endif

