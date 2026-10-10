// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RunnerTest.h"

#include "exahype2/runner/Runner.h"
#include "exahype2/runner/Statistics.h"
#include "peano4/Partition.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "peano4/topology/Tree.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"

exahype2::runner::tests::RunnerTest::RunnerTest() :
  TestCase("exahype2::runner::tests::RunnerTest") {}

void
exahype2::runner::tests::RunnerTest::run() {
  testMethod(testDescribeMeshUpdatesReportsTheStepCountAndWallClock);
  testMethod(testDescribeRepetitionsCountsEveryPhaseInItsUnit);
  testMethod(testFinishStepSumsTheSolverBytesAndCellUpdates);
  testMethod(testFinishStepReducesTheValuesOfTheSolvers);
  testMethod(testTheGridInitialisationAppliesOnlyTheVotesOfTheInitialCondition);
}

void
exahype2::runner::tests::RunnerTest::testDescribeMeshUpdatesReportsTheStepCountAndWallClock() {
  exahype2::runner::Runner::MeshUpdates singleStep;
  singleStep.regrids   = 1;
  singleStep.steps     = 1;
  singleStep.wallClock = 0.25;
  validateEquals(
    exahype2::runner::Runner::describeMeshUpdates(singleStep),
    std::string("(0.2500 s) · 1 Regrid · 1 Step")
  );

  exahype2::runner::Runner::MeshUpdates severalSteps;
  severalSteps.regrids   = 12;
  severalSteps.steps     = 2048;
  severalSteps.wallClock = 12.5;
  validateEquals(
    exahype2::runner::Runner::describeMeshUpdates(severalSteps),
    std::string("(12.5000 s) · 12 Regrids · 2,048 Steps")
  );
}

void
exahype2::runner::tests::RunnerTest::testDescribeRepetitionsCountsEveryPhaseInItsUnit() {
  validateEquals(exahype2::runner::Runner::describeRepetitions("Time Step", 1, 0), std::string("1 Step"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Time Step", 1024, 0), std::string("1,024 Steps"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Plot", 3, 0), std::string("3 Plots"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Dynamic AMR", 2, 1), std::string("1 Regrid · 2 Steps"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Grid Construction", 5, 0), std::string("5 Steps"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Grid Initialisation", 1, 0), std::string("1 Round"));
  validateEquals(exahype2::runner::Runner::describeRepetitions("Grid Initialisation", 2, 0), std::string("2 Rounds"));
}

void
exahype2::runner::tests::RunnerTest::testFinishStepSumsTheSolverBytesAndCellUpdates() {
  static const std::vector<peano4::RefinementRequest> noRequests;
  exahype2::runner::SolverRepositoryCallbacks         callbacks;
  callbacks.requestRefinementForGridConstruction = [] {};
  callbacks.allocate                             = [] {};
  callbacks.initialise                           = [] {};
  callbacks.performTimeStep                      = [] {};
  callbacks.regrid                               = []([[maybe_unused]] const peano4::Regrid& regrid) {};
  callbacks.plot                                 = [] {};
  callbacks.getRefinementRequests  = []() -> const std::vector<peano4::RefinementRequest>& { return noRequests; };
  callbacks.getAllocatedBytes      = [] { return tarch::AllocatedBytes{300, 20}; };
  callbacks.getNumberOfSolverCells = [] { return std::uint64_t{10}; };
  callbacks.getNumberOfCellDoFs    = [] { return std::uint64_t{640}; };

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
  validateEquals(statistics.getGlobalNumberOfSolverCells(), 10 * numberOfRanks);
  validateEquals(statistics.getGlobalNumberOfCellDoFs(), 640 * numberOfRanks);
}

/**
 * Every step reads the sum of the values that the ranks add to the global reduction.
 */
void
exahype2::runner::tests::RunnerTest::testFinishStepReducesTheValuesOfTheSolvers() {
  static const std::vector<peano4::RefinementRequest> noRequests;
  const auto rank          = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getRank());
  const auto numberOfRanks = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
  const auto sumOfRanks    = numberOfRanks * (numberOfRanks + 1) / 2;
  int        slot          = -1;
  std::uint64_t                               additions    = 0;
  std::uint64_t                               reads        = 0;
  std::uint64_t                               sumOverRanks = 0;
  exahype2::runner::SolverRepositoryCallbacks callbacks;
  callbacks.requestRefinementForGridConstruction = [] {};
  callbacks.allocate                             = [] {};
  callbacks.initialise                           = [] {};
  callbacks.performTimeStep                      = [] {};
  callbacks.regrid                               = []([[maybe_unused]] const peano4::Regrid& regrid) {};
  callbacks.plot                                 = [] {};
  callbacks.getRefinementRequests = []() -> const std::vector<peano4::RefinementRequest>& { return noRequests; };
  callbacks.addLocalValuesToGlobalReduction = [&slot, &additions, rank](tarch::mpi::ReductionRecord& record) {
    additions++;
    slot = record.addSum((rank + 1) * additions);
  };
  callbacks.readValuesFromGlobalReduction = [&slot, &reads, &sumOverRanks](const tarch::mpi::ReductionRecord& record) {
    reads++;
    sumOverRanks = record.getSum(slot);
  };
  exahype2::runner::Runner runner(
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    std::bitset<DIMENSIONS>(),
    callbacks
  );

  runner.finishStep(exahype2::runner::Runner::Step::TimeStep);
  validateEquals(reads, std::uint64_t{1});
  validateEquals(sumOverRanks, sumOfRanks);
  runner.finishStep(exahype2::runner::Runner::Step::TimeStep);
  validateEquals(reads, std::uint64_t{2});
  validateEquals(sumOverRanks, 2 * sumOfRanks);
  runner.finishStep(exahype2::runner::Runner::Step::PlotSolution);
  validateEquals(reads, std::uint64_t{3});
  validateEquals(sumOverRanks, 3 * sumOfRanks);
}

/**
 * A regrid between two initialisations applies only the votes of the initial condition.
 */
void
exahype2::runner::tests::RunnerTest::testTheGridInitialisationAppliesOnlyTheVotesOfTheInitialCondition() {
  constexpr int Depth = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;
  peano4::tests::buildRegularGrid(Depth);

  peano4::RefinementRequest cornerRefinement{};
  cornerRefinement.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
  cornerRefinement.level        = Depth;
  cornerRefinement.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(0);

  std::vector<peano4::RefinementRequest>      requests;
  int                                         initialisations = 0;
  int                                         regrids         = 0;
  exahype2::runner::SolverRepositoryCallbacks callbacks;
  callbacks.requestRefinementForGridConstruction = [] {};
  callbacks.allocate                             = [] {};
  callbacks.initialise                           = [&requests, &initialisations] {
    initialisations++;
    requests.clear();
    if (initialisations == 1) {
      requests.push_back(peano4::tests::getRefinementOfCentreCell(Depth));
    }
  };
  callbacks.performTimeStep = [] {};
  callbacks.regrid          = [&requests, &regrids, &cornerRefinement]([[maybe_unused]] const peano4::Regrid& regrid) {
    regrids++;
    requests = {cornerRefinement};
  };
  callbacks.plot                  = [] {};
  callbacks.getRefinementRequests = [&requests]() -> const std::vector<peano4::RefinementRequest>& { return requests; };

  exahype2::runner::Runner runner(
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    std::bitset<DIMENSIONS>(),
    callbacks
  );
  runner.initialiseGrid();

  const auto& spacetree = peano4::Spacetree::getInstance();
  const auto  centreChild =
    tarch::la::Vector<DIMENSIONS, int>(peano4::SubdivisionFactor * (peano4::topology::getCellsPerAxis(Depth) / 2) + 1);
  const auto corner = tarch::la::Vector<DIMENSIONS, int>(0);
  validateEquals(initialisations, 2);
  validateEquals(regrids, 1);
  validateEquals(peano4::tests::getGlobalSumOf(spacetree.getCellAt(Depth + 1, centreChild) >= 0 ? 1 : 0), 1);
  validateEquals(peano4::tests::getGlobalSumOf(spacetree.getCellAt(Depth, corner) >= 0 ? 1 : 0), 1);
  validateEquals(peano4::tests::getGlobalSumOf(spacetree.getCellAt(Depth + 1, corner) >= 0 ? 1 : 0), 0);
}
