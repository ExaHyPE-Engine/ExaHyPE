#pragma once

#include <string>
#include <vector>

namespace tarch::accelerator::info
{
  std::vector<std::vector<std::string>> queryDeviceInformation();
  void                                  printDeviceInformation();

  const std::vector<std::string> properties = {
    "Device Name",
    "Global Memory",
    "Free Memory",
    "Clock Frequency",
    "Compute Units",
    };

  size_t      getPropertyOffset(const char name[]);
  std::string parseDeviceInformation(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices);
  std::string parseDeviceInformationDeviceAsCol(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices);

} // namespace tarch::accelerator::info
