// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AllocateMemory.h"

#include "peano4/data/ContainerRegistry.h"
#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/Instrumentation.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

runner::AllocateMemory::AllocateMemory(runner::PhaseContext& context) :
  _context(context) {}

void runner::AllocateMemory::run() {
  SCOREP_USER_REGION(runner::PhaseName::AllocateMemory, SCOREP_USER_REGION_TYPE_PHASE)
  static_cast<void>(_context.engine.runStep(runner::Engine::AllocateMemoryProgramStep));
}

/**
 * Reports the pooled payload demand and allocates the pools. The report
 * precedes the allocation, so the demand stands in the log when the
 * allocation exhausts the machine's memory.
 */
void runner::AllocateMemory::executeOnEveryRank(runner::ReportBox& box) {
  auto&      registry      = peano4::data::ContainerRegistry::getInstance();
  const auto localBytes    = static_cast<std::uint64_t>(registry.getPooledPayloadBytes());
  auto       totalBytes    = localBytes;
  auto       minimumBytes  = localBytes;
  auto       maximumBytes  = localBytes;
  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks > 1) {
    const auto communicator = tarch::mpi::Rank::getInstance().getCommunicator();
    tarch::mpi::allreduce(&localBytes, &totalBytes, 1, MPI_UINT64_T, MPI_SUM, communicator);
    tarch::mpi::allreduce(&localBytes, &minimumBytes, 1, MPI_UINT64_T, MPI_MIN, communicator);
    tarch::mpi::allreduce(&localBytes, &maximumBytes, 1, MPI_UINT64_T, MPI_MAX, communicator);
  }

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::string line = std::string(runner::BlockIndent, ' ') + runner::PhaseName::AllocateMemory + " "
      + runner::Format::memory(totalBytes);
    if (numberOfRanks > 1) {
      line +=
        "  (" + runner::Format::memory(minimumBytes) + " .. " + runner::Format::memory(maximumBytes) + " per Rank)";
    }
    box.line(line);
  }

  registry.allocatePayloadPools();
}
