// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorMeasurementTest.h"

#include "exahype2/ErrorMeasurement.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/database/SnapshotDatabase.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  struct LinearSolver {
    static INLINE_METHOD GPU_CALLABLE_METHOD void analyticalSolution(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const double                                 timeStamp,
      const double                                 timeStepSize,
      const double* const                          state,
      double* const                                solution
    ) {
      solution[0] = state[0] + timeStamp + timeStepSize;
      solution[1] = x(0);
    }
  };

  TARGET_DECLARE
  GPU_CALLABLE_METHOD double getErrorOfCell(const int cell) { return 0.5 * (cell + 1); }

  struct WriteErrorsOfCell final {
    double* errors{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      const double error = getErrorOfCell(cell);
      const int    first = 3 * cell;
      errors[first]      = error;
      errors[first + 1]  = error * error;
      errors[first + 2]  = error;
    }
  };
  TARGET_DECLARE_END

  /**
   * Deletes the time series file. Only the global master writes this file.
   */
  void deleteTimeSeriesFile(const std::string& fileName) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      std::filesystem::remove(fileName + ".csv");
    }
  }
} // namespace

exahype2::tests::ErrorMeasurementTest::ErrorMeasurementTest() :
  tarch::tests::TestCase("exahype2::tests::ErrorMeasurementTest") {}

void
exahype2::tests::ErrorMeasurementTest::run() {
  testMethod(testRecordSumsTheIntegralNormsAndTakesTheMaximumAcrossRanks);
  testMethod(testDeviceSumsMatchTheHostSumsWhateverTheCellOrder);
  testMethod(testRecordIsExactAcrossRankPartitions);
  testMethod(testAccumulationWeightsTheMeasuredUnknownsOnly);
  testMethod(testSolutionErrorComparesAgainstTheAnalyticalSolutionAtTheDegreeOfFreedom);
  testMethod(testEveryRankFindsASnapshotDueForTheNewestTimeStampAndAfterTheTimeDelta);
  testMethod(testEveryLaterTimeStampIsDueBeforeTheTimeDelta);
  testMethod(testTheOwnedBufferSumsTheErrorsOfEveryCell);
}

void
exahype2::tests::ErrorMeasurementTest::testEveryRankFindsASnapshotDueForTheNewestTimeStampAndAfterTheTimeDelta() {
  exahype2::ErrorMeasurement measurement;
  validateEquals(measurement.isSnapshotDue(0.0), false);

  const std::string fileName = (tarch::tests::getWritableTestDirectory() / "ErrorMeasurementDueTest").string();
  measurement.open(fileName, 1.0);
  validateEquals(measurement.isSnapshotDue(0.5), true);
  [[maybe_unused]] const auto norms = measurement.record(std::vector<double>{}, 0.5);

  validateEquals(measurement.isSnapshotDue(0.5), true);
  validateEquals(measurement.isSnapshotDue(0.9), false);
  validateEquals(measurement.isSnapshotDue(1.5), true);
  deleteTimeSeriesFile(fileName);
}

void
exahype2::tests::ErrorMeasurementTest::testEveryLaterTimeStampIsDueBeforeTheTimeDelta() {
  exahype2::ErrorMeasurement measurement;
  const std::string fileName = (tarch::tests::getWritableTestDirectory() / "ErrorMeasurementLaterTest").string();
  measurement.open(fileName, 1.0);
  [[maybe_unused]] const auto norms = measurement.record(exahype2::ErrorSums{}, 0.5);
  validateEquals(measurement.isSnapshotDue(0.75), false);

  measurement.setSnapshotRule(tarch::database::SnapshotRule::EveryLaterTimestamp);
  validateEquals(measurement.isSnapshotDue(0.75), true);
  validateEquals(measurement.isSnapshotDue(0.5), false);
  deleteTimeSeriesFile(fileName);
}

/**
 * Redistributing contributions smaller than one rounding unit over the ranks preserves the norms bitwise.
 */
void
exahype2::tests::ErrorMeasurementTest::testRecordIsExactAcrossRankPartitions() {
  const int                  rank          = tarch::mpi::Rank::getInstance().getRank();
  const int                  numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  exahype2::ErrorMeasurement measurement;
  const std::string fileName = (tarch::tests::getWritableTestDirectory() / "ErrorMeasurementPartitionTest").string();
  measurement.open(fileName, 0.0);
  const std::vector<double> errors{1.0, 4.0, 1.0, 0x1p-53, 0x1p-50, 0.0, 0x1p-53, 0x1p-50, 0.0};

  const auto together = measurement.record(rank == 0 ? errors : std::vector<double>{}, 0.0);

  const int           lastOwner = numberOfRanks > 1 ? 1 : 0;
  std::vector<double> partitionedErrors;
  if (rank == 0) {
    partitionedErrors.insert(partitionedErrors.end(), errors.begin(), errors.begin() + 6);
  }
  if (rank == lastOwner) {
    partitionedErrors.insert(partitionedErrors.end(), errors.begin() + 6, errors.end());
  }
  const auto partitioned = measurement.record(partitionedErrors, 1.0);

  validateEquals(together[0], 1.0 + 0x1p-52);
  for (int norm = 0; norm < exahype2::ErrorMeasurement::NumberOfNorms; norm++) {
    validateEquals(together[norm], partitioned[norm]);
  }
  deleteTimeSeriesFile(fileName);
}

void
exahype2::tests::ErrorMeasurementTest::testSolutionErrorComparesAgainstTheAnalyticalSolutionAtTheDegreeOfFreedom() {
  constexpr std::array<int, 2> MeasuredUnknowns{0, 1};
  const double                 value[] = {1.0, 2.0, 99.0};
  double                       errors[3]{};

  tarch::la::Vector<DIMENSIONS, double> x(0.0);
  x(0) = 4.0;

  exahype2::accumulateSolutionError<LinearSolver, 2, 1>(MeasuredUnknowns, value, x, 0.4, 0.1, 2.0, errors);

  validateNumericalEquals(errors[0], 2.0 * 0.5 + 2.0 * 2.0);
  validateNumericalEquals(errors[1], 2.0 * 0.25 + 2.0 * 4.0);
  validateNumericalEquals(errors[2], 2.0);
}

void
exahype2::tests::ErrorMeasurementTest::testAccumulationWeightsTheMeasuredUnknownsOnly() {
  constexpr std::array<int, 2> MeasuredUnknowns{0, 2};
  const float                  value[]         = {1.0F, 100.0F, 4.0F};
  const double                 exactSolution[] = {3.0, 0.0, 3.0};
  double                       errors[3]       = {0.5, 0.25, 1.5};

  exahype2::accumulateError(MeasuredUnknowns, value, exactSolution, 0.5, errors);

  validateNumericalEquals(errors[0], 0.5 + 0.5 * 2.0 + 0.5 * 1.0);
  validateNumericalEquals(errors[1], 0.25 + 0.5 * 4.0 + 0.5 * 1.0);
  validateNumericalEquals(errors[2], 2.0);
}

void
exahype2::tests::ErrorMeasurementTest::testRecordSumsTheIntegralNormsAndTakesTheMaximumAcrossRanks() {
  const auto rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
  const auto numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());

  exahype2::ErrorMeasurement measurement;
  const std::string          fileName = (tarch::tests::getWritableTestDirectory() / "ErrorMeasurementTest").string();
  measurement.open(fileName, 0.0);
  const std::vector<double> errorsPerCell{1.0, 4.0, rank, 2.0, 5.0, 0.5};

  const auto norms = measurement.record(errorsPerCell, 0.25);
  validateNumericalEquals(norms[0], 3.0 * numberOfRanks);
  validateNumericalEquals(norms[1], std::sqrt(9.0 * numberOfRanks));
  validateNumericalEquals(norms[2], std::max(numberOfRanks - 1.0, 0.5));

  const auto emptyNorms = measurement.record(std::vector<double>{}, 0.5);
  validateNumericalEquals(emptyNorms[0], 0.0);
  validateNumericalEquals(emptyNorms[1], 0.0);
  validateNumericalEquals(emptyNorms[2], 0.0);
  deleteTimeSeriesFile(fileName);
}

void
exahype2::tests::ErrorMeasurementTest::testDeviceSumsMatchTheHostSumsWhateverTheCellOrder() {
  constexpr int       NumberOfCells = 3000;
  std::vector<double> ascending;
  ascending.reserve(static_cast<std::size_t>(NumberOfCells) * 3);
  exahype2::LargestErrors largestErrors;
  double                  sumInIndexOrder = 0.0;
  for (int cell = 0; cell < NumberOfCells; cell++) {
    const double error = std::pow(10.0, -static_cast<double>(cell % 9)) * (1.0 + cell);
    ascending.push_back(error);
    ascending.push_back(error * error);
    ascending.push_back(error);
    largestErrors.error        = std::max(largestErrors.error, error);
    largestErrors.squaredError = std::max(largestErrors.squaredError, error * error);
    sumInIndexOrder += error;
  }
  const exahype2::ErrorSums emptySums = exahype2::ErrorMeasurement::getEmptySumsOfAllRanks(largestErrors);
  exahype2::ErrorSums       hostSums  = emptySums;
  for (int cell = 0; cell < NumberOfCells; cell++) {
    hostSums.add(ascending.data() + 3 * static_cast<std::ptrdiff_t>(cell));
  }
  std::vector<double> descending;
  descending.reserve(ascending.size());
  for (int cell = NumberOfCells - 1; cell >= 0; cell--) {
    const auto first = 3 * static_cast<std::ptrdiff_t>(cell);
    descending.insert(descending.end(), ascending.begin() + first, ascending.begin() + first + 3);
  }

  tarch::accelerator::Buffer<double>              errorsOnDevice(ascending.size(), tarch::MemoryLocation::Device);
  tarch::accelerator::Buffer<exahype2::ErrorSums> partialSums(tarch::MemoryLocation::Device);
  errorsOnDevice.setValues(ascending);
  const exahype2::ErrorSums ascendingSums =
    exahype2::ErrorMeasurement::sumErrorsOfCells(errorsOnDevice.getData(), NumberOfCells, emptySums, partialSums);
  errorsOnDevice.setValues(descending);
  const exahype2::ErrorSums descendingSums =
    exahype2::ErrorMeasurement::sumErrorsOfCells(errorsOnDevice.getData(), NumberOfCells, emptySums, partialSums);
  const exahype2::ErrorSums noSums =
    exahype2::ErrorMeasurement::sumErrorsOfCells(errorsOnDevice.getData(), 0, emptySums, partialSums);

  for (const exahype2::ErrorSums& deviceSums : {ascendingSums, descendingSums}) {
    validateEquals(deviceSums.summedError.round(), hostSums.summedError.round());
    validateEquals(deviceSums.summedSquaredError.round(), hostSums.summedSquaredError.round());
    validateEquals(deviceSums.maxError, hostSums.maxError);
  }
  validateNumericalEqualsWithEps(hostSums.summedError.round(), sumInIndexOrder, 1.0e-12 * sumInIndexOrder);
  validateEquals(noSums.summedError.round(), 0.0);
  validateEquals(noSums.maxError, 0.0);
}

void
exahype2::tests::ErrorMeasurementTest::testTheOwnedBufferSumsTheErrorsOfEveryCell() {
  constexpr int              NumberOfCells = 5;
  exahype2::ErrorMeasurement measurement;
  const std::string fileName = (tarch::tests::getWritableTestDirectory() / "ErrorMeasurementBufferTest").string();
  measurement.open(fileName, 0.0);

  std::vector<double> errorsOnHost;
  for (int cell = 0; cell < NumberOfCells; cell++) {
    errorsOnHost.push_back(getErrorOfCell(cell));
    errorsOnHost.push_back(getErrorOfCell(cell) * getErrorOfCell(cell));
    errorsOnHost.push_back(getErrorOfCell(cell));
  }
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(NumberOfCells),
    WriteErrorsOfCell{measurement.getErrorsPerCell(NumberOfCells)}
  );
  validate(measurement.getAllocatedBytes().getTotal() >= static_cast<std::size_t>(NumberOfCells) * 3 * sizeof(double));

  const auto norms    = measurement.record(NumberOfCells, 0.25);
  const auto expected = measurement.record(errorsOnHost, 0.5);
  for (int norm = 0; norm < exahype2::ErrorMeasurement::NumberOfNorms; norm++) {
    validateEqualsWithParams1(norms[static_cast<std::size_t>(norm)], expected[static_cast<std::size_t>(norm)], norm);
  }

  measurement.release();
  validateEquals(measurement.getAllocatedBytes().getTotal(), std::size_t{0});
  deleteTimeSeriesFile(fileName);
}
