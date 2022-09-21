// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_COMPILER_COMPILER_SPECIFIC_SETTINGS
#define _TARCH_COMPILER_COMPILER_SPECIFIC_SETTINGS

  #if defined(__INTEL_COMPILER) && defined(__WIN32__)
    #include "tarch/compiler/WindowsIntelLegacy.h"
  #elif defined(__GNUC__) && defined(__WIN32__)
    #include "tarch/compiler/WindowsGCC.h"
  /**
   * icpx does not define __INTEL_COMPILER anymore
   */
  #elif defined(__INTEL_COMPILER) && defined(__linux)
    #include "tarch/compiler/LinuxIntelLegacy.h"
  #elif defined(SYCL_LANGUAGE_VERSION) && defined(__INTEL_LLVM_COMPILER) && defined(__linux)
    #include "tarch/compiler/LinuxIntelSYCL.h"
  #elif defined(__INTEL_LLVM_COMPILER) && defined(__linux)
    #include "tarch/compiler/LinuxIntelICPX.h"
  /**
   * The icpx compiler defined __GNUC__ as well, so it has to come last
   */
  #elif defined(__AMDGPU__) && defined(__linux)
    #include "tarch/compiler/LinuxAMD.h"
  #elif defined(__clang__) && defined(__linux)
    #include "tarch/compiler/LinuxClang.h"
  #elif defined(__GNUC__) && defined(__linux)
    #include "tarch/compiler/LinuxGCC.h"
  #elif defined(__clang__) && defined(__APPLE__)
    #include "tarch/compiler/MacOSClang.h"
  #elif defined(__GNUC__) && defined(__APPLE__)
    #include "tarch/compiler/MacOSGCC.h"
  #elif defined(__APPLE__)
    #error Builds on MacOS are only supported with GCC, yet. Please adopt CompilerSpecificSettings.h
  #else
    #error Unknown compiler target. Please adopt CompilerSpecificSettings.h
  #endif


#endif
