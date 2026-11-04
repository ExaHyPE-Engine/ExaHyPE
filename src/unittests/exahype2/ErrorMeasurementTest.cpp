// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/ErrorMeasurement.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/database/SnapshotDatabase.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/testing/UnitTest.h"

namespace {
  struct LinearSolver final {
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

  std::string getTimeSeriesFileName(const std::string& name) {
    return (tarch::testing::getWritableTestDirectory() / name).string();
  }

  /**
   * Deletes the time series file that only the global master writes.
   */
  void deleteTimeSeriesFile(const std::string& fileName) {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      std::filesystem::remove(fileName + ".csv");
    }
  }
} // namespace

UNIT_TEST("A snapshot is due for the newest time stamp and one time delta after it") {
  GIVEN("A measurement and the name of its time series file") {
    exahype2::ErrorMeasurement measurement;
    const std::string          fileName = getTimeSeriesFileName("ErrorMeasurementDueTest");

    WHEN("The measurement answers whether a snapshot is due before it opens") {
      const bool isDue = measurement.isSnapshotDue(0.0);

      THEN("isSnapshotDue() returns false") { VALIDATE_EQUALS(isDue, false); }
    }

    WHEN("The measurement opens with the time delta one and records the time stamp one half") {
      measurement.open(fileName, 1.0);
      const bool                  isDueBeforeTheRecord = measurement.isSnapshotDue(0.5);
      [[maybe_unused]] const auto norms                = measurement.record(std::vector<double>{}, 0.5);
      const bool                  isDueAtTheRecord     = measurement.isSnapshotDue(0.5);
      const bool                  isDueWithinTheDelta  = measurement.isSnapshotDue(0.9);
      const bool                  isDueAfterTheDelta   = measurement.isSnapshotDue(1.5);

      THEN("A snapshot is due before and at the record and one time delta later but not in between") {
        VALIDATE_EQUALS(isDueBeforeTheRecord, true);
        VALIDATE_EQUALS(isDueAtTheRecord, true);
        VALIDATE_EQUALS(isDueWithinTheDelta, false);
        VALIDATE_EQUALS(isDueAfterTheDelta, true);
      }
      deleteTimeSeriesFile(fileName);
    }
  }
}

UNIT_TEST("Under the rule EveryLaterTimestamp a snapshot is due for every later time stamp within the time delta") {
  GIVEN("A measurement that opens with the time delta one and records the time stamp one half") {
    exahype2::ErrorMeasurement measurement;
    const std::string          fileName = getTimeSeriesFileName("ErrorMeasurementLaterTest");
    measurement.open(fileName, 1.0);
    [[maybe_unused]] const auto norms               = measurement.record(std::vector<double>{}, 0.5);
    const bool                  isDueWithinTheDelta = measurement.isSnapshotDue(0.75);

    WHEN("The measurement switches to the rule EveryLaterTimestamp") {
      measurement.setSnapshotRule(tarch::database::SnapshotRule::EveryLaterTimestamp);
      const bool isDueAtALaterTimeStamp      = measurement.isSnapshotDue(0.75);
      const bool isDueAtTheRecordedTimeStamp = measurement.isSnapshotDue(0.5);

      THEN("A snapshot is due at the later time stamp within the time delta but not at the recorded one") {
        VALIDATE_EQUALS(isDueWithinTheDelta, false);
        VALIDATE_EQUALS(isDueAtALaterTimeStamp, true);
        VALIDATE_EQUALS(isDueAtTheRecordedTimeStamp, false);
      }
    }
    deleteTimeSeriesFile(fileName);
  }
}

UNIT_TEST("Redistributing contributions below one rounding unit over the ranks preserves the norms bitwise") {
  GIVEN("An open measurement and three cells, two of which contribute less than one rounding unit of the first") {
    const int                  rank          = tarch::mpi::Rank::getInstance().getRank();
    const int                  numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const int                  lastOwner     = numberOfRanks > 1 ? 1 : 0;
    const std::string          fileName      = getTimeSeriesFileName("ErrorMeasurementPartitionTest");
    exahype2::ErrorMeasurement measurement;
    measurement.open(fileName, 0.0);
    const std::vector<double> errors{1.0, 4.0, 1.0, 0x1p-53, 0x1p-50, 0.0, 0x1p-53, 0x1p-50, 0.0};

    WHEN("Rank zero records the three cells and then rank zero and another rank record them split") {
      const auto together = measurement.record(rank == 0 ? errors : std::vector<double>{}, 0.0);

      std::vector<double> partitionedErrors;
      if (rank == 0) {
        partitionedErrors.insert(partitionedErrors.end(), errors.begin(), errors.begin() + 6);
      }
      if (rank == lastOwner) {
        partitionedErrors.insert(partitionedErrors.end(), errors.begin() + 6, errors.end());
      }
      const auto partitioned = measurement.record(partitionedErrors, 1.0);

      THEN("The L1 norm keeps both small contributions and both records give the same norms bitwise") {
        VALIDATE_EQUALS(together[0], 1.0 + 0x1p-52);
        for (int norm = 0; norm < exahype2::ErrorMeasurement::NumberOfNorms; norm++) {
          VALIDATE_EQUALS(together[norm], partitioned[norm], norm);
        }
      }
    }
    deleteTimeSeriesFile(fileName);
  }
}

UNIT_TEST("The solution error compares against the analytical solution at the degree of freedom") {
  GIVEN("A state of two unknowns and one auxiliary variable at the coordinate four along the first axis") {
    constexpr std::array<int, 2> MeasuredUnknowns{0, 1};
    const double                 value[] = {1.0, 2.0, 99.0};

    tarch::la::Vector<DIMENSIONS, double> x(0.0);
    x(0) = 4.0;

    WHEN("The error against the analytical solution at the time 0.4 and the step 0.1 accumulates with weight two") {
      double errors[3]{};
      exahype2::accumulateSolutionError<LinearSolver, 2, 1>(MeasuredUnknowns, value, x, 0.4, 0.1, 2.0, errors);

      THEN("The weighted L1 and squared L2 errors and the largest error of both unknowns reach the accumulator") {
        VALIDATE_NUMERICAL_EQUALS(errors[0], 2.0 * 0.5 + 2.0 * 2.0);
        VALIDATE_NUMERICAL_EQUALS(errors[1], 2.0 * 0.25 + 2.0 * 4.0);
        VALIDATE_NUMERICAL_EQUALS(errors[2], 2.0);
      }
    }
  }
}

UNIT_TEST("The error accumulation weights the measured unknowns and skips every other one") {
  GIVEN("A single-precision state of three values, its exact solution and accumulated errors") {
    constexpr std::array<int, 2> MeasuredUnknowns{0, 2};
    const float                  value[]         = {1.0F, 100.0F, 4.0F};
    const double                 exactSolution[] = {3.0, 0.0, 3.0};
    double                       errors[3]       = {0.5, 0.25, 1.5};

    WHEN("The errors of the unknowns zero and two accumulate with the weight one half") {
      exahype2::accumulateError(MeasuredUnknowns, value, exactSolution, 0.5, errors);

      THEN("The errors grow by the weighted errors of the unknowns zero and two and the largest error is two") {
        VALIDATE_NUMERICAL_EQUALS(errors[0], 0.5 + 0.5 * 2.0 + 0.5 * 1.0);
        VALIDATE_NUMERICAL_EQUALS(errors[1], 0.25 + 0.5 * 4.0 + 0.5 * 1.0);
        VALIDATE_NUMERICAL_EQUALS(errors[2], 2.0);
      }
    }
  }
}

UNIT_TEST("A record sums the integral norms over the ranks and takes the largest error of all ranks") {
  GIVEN("An open measurement and two cells on every rank, the first of which has the rank as its largest error") {
    const auto                 rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
    const auto                 numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
    const std::string          fileName      = getTimeSeriesFileName("ErrorMeasurementTest");
    exahype2::ErrorMeasurement measurement;
    measurement.open(fileName, 0.0);
    const std::vector<double> errorsPerCell{1.0, 4.0, rank, 2.0, 5.0, 0.5};

    WHEN("The measurement records the errors of the cells") {
      const auto norms = measurement.record(errorsPerCell, 0.25);

      THEN("The L1 and L2 norms sum over the ranks and the Linf norm is the largest error of all ranks") {
        VALIDATE_NUMERICAL_EQUALS(norms[0], 3.0 * numberOfRanks);
        VALIDATE_NUMERICAL_EQUALS(norms[1], std::sqrt(9.0 * numberOfRanks));
        VALIDATE_NUMERICAL_EQUALS(norms[2], std::max(numberOfRanks - 1.0, 0.5));
      }
    }

    WHEN("The measurement records zero cells") {
      const auto emptyNorms = measurement.record(std::vector<double>{}, 0.5);

      THEN("Every norm is zero") {
        VALIDATE_NUMERICAL_EQUALS(emptyNorms[0], 0.0);
        VALIDATE_NUMERICAL_EQUALS(emptyNorms[1], 0.0);
        VALIDATE_NUMERICAL_EQUALS(emptyNorms[2], 0.0);
      }
    }
    deleteTimeSeriesFile(fileName);
  }
}

UNIT_TEST("The device sums of the cell errors give bitwise the norms of the host sums in either cell order") {
  GIVEN("The errors of 3000 cells over nine orders of magnitude and a measurement that recorded them on the host") {
    constexpr int       NumberOfCells = 3000;
    const auto          numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
    std::vector<double> ascending;
    ascending.reserve(static_cast<std::size_t>(NumberOfCells) * 3);
    for (int cell = 0; cell < NumberOfCells; cell++) {
      const double error = std::pow(10.0, -static_cast<double>(cell % 9)) * (1.0 + cell);
      ascending.push_back(error);
      ascending.push_back(error * error);
      ascending.push_back(error);
    }
    std::vector<double> descending;
    descending.reserve(ascending.size());
    for (int cell = NumberOfCells - 1; cell >= 0; cell--) {
      const auto first = 3 * static_cast<std::ptrdiff_t>(cell);
      descending.insert(descending.end(), ascending.begin() + first, ascending.begin() + first + 3);
    }

    const std::string          fileName = getTimeSeriesFileName("ErrorMeasurementOrderTest");
    exahype2::ErrorMeasurement measurement;
    measurement.open(fileName, 0.0);
    const auto hostNorms = measurement.record(ascending, 0.0);

    WHEN("Every rank adds up the errors of the cells in cell order in double precision") {
      double sumInCellOrder = 0.0;
      for (int cell = 0; cell < NumberOfCells; cell++) {
        sumInCellOrder += ascending[3 * static_cast<std::size_t>(cell)];
      }
      const double sumOfAllRanks = numberOfRanks * sumInCellOrder;

      THEN("The L1 norm of the host record divided by the sum of all ranks equals one") {
        VALIDATE_NUMERICAL_EQUALS(hostNorms[0] / sumOfAllRanks, 1.0, hostNorms[0], sumOfAllRanks);
      }
    }

    WHEN("The measurement sums the errors in ascending cell order on the device") {
      tarch::copyTo(
        measurement.getErrorsPerCell(NumberOfCells),
        ascending.data(),
        ascending.size(),
        tarch::MemoryLocation::Device
      );
      const auto norms = measurement.record(NumberOfCells, 1.0);

      THEN("The norms equal the norms of the host sums bitwise") {
        for (int norm = 0; norm < exahype2::ErrorMeasurement::NumberOfNorms; norm++) {
          VALIDATE_EQUALS(norms[norm], hostNorms[norm], norm);
        }
      }
    }

    WHEN("The measurement sums the errors in descending cell order on the device") {
      tarch::copyTo(
        measurement.getErrorsPerCell(NumberOfCells),
        descending.data(),
        descending.size(),
        tarch::MemoryLocation::Device
      );
      const auto norms = measurement.record(NumberOfCells, 1.0);

      THEN("The norms equal the norms of the host sums bitwise") {
        for (int norm = 0; norm < exahype2::ErrorMeasurement::NumberOfNorms; norm++) {
          VALIDATE_EQUALS(norms[norm], hostNorms[norm], norm);
        }
      }
    }

    WHEN("The measurement records the errors of zero cells") {
      const auto norms = measurement.record(0, 1.0);

      THEN("Every norm is zero") {
        for (int norm = 0; norm < exahype2::ErrorMeasurement::NumberOfNorms; norm++) {
          VALIDATE_EQUALS(norms[norm], 0.0, norm);
        }
      }
    }
    deleteTimeSeriesFile(fileName);
  }
}

UNIT_TEST("A record of the owned buffer gives the norms of the errors a kernel wrote into it") {
  GIVEN("An open measurement and the errors of five cells on the host") {
    constexpr int              NumberOfCells = 5;
    const std::string          fileName      = getTimeSeriesFileName("ErrorMeasurementBufferTest");
    exahype2::ErrorMeasurement measurement;
    measurement.open(fileName, 0.0);

    std::vector<double> errorsOnHost;
    for (int cell = 0; cell < NumberOfCells; cell++) {
      errorsOnHost.push_back(getErrorOfCell(cell));
      errorsOnHost.push_back(getErrorOfCell(cell) * getErrorOfCell(cell));
      errorsOnHost.push_back(getErrorOfCell(cell));
    }

    WHEN("A kernel writes the errors into the buffer of the measurement and the measurement records them") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(NumberOfCells),
        WriteErrorsOfCell{measurement.getErrorsPerCell(NumberOfCells)}
      );
      const std::size_t allocatedBytes = measurement.getAllocatedBytes().getTotal();
      const auto        norms          = measurement.record(NumberOfCells, 0.25);
      const auto        expected       = measurement.record(errorsOnHost, 0.5);

      THEN("The buffer stores the errors of every cell and the norms equal the norms of the host record") {
        VALIDATE(allocatedBytes >= static_cast<std::size_t>(NumberOfCells) * 3 * sizeof(double));
        for (int norm = 0; norm < exahype2::ErrorMeasurement::NumberOfNorms; norm++) {
          VALIDATE_EQUALS(norms[static_cast<std::size_t>(norm)], expected[static_cast<std::size_t>(norm)], norm);
        }
      }

      AND_WHEN("The measurement releases its buffers") {
        measurement.release();
        const std::size_t bytesAfterTheRelease = measurement.getAllocatedBytes().getTotal();

        THEN("The measurement occupies zero bytes") { VALIDATE_EQUALS(bytesAfterTheRelease, std::size_t{0}); }
      }
    }
    deleteTimeSeriesFile(fileName);
  }
}
