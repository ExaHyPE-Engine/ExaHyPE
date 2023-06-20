#pragma once

#include <string>
#include <vector>

#include "tarch/logging/Log.h"

namespace tarch::accelerator::info
{
  struct BenchmarkResultInNanoSeconds;
}

namespace tarch::accelerator::info::cuda
{
  static tarch::logging::Log _log("tarch::accelerator::info::cuda");

  void queryDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);
  void printDeviceInformation(std::vector<std::vector<std::string>> &propertyMatrix);
  std::vector<bool> testKernelLaunch();
  std::vector<tarch::accelerator::info::BenchmarkResultInNanoSeconds> runBenchmarks();
  size_t allocableSize(size_t base);
} // namespace tarch::accelerator::info::cuda
