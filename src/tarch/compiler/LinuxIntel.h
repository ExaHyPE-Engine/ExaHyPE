// Default variant of CompilerSpecificSettings.h for IBM's XLC compiler.
// For a detailed description of the semantics of the settings, please
// consult LinuxIntel.h which is the most elaborate documentation.


/**
 * Switch off Optimisation
 *
 * Some compilers (icc 10.x, e.g.) run into problems compiling the test cases
 * as they run out of memory. In this case, one can use these two defined
 * within the implementation files. Unfortunately, the corresponding pragmas
 * are not supported by all compilers (gcc 4.x.x, e.g.). Therefore, I
 * encapsulated them within my own defines.
 *
 * To make define work, see the documentation of the test super class.
 */
#define UseTestSpecificCompilerSettings


#ifndef noCompilerICC
  #define CompilerICC
#endif

#define CompilerHasProcStat

#define CompilerHasUTSName

#define CompilerHasTimespec

#define CompilerHasSysinfo

if !defined(noMPISupportsSingleSidedCommunication) and !defined(MPISupportsSingleSidedCommunication)
#define MPISupportsSingleSidedCommunication
#endif

//#define CompilerDefinesMPIMaxNameString



#if !defined(UseManualInlining) &&  !defined(noUseManualInlining)
/**
 * I use this flag to switch on statements alike  __attribute__((always_inline))
 * which I otherwise mask out.
 */
#define UseManualInlining
#define UseManualInlining
#endif

/**
 * I basically need these settings only in the plotters
 */
#define LittleEndian
