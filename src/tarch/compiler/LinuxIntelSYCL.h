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

#if !defined(Alignment)
#define Alignment 64
#endif

#if !defined(UseManualInlining) &&  !defined(noUseManualInlining)
/**
 * I use this flag to switch on __attribute__((always_inline))
 * which I otherwise mask out.
 */
#define UseManualInlining
#endif


#define LittleEndian


/**
 * Ugly hacks, but SYCL uses Dimensions as template argument. So I quickly
 * memorise them, undef them, and then redefine them once SYCL is loadd.
 */
#pragma push_macro("Dimensions")
#pragma push_macro("assertion")
#undef Dimensions
#undef assertion
#include <CL/sycl.hpp>
#pragma pop_macro("Dimensions")
#pragma pop_macro("assertion")


/**
 * Clang requires us to put a static keyword in front of header file implementations,
 * so we avoid duplicate symbols. This is not C++ conformal, and therefore GCC doesn't
 * eat stuff. But Clang needs a static.
 */
#define KeywordToAvoidDuplicateSymbols static

