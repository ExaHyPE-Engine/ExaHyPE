#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/mpi/Rank.h"

tarch::logging::Log  tarch::accelerator::Device::_log( "tarch::accelerator::Device" );

int tarch::accelerator::Device::_localDeviceId = 0;

#if defined(GPUOffloadingOff)

tarch::accelerator::Device::Device() {}

tarch::accelerator::Device::~Device() {}

tarch::accelerator::Device& tarch::accelerator::Device::getInstance() {
  static Device instance;
  return instance;
}

void tarch::accelerator::Device::configure([[maybe_unused]] const std::set<int>& devicesToUse) {}

void tarch::accelerator::Device::shutdown() {}

bool tarch::accelerator::Device::isInitialised() const {
  return true;
}

int tarch::accelerator::Device::getNumberOfDevices() const {
  return 0;
}

#endif

int tarch::accelerator::Device::getLocalDeviceId() const {
/*#ifdef Parallel
  MPI_Comm nodeLocalComm;
  int worldRank = tarch::mpi::Rank::getInstance().getRank();

  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, worldRank, MPI_INFO_NULL, &nodeLocalComm);

  int nodeLocalRank = -1;
  MPI_Comm_rank(nodeLocalComm, &nodeLocalRank);

  _localDeviceId = nodeLocalRank;

  int numberOfAcceleratorDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
  int nodeLocalSize = -1;
  MPI_Comm_size(nodeLocalComm, &nodeLocalSize);
  if (numberOfAcceleratorDevices != nodeLocalSize) {
    logWarning("tarch::accelerator::Device::getLocalDeviceId()",
      "Number of ranks per node " << nodeLocalSize << " is not equal to number of accelerator devices available per node "
      << numberOfAcceleratorDevices);
  }

  MPI_Comm_free(&nodeLocalComm);
#endif*/

  return _localDeviceId;
}
