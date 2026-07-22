// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Init.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryBlocks.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/services/ServiceRepository.h"

void tarch::init(int* argc, char*** argv, const InitConfiguration& configuration) {
  if (tarch::mpi::Rank::getInstance().init(argc, argv)) {
    tarch::mpi::Rank::getInstance().setTimeoutWarning(configuration.mpiTimeoutWarning);
    tarch::mpi::Rank::getInstance().setDeadlockTimeout(configuration.mpiDeadlockTimeout);
    tarch::mpi::Rank::getInstance().setDeadlockDetection(configuration.mpiDeadlockDetectionEnabled);
  } else {
    throw std::runtime_error("Cannot initialise parallel environment");
  }

  if (not tarch::multicore::Core::getInstance().init()) {
    throw std::runtime_error("Cannot initialise shared memory environment");
  }

  if (not tarch::accelerator::Device::getInstance().init()) {
    throw std::runtime_error("Cannot initialise accelerator environment");
  }

  tarch::services::ServiceRepository::getInstance().init();

  // Register the event-driven cross-rank propagation of non-critical assertions. Must run after the
  // service repository is up; the tag reservation is collective, so every rank reaches it here in
  // the same order.
  tarch::initNonCriticalAssertionEnvironment();
}

void tarch::shutdown() {
  tarch::services::ServiceRepository::getInstance().shutdown();
  tarch::accelerator::Device::getInstance().shutdown();
#if PEANO_DEBUG
  tarch::accelerator::MemoryBlocks::getInstance().checkForMemoryLeaks();
  tarch::accelerator::MemoryBlocks::getInstance().clear();
#endif
  tarch::multicore::Core::getInstance().shutdown();
  tarch::instrumentation::Log::close();
  tarch::mpi::Rank::getInstance().shutdown();
}
