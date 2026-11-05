// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/fv/singlestep/Solver.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/Patch.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"
#include "tarch/plotter/Format.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  TARGET_DECLARE
  constexpr int    Depth          = peano4::unittests::DepthWithCoarseCellsBesideTheCentreBlock;
  constexpr int    CellsPerAxis   = peano4::power(peano4::SubdivisionFactor, Depth);
  constexpr int    RegularCells   = peano4::power(CellsPerAxis, DIMENSIONS);
  constexpr double CoarseWidth    = 1.0 / CellsPerAxis;
  constexpr int    PatchSize      = 2;
  constexpr int    ValuesPerPatch = peano4::power(PatchSize, DIMENSIONS);
  constexpr double TimeStepFactor = 0.5;
  TARGET_DECLARE_END

  struct RuntimeParameters final {};

  /**
   * LinearSolverFor evolves one unknown on constant subcells from a linear initial condition without
   * flux. Under the fixed rule it reports the wave speed zero.
   */
  template <exahype2::TimeStepping::Rule Rule>
  struct LinearSolverFor final : exahype2::fv::singlestep::Solver<LinearSolverFor<Rule>, double, double> {
    using Base = exahype2::fv::singlestep::Solver<LinearSolverFor<Rule>, double, double>;

    static constexpr int PatchSize                  = ::PatchSize;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    using RuntimeParameters = ::RuntimeParameters;
    using Reconstruction    = exahype2::fv::reconstruction::Constant;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = Rule;
    static constexpr std::array<int, 1>           MeasuredUnknowns{0};

    LinearSolverFor() :
      Base(TimeStepFactor, "LinearSolver", {{"q", 1}}) {}

    const typename Base::Faces& getHalo() const { return this->_halo; }

    static INLINE_METHOD GPU_CALLABLE_METHOD double getProfile(const tarch::la::Vector<DIMENSIONS, double>& x) {
      double value = 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        value += (axis + 2.0) * x(axis);
      }
      return value;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                                 Q,
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      Q[0] = getProfile(x);
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void analyticalSolution(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const double                t,
      [[maybe_unused]] const double                dt,
      [[maybe_unused]] const double* const         Q,
      double* const                                solution,
      [[maybe_unused]] const RuntimeParameters&    parameters
    ) {
      solution[0] = getProfile(x);
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      const double* const                                           inside,
      double* const                                                 outside,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      outside[0] = inside[0];
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return exahype2::RefinementCommand::Keep;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return Rule == exahype2::TimeStepping::Rule::Adaptive ? 1.0 : 0.0;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      double* const                                                 F,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      F[0] = 0.0;
    }
  };

  using LinearSolver = LinearSolverFor<exahype2::TimeStepping::Rule::Fixed>;

  TARGET_DECLARE
  constexpr double PostprocessedValue     = 20.0;
  constexpr double PostprocessedWaveSpeed = 2.0;
  TARGET_DECLARE_END

  /**
   * PostprocessingSolver works like LinearSolverFor and writes PostprocessedValue into every updated
   * volume in its patch postprocessing. Its wave speed on such a volume is PostprocessedWaveSpeed.
   */
  struct PostprocessingSolver final : exahype2::fv::singlestep::Solver<PostprocessingSolver, double, double> {
    using Base = exahype2::fv::singlestep::Solver<PostprocessingSolver, double, double>;

    static constexpr int PatchSize                  = ::PatchSize;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    using RuntimeParameters = ::RuntimeParameters;
    using Reconstruction    = exahype2::fv::reconstruction::Constant;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Adaptive;

    PostprocessingSolver() :
      Base(TimeStepFactor, "PostprocessingSolver", {{"q", 1}}) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                                 Q,
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      Q[0] = LinearSolver::getProfile(x);
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      const double* const                                           inside,
      double* const                                                 outside,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      outside[0] = inside[0];
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return exahype2::RefinementCommand::Keep;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const double* const                                           Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return Q[0] == PostprocessedValue ? PostprocessedWaveSpeed : 1.0;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      double* const                                                 F,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      F[0] = 0.0;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void postprocessUpdatedPatch(
      double* const                                                 Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int value = 0; value < ValuesPerPatch; value++) {
        Q[value] = PostprocessedValue;
      }
    }
  };

  const peano4::CellConnectivity& buildGrid() {
    peano4::unittests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getCellConnectivity();
  }

  /**
   * Reduces the values of solver over all ranks in the one allreduce that ends a step of a run.
   */
  template <typename Solver>
  void finishStep(Solver& solver) {
    tarch::mpi::ReductionRecord record;
    solver.addLocalValuesToGlobalReduction(record);
    record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());
    solver.readValuesFromGlobalReduction(record);
  }

  /**
   * Applies the refinement requests to the standing grid and moves the solver data through the
   * regrid. Every slot the regrid keeps then contains its data. The reduction of the step follows.
   */
  template <typename Solver>
  void regridWithLiveData(
    Solver&                                       solver,
    const std::vector<peano4::RefinementRequest>& refinementRequests,
    exahype2::RefinementRequests&                 requests
  ) {
    auto&          spacetree = peano4::Spacetree::getInstance();
    peano4::Regrid regrid;
    if (not spacetree.regrid(refinementRequests, regrid)) {
      tarch::terminateWithError("The regrid of the test grid must change the grid.");
    }
    const auto& cellConnectivity = spacetree.getCellConnectivity();
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.regrid(cellConnectivity, regrid, requests, RuntimeParameters{});
    spacetree.finishRegrid();
    finishStep(solver);
  }

  int getNumberOfCellsOfLevel(const peano4::CellConnectivity& cellConnectivity, const int level) {
    return cellConnectivity.getCells(peano4::Adjacency::Local, level).size
      + cellConnectivity.getCells(peano4::Adjacency::Remote, level).size;
  }

  exahype2::Limits getLimitsOfTheDomain() {
    exahype2::Limits limits;
    limits.terminalTime           = 1.0;
    limits.minAdmissiblePatchSize = 0.0;
    limits.maxAdmissiblePatchSize = 1.0;
    limits.maxAdmissibleMeshWidth = CoarseWidth / PatchSize;
    return limits;
  }

  std::vector<std::string> readLinesOf(const std::string& fileName) {
    std::vector<std::string> lines;
    std::ifstream            file(fileName);
    std::string              line;
    while (std::getline(file, line)) {
      lines.push_back(line);
    }
    return lines;
  }

  TARGET_DECLARE
  /**
   * Adds one to the accumulator for every volume of one cell whose value differs from the profile
   * at the centre of the volume of level that covers it.
   */
  struct CountMismatchesAgainstTheProfile final {
    peano4::CellConnectivity         cellConnectivity{};
    LinearSolver::Patches::ConstView solution{};
    int                              level{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const int                             cell      = cellConnectivity.getCells()[entry];
      const int                             cellLevel = cellConnectivity.getCellLevel(cell);
      const auto                            lattice   = cellConnectivity.getCellLattice(cell);
      tarch::la::Vector<DIMENSIONS, double> coarseCentre;
      tarch::la::Vector<DIMENSIONS, double> coarseWidth;
      tarch::la::Vector<DIMENSIONS, int>    childPosition(0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (cellLevel == level) {
          coarseCentre(axis) = cellConnectivity.getCellCentre(cell, axis);
          coarseWidth(axis)  = cellConnectivity.getCellWidth(cell, axis);
        } else {
          const int parentLattice = lattice(axis) / peano4::SubdivisionFactor;
          coarseWidth(axis)       = cellConnectivity.getWidthOfLevel(level, axis);
          coarseCentre(axis)  = cellConnectivity.getDomainOffset()(axis) + (parentLattice + 0.5) * coarseWidth(axis);
          childPosition(axis) = lattice(axis) % peano4::SubdivisionFactor;
        }
      }
      const double* const values = solution.getChunk(cell);
      peano4::dForEach(PatchSize, [&](const tarch::la::Vector<DIMENSIONS, int>& fineVolume) {
        tarch::la::Vector<DIMENSIONS, int> coarseVolume;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          coarseVolume(axis) = cellLevel == level
            ? fineVolume(axis)
            : (fineVolume(axis) + PatchSize * childPosition(axis)) / peano4::SubdivisionFactor;
        }
        const auto   centre = exahype2::fv::getVolumeCentre(coarseCentre, coarseWidth, PatchSize, coarseVolume);
        const double value  = values[peano4::dLinearised(fineVolume, PatchSize)];
        if (std::abs(value - LinearSolver::getProfile(centre)) > tarch::testing::NumericalTolerance) {
          accumulator++;
        }
      });
    }
  };

  /**
   * Adds one to the accumulator for every value of one cell that differs from PostprocessedValue.
   */
  struct CountValuesWithoutPostprocessing final {
    peano4::CellConnectivity                 cellConnectivity{};
    PostprocessingSolver::Patches::ConstView solution{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const int cell = cellConnectivity.getCells()[entry];
      for (int value = 0; value < ValuesPerPatch; value++) {
        if (solution.getChunk(cell)[value] != PostprocessedValue) {
          accumulator++;
        }
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Returns the number of volumes whose value differs from the profile at the centre of the
   * covering volume of level.
   */
  template <typename SolverType>
  int getNumberOfMismatchesAgainstTheProfileOfLevel(
    const SolverType&               solver,
    const peano4::CellConnectivity& cellConnectivity,
    const int                       level
  ) {
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getCells().size),
      CountMismatchesAgainstTheProfile{cellConnectivity.onDevice(), solver.getSolution().getView(), level},
      tarch::accelerator::Sum<int>{mismatches}
    );
    return mismatches;
  }

  /**
   * Counts the values of every cell that the patch postprocessing has not overwritten.
   */
  int getNumberOfValuesWithoutPostprocessing(
    const PostprocessingSolver&     solver,
    const peano4::CellConnectivity& cellConnectivity
  ) {
    int values = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getCells().size),
      CountValuesWithoutPostprocessing{cellConnectivity.onDevice(), solver.getSolution().getView()},
      tarch::accelerator::Sum<int>{values}
    );
    return values;
  }
} // namespace

UNIT_TEST("The initialisation writes the initial condition into every volume") {
  GIVEN("A solver allocated on a regular grid over every rank") {
    const auto&  cellConnectivity = buildGrid();
    LinearSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});

    WHEN("The solver initialises every cell with an erase request") {
      exahype2::RefinementRequests requests;
      requests.clear(cellConnectivity.getNumberOfSlots());
      requests.merge(exahype2::RefinementCommand::Erase, cellConnectivity.getCells());
      solver.initialise(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);
      const int cells      = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
      const int mismatches = peano4::unittests::getGlobalSumOf(
        getNumberOfMismatchesAgainstTheProfileOfLevel(solver, cellConnectivity, Depth)
      );
      const std::vector<peano4::RefinementRequest> remainingRequests = requests.getRefinementRequests(cellConnectivity);

      THEN("Every volume stores the profile, the grid stays regular and the refinement requests stay empty") {
        VALIDATE_EQUALS(mismatches, 0);
        VALIDATE_EQUALS(cells, RegularCells);
        VALIDATE_EQUALS(solver.getNumberOfDoFs(), ValuesPerPatch);
        VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMinMeshWidth(), CoarseWidth / PatchSize);
        VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth / PatchSize);
        VALIDATE(remainingRequests.empty());
        VALIDATE(solver.getAllocatedBytes().getTotal() > 0);
      }
    }
    solver.release();
  }
}

UNIT_TEST("A time step without flux keeps the solution and advances the time stamp") {
  GIVEN("A solver without flux initialised on a regular grid") {
    const auto&  cellConnectivity = buildGrid();
    LinearSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);

    WHEN("The solver performs three time steps") {
      const double        firstStepSize = solver.getTimeStepping().getTimeStepSize();
      std::vector<int>    mismatchesOfStep;
      std::vector<double> timeStampOfStep;
      std::vector<double> sizeOfStep;
      std::vector<double> sizeOfNextStep;
      for (int step = 0; step < 3; step++) {
        requests.clear(cellConnectivity.getNumberOfSlots());
        solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
        finishStep(solver);
        mismatchesOfStep.push_back(
          peano4::unittests::getGlobalSumOf(
            getNumberOfMismatchesAgainstTheProfileOfLevel(solver, cellConnectivity, Depth)
          )
        );
        timeStampOfStep.push_back(solver.getTimeStepping().getTimeStamp());
        sizeOfStep.push_back(solver.getTimeStepping().getTimeStepSizeOfLastStep());
        sizeOfNextStep.push_back(solver.getTimeStepping().getTimeStepSize());
      }
      const std::vector<peano4::RefinementRequest> remainingRequests = requests.getRefinementRequests(cellConnectivity);

      THEN("Every volume keeps its value and the refinement requests stay empty") {
        for (std::size_t step = 0; step < mismatchesOfStep.size(); step++) {
          VALIDATE_EQUALS(mismatchesOfStep[step], 0, step);
        }
        VALIDATE(remainingRequests.empty());
      }

      THEN("The time stamp advances by a first step of size zero and then by the time step factor") {
        VALIDATE_NUMERICAL_EQUALS(firstStepSize, 0.0);
        for (std::size_t step = 0; step < timeStampOfStep.size(); step++) {
          VALIDATE_NUMERICAL_EQUALS(timeStampOfStep[step], static_cast<double>(step) * TimeStepFactor, step);
          VALIDATE_NUMERICAL_EQUALS(sizeOfStep[step], step == 0 ? 0.0 : TimeStepFactor, step);
          VALIDATE_NUMERICAL_EQUALS(sizeOfNextStep[step], TimeStepFactor, step);
        }
      }
    }
    solver.release();
  }
}

UNIT_TEST("Recording the last time stamp appends the row of that time stamp to the error file once") {
  GIVEN("A solver without flux that measures its error into a file, after its initialisation and three time steps") {
    constexpr double  TimeBetweenTwoRecords = 100.0;
    const auto&       cellConnectivity      = buildGrid();
    const std::string fileName       = (tarch::testing::getWritableTestDirectory() / "FVSolverTestError").string();
    const bool        isGlobalMaster = tarch::mpi::Rank::getInstance().isGlobalMaster();
    LinearSolver      solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.openErrorMeasurement(fileName, TimeBetweenTwoRecords);
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    for (int step = 0; step < 3; step++) {
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);
    }
    // Only the global master writes the error file.
    const std::size_t linesBeforeTheRecord = isGlobalMaster ? readLinesOf(fileName + ".csv").size() : 0;

    WHEN("The solver records the last time stamp twice") {
      solver.recordLastTimeStamp(cellConnectivity, RuntimeParameters{});
      solver.recordLastTimeStamp(cellConnectivity, RuntimeParameters{});
      const auto lines = isGlobalMaster ? readLinesOf(fileName + ".csv") : std::vector<std::string>{};
      if (isGlobalMaster) {
        std::filesystem::remove(fileName + ".csv");
      }

      THEN("The file grows from the header and the initial row by the one row of the last time stamp") {
        if (isGlobalMaster) {
          VALIDATE_EQUALS(linesBeforeTheRecord, std::size_t{2});
          VALIDATE_EQUALS(lines.size(), std::size_t{3});
          VALIDATE_NUMERICAL_EQUALS(std::stod(lines.back()), solver.getTimeStepping().getTimeStamp());
        }
      }
    }
    solver.release();
  }
}

UNIT_TEST("A refinement and the erase back keep the solution") {
  GIVEN("A solver without flux on a regular grid after one time step, with live solver data") {
    const auto&  cellConnectivity = buildGrid();
    LinearSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    const double coarseTimeStepSize = solver.getTimeStepping().getTimeStepSize();
    peano4::Spacetree::getInstance().markPersistentDataLive();

    WHEN("The cell in the centre refines") {
      regridWithLiveData(solver, {peano4::unittests::getRefinementOfCentreCell(Depth)}, requests);
      const int fineCells  = peano4::unittests::getGlobalSumOf(getNumberOfCellsOfLevel(cellConnectivity, Depth + 1));
      const int mismatches = peano4::unittests::getGlobalSumOf(
        getNumberOfMismatchesAgainstTheProfileOfLevel(solver, cellConnectivity, Depth)
      );

      THEN("Every fine volume copies its coarse volume and the step size shrinks by the subdivision factor") {
        VALIDATE(fineCells > 0);
        VALIDATE_EQUALS(mismatches, 0);
        VALIDATE_NUMERICAL_EQUALS(
          solver.getTimeStepping().getTimeStepSize(),
          coarseTimeStepSize / peano4::SubdivisionFactor
        );
        VALIDATE_NUMERICAL_EQUALS(
          solver.getTimeStepping().getMinMeshWidth(),
          CoarseWidth / (peano4::SubdivisionFactor * PatchSize)
        );
        VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth / PatchSize);
      }

      AND_WHEN("The solver performs a time step") {
        requests.clear(cellConnectivity.getNumberOfSlots());
        solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
        finishStep(solver);
        const int mismatchesAfterTheTimeStep = peano4::unittests::getGlobalSumOf(
          getNumberOfMismatchesAgainstTheProfileOfLevel(solver, cellConnectivity, Depth)
        );

        THEN("The step has the fine step size and keeps every value") {
          VALIDATE_NUMERICAL_EQUALS(
            solver.getTimeStepping().getTimeStepSizeOfLastStep(),
            coarseTimeStepSize / peano4::SubdivisionFactor
          );
          VALIDATE_EQUALS(mismatchesAfterTheTimeStep, 0);
        }

        AND_WHEN("Every fine cell erases back") {
          regridWithLiveData(solver, peano4::unittests::getErasureOfEveryCell(Depth), requests);
          const int cells                   = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
          const int mismatchesAfterTheErase = peano4::unittests::getGlobalSumOf(
            getNumberOfMismatchesAgainstTheProfileOfLevel(solver, cellConnectivity, Depth)
          );

          THEN("The restriction returns the coarse profile within rounding and the coarse step size") {
            VALIDATE_EQUALS(cells, RegularCells);
            VALIDATE_EQUALS(mismatchesAfterTheErase, 0);
            VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getTimeStepSize(), coarseTimeStepSize);
            VALIDATE_EQUALS(cellConnectivity.getMaximumLevel(), Depth);
          }
        }
      }
    }
    solver.release();
  }
}

UNIT_TEST("A refinement reduces the next adaptive time step to the bound of the smaller volumes") {
  GIVEN("A solver with the adaptive rule and the wave speed one on a regular grid after one time step") {
    const auto&                                             cellConnectivity = buildGrid();
    LinearSolverFor<exahype2::TimeStepping::Rule::Adaptive> solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    peano4::Spacetree::getInstance().markPersistentDataLive();

    WHEN("The cell in the centre refines and the solver performs the next time step") {
      regridWithLiveData(solver, {peano4::unittests::getRefinementOfCentreCell(Depth)}, requests);
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);

      THEN("The step size is the time step factor times the fine volume width over the number of dimensions") {
        VALIDATE_NUMERICAL_EQUALS(
          solver.getTimeStepping().getTimeStepSizeOfLastStep(),
          TimeStepFactor * CoarseWidth / (PatchSize * peano4::SubdivisionFactor * DIMENSIONS)
        );
      }
    }
    solver.release();
  }
}

UNIT_TEST("The postprocessing of the updated patch writes every volume and sets the size of the next time step") {
  GIVEN("A solver whose postprocessing writes twenty with the wave speed two, initialised on a regular grid") {
    const auto&          cellConnectivity = buildGrid();
    PostprocessingSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    const int initialValuesWithoutPostprocessing =
      peano4::unittests::getGlobalSumOf(getNumberOfValuesWithoutPostprocessing(solver, cellConnectivity));

    WHEN("The solver performs one time step") {
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);
      const int valuesWithoutPostprocessing =
        peano4::unittests::getGlobalSumOf(getNumberOfValuesWithoutPostprocessing(solver, cellConnectivity));

      THEN("Every volume stores the postprocessed value and its wave speed sets the size of the next step") {
        VALIDATE(initialValuesWithoutPostprocessing > 0, initialValuesWithoutPostprocessing);
        VALIDATE_EQUALS(valuesWithoutPostprocessing, 0);
        VALIDATE_NUMERICAL_EQUALS(
          solver.getTimeStepping().getTimeStepSizeOfNextStep(),
          TimeStepFactor * CoarseWidth / (PatchSize * DIMENSIONS * PostprocessedWaveSpeed)
        );
      }
    }
    solver.release();
  }
}

UNIT_TEST(
  "A plot writes the file of every rank and allocates host copies of the solution and the halo only with a GPU backend"
) {
  GIVEN("A solver initialised on a regular grid over every rank and an output directory") {
    const auto&  cellConnectivity = buildGrid();
    const auto&  rank             = tarch::mpi::Rank::getInstance();
    LinearSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "FiniteVolumeSolverPlotTest").string() + "/";
    const std::string           vtuName = "LinearSolver-0_000000-rank-" + std::to_string(rank.getRank()) + ".vtu";
    const std::filesystem::path vtuPath = std::filesystem::path(outputDirectory) / "patch" / vtuName;
    std::filesystem::create_directories(outputDirectory + "patch/");

    WHEN("The solver plots its patches and every rank downloads its solution and looks for its file") {
      const tarch::AllocatedBytes bytesBeforeThePlot = solver.getAllocatedBytes();
      solver.plot(cellConnectivity, tarch::plotter::Format::VTU, outputDirectory, RuntimeParameters{});
      const tarch::AllocatedBytes bytesAfterThePlot = solver.getAllocatedBytes();
      const auto&                 solution          = solver.getSolution();
      LinearSolver::Patches       solutionCopy(solution.getChunkSize(), tarch::MemoryLocation::Heap);
      static_cast<void>(solution.download(solutionCopy));
      LinearSolver::Faces haloCopy(solver.getHalo().getChunkSize(), tarch::MemoryLocation::Heap);
      static_cast<void>(solver.getHalo().download(haloCopy));
      const bool fileExists = std::filesystem::exists(vtuPath);
      tarch::mpi::barrier(rank.getCommunicator());
      if (rank.isGlobalMaster()) {
        std::filesystem::remove_all(outputDirectory);
      }

      THEN("The file exists, the host bytes grow by the host copies and the device bytes stay") {
        VALIDATE(fileExists, vtuPath.string());
        VALIDATE_EQUALS(
          bytesAfterThePlot.host,
          bytesBeforeThePlot.host + solutionCopy.getAllocatedBytes().host + haloCopy.getAllocatedBytes().host
        );
        VALIDATE_EQUALS(bytesAfterThePlot.device, bytesBeforeThePlot.device);
      }
    }
    solver.release();
  }
}

UNIT_TEST("The volume centres interpolate a linear profile across the patch faces") {
  GIVEN("A linear profile on the patch of the unit cell and on the volumes beyond its faces") {
    using Patch                   = LinearSolver::Patch;
    constexpr int HaloSize        = Patch::HaloSize;
    constexpr int ExtendedPerAxis = PatchSize + 2;
    const auto    getVolumeCentre = [](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      tarch::la::Vector<DIMENSIONS, double> x;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        x(axis) = (volume(axis) + 0.5) / PatchSize;
      }
      return x;
    };

    std::vector<double> patch(ValuesPerPatch);
    peano4::dForEach(PatchSize, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      patch[static_cast<std::size_t>(peano4::dLinearised(volume, PatchSize))] =
        LinearSolver::getProfile(getVolumeCentre(volume));
    });
    std::vector<std::vector<double>> faces(peano4::FacesPerCell, std::vector<double>(Patch::ValuesPerFace));
    const double*                    facePointers[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      const int                          normal = face % DIMENSIONS;
      const int                          shift  = face < DIMENSIONS ? -HaloSize : PatchSize - HaloSize;
      tarch::la::Vector<DIMENSIONS, int> extent(PatchSize);
      extent(normal) = 2 * HaloSize;
      peano4::dForEach(extent, [&](const tarch::la::Vector<DIMENSIONS, int>& layer) {
        auto volume = layer;
        volume(normal) += shift;
        faces[static_cast<std::size_t>(face)]
             [static_cast<std::size_t>(exahype2::fv::getFaceVolumeIndex(layer, PatchSize, HaloSize, normal))] =
               LinearSolver::getProfile(getVolumeCentre(volume));
      });
      facePointers[face] = faces[static_cast<std::size_t>(face)].data();
    }

    WHEN("The solver evaluates the plot points of the patch") {
      std::vector<double> values(static_cast<std::size_t>(peano4::power(ExtendedPerAxis, DIMENSIONS)));
      LinearSolver::evaluateVolumeCentres(patch.data(), facePointers, values.data());

      THEN("Every point at a volume centre, on a face and in a corner receives the profile") {
        for (int index = 0; index < peano4::power(ExtendedPerAxis, DIMENSIONS); index++) {
          const auto                            point = peano4::dDelinearised(index, ExtendedPerAxis);
          tarch::la::Vector<DIMENSIONS, double> x;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            x(axis) = std::clamp((point(axis) - 0.5) / PatchSize, 0.0, 1.0);
          }
          VALIDATE_NUMERICAL_EQUALS(values[static_cast<std::size_t>(index)], LinearSolver::getProfile(x), index, x);
        }
      }
    }
  }
}

UNIT_TEST("A release frees every pool that the allocation reserved") {
  GIVEN("A solver and a regular grid") {
    const auto&  cellConnectivity = buildGrid();
    LinearSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    const std::size_t bytesBeforeTheAllocation = solver.getAllocatedBytes().getTotal();

    WHEN("The solver allocates its pools on the grid and releases them") {
      solver.allocate(cellConnectivity, RuntimeParameters{});
      const tarch::AllocatedBytes bytesAfterTheAllocation = solver.getAllocatedBytes();
      const tarch::AllocatedBytes solutionBytes           = solver.getSolution().getAllocatedBytes();
      solver.release();

      THEN("The pools occupy the bytes of the solution and more between the allocation and the release only") {
        VALIDATE_EQUALS(bytesBeforeTheAllocation, std::size_t{0});
        VALIDATE(solutionBytes.getTotal() > 0);
        VALIDATE(bytesAfterTheAllocation.host >= solutionBytes.host);
        VALIDATE(bytesAfterTheAllocation.device >= solutionBytes.device);
        VALIDATE_EQUALS(solver.getAllocatedBytes().getTotal(), std::size_t{0});
      }
    }
  }
}
