#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator::info
{
  struct BenchmarkResultInNanoSeconds;
}

namespace tarch::accelerator::info::sycl
{
  static tarch::logging::Log _log("tarch::accelerator::info::sycl");

  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);
  void printDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);
  std::vector<bool> testKernelLaunch();
  std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds> runBenchmarks();
} // namespace tarch::accelerator::info::sycl
