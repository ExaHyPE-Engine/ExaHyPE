#include "Device.h"
#include "tarch/accelerator/Device.h"

#include <map>

#include "tarch/Assertions.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#if defined(GPUOffloadingSYCL)

namespace
{
  std::map<int, sycl::queue> syclQueues;
} // namespace

tarch::accelerator::Device::Device() { listSYCLQueues(); }

tarch::accelerator::Device::~Device() { shutdown(); }

tarch::accelerator::Device& tarch::accelerator::Device::getInstance()
{
  static Device instance;
  return instance;
}

void tarch::accelerator::Device::configure(const std::set<int>& devicesToUse)
{
  auto platforms               = sycl::platform::get_platforms();
  int  physicalNumberOfDevices = 0;
  int  logicalNumberOfDevices  = 0;

  syclQueues.clear();

  for (auto& platform : platforms)
  {
    auto devices = platform.get_devices();
    for (auto& device : devices)
    {
      if (syclQueues.count(physicalNumberOfDevices) > 0)
      {
        logInfo("initSYCLQueues(...)", "skip device #" << physicalNumberOfDevices << " (SYCL queue already set up)");
      }
      else if (syclQueues.count(physicalNumberOfDevices) == 0 and (devicesToUse.empty() or devicesToUse.count(physicalNumberOfDevices) > 0))
      {
        logInfo(
          "initSYCLQueues(...)",
          "init physical device #"
            << physicalNumberOfDevices << " as logical device " << logicalNumberOfDevices
            << ". Device numbers from hereon refer to logical devices"
        );
        syclQueues.insert(std::pair<int, sycl::queue>(logicalNumberOfDevices, sycl::queue(device)));
        logicalNumberOfDevices++;
      }
      else
      {
#if PeanoDebug > 0
        logInfo("initSYCLQueues(...)", "skip device #" << physicalNumberOfDevices << " (masked out)");
#endif
      }
      physicalNumberOfDevices++;
    }
  }
}

void tarch::accelerator::Device::shutdown() { syclQueues.clear(); }

bool tarch::accelerator::Device::isInitialised() const { return not syclQueues.empty(); }

int tarch::accelerator::Device::getNumberOfDevices() const { return syclQueues.size(); }

sycl::queue& tarch::accelerator::getSYCLQueue(int number)
{
  assertion1(syclQueues.count(number) == 1, number);
  return syclQueues[number];
}

void tarch::accelerator::listSYCLQueues()
{
  auto platforms       = sycl::platform::get_platforms();
  int  numberOfDevices = 0;
  std::cout << "SYCL system overview:" << std::endl;
  for (auto& platform : platforms)
  {
    auto devices = platform.get_devices();
    for (auto& device : devices)
    {
      std::cout
        << "device #" << numberOfDevices << ":  platform " << platform.get_info<sycl::info::platform::name>() << " / "
        << device.get_info<sycl::info::device::name>() << std::endl;
      numberOfDevices++;
    }
  }
}

#endif
