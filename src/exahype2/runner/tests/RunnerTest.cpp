// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RunnerTest.h"

#include "exahype2/runner/Runner.h"
#include "exahype2/runner/Statistics.h"
#include "peano4/RefinementRequest.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

exahype2::runner::tests::RunnerTest::RunnerTest() :
  TestCase("exahype2::runner::tests::RunnerTest") {}

void
exahype2::runner::tests::RunnerTest::run() {
  testMethod(testTheMasterStepReachesEveryRank);
  testMethod(testTheTerminationStepReachesEveryRank);
  testMethod(testDescribeMeshUpdatesReportsTheSweepCountAndWallClock);
  testMethod(testDescribeRepetitionsCountsEveryPhaseInItsUnit);
  testMethod(testFinishStepSumsTheSolverBytesAndCellDoFs);
}

/** Every rank continues with the step of the global master, whatever it passes itself. */
void
exahype2::runner::tests::RunnerTest::testTheMasterStepReachesEveryRank() {
  const bool master = tarch::mpi::Rank::getInstance().isGlobalMaster();
  validateEquals(exahype2::runner::Runner::exchangeStep(master ? 23 : exahype2::runner::Runner::NoStep), 23);
  validateEquals(exahype2::runner::Runner::exchangeStep(master ? 7 : exahype2::runner::Runner::NoStep), 7);
}

void
exahype2::runner::tests::RunnerTest::testTheTerminationStepReachesEveryRank() {
  const bool master = tarch::mpi::Rank::getInstance().isGlobalMaster();
  validateEquals(
    exahype2::runner::Runner::exchangeStep(
      master ? exahype2::runner::Runner::TerminationStep : exahype2::runner::Runner::NoStep
    ),
    exahype2::runner::Runner::TerminationStep
  );
}

/** describeMeshUpdates() writes the wall clock and the regrid and sweep counts in singular and plural. */
void
exahype2::runner::tests::RunnerTest::testDescribeMeshUpdatesReportsTheSweepCountAndWallClock() {
  exahype2::runner::Runner::MeshUpdates singleSweep;
  singleSweep.regrids   = 1;
  singleSweep.sweeps    = 1;
  singleSweep.wallClock = 0.25;
  validateEquals(
    exahype2::runner::Runner::describeMeshUpdates(singleSweep),
    std::string("(0.2500 s) · 1 Regrid · 1 Sweep")
  );

  exahype2::runner::Runner::MeshUpdates severalSweeps;
  severalSweeps.regrids   = 12;
  severalSweeps.sweeps    = 2048;
  severalSweeps.wallClock = 12.5;
  validateEquals(
    exahype2::runner::Runner::describeMeshUpdates(severalSweeps),
    std::string("(12.5000 s) · 12 Regrids · 2,048 Sweeps")
  );
}

/**
 * describeRepetitions() counts time steps as steps, plots as plots, dynamic AMR as regrids and sweeps and the grid
 * phases as sweeps.
 */
void
exahype2::runner::tests::RunnerTest::testDescribeRepetitionsCountsEveryPhaseInItsUnit() {
  validateEquals(exahype2::runner::Runner::describeRepetitions("Time Step", 1, 0), std::string("1 Step"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Time Step", 1024, 0), std::string("1,024 Steps"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Plot", 3, 0), std::string("3 Plots"));
  validateEquals(
    exahype2::runner::Runner::describeRepetitions("Dynamic AMR", 2, 1),
    std::string("1 Regrid · 2 Sweeps")
  );
  validateEquals(exahype2::runner::Runner::describeRepetitions("Grid Construction", 5, 0), std::string("5 Sweeps"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Grid Initialisation", 1, 0), std::string("1 Sweep"));
}

/**
 * finishStep() sums the host bytes, the device bytes and the cell degrees of freedom of the
 * solvers over the ranks.
 */
void
exahype2::runner::tests::RunnerTest::testFinishStepSumsTheSolverBytesAndCellDoFs() {
  static const std::vector<peano4::RefinementRequest> noRequests;
  exahype2::runner::SolverRepositoryCallbacks         callbacks;
  callbacks.requestRefinementForGridConstruction = [] {};
  callbacks.allocate                             = [] {};
  callbacks.initialise                           = [] {};
  callbacks.performTimeStep                      = [] {};
  callbacks.regrid                               = []([[maybe_unused]] const peano4::Regrid& regrid) {};
  callbacks.plot                                 = [] {};
  callbacks.getRefinementRequests = []() -> const std::vector<peano4::RefinementRequest>& { return noRequests; };
  callbacks.getAllocatedBytes     = [] { return tarch::AllocatedBytes{300, 20}; };
  callbacks.getNumberOfCellDoFs   = [] { return std::uint64_t{640}; };

  exahype2::runner::Runner runner(
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    std::bitset<DIMENSIONS>(),
    callbacks
  );
  runner.finishStep(exahype2::runner::Runner::Step::TimeStep);

  const auto& statistics    = exahype2::runner::getGlobalStatistics();
  const auto  numberOfRanks = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
  validateEquals(statistics.getGlobalSolverHostBytes(), 300 * numberOfRanks);
  validateEquals(statistics.getGlobalSolverDeviceBytes(), 20 * numberOfRanks);
  validateEquals(statistics.getGlobalNumberOfCellDoFs(), 640 * numberOfRanks);
}
