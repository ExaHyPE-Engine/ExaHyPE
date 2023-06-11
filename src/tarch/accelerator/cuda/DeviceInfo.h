#pragma once

#include <string>
#include <vector>

#define THREADS_PER_BLOCK 32

namespace tarch::accelerator::info {
  struct BenchmarkResultInNanoSeconds;
}


namespace tarch::accelerator::info::cuda
{
  std::vector<std::vector<std::string>>     queryDeviceInformation();
  void                                      printDeviceInformation();
  std::vector<bool>                         testKernelLaunch();
  std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::info::cuda
