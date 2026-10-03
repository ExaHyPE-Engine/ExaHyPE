// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeSteppingTest.h"

#include "exahype2/Limits.h"
#include "exahype2/TimeStepping.h"
#include "tarch/mpi/Rank.h"

namespace {
  using Rule      = exahype2::TimeStepping::Rule;
  using Stability = exahype2::TimeStepping::Stability;

  // Every ordered comparison against a NaN raises the invalid-operation exception, which the
  // test runner traps, so the faults below are a negative and an infinite wave speed.
  constexpr double NegativeWaveSpeed = -1.25;
  constexpr double InfiniteWaveSpeed = std::numeric_limits<double>::infinity();

  constexpr std::array PhysicalWaveSpeeds{-0.0, 0.0, std::numeric_limits<double>::denorm_min(), 0.5, 1.0, 2.0};

  std::uint64_t getBitsOf(const double value) { return std::bit_cast<std::uint64_t>(value); }

  exahype2::TimeStepping makeAdaptive(const Stability stability) {
    return exahype2::TimeStepping{Rule::Adaptive, 1.0, stability};
  }

  void placeBeforeTheTerminalTime(exahype2::TimeStepping& timeStepping, const double terminalTime) {
    exahype2::Limits limits;
    limits.terminalTime = terminalTime;
    timeStepping.setLimits(limits);
    timeStepping.advanceTimeStamp(1.0);
  }
} // namespace

exahype2::tests::TimeSteppingTest::TimeSteppingTest() :
  tarch::tests::TestCase("exahype2::tests::TimeSteppingTest") {}

void
exahype2::tests::TimeSteppingTest::run() {
  testMethod(testTheReductionTakesTheLargerPhysicalWaveSpeedBitwise);
  testMethod(testTheReductionKeepsAFaultyWaveSpeed);
  testMethod(testANewSolverStandsAtTimeZeroWithoutMeshWidths);
  testMethod(testTheLimitsReadBackAsSet);
  testMethod(testAdvancingTheTimeStampRecordsTheStepSize);
  testMethod(testTheMeshWidthsReduceOverEveryRank);
  testMethod(testMaxEigenvaluesKeepTheLargestPositiveValue);
  testMethod(testStartTimeStepCommitsTheProposalAndClearsTheMaxima);
  testMethod(testRankReductionTakesTheMaximaOfAllRanks);
  testMethod(testStabilityEigenvalueIsThePhysicalOrTheScaledMaximum);
  testMethod(testProposalStopsAtTheTerminalTime);
  testMethod(testAFixedRuleKeepsItsSizeAcrossSteps);
}

/**
 * The signed zeroes have to agree as well, so the comparison is bitwise.
 */
void
exahype2::tests::TimeSteppingTest::testTheReductionTakesTheLargerPhysicalWaveSpeedBitwise() {
  validateEquals(getBitsOf(exahype2::MaxEigenvalueReduction::identity()), getBitsOf(0.0));
  for (const auto left : PhysicalWaveSpeeds) {
    validate(exahype2::MaxEigenvalueReduction::isPhysicalValue(left));
    for (const auto right : PhysicalWaveSpeeds) {
      validateEqualsWithParams2(
        getBitsOf(exahype2::MaxEigenvalueReduction::combine(left, right)),
        getBitsOf(std::max(left, right)),
        left,
        right
      );
    }
  }
}

/**
 * A fault survives in either operand position and in every later reduction of the result.
 */
void
exahype2::tests::TimeSteppingTest::testTheReductionKeepsAFaultyWaveSpeed() {
  for (const auto fault : {NegativeWaveSpeed, InfiniteWaveSpeed}) {
    validateWithParams1(not exahype2::MaxEigenvalueReduction::isPhysicalValue(fault), fault);
    const double asRight = exahype2::MaxEigenvalueReduction::combine(1.0, fault);
    const double asLeft  = exahype2::MaxEigenvalueReduction::combine(fault, 1.0);
    validateEqualsWithParams1(getBitsOf(asRight), getBitsOf(fault), fault);
    validateEqualsWithParams1(getBitsOf(asLeft), getBitsOf(fault), fault);
    validateEqualsWithParams1(
      getBitsOf(exahype2::MaxEigenvalueReduction::combine(asRight, 2.0)),
      getBitsOf(fault),
      fault
    );
    validateEqualsWithParams1(
      getBitsOf(exahype2::MaxEigenvalueReduction::combine(exahype2::MaxEigenvalueReduction::identity(), fault)),
      getBitsOf(fault),
      fault
    );
  }
}

void
exahype2::tests::TimeSteppingTest::testANewSolverStandsAtTimeZeroWithoutMeshWidths() {
  const exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);
  validateNumericalEquals(timeStepping.getTimeStamp(), 0.0);
  validateNumericalEquals(timeStepping.getTimeStepSize(), 0.0);
  validateNumericalEquals(timeStepping.getTimeStepSizeOfLastStep(), 0.0);
  validate(std::isinf(timeStepping.getMinMeshWidth()));
  validateNumericalEquals(timeStepping.getMaxMeshWidth(), 0.0);
  validateNumericalEquals(timeStepping.getFactor(), 1.0);
  validateEquals(timeStepping.getLimits().terminalTime, std::numeric_limits<double>::max());
}

void
exahype2::tests::TimeSteppingTest::testTheLimitsReadBackAsSet() {
  exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);
  exahype2::Limits       limits;
  limits.terminalTime           = 2.0;
  limits.minAdmissiblePatchSize = 0.125;
  limits.maxAdmissiblePatchSize = 0.5;
  limits.maxAdmissibleMeshWidth = 0.25;
  timeStepping.setLimits(limits);
  validateNumericalEquals(timeStepping.getLimits().terminalTime, 2.0);
  validateNumericalEquals(timeStepping.getLimits().minAdmissiblePatchSize, 0.125);
  validateNumericalEquals(timeStepping.getLimits().maxAdmissiblePatchSize, 0.5);
  validateNumericalEquals(timeStepping.getLimits().maxAdmissibleMeshWidth, 0.25);
}

void
exahype2::tests::TimeSteppingTest::testAdvancingTheTimeStampRecordsTheStepSize() {
  exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);
  timeStepping.advanceTimeStamp(0.25);
  timeStepping.advanceTimeStamp(0.125);
  validateNumericalEquals(timeStepping.getTimeStamp(), 0.375);
  validateNumericalEquals(timeStepping.getTimeStepSizeOfLastStep(), 0.125);
}

void
exahype2::tests::TimeSteppingTest::testTheMeshWidthsReduceOverEveryRank() {
  const auto             rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
  const auto             numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
  exahype2::TimeStepping timeStepping  = makeAdaptive(Stability::PhysicalEigenvalue);
  timeStepping.reduceMeshWidthsAcrossRanks(1.0 + rank, 10.0 + rank);
  validateNumericalEquals(timeStepping.getMinMeshWidth(), 1.0);
  validateNumericalEquals(timeStepping.getMaxMeshWidth(), 9.0 + numberOfRanks);
}

void
exahype2::tests::TimeSteppingTest::testMaxEigenvaluesKeepTheLargestPositiveValue() {
  exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);
  validateNumericalEquals(timeStepping.getMaxEigenvalue(), 0.0);
  constexpr double SmallEigenvalue = 1e-15;
  timeStepping.raiseMaxEigenvalue(SmallEigenvalue);
  timeStepping.raiseMaxScaledEigenvalue(SmallEigenvalue);
  validateEquals(timeStepping.getMaxEigenvalueOfThisStep(), SmallEigenvalue);
  validateEquals(timeStepping.getMaxScaledEigenvalueOfThisStep(), SmallEigenvalue);
  validateEquals(timeStepping.hasPositiveStabilityEigenvalue(), true);
  timeStepping.raiseMaxEigenvalue(0.5);
  timeStepping.raiseMaxEigenvalue(2.0);
  timeStepping.raiseMaxEigenvalue(1.0);
  timeStepping.raiseMaxEigenvalue(0.0);
  timeStepping.raiseMaxScaledEigenvalue(3.0);
  timeStepping.raiseMaxScaledEigenvalue(4.0);
  validateNumericalEquals(timeStepping.getMaxEigenvalueOfThisStep(), 2.0);
  validateNumericalEquals(timeStepping.getMaxScaledEigenvalueOfThisStep(), 4.0);
  validateNumericalEquals(timeStepping.getMaxEigenvalue(), 0.0);
}

void
exahype2::tests::TimeSteppingTest::testStartTimeStepCommitsTheProposalAndClearsTheMaxima() {
  exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);
  placeBeforeTheTerminalTime(timeStepping, 10.0);
  timeStepping.raiseMaxEigenvalue(2.0);
  timeStepping.raiseMaxScaledEigenvalue(3.0);
  timeStepping.setAdmissibleTimeStepSize(0.5);
  validateNumericalEquals(timeStepping.getTimeStepSize(), 0.0);
  validateNumericalEquals(timeStepping.getTimeStepSizeOfNextStep(), 0.5);
  validateNumericalEquals(timeStepping.startTimeStep(), 0.5);
  validateNumericalEquals(timeStepping.getTimeStepSize(), 0.5);
  validateNumericalEquals(timeStepping.getMaxEigenvalueOfThisStep(), 0.0);
  validateNumericalEquals(timeStepping.getMaxScaledEigenvalueOfThisStep(), 0.0);
}

void
exahype2::tests::TimeSteppingTest::testRankReductionTakesTheMaximaOfAllRanks() {
  const auto             rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
  const auto             numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
  exahype2::TimeStepping timeStepping  = makeAdaptive(Stability::ScaledEigenvalue);
  timeStepping.raiseMaxEigenvalue(1.0 + rank);
  timeStepping.raiseMaxScaledEigenvalue(10.0 + rank);
  timeStepping.reduceMaxEigenvaluesAcrossRanks();
  validateNumericalEquals(timeStepping.getMaxEigenvalue(), numberOfRanks);
  validateNumericalEquals(timeStepping.getMaxEigenvalueOfThisStep(), numberOfRanks);
  validateNumericalEquals(timeStepping.getMaxScaledEigenvalueOfThisStep(), 9.0 + numberOfRanks);
}

void
exahype2::tests::TimeSteppingTest::testStabilityEigenvalueIsThePhysicalOrTheScaledMaximum() {
  exahype2::TimeStepping physical = makeAdaptive(Stability::PhysicalEigenvalue);
  physical.raiseMaxScaledEigenvalue(1.0);
  validateEquals(physical.hasPositiveStabilityEigenvalue(), false);
  physical.raiseMaxEigenvalue(1.0);
  validateEquals(physical.hasPositiveStabilityEigenvalue(), true);

  exahype2::TimeStepping scaled = makeAdaptive(Stability::ScaledEigenvalue);
  scaled.raiseMaxEigenvalue(1.0);
  validateEquals(scaled.hasPositiveStabilityEigenvalue(), false);
  scaled.raiseMaxScaledEigenvalue(1.0);
  validateEquals(scaled.hasPositiveStabilityEigenvalue(), true);
}

void
exahype2::tests::TimeSteppingTest::testProposalStopsAtTheTerminalTime() {
  exahype2::TimeStepping nearTheEnd = makeAdaptive(Stability::PhysicalEigenvalue);
  placeBeforeTheTerminalTime(nearTheEnd, 1.25);
  nearTheEnd.setAdmissibleTimeStepSize(0.5);
  validateNumericalEquals(nearTheEnd.startTimeStep(), 0.25);

  exahype2::TimeStepping farFromTheEnd = makeAdaptive(Stability::PhysicalEigenvalue);
  placeBeforeTheTerminalTime(farFromTheEnd, 3.0);
  farFromTheEnd.setAdmissibleTimeStepSize(0.5);
  validateNumericalEquals(farFromTheEnd.startTimeStep(), 0.5);
}

void
exahype2::tests::TimeSteppingTest::testAFixedRuleKeepsItsSizeAcrossSteps() {
  exahype2::TimeStepping fixed{Rule::Fixed, 0.5, Stability::PhysicalEigenvalue};
  validateNumericalEquals(fixed.getTimeStepSize(), 0.0);
  fixed.setFixedTimeStepSize(0.25);
  validateNumericalEquals(fixed.getTimeStepSize(), 0.25);
  validateNumericalEquals(fixed.getTimeStepSizeOfNextStep(), 0.25);
  validateNumericalEquals(fixed.startTimeStep(), 0.25);
  validateNumericalEquals(fixed.startTimeStep(), 0.25);
}
