// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Init.h"

#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

void tarch::init(int* argc, char*** argv, const InitConfiguration& configuration) {
  if (tarch::mpi::Rank::getInstance().init(argc, argv)) {
    tarch::mpi::Diagnostics::getInstance().setTimeoutWarning(configuration.mpiTimeoutWarning);
    tarch::mpi::Diagnostics::getInstance().setDeadlockTimeout(configuration.mpiDeadlockTimeout);
    tarch::mpi::Diagnostics::getInstance().setRequestTracking(configuration.mpiRequestTrackingEnabled);
  } else {
    throw std::runtime_error("Cannot initialise parallel environment");
  }

  if (not tarch::multicore::Core::getInstance().init()) {
    throw std::runtime_error("Cannot initialise shared memory environment");
  }

  if (not tarch::accelerator::Device::getInstance().init()) {
    throw std::runtime_error("Cannot initialise accelerator environment");
  }
}

void tarch::shutdown() {
  tarch::accelerator::Device::getInstance().shutdown();
#if PEANO_DEBUG
  tarch::accelerator::MemoryBlocks::getInstance().checkForMemoryLeaks();
  tarch::accelerator::MemoryBlocks::getInstance().clear();
#endif
  tarch::multicore::Core::getInstance().shutdown();
  tarch::instrumentation::Log::close();
  tarch::mpi::Rank::getInstance().shutdown();
}
