// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include "exahype2/Solver.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr double Infinity           = std::numeric_limits<double>::infinity();
  constexpr double InvalidObservation = -1.25;

  std::uint64_t bits(const double value) { return std::bit_cast<std::uint64_t>(value); }

  class Probe final : public exahype2::Solver {
  public:
    using exahype2::Solver::commitTimeStepStatistics;
    using exahype2::Solver::reduceTimeStepStatisticsAcrossRanks;
    using exahype2::Solver::resetTimeStepStatistics;

    [[nodiscard]] double minMeshWidthThisTimeStep() const { return _minMeshWidthThisTimeStep; }
    [[nodiscard]] double maxMeshWidthThisTimeStep() const { return _maxMeshWidthThisTimeStep; }
    [[nodiscard]] bool   hasHangingFacesThisTimeStep() const { return _hasHangingFacesThisTimeStep; }
    [[nodiscard]] bool   meshRefinementRanSinceLastTimeStep() const { return _meshRefinementRanSinceLastTimeStep; }

    void                 startGridConstructionStep() override {}
    void                 finishGridConstructionStep() override {}
    void                 suspendSolversForOneGridSweep() override {}
    void                 startGridInitialisationStep() override {}
    void                 finishGridInitialisationStep() override { _hasStoredFaceData = true; }
    void                 startTimeStep() override {}
    void                 finishTimeStep() override {}
    void                 startPlottingStep() override {}
    void                 finishPlottingStep() override {}
    [[nodiscard]] double getMaxPatchSize() const override { return getMaxCellSize(); }
    [[nodiscard]] double getMinPatchSize() const override { return getMinCellSize(); }
    [[nodiscard]] int    getNumberOfDoFs() const override { return 0; }
    [[nodiscard]] bool   mayPlot() const override { return true; }
    void                 startSimulation() override {}
    void                 finishSimulation() override {}
  };

  exahype2::Solver::LocalUpdates record(
    const double timeStepSize,
    const double timeStamp,
    const double minimumMeshWidth,
    const double maximumMeshWidth,
    const bool   hasHangingFaces
  ) {
    exahype2::Solver::LocalUpdates result;
    result.addTimeStep(timeStepSize);
    result.addTimeStamp(timeStamp);
    result.addMeshWidths(minimumMeshWidth, maximumMeshWidth);
    result.hasHangingFaces = hasHangingFaces;
    return result;
  }

  exahype2::Solver::LocalUpdates invalidRecord(const double observation) {
    exahype2::Solver::LocalUpdates result;
    result.timeStepSizeStatistics = exahype2::NonNegativeStatistics::accumulate({}, observation);
    result.addTimeStamp(1.0);
    result.addMeshWidths(1.0, 1.0);
    return result;
  }
} // namespace

exahype2::tests::SolverTest::SolverTest() :
  tarch::tests::TestCase("exahype2::tests::SolverTest") {}

void exahype2::tests::SolverTest::run() {
  testMethod(testLocalUpdatesTrackTheExtrema);
  testMethod(testMergeTakesTheExtremaOfEveryTaskAndClearsTheRecord);
  testMethod(testAnInvalidTimeStepObservationSurvivesTheMerge);
  testMethod(testRankReductionTakesTheExtremaOfAllRanks);
  testMethod(testCommitAndResetSwitchTheAccessors);
}

void exahype2::tests::SolverTest::testLocalUpdatesTrackTheExtrema() {
  exahype2::Solver::LocalUpdates updates;
  validateEquals(updates.empty, true);
  validateEquals(updates.timeStepSizeStatistics.hasObservation(), false);

  updates.addTimeStep(0.5);
  updates.addTimeStep(0.25);
  updates.addTimeStamp(2.0);
  updates.addTimeStamp(1.0);
  updates.addMeshWidths(0.5, 1.0);
  updates.addMeshWidths(0.25, 0.75);

  validateEquals(updates.empty, false);
  validateNumericalEquals(updates.timeStepSizeStatistics.minimum, 0.25);
  validateNumericalEquals(updates.timeStepSizeStatistics.maximum, 0.5);
  validateNumericalEquals(updates.minTimeStamp, 1.0);
  validateNumericalEquals(updates.maxTimeStamp, 2.0);
  validateNumericalEquals(updates.minMeshWidth, 0.25);
  validateNumericalEquals(updates.maxMeshWidth, 1.0);
  validateEquals(updates.hasHangingFaces, false);
}

void exahype2::tests::SolverTest::testMergeTakesTheExtremaOfEveryTaskAndClearsTheRecord() {
  Probe solver;
  solver.resetTimeStepStatistics();

  auto first  = record(0.5, 1.0, 0.5, 0.5, false);
  auto second = record(0.25, 3.0, 0.25, 1.0, true);
  auto empty  = exahype2::Solver::LocalUpdates{};
  solver.mergeLocalUpdates(first);
  solver.mergeLocalUpdates(second);
  solver.mergeLocalUpdates(empty);

  validateNumericalEquals(solver.getMinTimeStamp(), 1.0);
  validateNumericalEquals(solver.getMaxTimeStamp(), 3.0);
  validateNumericalEquals(solver.getMinTimeStepSize(), 0.25);
  validateNumericalEquals(solver.getMaxTimeStepSize(), 0.5);
  validateNumericalEquals(solver.minMeshWidthThisTimeStep(), 0.25);
  validateNumericalEquals(solver.maxMeshWidthThisTimeStep(), 1.0);
  validateEquals(solver.hasHangingFacesThisTimeStep(), true);

  validateEquals(first.empty, true);
  validateEquals(second.empty, true);
  validateEquals(first.minTimeStamp, Infinity);
  validateEquals(second.hasHangingFaces, false);
}

void exahype2::tests::SolverTest::testAnInvalidTimeStepObservationSurvivesTheMerge() {
  Probe solver;
  solver.resetTimeStepStatistics();

  auto valid   = record(0.5, 1.0, 1.0, 1.0, false);
  auto invalid = invalidRecord(InvalidObservation);
  solver.mergeLocalUpdates(valid);
  solver.mergeLocalUpdates(invalid);
  validateEquals(bits(solver.getMinTimeStepSize()), bits(InvalidObservation));
  validateEquals(bits(solver.getMaxTimeStepSize()), bits(InvalidObservation));

  // A later valid task lifts the maximum, and the minimum keeps the invalid observation
  // for the rank reduction.
  auto later = record(0.5, 1.0, 1.0, 1.0, false);
  solver.mergeLocalUpdates(later);
  validateEquals(bits(solver.getMinTimeStepSize()), bits(InvalidObservation));

  // A later valid task lowers an infinite observation out of the minimum, and the maximum keeps it.
  Probe infiniteFirst;
  infiniteFirst.resetTimeStepStatistics();
  auto infinite = invalidRecord(Infinity);
  auto finite   = record(0.5, 1.0, 1.0, 1.0, false);
  infiniteFirst.mergeLocalUpdates(infinite);
  infiniteFirst.mergeLocalUpdates(finite);
  const auto merged =
    exahype2::NonNegativeStatistics{infiniteFirst.getMinTimeStepSize(), infiniteFirst.getMaxTimeStepSize()};
  validateEquals(merged.invalidObservationBitsOrIdentity(), bits(Infinity));
}

void exahype2::tests::SolverTest::testRankReductionTakesTheExtremaOfAllRanks() {
  const auto rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
  const auto numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());

  Probe solver;
  solver.resetTimeStepStatistics();
  auto contribution = record(1.0 + rank, 10.0 + rank, 0.5 + rank, 2.0 + rank, rank + 1.0 == numberOfRanks);
  solver.mergeLocalUpdates(contribution);
  solver.reduceTimeStepStatisticsAcrossRanks();

  validateNumericalEquals(solver.getMinTimeStamp(), 10.0);
  validateNumericalEquals(solver.getMaxTimeStamp(), 10.0 + numberOfRanks - 1.0);
  validateNumericalEquals(solver.getMinTimeStepSize(), 1.0);
  validateNumericalEquals(solver.getMaxTimeStepSize(), numberOfRanks);
  validateNumericalEquals(solver.minMeshWidthThisTimeStep(), 0.5);
  validateNumericalEquals(solver.maxMeshWidthThisTimeStep(), 2.0 + numberOfRanks - 1.0);
  validateEquals(solver.hasHangingFacesThisTimeStep(), true);

  // One rank's invalid observation reaches every rank as the same bit pattern.
  solver.resetTimeStepStatistics();
  auto secondContribution = rank == 0.0 ? invalidRecord(InvalidObservation) : record(1.0, 1.0, 1.0, 1.0, false);
  solver.mergeLocalUpdates(secondContribution);
  solver.reduceTimeStepStatisticsAcrossRanks();
  validateEquals(bits(solver.getMinTimeStepSize()), bits(InvalidObservation));
  validateEquals(bits(solver.getMaxTimeStepSize()), bits(InvalidObservation));
  validateEquals(solver.hasHangingFacesThisTimeStep(), false);
}

void exahype2::tests::SolverTest::testCommitAndResetSwitchTheAccessors() {
  Probe solver;
  validateEquals(solver.hasStoredFaceData(), false);
  validateEquals(solver.meshRefinementRanSinceLastTimeStep(), false);
  validateNumericalEquals(solver.getMinTimeStamp(), 0.0);
  validateEquals(solver.getMinTimeStepSize(), Infinity);
  validateEquals(solver.getMinCellSize(), Infinity);

  solver.resetTimeStepStatistics();
  auto contribution = record(0.5, 2.0, 0.25, 0.75, false);
  solver.mergeLocalUpdates(contribution);
  validateNumericalEquals(solver.getMinTimeStamp(), 2.0);
  validateNumericalEquals(solver.getMaxTimeStamp(), 2.0);
  validateNumericalEquals(solver.getMinTimeStepSize(), 0.5);
  validateEquals(solver.getMinCellSize(), Infinity);

  solver.commitTimeStepStatistics();
  validateNumericalEquals(solver.getMinCellSize(), 0.25);
  validateNumericalEquals(solver.getMaxCellSize(), 0.75);

  // After the reset the accessors fall back to the committed extrema.
  solver.resetTimeStepStatistics();
  validateNumericalEquals(solver.getMinTimeStamp(), 2.0);
  validateNumericalEquals(solver.getMaxTimeStamp(), 2.0);
  validateNumericalEquals(solver.getMinTimeStepSize(), 0.5);
  validateNumericalEquals(solver.getMaxTimeStepSize(), 0.5);
  validateEquals(solver.hasHangingFacesThisTimeStep(), false);

  solver.startDynamicMeshRefinementStep();
  validateEquals(solver.meshRefinementRanSinceLastTimeStep(), true);
  solver.finishGridInitialisationStep();
  validateEquals(solver.hasStoredFaceData(), true);
}
