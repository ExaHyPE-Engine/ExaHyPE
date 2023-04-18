#include "ErrorCheck.h"

#include "tarch/Assertions.h"

#include <sstream>

#if defined(__NVCOMPILER_CUDA__)
void tarch::accelerator::cuda::checkError(cudaError_t code, const char *file, int line) {
  assertion(code == cudaSuccess);
  if (code != cudaSuccess) {
    std::stringstream ss;
    ss << "CUDA assertion: " << cudaGetErrorString(code) << " " << file << " " << line;
    throw std::runtime_error(ss.str());
  }
}
#endif
