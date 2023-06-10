#pragma once

#include <string>
#include <vector>

namespace tarch::accelerator::info::sycl
{
  std::vector<std::vector<std::string>> queryDeviceInformation();
  void                                  printDeviceInformation();
  std::vector<bool>                     testKernelLaunch();
} // namespace tarch::accelerator::info::sycl
