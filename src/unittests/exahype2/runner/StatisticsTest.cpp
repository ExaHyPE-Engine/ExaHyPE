// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/runner/Statistics.h"

#include "exahype2/TimeStepping.h"
#include "exahype2/runner/Console.h"
#include "peano4/Spacetree.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"
#include "tarch/testing/UnitTest.h"

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

  void reduceOverTheRanks(exahype2::TimeStepping& timeStepping) {
    tarch::mpi::ReductionRecord record;
    timeStepping.addLocalValuesToGlobalReduction(record);
    record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());
    timeStepping.readValuesFromGlobalReduction(record);
  }

  /**
   * Reduces the local statistics over all ranks in one reduction record with fresh memory figures.
   */
  void reduceOverTheRanks(exahype2::runner::GlobalStatistics& statistics) {
    tarch::mpi::ReductionRecord record;
    statistics.addLocalValuesToGlobalReduction(record, true);
    record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());
    statistics.readValuesFromGlobalReduction(record);
  }

  /**
   * A solver with a fixed time step rule. The solver reports zero wave speeds.
   */
  struct PlainSolver final {
    PlainSolver() {
      timeStepping.advanceTimeStamp(0.25);
      timeStepping.setLocalMeshWidths(0.5, 1.5);
      reduceOverTheRanks(timeStepping);
    }
    [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const { return timeStepping; }
    exahype2::TimeStepping timeStepping{
      exahype2::TimeStepping::Rule::Fixed,
      1.0,
      exahype2::TimeStepping::Stability::PhysicalEigenvalue
    };
  };
} // namespace

namespace exahype2::runner::unittests {
  struct LimitedSolver final {
    static constexpr bool HasLimiter = true;
    LimitedSolver() {
      timeStepping.advanceTimeStamp(0.125);
      timeStepping.setLocalMeshWidths(1.0, 3.0);
      timeStepping.raiseMaxEigenvalue(7.0);
      reduceOverTheRanks(timeStepping);
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
   * A solver with an adaptive time step rule and without a limiter. The solver offers a troubled cell
   * count all the same.
   */
  struct UnlimitedSolver final {
    static constexpr bool HasLimiter = false;
    UnlimitedSolver() {
      timeStepping.advanceTimeStamp(0.125);
      timeStepping.setLocalMeshWidths(1.0, 3.0);
      timeStepping.raiseMaxEigenvalue(7.0);
      reduceOverTheRanks(timeStepping);
    }
    [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const { return timeStepping; }
    [[nodiscard]] long getNumberOfTroubledCells() const { return 0; }
    exahype2::TimeStepping timeStepping{
      exahype2::TimeStepping::Rule::Adaptive,
      1.0,
      exahype2::TimeStepping::Stability::PhysicalEigenvalue
    };
  };
} // namespace exahype2::runner::unittests

UNIT_TEST("The global view sums and bounds the statistics of every rank and counts a rank boundary face once") {
  GIVEN("The grid statistics of this rank and solver bytes and cell updates that grow with the rank") {
    const auto  rank       = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getRank());
    const auto& local      = peano4::Spacetree::getInstance().getStatistics();
    auto&       statistics = exahype2::runner::getGlobalStatistics();
    statistics.setLocalSolverBytes(100 + rank, 7 + rank);
    statistics.setLocalCellUpdates(8 * (rank + 1), 64 * (rank + 1));
    const auto cells             = static_cast<std::uint64_t>(local.numberOfCells);
    const auto faces             = static_cast<std::uint64_t>(local.numberOfFaces);
    const auto rankBoundaryFaces = static_cast<std::uint64_t>(local.numberOfExchangedFaces);
    const auto minimumLevel      = static_cast<std::uint64_t>(local.minimumLevel);
    const auto maximumLevel      = static_cast<std::uint64_t>(local.maximumLevel);
    const auto stationaryRegrids = static_cast<std::uint64_t>(local.numberOfStationaryRegrids);

    WHEN("The ranks reduce the global view in one reduction record and their own statistics one by one") {
      reduceOverTheRanks(statistics);
      const auto cellsOfAllRanks             = getGlobalSumOf(cells);
      const auto refinedCellsOfAllRanks      = getGlobalSumOf(local.numberOfRefinedCells);
      const auto rankBoundaryFacesOfAllRanks = getGlobalSumOf(rankBoundaryFaces);
      const auto facesOfAllRanks             = getGlobalSumOf(faces);
      const auto hangingFacesOfAllRanks      = getGlobalSumOf(local.numberOfHangingFaces);
      const auto fewestRankBoundaryFaces     = getGlobalMinimumOf(rankBoundaryFaces);
      const auto mostRankBoundaryFaces       = getGlobalMaximumOf(rankBoundaryFaces);
      const auto smallestRankLoad            = getGlobalMinimumOf(cells);
      const auto largestRankLoad             = getGlobalMaximumOf(cells);
      const auto cellConnectivityHostBytes   = getGlobalSumOf(local.cellConnectivityBytes.host);
      const auto cellConnectivityDeviceBytes = getGlobalSumOf(local.cellConnectivityBytes.device);
      const auto treeBytes                   = getGlobalSumOf(local.treeBytes);
      const auto solverHostBytes             = getGlobalSumOf(100 + rank);
      const auto solverDeviceBytes           = getGlobalSumOf(7 + rank);
      const auto solverCells                 = getGlobalSumOf(8 * (rank + 1));
      const auto cellDoFs                    = getGlobalSumOf(64 * (rank + 1));
      const auto smallestLevel               = getGlobalMinimumOf(minimumLevel);
      const auto largestLevel                = getGlobalMaximumOf(maximumLevel);
      const auto fewestStationaryRegrids     = getGlobalMinimumOf(stationaryRegrids);
      const auto distinctRankBoundaryFaces   = rankBoundaryFacesOfAllRanks / 2;
      const auto globalFaces                 = statistics.getGlobalNumberOfFaces();
      const auto expectedPercentage          = globalFaces > 0
        ? 100.0 * static_cast<double>(distinctRankBoundaryFaces) / static_cast<double>(globalFaces)
        : 0.0;

      THEN("The view returns the sums, minima and maxima over the ranks and counts a rank boundary face once") {
        VALIDATE_EQUALS(statistics.getGlobalNumberOfInnerUnrefinedCells(), cellsOfAllRanks);
        VALIDATE_EQUALS(statistics.getGlobalNumberOfInnerRefinedCells(), refinedCellsOfAllRanks);
        VALIDATE_EQUALS(rankBoundaryFacesOfAllRanks % 2, std::uint64_t{0});
        VALIDATE_EQUALS(statistics.getGlobalNumberOfFaces(), facesOfAllRanks - distinctRankBoundaryFaces);
        VALIDATE_EQUALS(statistics.getGlobalNumberOfHangingFaces(), hangingFacesOfAllRanks);
        VALIDATE_EQUALS(statistics.getGlobalMinRankBoundaryFaces(), fewestRankBoundaryFaces);
        VALIDATE_EQUALS(statistics.getGlobalMaxRankBoundaryFaces(), mostRankBoundaryFaces);
        VALIDATE_EQUALS(statistics.getGlobalMinRankLoad(), smallestRankLoad);
        VALIDATE_EQUALS(statistics.getGlobalMaxRankLoad(), largestRankLoad);
        VALIDATE_EQUALS(statistics.getGlobalCellConnectivityHostBytes(), cellConnectivityHostBytes);
        VALIDATE_EQUALS(statistics.getGlobalCellConnectivityDeviceBytes(), cellConnectivityDeviceBytes);
        VALIDATE_EQUALS(statistics.getGlobalTreeBytes(), treeBytes);
        VALIDATE_EQUALS(statistics.getGlobalSolverHostBytes(), solverHostBytes);
        VALIDATE_EQUALS(statistics.getGlobalSolverDeviceBytes(), solverDeviceBytes);
        VALIDATE_EQUALS(statistics.getGlobalNumberOfSolverCells(), solverCells);
        VALIDATE_EQUALS(statistics.getGlobalNumberOfCellDoFs(), cellDoFs);
        VALIDATE_EQUALS(static_cast<std::uint64_t>(statistics.getGlobalMinimumLevel()), smallestLevel);
        VALIDATE_EQUALS(static_cast<std::uint64_t>(statistics.getGlobalMaximumLevel()), largestLevel);
        VALIDATE_EQUALS(
          static_cast<std::uint64_t>(statistics.getGlobalNumberOfStationaryRegrids()),
          fewestStationaryRegrids
        );
        VALIDATE_NUMERICAL_EQUALS(statistics.getGlobalRankBoundaryFacePercentage(), expectedPercentage);
        VALIDATE(statistics.getGlobalInterRankBalance() > 0.0 and statistics.getGlobalInterRankBalance() <= 100.0);
        VALIDATE(statistics.getGlobalTotalMemoryInBytes() > 0);
        VALIDATE(statistics.getGlobalAvailableMemoryInBytes() > 0);
        VALIDATE(statistics.getGlobalAvailableMemoryInBytes() <= statistics.getGlobalTotalMemoryInBytes());
        VALIDATE(statistics.getGlobalProcessMemoryUsageInBytes() > 0);
      }
    }
  }
}

UNIT_TEST("A snapshot fills the CPU memory rows and the GPU memory rows only in a build with a GPU backend") {
  GIVEN("A global view with solver data of 2048 bytes on the host and 1024 bytes on the device of every rank") {
    using GridSnapshot = exahype2::runner::GridSnapshot;
    auto& statistics   = exahype2::runner::getGlobalStatistics();
    statistics.setLocalSolverBytes(2048, 1024);

    WHEN("The ranks reduce the view and a snapshot collects its rows") {
      reduceOverTheRanks(statistics);
      const auto snapshot = exahype2::runner::collectGridSnapshot();

      THEN("The snapshot fills every CPU memory row and fills the GPU memory rows only with a GPU backend") {
        VALIDATE_EQUALS(
          snapshot.entries[GridSnapshot::MemoryCPUSolver].value,
          exahype2::runner::Format::getMemory(statistics.getGlobalSolverHostBytes())
        );
        VALIDATE(not snapshot.entries[GridSnapshot::MemoryCPUGrid].value.empty());
        VALIDATE(not snapshot.entries[GridSnapshot::MemoryResident].value.empty());
        VALIDATE(not snapshot.entries[GridSnapshot::MemoryFree].value.empty());
#if defined(PEANO_WITH_GPU_BACKEND)
        VALIDATE_EQUALS(
          snapshot.entries[GridSnapshot::MemoryGPUSolver].value,
          exahype2::runner::Format::getMemory(statistics.getGlobalSolverDeviceBytes())
        );
        VALIDATE(not snapshot.entries[GridSnapshot::MemoryGPUGrid].value.empty());
#else
        VALIDATE(snapshot.entries[GridSnapshot::MemoryGPUSolver].value.empty());
        VALIDATE(snapshot.entries[GridSnapshot::MemoryGPUGrid].value.empty());
#endif
      }
    }
  }
}

UNIT_TEST("A filled row renders as value, suffix and bracketed detail under its section and an empty row stays out") {
  GIVEN("A snapshot with filled rows for the cells, the levels and the free memory") {
    using GridSnapshot = exahype2::runner::GridSnapshot;
    GridSnapshot snapshot;
    snapshot.entries[GridSnapshot::GridCells]  = {"81", "Refined 10", ""};
    snapshot.entries[GridSnapshot::GridLevels] = {"2 .. 4", "", ""};
    snapshot.entries[GridSnapshot::MemoryFree] = {"4.29 GiB", "", " of 15.56 GiB"};

    WHEN("describeGridSnapshot() writes the snapshot") {
      const auto lines = exahype2::runner::describeGridSnapshot(snapshot);

      THEN("The grid section lists the cells with their detail and the levels, the memory section the free memory") {
        VALIDATE_EQUALS(lines.size(), std::size_t{5});
        VALIDATE_EQUALS(lines[0], std::string("  Grid"));
        VALIDATE_EQUALS(lines[1], std::string("    Cells   81  (Refined 10)"));
        VALIDATE_EQUALS(lines[2], std::string("    Levels  2 .. 4"));
        VALIDATE_EQUALS(lines[3], std::string("  Memory"));
        VALIDATE_EQUALS(lines[4], std::string("    Free    4.29 GiB of 15.56 GiB"));
      }
    }
  }
}

UNIT_TEST("A transition writes an arrow for a changed value and the value alone for an unchanged one") {
  GIVEN("Two snapshots in which the cells, the solver memory and the free memory change and the levels stay") {
    using GridSnapshot = exahype2::runner::GridSnapshot;
    GridSnapshot before;
    GridSnapshot after;
    before.entries[GridSnapshot::GridCells]       = {"81", "Refined 10", ""};
    after.entries[GridSnapshot::GridCells]        = {"633", "Refined 79", ""};
    before.entries[GridSnapshot::GridLevels]      = {"2 .. 4", "", ""};
    after.entries[GridSnapshot::GridLevels]       = {"2 .. 4", "", ""};
    before.entries[GridSnapshot::MemoryCPUSolver] = {"19.55 MiB", "", ""};
    after.entries[GridSnapshot::MemoryCPUSolver]  = {"24.10 MiB", "", ""};
    before.entries[GridSnapshot::MemoryFree]      = {"4.29 GiB", "", " of 15.56 GiB"};
    after.entries[GridSnapshot::MemoryFree]       = {"4.10 GiB", "", " of 15.56 GiB"};

    WHEN("describeGridTransition() writes the transition between the snapshots") {
      const auto lines = exahype2::runner::describeGridTransition(before, after);

      THEN("Every changed row shows both values with an arrow between them and the levels show their one value") {
        VALIDATE_EQUALS(lines.size(), std::size_t{6});
        VALIDATE_EQUALS(lines[1], std::string("    Cells       81 → 633  (Refined 79)"));
        VALIDATE_EQUALS(lines[2], std::string("    Levels      2 .. 4"));
        VALIDATE_EQUALS(lines[3], std::string("  Memory"));
        VALIDATE_EQUALS(lines[4], std::string("    CPU Solver  19.55 MiB → 24.10 MiB"));
        VALIDATE_EQUALS(lines[5], std::string("    Free        4.29 GiB → 4.10 GiB of 15.56 GiB"));
      }
    }
  }
}

UNIT_TEST("The free memory continues the line of the resident memory below the grid and the solver memory") {
  GIVEN("Two snapshots with the same CPU grid memory and a different resident and free memory") {
    using GridSnapshot = exahype2::runner::GridSnapshot;
    GridSnapshot before;
    GridSnapshot after;
    before.entries[GridSnapshot::MemoryCPUGrid]  = {"3.04 MiB", "", ""};
    after.entries[GridSnapshot::MemoryCPUGrid]   = {"3.04 MiB", "", ""};
    before.entries[GridSnapshot::MemoryResident] = {"75.56 MiB", "", ""};
    after.entries[GridSnapshot::MemoryResident]  = {"109.91 MiB", "", ""};
    before.entries[GridSnapshot::MemoryFree]     = {"13.45 GiB", "", " of 15.56 GiB"};
    after.entries[GridSnapshot::MemoryFree]      = {"13.42 GiB", "", " of 15.56 GiB"};

    WHEN("describeGridSnapshot() writes the first snapshot and describeGridTransition() the transition") {
      const auto snapshot   = exahype2::runner::describeGridSnapshot(before);
      const auto transition = exahype2::runner::describeGridTransition(before, after);

      THEN("The resident and the free memory form one line in the snapshot and in the transition") {
        VALIDATE_EQUALS(snapshot.size(), std::size_t{3});
        VALIDATE_EQUALS(snapshot[0], std::string("  Memory"));
        VALIDATE_EQUALS(snapshot[1], std::string("    CPU Grid  3.04 MiB"));
        VALIDATE_EQUALS(snapshot[2], std::string("    Resident  75.56 MiB · Free 13.45 GiB of 15.56 GiB"));
        VALIDATE_EQUALS(transition.size(), std::size_t{3});
        VALIDATE_EQUALS(
          transition[2],
          std::string("    Resident  75.56 MiB → 109.91 MiB · Free 13.45 GiB → 13.42 GiB of 15.56 GiB")
        );
      }
    }
  }
}

UNIT_TEST("A value, a range and a count statistic store their label, their kind and their numbers") {
  GIVEN("The labels dt, h and Troubled with the value two, the range from one to four and the count 12") {
    constexpr const char* ValueLabel = "dt";
    constexpr double      Value      = 2.0;
    constexpr const char* RangeLabel = "h";
    constexpr double      Minimum    = 1.0;
    constexpr double      Maximum    = 4.0;
    constexpr const char* CountLabel = "Troubled";
    constexpr long        Count      = 12L;

    WHEN("The test makes the three statistics") {
      const auto value = exahype2::runner::makeValueStatistic(ValueLabel, Value);
      const auto range = exahype2::runner::makeRangeStatistic(RangeLabel, Minimum, Maximum);
      const auto count = exahype2::runner::makeCountStatistic(CountLabel, Count);

      THEN("The value and the range store their bounds with the kind Value and the count its number with Count") {
        VALIDATE_EQUALS(value.label, std::string("dt"));
        VALIDATE(value.kind == exahype2::runner::SolverStatistic::Kind::Value);
        VALIDATE_NUMERICAL_EQUALS(value.minimum, 2.0);
        VALIDATE_NUMERICAL_EQUALS(value.maximum, 2.0);
        VALIDATE(range.kind == exahype2::runner::SolverStatistic::Kind::Value);
        VALIDATE_NUMERICAL_EQUALS(range.minimum, 1.0);
        VALIDATE_NUMERICAL_EQUALS(range.maximum, 4.0);
        VALIDATE(count.kind == exahype2::runner::SolverStatistic::Kind::Count);
        VALIDATE_EQUALS(count.count, 12L);
      }
    }
  }
}

UNIT_TEST("A solver without an eigenvalue reports the time step size and the mesh width range") {
  GIVEN("A solver with the fixed rule after the time step one quarter on mesh widths from one half to 1.5") {
    const PlainSolver solver;

    WHEN("The test makes the statistics of the solver") {
      const auto statistics = exahype2::runner::makeSolverStatistics("Plain", solver, appendNothing);

      THEN("The statistics name the solver and list the time step size and the mesh width range") {
        VALIDATE_EQUALS(statistics.first, std::string("Plain"));
        VALIDATE_EQUALS(statistics.second.size(), 2U);
        VALIDATE_EQUALS(statistics.second[0].label, std::string("dt"));
        VALIDATE_NUMERICAL_EQUALS(statistics.second[0].minimum, 0.25);
        VALIDATE_EQUALS(statistics.second[1].label, std::string("h"));
        VALIDATE_NUMERICAL_EQUALS(statistics.second[1].minimum, 0.5);
        VALIDATE_NUMERICAL_EQUALS(statistics.second[1].maximum, 1.5);
      }
    }
  }
}

UNIT_TEST("A solver with an eigenvalue and a limiter appends the largest eigenvalue and the troubled cells") {
  GIVEN("A solver with the adaptive rule, the largest eigenvalue seven and a limiter") {
    exahype2::runner::unittests::LimitedSolver solver;

    WHEN("The test makes the statistics of the solver without troubled cells and with five troubled cells") {
      const auto calm      = exahype2::runner::makeSolverStatistics("Limited", solver, appendNothing);
      solver.troubledCells = 5;
      const auto troubled  = exahype2::runner::makeSolverStatistics("Limited", solver, appendNothing);

      THEN("Both statistics end with the largest eigenvalue and the troubled cell count, zero included") {
        VALIDATE_EQUALS(calm.second.size(), 4U);
        VALIDATE_EQUALS(calm.second[2].label, std::string("λ_max"));
        VALIDATE_NUMERICAL_EQUALS(calm.second[2].maximum, 7.0);
        VALIDATE_EQUALS(calm.second[3].label, std::string("Troubled"));
        VALIDATE_EQUALS(calm.second[3].count, 0L);
        VALIDATE_EQUALS(troubled.second.size(), 4U);
        VALIDATE_EQUALS(troubled.second[3].label, std::string("Troubled"));
        VALIDATE_EQUALS(troubled.second[3].count, 5L);
      }
    }
  }
}

UNIT_TEST("The statistics of a solver with HasLimiter false skip the troubled cell count of the solver") {
  GIVEN("A solver with the adaptive rule, HasLimiter false and a troubled cell count") {
    const exahype2::runner::unittests::UnlimitedSolver solver;

    WHEN("The test makes the statistics of the solver") {
      const auto statistics = exahype2::runner::makeSolverStatistics("Unlimited", solver, appendNothing);

      THEN("The statistics end with the largest eigenvalue") {
        VALIDATE_EQUALS(statistics.second.size(), 3U);
        VALIDATE_EQUALS(statistics.second[2].label, std::string("λ_max"));
      }
    }
  }
}

UNIT_TEST("The additional statistics of a solver follow the mesh width") {
  GIVEN("A solver with the fixed rule") {
    const PlainSolver solver;

    WHEN("The test makes the statistics of the solver with a tracer count of three as additional statistic") {
      const auto statistics = exahype2::runner::makeSolverStatistics(
        "Plain",
        solver,
        [](std::vector<exahype2::runner::SolverStatistic>& additionalStatistics) {
          additionalStatistics.push_back(exahype2::runner::makeCountStatistic("Tracers", 3L));
        }
      );

      THEN("The tracer count follows the time step size and the mesh width") {
        VALIDATE_EQUALS(statistics.second.size(), 3U);
        VALIDATE_EQUALS(statistics.second[2].label, std::string("Tracers"));
        VALIDATE_EQUALS(statistics.second[2].count, 3L);
      }
    }
  }
}
