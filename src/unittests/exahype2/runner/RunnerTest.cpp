// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/runner/Runner.h"

#include "exahype2/runner/RuntimeParameters.h"
#include "exahype2/runner/Statistics.h"
#include "peano4/CellConnectivity.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/topology/Tree.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  constexpr int Depth = peano4::unittests::DepthWithCoarseCellsBesideTheCentreBlock;

  /**
   * Builds the regular grid of Depth levels and sets the runtime parameters to Depth levels and the terminal time zero.
   * The grid construction of a run then keeps this grid. Solvers at the time stamp zero have reached the terminal time.
   */
  void prepareGridForARun() {
    peano4::unittests::buildRegularGrid(Depth);
    exahype2::runner::RuntimeParameters::getInstance().setGeneratedDefaults(0.0, 0.0, Depth, Depth, "");
  }

  /**
   * Returns callbacks with empty phases and solvers that stay at the time stamp zero. The callbacks
   * return requests as their refinement requests. The caller keeps requests alive as long as the callbacks.
   */
  exahype2::runner::SolverRepositoryCallbacks getIdleCallbacks(const std::vector<peano4::RefinementRequest>& requests) {
    exahype2::runner::SolverRepositoryCallbacks callbacks;
    callbacks.requestRefinementForGridConstruction = [] {};
    callbacks.allocate                             = [] {};
    callbacks.initialise                           = [] {};
    callbacks.performTimeStep                      = [] {};
    callbacks.regrid                               = []([[maybe_unused]] const peano4::Regrid& regrid) {};
    callbacks.plot                                 = [] {};
    callbacks.getRefinementRequests                = [&requests]() -> const std::vector<peano4::RefinementRequest>& {
      return requests;
    };
    callbacks.getMinTimeStamp = [] { return 0.0; };
    callbacks.getMaxTimeStamp = [] { return 0.0; };
    return callbacks;
  }

  /**
   * Passes the number of snapshots to the runtime parameters as a command line does.
   */
  void setNumberOfSnapshots(const int numberOfSnapshots) {
    std::string executable        = "UnitTests";
    std::string option            = "-ns";
    std::string value             = std::to_string(numberOfSnapshots);
    char*       arguments[]       = {executable.data(), option.data(), value.data(), nullptr};
    int         numberOfArguments = 3;
    exahype2::runner::RuntimeParameters::getInstance().configure(numberOfArguments, arguments);
  }

  struct TimeLoopCalls final {
    int timeSteps{0};
    int plots{0};
    int records{0};
    int timeStepsBeforeTheFirstPlot{0};
    int timeStepsBeforeTheLastPlot{0};
    int timeStepsBeforeTheRecord{0};
  };

  /**
   * Runs the program up to the terminal time one tenth with numberOfSnapshots snapshots and solvers whose time steps
   * advance the time stamp by 1/128. Returns the calls of the callbacks and restores the number of snapshots.
   */
  TimeLoopCalls runWithFixedTimeSteps(const int numberOfSnapshots) {
    constexpr double TerminalTime             = 0.1;
    constexpr double TimeStepSize             = 0.0078125;
    auto&            parameters               = exahype2::runner::RuntimeParameters::getInstance();
    const int        numberOfSnapshotsOnEntry = parameters.getNumberOfSnapshots();
    prepareGridForARun();
    parameters.setGeneratedDefaults(TerminalTime, TerminalTime, Depth, Depth, "");
    setNumberOfSnapshots(numberOfSnapshots);

    const std::vector<peano4::RefinementRequest> requests;
    double                                       timeStamp = 0.0;
    TimeLoopCalls                                calls;
    auto                                         callbacks = getIdleCallbacks(requests);
    callbacks.performTimeStep                              = [&timeStamp, &calls] {
      timeStamp += TimeStepSize;
      calls.timeSteps++;
    };
    callbacks.plot = [&calls] {
      if (calls.plots == 0) {
        calls.timeStepsBeforeTheFirstPlot = calls.timeSteps;
      }
      calls.plots++;
      calls.timeStepsBeforeTheLastPlot = calls.timeSteps;
    };
    callbacks.recordLastTimeStamp = [&calls] {
      calls.records++;
      calls.timeStepsBeforeTheRecord = calls.timeSteps;
    };
    callbacks.getMinTimeStamp = [&timeStamp] { return timeStamp; };
    callbacks.getMaxTimeStamp = [&timeStamp] { return timeStamp; };
    exahype2::runner::Runner runner(
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0),
      std::bitset<DIMENSIONS>(),
      callbacks
    );
    runner.run();
    setNumberOfSnapshots(numberOfSnapshotsOnEntry);
    return calls;
  }
} // namespace

UNIT_TEST("A run sums the bytes, the cells and the cell degrees of freedom of the solvers over the ranks") {
  GIVEN("A regular grid prepared for a run and a runner whose solvers report fixed figures on every rank") {
    prepareGridForARun();
    const std::vector<peano4::RefinementRequest> requests;
    auto                                         callbacks = getIdleCallbacks(requests);
    callbacks.getAllocatedBytes                            = [] { return tarch::AllocatedBytes{300, 20}; };
    callbacks.getNumberOfSolverCells                       = [] { return std::uint64_t{10}; };
    callbacks.getNumberOfCellDoFs                          = [] { return std::uint64_t{640}; };
    exahype2::runner::Runner runner(
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0),
      std::bitset<DIMENSIONS>(),
      callbacks
    );
    const auto numberOfRanks = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getNumberOfRanks());

    WHEN("Every rank runs the program") {
      runner.run();
      const auto&         statistics        = exahype2::runner::getGlobalStatistics();
      const std::uint64_t solverHostBytes   = statistics.getGlobalSolverHostBytes();
      const std::uint64_t solverDeviceBytes = statistics.getGlobalSolverDeviceBytes();
      const std::uint64_t solverCells       = statistics.getGlobalNumberOfSolverCells();
      const std::uint64_t cellDoFs          = statistics.getGlobalNumberOfCellDoFs();

      THEN("The statistics count 300 host bytes, 20 device bytes, 10 cells and 640 degrees of freedom per rank") {
        VALIDATE_EQUALS(solverHostBytes, 300 * numberOfRanks);
        VALIDATE_EQUALS(solverDeviceBytes, 20 * numberOfRanks);
        VALIDATE_EQUALS(solverCells, 10 * numberOfRanks);
        VALIDATE_EQUALS(cellDoFs, 640 * numberOfRanks);
      }
    }
  }
}

UNIT_TEST("Every step of a run reads the sum of the values that the ranks add to the global reduction") {
  GIVEN("A prepared grid with the terminal time one and solvers that advance by one half per time step") {
    prepareGridForARun();
    exahype2::runner::RuntimeParameters::getInstance().setGeneratedDefaults(1.0, 1.0, Depth, Depth, "");
    const std::vector<peano4::RefinementRequest> requests;
    const auto    rank          = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getRank());
    const auto    numberOfRanks = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
    const auto    sumOfRanks    = numberOfRanks * (numberOfRanks + 1) / 2;
    int           timeSteps     = 0;
    int           slot          = -1;
    std::uint64_t additions     = 0;
    std::uint64_t reads         = 0;
    std::uint64_t mismatches    = 0;
    auto          callbacks     = getIdleCallbacks(requests);
    callbacks.performTimeStep   = [&timeSteps] { timeSteps++; };
    callbacks.getMinTimeStamp   = [&timeSteps] { return 0.5 * timeSteps; };
    callbacks.getMaxTimeStamp   = [&timeSteps] { return 0.5 * timeSteps; };
    callbacks.addLocalValuesToGlobalReduction = [&slot, &additions, rank](tarch::mpi::ReductionRecord& record) {
      additions++;
      slot = record.addSum((rank + 1) * additions);
    };
    callbacks.readValuesFromGlobalReduction =
      [&slot, &additions, &reads, &mismatches, sumOfRanks](const tarch::mpi::ReductionRecord& record) {
        reads++;
        if (record.getSum(slot) != additions * sumOfRanks) {
          mismatches++;
        }
      };
    exahype2::runner::Runner runner(
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0),
      std::bitset<DIMENSIONS>(),
      callbacks
    );

    WHEN("Every rank runs the program") {
      runner.run();

      THEN("Two time steps run, every step adds and reads once and every read returns the sum over the ranks") {
        VALIDATE_EQUALS(timeSteps, 2);
        VALIDATE(reads > std::uint64_t{2}, reads);
        VALIDATE_EQUALS(reads, additions);
        VALIDATE_EQUALS(mismatches, std::uint64_t{0});
      }
    }
  }
}

UNIT_TEST("The grid initialisation of a run applies only the votes of the initial condition") {
  GIVEN("A prepared grid, an initial condition that refines the centre once and a regrid that votes for a corner") {
    prepareGridForARun();
    peano4::RefinementRequest cornerRefinement{};
    cornerRefinement.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
    cornerRefinement.level        = Depth;
    cornerRefinement.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(0);

    std::vector<peano4::RefinementRequest> requests;
    int                                    initialisations = 0;
    int                                    regrids         = 0;
    auto                                   callbacks       = getIdleCallbacks(requests);
    callbacks.initialise                                   = [&requests, &initialisations] {
      initialisations++;
      requests.clear();
      if (initialisations == 1) {
        requests.push_back(peano4::unittests::getRefinementOfCentreCell(Depth));
      }
    };
    callbacks.regrid = [&requests, &regrids, &cornerRefinement]([[maybe_unused]] const peano4::Regrid& regrid) {
      regrids++;
      requests = {cornerRefinement};
    };
    exahype2::runner::Runner runner(
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0),
      std::bitset<DIMENSIONS>(),
      callbacks
    );

    WHEN("Every rank runs the program") {
      runner.run();
      const auto& spacetree   = peano4::Spacetree::getInstance();
      const auto  centreChild = tarch::la::Vector<DIMENSIONS, int>(
        peano4::SubdivisionFactor * (peano4::topology::getCellsPerAxis(Depth) / 2) + 1
      );
      const auto corner = tarch::la::Vector<DIMENSIONS, int>(0);
      const int  refinedCentreCells =
        peano4::unittests::getGlobalSumOf(spacetree.getCellAt(Depth + 1, centreChild) >= 0 ? 1 : 0);
      const int coarseCornerCells = peano4::unittests::getGlobalSumOf(spacetree.getCellAt(Depth, corner) >= 0 ? 1 : 0);
      const int refinedCornerCells =
        peano4::unittests::getGlobalSumOf(spacetree.getCellAt(Depth + 1, corner) >= 0 ? 1 : 0);

      THEN("The centre refines, the corner stays coarse and two initialisations frame the one regrid") {
        VALIDATE_EQUALS(refinedCentreCells, 1);
        VALIDATE_EQUALS(coarseCornerCells, 1);
        VALIDATE_EQUALS(refinedCornerCells, 0);
        VALIDATE_EQUALS(initialisations, 2);
        VALIDATE_EQUALS(regrids, 1);
      }
    }
  }
}

UNIT_TEST("The time loop plots before its first and after its last time step and records the last time stamp once") {
  GIVEN("Solvers whose time steps advance the time stamp by 1/128 up to the terminal time one tenth") {
    WHEN("Every rank runs the program with ten snapshots") {
      const TimeLoopCalls calls = runWithFixedTimeSteps(10);

      THEN("More than two plots frame the time steps and the record of the last time stamp follows the last step") {
        VALIDATE(calls.timeSteps > 0, calls.timeSteps);
        VALIDATE(calls.plots > 2, calls.plots);
        VALIDATE_EQUALS(calls.timeStepsBeforeTheFirstPlot, 0);
        VALIDATE_EQUALS(calls.timeStepsBeforeTheLastPlot, calls.timeSteps);
        VALIDATE_EQUALS(calls.records, 1);
        VALIDATE_EQUALS(calls.timeStepsBeforeTheRecord, calls.timeSteps);
      }
    }

    WHEN("Every rank runs the program with one snapshot") {
      const TimeLoopCalls calls = runWithFixedTimeSteps(1);

      THEN("One plot of the initial condition and one of the last time stamp frame the steps and one record follows") {
        VALIDATE(calls.timeSteps > 0, calls.timeSteps);
        VALIDATE_EQUALS(calls.plots, 2);
        VALIDATE_EQUALS(calls.timeStepsBeforeTheFirstPlot, 0);
        VALIDATE_EQUALS(calls.timeStepsBeforeTheLastPlot, calls.timeSteps);
        VALIDATE_EQUALS(calls.records, 1);
      }
    }
  }
}
