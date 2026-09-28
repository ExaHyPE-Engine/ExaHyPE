// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StatisticsTest.h"

#include "exahype2/TimeStepping.h"
#include "exahype2/runner/Console.h"
#include "exahype2/runner/Statistics.h"
#include "peano4/Spacetree.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

namespace {
  std::uint64_t getGlobalSumOf(const std::uint64_t localValue) {
    std::uint64_t globalValue = 0;
    tarch::mpi::allreduce(
      &localValue,
      &globalValue,
      1,
      MPI_UINT64_T,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    return globalValue;
  }

  std::uint64_t getGlobalMinimumOf(const std::uint64_t localValue) {
    std::uint64_t globalValue = 0;
    tarch::mpi::allreduce(
      &localValue,
      &globalValue,
      1,
      MPI_UINT64_T,
      MPI_MIN,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    return globalValue;
  }

  std::uint64_t getGlobalMaximumOf(const std::uint64_t localValue) {
    std::uint64_t globalValue = 0;
    tarch::mpi::allreduce(
      &localValue,
      &globalValue,
      1,
      MPI_UINT64_T,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    return globalValue;
  }

  void appendNothing([[maybe_unused]] const std::vector<exahype2::runner::SolverStatistic>& statistics) {}
} // namespace

/**
 * A solver with a fixed rule, which reports no wave speed.
 */
struct exahype2::runner::tests::StatisticsTest::PlainSolver final {
  PlainSolver() {
    timeStepping.advanceTimeStamp(0.25);
    timeStepping.reduceMeshWidthsAcrossRanks(0.5, 1.5);
  }
  [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const { return timeStepping; }
  exahype2::TimeStepping timeStepping{
    exahype2::TimeStepping::Rule::Fixed,
    1.0,
    exahype2::TimeStepping::Stability::PhysicalEigenvalue
  };
};

/**
 * A solver with an adaptive rule and a limiter.
 */
struct exahype2::runner::tests::StatisticsTest::LimitedSolver final {
  static constexpr bool HasLimiter = true;
  LimitedSolver() {
    timeStepping.advanceTimeStamp(0.125);
    timeStepping.reduceMeshWidthsAcrossRanks(1.0, 3.0);
    timeStepping.raiseMaxEigenvalue(7.0);
    timeStepping.reduceMaxEigenvaluesAcrossRanks();
  }
  [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const { return timeStepping; }
  [[nodiscard]] long getNumberOfTroubledCells() const { return troubledCells; }
  exahype2::TimeStepping timeStepping{
    exahype2::TimeStepping::Rule::Adaptive,
    1.0,
    exahype2::TimeStepping::Stability::PhysicalEigenvalue
  };
  long troubledCells{0};
};

/**
 * A solver with an adaptive rule and no limiter, which offers a troubled cell count all the same.
 */
struct exahype2::runner::tests::StatisticsTest::UnlimitedSolver final {
  static constexpr bool HasLimiter = false;
  UnlimitedSolver() {
    timeStepping.advanceTimeStamp(0.125);
    timeStepping.reduceMeshWidthsAcrossRanks(1.0, 3.0);
    timeStepping.raiseMaxEigenvalue(7.0);
    timeStepping.reduceMaxEigenvaluesAcrossRanks();
  }
  [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const { return timeStepping; }
  [[nodiscard]] long getNumberOfTroubledCells() const { return 0; }
  exahype2::TimeStepping timeStepping{
    exahype2::TimeStepping::Rule::Adaptive,
    1.0,
    exahype2::TimeStepping::Stability::PhysicalEigenvalue
  };
};

exahype2::runner::tests::StatisticsTest::StatisticsTest() :
  tarch::tests::TestCase("exahype2::runner::tests::StatisticsTest") {}

void
exahype2::runner::tests::StatisticsTest::run() {
  testMethod(testTheGlobalViewSumsAndBoundsTheRankStatistics);
  testMethod(testSnapshotFillsTheMemoryRowsOfTheBuild);
  testMethod(testDescribeGridSnapshotRendersFilledRows);
  testMethod(testDescribeGridTransitionMarksChangedValuesOnly);
  testMethod(testUsedAndFreeMemoryFormOneLine);
  testMethod(testValueRangeAndCountCarryTheirKind);
  testMethod(testASolverWithoutEigenvalueReportsStepAndMeshWidth);
  testMethod(testASolverWithEigenvalueAndTroubledCellsAppendsBoth);
  testMethod(testASolverWithoutLimiterReportsNoTroubledCells);
  testMethod(testAdditionalStatisticsFollowTheMeshWidth);
}

/**
 * The synchronised view holds the sums, minima and maxima of the statistics every rank derives,
 * and counts a rank boundary face once.
 */
void
exahype2::runner::tests::StatisticsTest::testTheGlobalViewSumsAndBoundsTheRankStatistics() {
  const auto  rank       = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getRank());
  const auto& local      = peano4::Spacetree::getInstance().getStatistics();
  auto&       statistics = exahype2::runner::getGlobalStatistics();
  statistics.setLocalSolverBytes(100 + rank, 7 + rank);
  statistics.setLocalCellUpdates(8 * (rank + 1), 64 * (rank + 1));
  statistics.synchronise(true);

  const auto cells             = static_cast<std::uint64_t>(local.numberOfCells);
  const auto faces             = static_cast<std::uint64_t>(local.numberOfFaces);
  const auto rankBoundaryFaces = static_cast<std::uint64_t>(local.numberOfExchangedFaces);
  validateEquals(statistics.getGlobalNumberOfInnerUnrefinedCells(), getGlobalSumOf(cells));
  validateEquals(statistics.getGlobalNumberOfInnerRefinedCells(), getGlobalSumOf(local.numberOfRefinedCells));
  const auto distinctRankBoundaryFaces = getGlobalSumOf(rankBoundaryFaces) / 2;
  validateEquals(getGlobalSumOf(rankBoundaryFaces) % 2, std::uint64_t{0});
  validateEquals(statistics.getGlobalNumberOfFaces(), getGlobalSumOf(faces) - distinctRankBoundaryFaces);
  validateEquals(statistics.getGlobalNumberOfHangingFaces(), getGlobalSumOf(local.numberOfHangingFaces));
  validateEquals(statistics.getGlobalMinRankBoundaryFaces(), getGlobalMinimumOf(rankBoundaryFaces));
  validateEquals(statistics.getGlobalMaxRankBoundaryFaces(), getGlobalMaximumOf(rankBoundaryFaces));
  validateEquals(statistics.getGlobalMinRankLoad(), getGlobalMinimumOf(cells));
  validateEquals(statistics.getGlobalMaxRankLoad(), getGlobalMaximumOf(cells));
  validateEquals(statistics.getGlobalPartitionHostBytes(), getGlobalSumOf(local.partitionBytes.host));
  validateEquals(statistics.getGlobalPartitionDeviceBytes(), getGlobalSumOf(local.partitionBytes.device));
  validateEquals(statistics.getGlobalTreeBytes(), getGlobalSumOf(local.treeBytes));
  validateEquals(statistics.getGlobalSolverHostBytes(), getGlobalSumOf(100 + rank));
  validateEquals(statistics.getGlobalSolverDeviceBytes(), getGlobalSumOf(7 + rank));
  validateEquals(statistics.getGlobalNumberOfSolverCells(), getGlobalSumOf(8 * (rank + 1)));
  validateEquals(statistics.getGlobalNumberOfCellDoFs(), getGlobalSumOf(64 * (rank + 1)));
  validateEquals(
    static_cast<std::uint64_t>(statistics.getGlobalMinimumLevel()),
    getGlobalMinimumOf(static_cast<std::uint64_t>(local.minimumLevel))
  );
  validateEquals(
    static_cast<std::uint64_t>(statistics.getGlobalMaximumLevel()),
    getGlobalMaximumOf(static_cast<std::uint64_t>(local.maximumLevel))
  );
  validateEquals(
    static_cast<std::uint64_t>(statistics.getGlobalNumberOfStationaryRegrids()),
    getGlobalMinimumOf(static_cast<std::uint64_t>(local.numberOfStationaryRegrids))
  );
  const auto globalFaces = statistics.getGlobalNumberOfFaces();
  const auto expectedPercentage =
    globalFaces > 0 ? 100.0 * static_cast<double>(distinctRankBoundaryFaces) / static_cast<double>(globalFaces) : 0.0;
  validateNumericalEquals(statistics.getGlobalRankBoundaryFacePercentage(), expectedPercentage);
  validate(statistics.getGlobalInterRankBalance() > 0.0 and statistics.getGlobalInterRankBalance() <= 100.0);
  validate(statistics.getGlobalTotalMemoryInBytes() > 0);
  validate(statistics.getGlobalAvailableMemoryInBytes() > 0);
  validate(statistics.getGlobalAvailableMemoryInBytes() <= statistics.getGlobalTotalMemoryInBytes());
  validate(statistics.getGlobalProcessMemoryUsageInBytes() > 0);
}

/**
 * A snapshot fills the CPU memory rows, and the GPU memory rows only in a build with a GPU backend.
 */
void
exahype2::runner::tests::StatisticsTest::testSnapshotFillsTheMemoryRowsOfTheBuild() {
  using GridSnapshot = exahype2::runner::GridSnapshot;
  auto& statistics   = exahype2::runner::getGlobalStatistics();
  statistics.setLocalSolverBytes(2048, 1024);
  statistics.synchronise(true);
  const auto snapshot = exahype2::runner::collectGridSnapshot();

  validateEquals(
    snapshot.entries[GridSnapshot::MemoryCPUSolverData].value,
    exahype2::runner::Format::getMemory(statistics.getGlobalSolverHostBytes())
  );
  validate(not snapshot.entries[GridSnapshot::MemoryCPUGrid].value.empty());
  validate(not snapshot.entries[GridSnapshot::MemoryUsed].value.empty());
  validate(not snapshot.entries[GridSnapshot::MemoryFree].value.empty());
#if defined(PEANO_WITH_GPU_BACKEND)
  validateEquals(
    snapshot.entries[GridSnapshot::MemoryGPUSolverData].value,
    exahype2::runner::Format::getMemory(statistics.getGlobalSolverDeviceBytes())
  );
  validate(not snapshot.entries[GridSnapshot::MemoryGPUGrid].value.empty());
#else
  validate(snapshot.entries[GridSnapshot::MemoryGPUSolverData].value.empty());
  validate(snapshot.entries[GridSnapshot::MemoryGPUGrid].value.empty());
#endif
}

/**
 * A filled row renders as value, suffix and bracketed detail under its section; an empty row stays out.
 */
void
exahype2::runner::tests::StatisticsTest::testDescribeGridSnapshotRendersFilledRows() {
  using GridSnapshot = exahype2::runner::GridSnapshot;
  GridSnapshot snapshot;
  snapshot.entries[GridSnapshot::GridCells]  = {"81", "Refined 10", ""};
  snapshot.entries[GridSnapshot::GridLevels] = {"2 .. 4", "", ""};
  snapshot.entries[GridSnapshot::MemoryFree] = {"4.29 GiB", "", " of 15.56 GiB"};
  const auto lines                           = exahype2::runner::describeGridSnapshot(snapshot);
  validateEquals(lines.size(), std::size_t{5});
  validateEquals(lines[0], std::string("  Grid"));
  validateEquals(lines[1], std::string("    Cells   81  (Refined 10)"));
  validateEquals(lines[2], std::string("    Levels  2 .. 4"));
  validateEquals(lines[3], std::string("  Memory"));
  validateEquals(lines[4], std::string("    Free    4.29 GiB of 15.56 GiB"));
}

/**
 * describeGridTransition() writes an arrow for a changed value and the value alone for an unchanged one.
 */
void
exahype2::runner::tests::StatisticsTest::testDescribeGridTransitionMarksChangedValuesOnly() {
  using GridSnapshot = exahype2::runner::GridSnapshot;
  GridSnapshot before;
  GridSnapshot after;
  before.entries[GridSnapshot::GridCells]           = {"81", "Refined 10", ""};
  after.entries[GridSnapshot::GridCells]            = {"633", "Refined 79", ""};
  before.entries[GridSnapshot::GridLevels]          = {"2 .. 4", "", ""};
  after.entries[GridSnapshot::GridLevels]           = {"2 .. 4", "", ""};
  before.entries[GridSnapshot::MemoryCPUSolverData] = {"19.55 MiB", "", ""};
  after.entries[GridSnapshot::MemoryCPUSolverData]  = {"24.10 MiB", "", ""};
  before.entries[GridSnapshot::MemoryFree]          = {"4.29 GiB", "", " of 15.56 GiB"};
  after.entries[GridSnapshot::MemoryFree]           = {"4.10 GiB", "", " of 15.56 GiB"};
  const auto lines                                  = exahype2::runner::describeGridTransition(before, after);
  validateEquals(lines.size(), std::size_t{6});
  validateEquals(lines[1], std::string("    Cells            81 → 633  (Refined 79)"));
  validateEquals(lines[2], std::string("    Levels           2 .. 4"));
  validateEquals(lines[4], std::string("    CPU Solver Data  19.55 MiB → 24.10 MiB"));
  validateEquals(lines[5], std::string("    Free             4.29 GiB → 4.10 GiB of 15.56 GiB"));
}

/**
 * The free memory continues the line of the used memory below the grid and the solver memory.
 */
void
exahype2::runner::tests::StatisticsTest::testUsedAndFreeMemoryFormOneLine() {
  using GridSnapshot = exahype2::runner::GridSnapshot;
  GridSnapshot before;
  GridSnapshot after;
  before.entries[GridSnapshot::MemoryCPUGrid] = {"3.04 MiB", "", ""};
  after.entries[GridSnapshot::MemoryCPUGrid]  = {"3.04 MiB", "", ""};
  before.entries[GridSnapshot::MemoryUsed]    = {"75.56 MiB", "", ""};
  after.entries[GridSnapshot::MemoryUsed]     = {"109.91 MiB", "", ""};
  before.entries[GridSnapshot::MemoryFree]    = {"13.45 GiB", "", " of 15.56 GiB"};
  after.entries[GridSnapshot::MemoryFree]     = {"13.42 GiB", "", " of 15.56 GiB"};

  const auto snapshot = exahype2::runner::describeGridSnapshot(before);
  validateEquals(snapshot.size(), std::size_t{3});
  validateEquals(snapshot[1], std::string("    CPU Grid  3.04 MiB"));
  validateEquals(snapshot[2], std::string("    Used      75.56 MiB · Free 13.45 GiB of 15.56 GiB"));

  const auto transition = exahype2::runner::describeGridTransition(before, after);
  validateEquals(transition.size(), std::size_t{3});
  validateEquals(
    transition[2],
    std::string("    Used      75.56 MiB → 109.91 MiB · Free 13.45 GiB → 13.42 GiB of 15.56 GiB")
  );
}

void
exahype2::runner::tests::StatisticsTest::testValueRangeAndCountCarryTheirKind() {
  const auto value = exahype2::runner::makeValueStatistic("dt", 2.0);
  validateEquals(value.label, std::string("dt"));
  validateTrue(value.kind == exahype2::runner::SolverStatistic::Kind::Value);
  validateNumericalEquals(value.minimum, 2.0);
  validateNumericalEquals(value.maximum, 2.0);

  const auto range = exahype2::runner::makeRangeStatistic("h", 1.0, 4.0);
  validateTrue(range.kind == exahype2::runner::SolverStatistic::Kind::Value);
  validateNumericalEquals(range.minimum, 1.0);
  validateNumericalEquals(range.maximum, 4.0);

  const auto count = exahype2::runner::makeCountStatistic("Troubled", 12L);
  validateTrue(count.kind == exahype2::runner::SolverStatistic::Kind::Count);
  validateEquals(count.count, 12L);
}

void
exahype2::runner::tests::StatisticsTest::testASolverWithoutEigenvalueReportsStepAndMeshWidth() {
  const PlainSolver solver;
  const auto        statistics = exahype2::runner::makeSolverStatistics("Plain", solver, appendNothing);
  validateEquals(statistics.first, std::string("Plain"));
  validateEquals(statistics.second.size(), 2U);
  validateEquals(statistics.second[0].label, std::string("dt"));
  validateNumericalEquals(statistics.second[0].minimum, 0.25);
  validateEquals(statistics.second[1].label, std::string("h"));
  validateNumericalEquals(statistics.second[1].minimum, 0.5);
  validateNumericalEquals(statistics.second[1].maximum, 1.5);
}

void
exahype2::runner::tests::StatisticsTest::testASolverWithEigenvalueAndTroubledCellsAppendsBoth() {
  LimitedSolver solver;
  const auto    calm = exahype2::runner::makeSolverStatistics("Limited", solver, appendNothing);
  validateEquals(calm.second.size(), 4U);
  validateEquals(calm.second[2].label, std::string("λ_max"));
  validateNumericalEquals(calm.second[2].maximum, 7.0);
  validateEquals(calm.second[3].label, std::string("Troubled"));
  validateEquals(calm.second[3].count, 0L);

  solver.troubledCells = 5;
  const auto troubled  = exahype2::runner::makeSolverStatistics("Limited", solver, appendNothing);
  validateEquals(troubled.second.size(), 4U);
  validateEquals(troubled.second[3].label, std::string("Troubled"));
  validateEquals(troubled.second[3].count, 5L);
}

/**
 * A solver without a limiter reports no troubled cells, although it offers the count.
 */
void
exahype2::runner::tests::StatisticsTest::testASolverWithoutLimiterReportsNoTroubledCells() {
  const UnlimitedSolver solver;
  const auto            statistics = exahype2::runner::makeSolverStatistics("Unlimited", solver, appendNothing);
  validateEquals(statistics.second.size(), 3U);
  validateEquals(statistics.second[2].label, std::string("λ_max"));
}

void
exahype2::runner::tests::StatisticsTest::testAdditionalStatisticsFollowTheMeshWidth() {
  const PlainSolver solver;
  const auto        statistics = exahype2::runner::makeSolverStatistics(
    "Plain",
    solver,
    [](std::vector<exahype2::runner::SolverStatistic>& additionalStatistics) {
      additionalStatistics.push_back(exahype2::runner::makeCountStatistic("Tracers", 3L));
    }
  );
  validateEquals(statistics.second.size(), 3U);
  validateEquals(statistics.second[2].label, std::string("Tracers"));
  validateEquals(statistics.second[2].count, 3L);
}
