// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorCheck.h"

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/Termination.h"

void
tarch::accelerator::hip::checkError(
  const hipError_t  code,
  const char* const expr,
  const char* const file,
  const int         line
) {
  if (code != hipSuccess) {
    static_cast<void>(hipDeviceSynchronize());
    static_cast<void>(hipDeviceReset());
    std::ostringstream message;
    message << "The HIP call " << expr << " in " << file << "(" << line << ") failed: " << hipGetErrorString(code);
    tarch::terminateWithError(message.str());
  }
}

#endif
