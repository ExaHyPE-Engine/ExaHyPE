#pragma once

#include <string>
#include <vector>

namespace tarch::accelerator::info::sycl
{
  std::vector<std::vector<std::string>> queryDeviceInformation();
  void                                  printDeviceInformation();
} // namespace tarch::accelerator::info::sycl
