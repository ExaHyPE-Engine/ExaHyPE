// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorCheck.h"

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/Termination.h"

void
tarch::accelerator::cuda::checkError(
  const cudaError_t code,
  const char* const expr,
  const char* const file,
  const int         line
) {
  if (code != cudaSuccess) {
    cudaDeviceSynchronize();
    cudaDeviceReset();
    std::ostringstream message;
    message << "The CUDA call " << expr << " in " << file << "(" << line << ") failed: " << cudaGetErrorString(code);
    tarch::terminateWithError(message.str());
  }
}

#endif
