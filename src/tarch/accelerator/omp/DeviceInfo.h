#pragma once

#include <string>
#include <vector>

namespace tarch::accelerator::info::omp
{
  std::vector<std::vector<std::string>> queryDeviceInformation();
  void                                  printDeviceInformation();
} // namespace tarch::accelerator::info::omp
