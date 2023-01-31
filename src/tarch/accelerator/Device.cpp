#include "tarch/accelerator/Device.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

tarch::logging::Log  tarch::accelerator::Device::_log( "tarch::accelerator::Device" );

#if defined(GPUOffloadingOff)

tarch::accelerator::Device::Device() {}

tarch::accelerator::Device::~Device() {}

tarch::accelerator::Device& tarch::accelerator::Device::getInstance() {
  static Device instance;
  return instance;
}

void tarch::accelerator::Device::configure( const std::set<int>& devicesToUse ) {}

void tarch::accelerator::Device::shutdown() {}

bool tarch::accelerator::Device::isInitialised() const {
  return true;
}

int tarch::accelerator::Device::getNumberOfDevices() const {
  return 0;
}

#endif
