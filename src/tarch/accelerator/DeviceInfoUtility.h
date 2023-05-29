#pragma once
#include <string>
#include <vector>

namespace tarch::accelerator::info::utility
{
  const std::vector<std::string> properties = {
    "Device Name",
    "Compute Architecture",
    "Global Memory",
    "Shared Memory",
    "Free Memory",
    "Constant Memory",
    "Clock Frequency",
    "Compute Units (Streaming Multiprocessors)",
    "Subgroup Size (Warp Size)",
    "Work Item Sizes (Block Size)",
    "Threads per Work Item",
    "Grid Size"};
  size_t      getPropertyOffset(const char name[]);
  std::string parseDeviceInformation(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices);
} // namespace tarch::accelerator::info::utility