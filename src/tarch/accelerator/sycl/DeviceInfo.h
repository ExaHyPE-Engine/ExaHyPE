#pragma once

#include <string>
#include <vector>

namespace tarch::accelerator::info {
  struct BenchmarkResultInNanoSeconds;
}

namespace tarch::accelerator::info::sycl
{
  std::vector<std::vector<std::string>>     queryDeviceInformation();
  void                                      printDeviceInformation();
  std::vector<bool>                         testKernelLaunch();
  std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::info::sycl
