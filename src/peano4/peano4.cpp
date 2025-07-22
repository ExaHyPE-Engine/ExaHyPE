// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4.h"

#include "parallel/Node.h"
#include "parallel/SpacetreeSet.h"
#include "tarch/accelerator/Device.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/tarch.h"
#include "utils/Globals.h"

#include <fstream>
#include <list>

/**
 * For the machine name. If it doesn't work, switch it off in the file
 * CompilerSpecificSettings.h.
 */
#ifdef COMPILER_HAS_UTS_NAME
#include <sys/utsname.h>
#endif

namespace {
  tarch::logging::Log _log("peano4");
} // namespace

void writeCopyrightMessage() {
  static bool wrote = false;

  if (not wrote and tarch::mpi::Rank::getInstance().getRank() == tarch::mpi::Rank::getGlobalMasterRank()) {
    logInfo("writeCopyrightMessage()", "Peano 4 (C) www.peano-framework.org");
    std::string buildInfo = "Build: ";

    buildInfo += std::to_string(Dimensions);
    buildInfo += "d";

#ifdef Parallel
    buildInfo += ", MPI (";
    buildInfo += std::to_string(tarch::mpi::Rank::getInstance().getNumberOfRanks());
    buildInfo += " Ranks)";
#endif

#if defined(SharedOMP)
    buildInfo += ", SharedOMP";
    buildInfo += " (";
    buildInfo += std::to_string(tarch::multicore::Core::getInstance().getNumberOfThreads());
    buildInfo += " Threads)";
#elif defined(SharedTBB)
    buildInfo += ", SharedTBB";
    buildInfo += " (";
    buildInfo += std::to_string(tarch::multicore::Core::getInstance().getNumberOfThreads());
    buildInfo += " Threads)";
#elif defined(SharedSYCL)
    buildInfo += ", SharedSYCL";
    buildInfo += " (";
    buildInfo += std::to_string(tarch::multicore::Core::getInstance().getNumberOfThreads());
    buildInfo += " Threads)";
#elif defined(SharedCPP)
    buildInfo += ", SharedCPP";
    buildInfo += " (";
    buildInfo += std::to_string(tarch::multicore::Core::getInstance().getNumberOfThreads());
    buildInfo += " Threads)";
#endif

#if defined(GPUOffloadingOMP)
    buildInfo += ", GPUOffloadingOMP";
    buildInfo += " (";
    buildInfo += std::to_string(tarch::accelerator::Device::getInstance().getNumberOfDevices());
    buildInfo += " GPUs)";
#elif defined(GPUOffloadingSYCL)
    buildInfo += ", GPUOffloadingSYCL";
    buildInfo += " (";
    buildInfo += std::to_string(tarch::accelerator::Device::getInstance().getNumberOfDevices());
    buildInfo += " GPUs)";
#elif defined(GPUOffloadingCPP)
    buildInfo += ", GPUOffloadingCPP";
    buildInfo += " (";
    buildInfo += std::to_string(tarch::accelerator::Device::getInstance().getNumberOfDevices());
    buildInfo += " GPUs)";
#elif defined(GPUOffloadingCUDA)
    buildInfo += ", GPUOffloadingCUDA";
    buildInfo += " (";
    buildInfo += std::to_string(tarch::accelerator::Device::getInstance().getNumberOfDevices());
    buildInfo += " GPUs)";
#endif

    buildInfo += ", Debug Level=" + std::to_string(PeanoDebug);

    logInfo("writeCopyrightMessage()", buildInfo);
  }

  wrote = true;
}

void peano4::init(
  int*                                         argc,
  char***                                      argv,
  const tarch::la::Vector<Dimensions, double>& offset,
  const tarch::la::Vector<Dimensions, double>& width,
  const std::bitset<Dimensions>&               periodicBC
) {
  peano4::utils::setupLookupTableForDLinearised();
  peano4::utils::setupLookupTableForDDelinearised();

  if (tarch::mpi::Rank::getInstance().init(argc, argv)) {
    peano4::parallel::Node::initMPIDatatypes();

    clock_t timeout = 3600;
    tarch::mpi::Rank::getInstance().setTimeOutWarning(timeout / 4);
    tarch::mpi::Rank::getInstance().setDeadlockTimeOut(timeout);
  } else {
    throw std::runtime_error("Cannot initialise parallel environment");
  }

  if (tarch::multicore::Core::getInstance().init()) {
  } else {
    throw std::runtime_error("Cannot initialise shared memory environment");
  }

  if (tarch::accelerator::Device::getInstance().init()) {
  } else {
    throw std::runtime_error("Cannot initialise accelerator environment");
  }

  tarch::initNonCriticalAssertionEnvironment();
  tarch::services::ServiceRepository::getInstance().init();
  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance().init();
  peano4::parallel::Node::getInstance().init();
  peano4::parallel::SpacetreeSet::getInstance().init(offset, width, periodicBC);

  writeCopyrightMessage();
}

void peano4::shutdown() {
  peano4::parallel::Node::getInstance().shutdown();
  peano4::parallel::Node::shutdownMPIDatatypes();
  peano4::parallel::SpacetreeSet::getInstance().shutdown();
  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance().shutdown();
  tarch::services::ServiceRepository::getInstance().shutdown();
  tarch::shutdownNonCriticalAssertionEnvironment();
  tarch::accelerator::Device::getInstance().shutdown();
  tarch::multicore::Core::getInstance().shutdown();
  tarch::mpi::Rank::getInstance().shutdown();
}
