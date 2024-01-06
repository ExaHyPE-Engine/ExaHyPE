// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch.h"

#include <climits>
#include <iostream>
#include <limits>
#include <stddef.h>

#include "accelerator/Device.h"
#include "mpi/Rank.h"
#include "multicore/Core.h"
#include "tarch/services/ServiceRepository.h"

static_assert(sizeof(void*) == 8, "Assuming 64-bit build; i.e., 8-byte pointers");
static_assert(std::numeric_limits<float>::is_iec559, "std::numeric_limits<float>::is_iec559");
static_assert(std::numeric_limits<double>::is_iec559, "std::numeric_limits<double>::is_iec55");
static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
static_assert(sizeof(char) == 1, "sizeof(short) != 1");
static_assert(sizeof(short) == 2, "sizeof(short) != 2");
static_assert(sizeof(int) == 4, "sizeof(int) != 4");
static_assert(sizeof(long long) == 8, "sizeof(long long) != 8");
static_assert('A' == 65, "A != 65");

namespace {
  tarch::logging::Log _log("tarch");
} // namespace

void tarch::dumpStatistics() {
  std::stringstream msg;

  int numberOfRanks        = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  int localNumberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();
  int localNumberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();

  int globalNumberOfThreads = 0;
  int globalNumberOfDevices = 0;

#ifdef Parallel
  tarch::mpi::Rank::getInstance().reduce(&localNumberOfThreads, &globalNumberOfThreads, 1, MPI_INT, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), [&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });

  tarch::mpi::Rank::getInstance().reduce(&localNumberOfDevices, &globalNumberOfDevices, 1, MPI_INT, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), [&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });
#else
  globalNumberOfThreads = localNumberOfThreads;
  globalNumberOfDevices = localNumberOfDevices;
#endif

  msg << numberOfRanks << " Rank(s), ";
  msg << globalNumberOfThreads << " Thread(s), ";
  msg << globalNumberOfDevices << " Device(s)";

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logInfo("dumpStatistics()", msg.str());
  }
}
