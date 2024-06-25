#include "peano.h"
#include "../config.h"

#include "peano4/utils/Loop.h"
#include "peano4/parallel/Node.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/accelerator/Device.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/BooleanSemaphore.h"
#include "tarch/tarch.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"

#include <list>
#include <fstream>

/**
 * For the machine name. If it doesn't work, switch it off in the file
 * CompilerSpecificSettings.h.
 */
#ifdef CompilerHasUTSName
#include <sys/utsname.h>
#endif

void peano4:: writeCopyrightMessage() {
  static bool wrote = false;
  if (
    not wrote and tarch::mpi::Rank::getInstance().getRank() == tarch::mpi::Rank::getGlobalMasterRank() ) {
    std::cout << "Peano 4 (C) www.peano-framework.org" << std::endl;
    std::string buildInfo = "build: ";

    buildInfo += std::to_string(Dimensions);
    buildInfo += "d";

    #ifdef Parallel
    buildInfo += ", with mpi (";
    buildInfo += std::to_string( tarch::mpi::Rank::getInstance().getNumberOfRanks() );
    buildInfo += " ranks)";
    #else
    buildInfo += ", no mpi";
    #endif

    #if defined(SharedOMP)
    buildInfo += ", omp";
    #elif defined(SharedTBB)
    buildInfo += ", tbb";
    #elif defined(SharedSYCL)
    buildInfo += ", sycl threading";
    #elif defined(SharedCPP)
    buildInfo += ", C++ threading";
    #else
    buildInfo += ", no threading";
    #endif

    if (tarch::multicore::Core::getInstance().getNumberOfThreads()>1) {
      buildInfo += " (";
      buildInfo += std::to_string( tarch::multicore::Core::getInstance().getNumberOfThreads() );
      buildInfo += " threads)";
    }

    #if defined(GPUOffloadingOMP)
    buildInfo += ", omp offloading";
    #elif defined(GPUOffloadingSYCL)
    buildInfo += ", sycl";
    #elif defined(GPUOffloadingCPP)
    buildInfo += ", C++ offloading";
    #elif defined(GPUOffloadingCUDA)
    buildInfo += ", CUDA offloading";
    #else
    buildInfo += ", no gpu support";
    #endif

    if (tarch::accelerator::Device::getInstance().getNumberOfDevices()>0) {
      buildInfo += " (";
      buildInfo += std::to_string( tarch::accelerator::Device::getInstance().getNumberOfDevices() );
      buildInfo += " gpus)";
    }

    #if PeanoDebug>0
    buildInfo += ", debug level=" + std::to_string(PeanoDebug);
    #endif

    std::cout << buildInfo << std::endl;
  }
  wrote = true;
}

void peano4::fillLookupTables() {
/*
  Had in in Globals.cpp

  setupLookupTableForTwoPowI();
  setupLookupTableForThreePowI();
  setupLookupTableForFourPowI();
  setupLookupTableForDPowI();
*/

  peano4::utils::setupLookupTableForDLinearised();
  peano4::utils::setupLookupTableForDDelinearised();
}

int peano4::initParallelEnvironment([[maybe_unused]] int* argc, [[maybe_unused]] char*** argv) {
  int result = 0;
  #ifdef Parallel
  if ( tarch::mpi::Rank::getInstance().init(argc, argv) ) {
    peano4::parallel::Node::initMPIDatatypes();

    clock_t timeout = 60;
    tarch::mpi::Rank::getInstance().setTimeOutWarning(timeout/4);
    tarch::mpi::Rank::getInstance().setDeadlockTimeOut(timeout);
  } else {
    result = -2;
  }
  #endif

  #ifdef SharedMemoryParallelisation
  if ( tarch::multicore::Core::getInstance().isInitialised() ) {
  } else {
    result = -3;
  }
  #endif

  writeCopyrightMessage();

  return result;
}

void peano4::shutdownParallelEnvironment() {
  tarch::multicore::Core::getInstance().shutdown();
  peano4::parallel::Node::shutdownMPIDatatypes();
  tarch::mpi::Rank::getInstance().shutdown();
}

void peano4::initSingletons(
  const tarch::la::Vector<Dimensions,double>&  offset,
  const tarch::la::Vector<Dimensions,double>&  width,
  const std::bitset<Dimensions>&               periodicBC
) {
  tarch::services::ServiceRepository::getInstance().init();
  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance().init();
  peano4::parallel::Node::getInstance().init();
  peano4::parallel::SpacetreeSet::getInstance().init(
    offset,
    width,
    periodicBC
  );

  writeCopyrightMessage();
}

void peano4::shutdownSingletons() {
  peano4::parallel::Node::getInstance().shutdown();
  peano4::parallel::SpacetreeSet::getInstance().shutdown();
  tarch::mpi::BooleanSemaphore::BooleanSemaphoreService::getInstance().shutdown();
  tarch::services::ServiceRepository::getInstance().shutdown();
}
