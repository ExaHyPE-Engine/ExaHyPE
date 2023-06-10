#pragma once

#include <string>
#include <vector>

namespace tarch::accelerator::info::cpp
{
  std::vector<std::vector<std::string>> queryDeviceInformation();
  void                                  printDeviceInformation();
  std::vector<bool>                     testKernelLaunch();
} // namespace tarch::accelerator::info::cpp
