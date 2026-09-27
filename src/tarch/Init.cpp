// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Init.h"

#include "tarch/Termination.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/DeviceCopies.h"
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

void
tarch::init(int* argc, char*** argv, const InitConfiguration& configuration) {
  if (tarch::mpi::Rank::getInstance().init(argc, argv)) {
    tarch::mpi::Diagnostics::getInstance().setTimeoutWarning(configuration.mpiTimeoutWarning);
    tarch::mpi::Diagnostics::getInstance().setDeadlockTimeout(configuration.mpiDeadlockTimeout);
    tarch::mpi::Diagnostics::getInstance().setRequestTracking(configuration.mpiRequestTrackingEnabled);
  } else {
    tarch::terminateWithError("Cannot initialise parallel environment.");
  }

  // The communication thread reserves one core, so this call runs before the task runtime sizes
  // its team.
  tarch::mpi::AbstractCommunicationManager::configure(configuration.communicationThreadEnabled);

  if (not tarch::tasking::Runtime::getInstance().init()) {
    tarch::terminateWithError("Cannot initialise multithreading environment.");
  }

  if (not tarch::accelerator::Device::getInstance().init()) {
    tarch::terminateWithError("Cannot initialise accelerator environment.");
  }
  // The ranks of a node take the devices of the node in turn. The selection precedes the first
  // allocation, because every buffer allocates on the current device.
  const int numberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();
  if (numberOfDevices > 0) {
    tarch::accelerator::setCurrentDevice(tarch::mpi::Rank::getInstance().getRankOnNode() % numberOfDevices);
  }
  tarch::accelerator::uploadDeviceCopies();
}

void
tarch::shutdown() {
  tarch::accelerator::Device::getInstance().shutdown();
#if PEANO_DEBUG
  tarch::accelerator::MemoryBlocks::getInstance().checkForMemoryLeaks();
  tarch::accelerator::MemoryBlocks::getInstance().clear();
#endif
  tarch::tasking::Runtime::getInstance().shutdown();
  tarch::instrumentation::Log::close();
  tarch::mpi::Rank::getInstance().shutdown();
}
