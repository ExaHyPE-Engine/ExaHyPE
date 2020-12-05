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
#define MPISupportsSingleSidedCommunication


#if !defined(UseManualInlining) &&  !defined(noUseManualInlining)
#define UseManualInlining
#endif

#define LittleEndian
