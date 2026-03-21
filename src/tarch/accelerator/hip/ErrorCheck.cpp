// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorCheck.h"

#if defined(WITH_GPU_HIP)

#include "tarch/Assertions.h"

#include <sstream>

void tarch::accelerator::hip::checkError(hipError_t code, const char* expr, const char* file, int line) {
  if (code != hipSuccess) {
    (void)hipDeviceSynchronize();
    (void)hipDeviceReset();
    std::stringstream ss;
    ss << "HIP assertion in " << expr << " " << file << "(" << line << "): " << hipGetErrorString(code);
    throw std::runtime_error(ss.str());
  }
}

void tarch::accelerator::hip::checkLastError(const char* file, int line) {
  hipError_t code = hipGetLastError();
  if (code != hipSuccess) {
    (void)hipDeviceSynchronize();
    (void)hipDeviceReset();
    std::stringstream ss;
    ss << "HIP assertion in " << file << "(" << line << "):" << hipGetErrorString(code);
    throw std::runtime_error(ss.str());
  }
}

#endif
