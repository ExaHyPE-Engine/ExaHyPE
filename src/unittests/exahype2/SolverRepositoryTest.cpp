// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/SolverRepository.h"

#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/TimeStepping.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"
#include "tarch/plotter/Format.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

#include <mpi.h>

namespace {
  constexpr int Depth = 2;

  struct RuntimeParameters final {
    int value{0};
  };

  /**
   * With OwnsCells, the solver reports its own number of cells. Without it, the repository counts
   * every cell of the cell connectivity. The initialisation adds parameterIncrement to the runtime
   * parameter value.
   */
  template <bool OwnsCells>
  struct RecordingSolver final {
    exahype2::TimeStepping timeStepping{
      exahype2::TimeStepping::Rule::Fixed,
      1.0,
      exahype2::TimeStepping::Stability::PhysicalEigenvalue
    };
    int                   numberOfDoFs{0};
    int                   numberOfCells{0};
    tarch::AllocatedBytes allocatedBytes{};

    int                         phaseCalls{0};
    int                         slot{-1};
    std::uint64_t               reducedValue{0};
    exahype2::RefinementCommand timeStepCommand{exahype2::RefinementCommand::Keep};
    exahype2::RefinementCommand regridCommand{exahype2::RefinementCommand::Keep};
    int                         parameterIncrement{0};
    int                         parameterValueOfStart{-1};
    int                         parameterValueOfInitialisation{-1};
    int                         parameterValueOfTimeStep{-1};

    [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const { return timeStepping; }
    [[nodiscard]] int getNumberOfDoFs() const { return numberOfDoFs; }
    [[nodiscard]] int getNumberOfCells([[maybe_unused]] const peano4::CellConnectivity& cellConnectivity) const
      requires OwnsCells
    {
      return numberOfCells;
    }
    [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const { return allocatedBytes; }

    void startSimulation(const RuntimeParameters& parameters) {
      phaseCalls++;
      parameterValueOfStart = parameters.value;
    }
    void finishSimulation([[maybe_unused]] const RuntimeParameters& parameters) { phaseCalls++; }
    void allocate(
      [[maybe_unused]] const peano4::CellConnectivity& cellConnectivity,
      [[maybe_unused]] const RuntimeParameters&        parameters
    ) {
      phaseCalls++;
    }
    void release() { phaseCalls++; }
    void requestRefinementForGridConstruction(
      [[maybe_unused]] const peano4::CellConnectivity& cellConnectivity,
      [[maybe_unused]] exahype2::RefinementRequests&   requests,
      [[maybe_unused]] const RuntimeParameters&        parameters
    ) {
      phaseCalls++;
    }
    void initialise(
      [[maybe_unused]] const peano4::CellConnectivity& cellConnectivity,
      [[maybe_unused]] exahype2::RefinementRequests&   requests,
      RuntimeParameters&                               parameters
    ) {
      phaseCalls++;
      parameters.value += parameterIncrement;
      parameterValueOfInitialisation = parameters.value;
    }
    void performTimeStep(
      const peano4::CellConnectivity& cellConnectivity,
      exahype2::RefinementRequests&   requests,
      const RuntimeParameters&        parameters
    ) {
      phaseCalls++;
      parameterValueOfTimeStep = parameters.value;
      requests.merge(timeStepCommand, cellConnectivity.getCells());
    }
    void regrid(
      const peano4::CellConnectivity&           cellConnectivity,
      [[maybe_unused]] const peano4::Regrid&    regrid,
      exahype2::RefinementRequests&             requests,
      [[maybe_unused]] const RuntimeParameters& parameters
    ) {
      phaseCalls++;
      requests.merge(regridCommand, cellConnectivity.getCells());
    }
    void plot(
      [[maybe_unused]] const peano4::CellConnectivity& cellConnectivity,
      [[maybe_unused]] const tarch::plotter::Format    format,
      [[maybe_unused]] const std::string&              outputDirectory,
      [[maybe_unused]] const RuntimeParameters&        parameters
    ) {
      phaseCalls++;
    }
    void recordLastTimeStamp(
      [[maybe_unused]] const peano4::CellConnectivity& cellConnectivity,
      [[maybe_unused]] const RuntimeParameters&        parameters
    ) {
      phaseCalls++;
    }
    void addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record) {
      phaseCalls++;
      slot = record.addSum(1);
    }
    void readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record) {
      phaseCalls++;
      reducedValue = record.getSum(slot);
    }
  };

  using CountingSolver = RecordingSolver<true>;
  using PlainSolver    = RecordingSolver<false>;

  using RepositoryOfOneSolver              = exahype2::SolverRepository<RuntimeParameters, PlainSolver>;
  using RepositoryOfTwoSolvers             = exahype2::SolverRepository<RuntimeParameters, PlainSolver, PlainSolver>;
  using RepositoryOfCountingAndPlainSolver = exahype2::SolverRepository<RuntimeParameters, CountingSolver, PlainSolver>;
} // namespace

UNIT_TEST("The repository reduces the values of every solver and counts every cell for a solver without a cell count") {
  GIVEN("Two solvers with time stamps, degrees of freedom and bytes, the first with a cell count and a regular grid") {
    CountingSolver coarse;
    PlainSolver    fine;
    coarse.timeStepping.advanceTimeStamp(0.5);
    fine.timeStepping.advanceTimeStamp(0.125);
    fine.timeStepping.advanceTimeStamp(0.125);
    coarse.numberOfDoFs   = 64;
    coarse.numberOfCells  = 3;
    coarse.allocatedBytes = {1024, 256};
    fine.numberOfDoFs     = 128;
    fine.allocatedBytes   = {512, 0};

    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();

    WHEN("A repository over both solvers reduces their values") {
      const RepositoryOfCountingAndPlainSolver repository{coarse, fine};
      const double                             minTimeStamp = repository.getMinTimeStamp();
      const double                             maxTimeStamp = repository.getMaxTimeStamp();
      const std::uint64_t         numberOfSolverCells       = repository.getNumberOfSolverCells(cellConnectivity);
      const std::uint64_t         numberOfCellDoFs          = repository.getNumberOfCellDoFs(cellConnectivity);
      const tarch::AllocatedBytes allocatedBytes            = repository.getAllocatedBytes();

      THEN("The time stamps range from one quarter to one half and the cells, degrees of freedom and bytes add up") {
        VALIDATE_NUMERICAL_EQUALS(minTimeStamp, 0.25);
        VALIDATE_NUMERICAL_EQUALS(maxTimeStamp, 0.5);
        VALIDATE_EQUALS(
          numberOfSolverCells,
          std::uint64_t{3} + static_cast<std::uint64_t>(cellConnectivity.getCells().size)
        );
        VALIDATE_EQUALS(
          numberOfCellDoFs,
          std::uint64_t{3} * 64 + static_cast<std::uint64_t>(cellConnectivity.getCells().size) * 128
        );
        VALIDATE_EQUALS(allocatedBytes.host, std::size_t{1536});
        VALIDATE_EQUALS(allocatedBytes.device, std::size_t{256});
      }
    }
  }
}

UNIT_TEST("Every phase call reaches every solver once and the requests of an empty cell connectivity stay empty") {
  GIVEN("A repository over two solvers, an empty cell connectivity and an empty regrid") {
    PlainSolver                    first;
    PlainSolver                    second;
    RepositoryOfTwoSolvers         repository{first, second};
    const peano4::CellConnectivity cellConnectivity{};
    const peano4::Regrid           regrid{};

    WHEN("The repository runs every phase and one reduction from the start to the finish of a simulation") {
      repository.startSimulation();
      repository.allocate(cellConnectivity);
      repository.requestRefinementForGridConstruction(cellConnectivity);
      repository.initialise(cellConnectivity);
      repository.performTimeStep(cellConnectivity);
      repository.regrid(cellConnectivity, regrid);
      repository.plot(cellConnectivity, tarch::plotter::Format::VTU, "");
      repository.recordLastTimeStamp(cellConnectivity);
      tarch::mpi::ReductionRecord record;
      repository.addLocalValuesToGlobalReduction(record);
      record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());
      repository.readValuesFromGlobalReduction(record);
      repository.finishSimulation();
      const auto  numberOfRanks      = static_cast<std::uint64_t>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
      const auto& refinementRequests = repository.getRefinementRequests();

      THEN("Every solver counts eleven phases and one release and reads the rank count and the requests stay empty") {
        VALIDATE_EQUALS(first.phaseCalls, 12);
        VALIDATE_EQUALS(second.phaseCalls, 12);
        VALIDATE_EQUALS(first.reducedValue, numberOfRanks);
        VALIDATE_EQUALS(second.reducedValue, numberOfRanks);
        VALIDATE(refinementRequests.empty());
      }
    }
  }
}

UNIT_TEST("A solver that overwrites the runtime parameters in its initialisation passes them to every later call") {
  GIVEN("A repository with the runtime parameter value three over two solvers, the first of which adds four to it") {
    PlainSolver first;
    PlainSolver second;
    first.parameterIncrement = 4;
    RepositoryOfTwoSolvers repository{first, second};
    repository.setParameters(RuntimeParameters{.value = 3});
    const peano4::CellConnectivity cellConnectivity{};

    WHEN("The repository starts the simulation, initialises both solvers and runs one time step") {
      repository.startSimulation();
      repository.initialise(cellConnectivity);
      repository.performTimeStep(cellConnectivity);

      THEN("Both solvers start with three and the second solver and every time step read seven") {
        VALIDATE_EQUALS(first.parameterValueOfStart, 3);
        VALIDATE_EQUALS(second.parameterValueOfStart, 3);
        VALIDATE_EQUALS(first.parameterValueOfInitialisation, 7);
        VALIDATE_EQUALS(second.parameterValueOfInitialisation, 7);
        VALIDATE_EQUALS(first.parameterValueOfTimeStep, 7);
        VALIDATE_EQUALS(second.parameterValueOfTimeStep, 7);
      }
    }
  }
}

UNIT_TEST("Every reduction gathers the refinement requests of the step that filled it") {
  GIVEN("A regular grid and a repository over one solver whose time step votes with a command that the test sets") {
    peano4::unittests::buildRegularGrid(Depth);
    const auto&           cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    PlainSolver           solver;
    RepositoryOfOneSolver repository{solver};
    const MPI_Comm        communicator = tarch::mpi::Rank::getInstance().getCommunicator();

    WHEN("A time step votes for refinement on every cell and the ranks reduce the step") {
      solver.timeStepCommand = exahype2::RefinementCommand::Refine;
      repository.performTimeStep(cellConnectivity);
      tarch::mpi::ReductionRecord refiningStep;
      repository.addLocalValuesToGlobalReduction(refiningStep);
      refiningStep.allreduce(communicator);
      repository.readValuesFromGlobalReduction(refiningStep);
      const bool refiningStepRequestsAChange = not repository.getRefinementRequests().empty();

      THEN("The repository returns the requests of the refining step") { VALIDATE(refiningStepRequestsAChange); }

      AND_WHEN("The next time step keeps every cell and the ranks reduce this step") {
        solver.timeStepCommand = exahype2::RefinementCommand::Keep;
        repository.performTimeStep(cellConnectivity);
        tarch::mpi::ReductionRecord keepingStep;
        repository.addLocalValuesToGlobalReduction(keepingStep);
        keepingStep.allreduce(communicator);
        repository.readValuesFromGlobalReduction(keepingStep);
        const bool keepingStepRequestsAChange = not repository.getRefinementRequests().empty();

        THEN("The repository returns an empty request list") { VALIDATE(not keepingStepRequestsAChange); }
      }
    }
  }
}

UNIT_TEST("A regrid deletes the erase request of every cell that it refines for the second time") {
  GIVEN("A repository over one solver whose time step and regrid vote with commands that the test sets") {
    const MPI_Comm        communicator  = tarch::mpi::Rank::getInstance().getCommunicator();
    const int             numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const peano4::Regrid  regrid{};
    PlainSolver           solver;
    RepositoryOfOneSolver repository{solver};

    const auto finishStep = [&repository, communicator] {
      tarch::mpi::ReductionRecord record;
      repository.addLocalValuesToGlobalReduction(record);
      record.allreduce(communicator);
      repository.readValuesFromGlobalReduction(record);
    };
    const auto runTimeStep = [&solver, &repository, &finishStep] {
      peano4::unittests::buildRegularGrid(Depth - 1);
      solver.timeStepCommand = exahype2::RefinementCommand::Refine;
      repository.performTimeStep(peano4::Spacetree::getInstance().getCellConnectivity());
      finishStep();
    };
    const auto runRegridStep =
      [&solver, &repository, &regrid, &finishStep](const int depth, const exahype2::RefinementCommand command) {
        peano4::unittests::buildRegularGrid(depth);
        solver.regridCommand = command;
        repository.regrid(peano4::Spacetree::getInstance().getCellConnectivity(), regrid);
        finishStep();
      };

    peano4::unittests::buildRegularGrid(Depth);
    const auto&                  cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    exahype2::RefinementRequests eraseVotes;
    eraseVotes.clear(cellConnectivity.getNumberOfSlots());
    eraseVotes.merge(exahype2::RefinementCommand::Erase, cellConnectivity.getCells());
    const auto eraseRequests = eraseVotes.getRefinementRequests(cellConnectivity);

    WHEN("A time step refines, three regrid steps erase, refine and erase, and one more time step and erase follow") {
      runTimeStep();
      runRegridStep(Depth, exahype2::RefinementCommand::Erase);
      const bool firstEraseRequestsEveryCell = repository.getRefinementRequests() == eraseRequests;
      runRegridStep(Depth - 1, exahype2::RefinementCommand::Refine);
      const bool refinementRequestsAChange = not repository.getRefinementRequests().empty();
      runRegridStep(Depth, exahype2::RefinementCommand::Erase);
      const bool secondEraseRequestsAChange = not repository.getRefinementRequests().empty();
      runTimeStep();
      runRegridStep(Depth, exahype2::RefinementCommand::Erase);
      const bool eraseOfTheNextRegridRequestsEveryCell = repository.getRefinementRequests() == eraseRequests;

      THEN("Only the erase that follows the second refinement in one regrid deletes its erase requests") {
        VALIDATE(not eraseRequests.empty() or numberOfRanks > 1);
        VALIDATE(firstEraseRequestsEveryCell);
        VALIDATE(refinementRequestsAChange);
        VALIDATE(not secondEraseRequestsAChange);
        VALIDATE(eraseOfTheNextRegridRequestsEveryCell);
      }
    }
  }
}
