// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/Solver.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/timestepping/SingleStep.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Partition.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
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

namespace {
  TARGET_DECLARE
  constexpr int    Depth          = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;
  constexpr int    CellsPerAxis   = peano4::power(peano4::SubdivisionFactor, Depth);
  constexpr int    RegularCells   = peano4::power(CellsPerAxis, DIMENSIONS);
  constexpr double CoarseWidth    = 1.0 / CellsPerAxis;
  constexpr int    PatchSize      = 2;
  constexpr int    ValuesPerPatch = peano4::power(PatchSize, DIMENSIONS);
  constexpr double TimeStepFactor = 0.5;
  constexpr double Tolerance      = 1.0e-12;
  TARGET_DECLARE_END

  /**
   * LinearSolverFor evolves one unknown on constant subcells from a linear initial condition without
   * flux. Under the fixed rule it reports no wave speed.
   */
  template <exahype2::TimeStepping::Rule Rule>
  struct LinearSolverFor final : exahype2::fv::Solver<LinearSolverFor<Rule>, double, double> {
    using Base = exahype2::fv::Solver<LinearSolverFor<Rule>, double, double>;

    static constexpr int PatchSize                  = ::PatchSize;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    using Reconstruction  = exahype2::fv::reconstruction::Constant;
    using TimeIntegration = exahype2::fv::timestepping::SingleStep;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = Rule;

    LinearSolverFor() :
      Base(TimeStepFactor, "LinearSolver", {{"q", 1}}) {}

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
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      Q[0] = getProfile(x);
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      const double* const                                           inside,
      double* const                                                 outside,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const int                                    normal
    ) {
      outside[0] = inside[0];
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t
    ) {
      return exahype2::RefinementCommand::Keep;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal
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
      double* const                                                 F
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
  struct PostprocessingSolver final : exahype2::fv::Solver<PostprocessingSolver, double, double> {
    using Base = exahype2::fv::Solver<PostprocessingSolver, double, double>;

    static constexpr int PatchSize                  = ::PatchSize;
    static constexpr int NumberOfUnknowns           = 1;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    using Reconstruction  = exahype2::fv::reconstruction::Constant;
    using TimeIntegration = exahype2::fv::timestepping::SingleStep;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Adaptive;

    PostprocessingSolver() :
      Base(TimeStepFactor, "PostprocessingSolver", {{"q", 1}}) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                                 Q,
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      Q[0] = LinearSolver::getProfile(x);
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void boundaryConditions(
      const double* const                                           inside,
      double* const                                                 outside,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const int                                    normal
    ) {
      outside[0] = inside[0];
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t
    ) {
      return exahype2::RefinementCommand::Keep;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      const double* const                                           Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal
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
      double* const                                                 F
    ) {
      F[0] = 0.0;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void postprocessUpdatedPatch(
      double* const                                                 Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt
    ) {
      for (int value = 0; value < ValuesPerPatch; value++) {
        Q[value] = PostprocessedValue;
      }
    }
  };

  const peano4::Partition& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getPartition();
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
   * regrid. Every slot the regrid keeps then holds its data. The reduction of the step follows.
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
    const auto& partition = spacetree.getPartition();
    requests.clear(partition.getNumberOfSlots());
    solver.regrid(partition, regrid, requests);
    spacetree.finishRegrid();
    finishStep(solver);
  }

  int getNumberOfCellsOfLevel(const peano4::Partition& partition, const int level) {
    return partition.getCells(peano4::Adjacency::Local, level).size
      + partition.getCells(peano4::Adjacency::Remote, level).size;
  }

  exahype2::Limits getLimitsOfTheDomain() {
    exahype2::Limits limits;
    limits.terminalTime           = 1.0;
    limits.minAdmissiblePatchSize = 0.0;
    limits.maxAdmissiblePatchSize = 1.0;
    limits.maxAdmissibleMeshWidth = CoarseWidth / PatchSize;
    return limits;
  }

  TARGET_DECLARE
  /**
   * Adds one to the accumulator for every volume of one cell whose value differs from the profile
   * at the centre of the volume of level that covers it.
   */
  struct CountMismatchesAgainstTheProfile final {
    peano4::Partition                partition{};
    LinearSolver::Patches::ConstView solution{};
    int                              level{0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const int                             cell      = partition.getCells()[entry];
      const int                             cellLevel = partition.getCellLevel(cell);
      const auto                            lattice   = partition.getCellLattice(cell);
      tarch::la::Vector<DIMENSIONS, double> coarseCentre;
      tarch::la::Vector<DIMENSIONS, double> coarseWidth;
      tarch::la::Vector<DIMENSIONS, int>    childPosition(0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (cellLevel == level) {
          coarseCentre(axis) = partition.getCellCentre(cell, axis);
          coarseWidth(axis)  = partition.getCellWidth(cell, axis);
        } else {
          const int parentLattice = lattice(axis) / peano4::SubdivisionFactor;
          coarseWidth(axis)       = partition.getWidthOfLevel(level, axis);
          coarseCentre(axis)      = partition.getDomainOffset()(axis) + (parentLattice + 0.5) * coarseWidth(axis);
          childPosition(axis)     = lattice(axis) % peano4::SubdivisionFactor;
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
        const auto centre = exahype2::fv::getVolumeCentre(coarseCentre, coarseWidth, PatchSize, coarseVolume);
        if (
          std::abs(values[peano4::dLinearised(fineVolume, PatchSize)] - LinearSolver::getProfile(centre)) > Tolerance
        ) {
          accumulator++;
        }
      });
    }
  };

  /**
   * Adds one to the accumulator for every value of one cell that differs from PostprocessedValue.
   */
  struct CountValuesWithoutPostprocessing final {
    peano4::Partition                        partition{};
    PostprocessingSolver::Patches::ConstView solution{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const int cell = partition.getCells()[entry];
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
    const SolverType&        solver,
    const peano4::Partition& partition,
    const int                level
  ) {
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getCells().size),
      CountMismatchesAgainstTheProfile{partition.onDevice(), solver.getSolution().getView(), level},
      tarch::accelerator::Sum<int>{mismatches}
    );
    return mismatches;
  }

  /**
   * Counts the values of every cell that the patch postprocessing has not overwritten.
   */
  int getNumberOfValuesWithoutPostprocessing(const PostprocessingSolver& solver, const peano4::Partition& partition) {
    int values = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getCells().size),
      CountValuesWithoutPostprocessing{partition.onDevice(), solver.getSolution().getView()},
      tarch::accelerator::Sum<int>{values}
    );
    return values;
  }
} // namespace

exahype2::fv::tests::SolverTest::SolverTest() :
  tarch::tests::TestCase("exahype2::fv::tests::SolverTest") {}

void
exahype2::fv::tests::SolverTest::run() {
  testMethod(testInitialisationWritesTheInitialConditionIntoEveryVolume);
  testMethod(testATimeStepWithoutFluxKeepsTheSolutionAndAdvancesTheTimeStamp);
  testMethod(testARefinementAndTheEraseBackKeepTheSolution);
  testMethod(testRefinementReducesTheNextAdaptiveTimeStep);
  testMethod(testThePostprocessingOfTheUpdatedPatchFeedsTheNextTimeStep);
  testMethod(testAPlotAllocatesHostCopiesOnlyWithAGPUBackend);
  testMethod(testReleaseFreesEveryPool);
}

/**
 * After one step every volume holds PostprocessedValue and PostprocessedWaveSpeed sets the size
 * of the next step.
 */
void
exahype2::fv::tests::SolverTest::testThePostprocessingOfTheUpdatedPatchFeedsTheNextTimeStep() {
  const auto&          partition = buildGrid();
  PostprocessingSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  finishStep(solver);
  validate(getNumberOfValuesWithoutPostprocessing(solver, partition) > 0);
  solver.performTimeStep(partition, requests);
  finishStep(solver);
  validateEquals(getNumberOfValuesWithoutPostprocessing(solver, partition), 0);
  validateNumericalEquals(
    solver.getTimeStepping().getTimeStepSizeOfNextStep(),
    TimeStepFactor * CoarseWidth / (PatchSize * DIMENSIONS * PostprocessedWaveSpeed)
  );
  solver.release();
}

/**
 * A refinement shrinks the next adaptive time step with the smaller finite volumes.
 */
void
exahype2::fv::tests::SolverTest::testRefinementReducesTheNextAdaptiveTimeStep() {
  const auto&                                             partition = buildGrid();
  LinearSolverFor<exahype2::TimeStepping::Rule::Adaptive> solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  finishStep(solver);
  solver.performTimeStep(partition, requests);
  finishStep(solver);
  peano4::Spacetree::getInstance().markPersistentDataLive();
  regridWithLiveData(solver, {peano4::tests::getRefinementOfCentreCell(Depth)}, requests);
  requests.clear(partition.getNumberOfSlots());
  solver.performTimeStep(partition, requests);
  finishStep(solver);
  validateNumericalEquals(
    solver.getTimeStepping().getTimeStepSizeOfLastStep(),
    TimeStepFactor * CoarseWidth / (PatchSize * peano4::SubdivisionFactor * DIMENSIONS)
  );
  solver.release();
}

void
exahype2::fv::tests::SolverTest::testInitialisationWritesTheInitialConditionIntoEveryVolume() {
  const auto& partition = buildGrid();

  LinearSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  requests.merge(exahype2::RefinementCommand::Erase, partition.getCells());
  solver.initialise(partition, requests);
  finishStep(solver);

  validateEquals(getNumberOfMismatchesAgainstTheProfileOfLevel(solver, partition, Depth), 0);
  validateEquals(solver.getNumberOfDoFs(), ValuesPerPatch);
  validateEquals(peano4::tests::getGlobalSumOf(partition.getCells().size), RegularCells);
  validateNumericalEquals(solver.getTimeStepping().getMinMeshWidth(), CoarseWidth / PatchSize);
  validateNumericalEquals(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth / PatchSize);
  validate(solver.getAllocatedBytes().getTotal() > 0);
  validate(requests.getRefinementRequests(partition).empty());
  solver.release();
}

/**
 * The first step runs with a zero step size. Every later one advances by the factor times the
 * mesh width over the admissible width.
 */
void
exahype2::fv::tests::SolverTest::testATimeStepWithoutFluxKeepsTheSolutionAndAdvancesTheTimeStamp() {
  const auto& partition = buildGrid();

  LinearSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  finishStep(solver);
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSize(), 0.0);

  for (int step = 0; step < 3; step++) {
    requests.clear(partition.getNumberOfSlots());
    solver.performTimeStep(partition, requests);
    finishStep(solver);
    const double expectedStepSize = step == 0 ? 0.0 : TimeStepFactor;
    validateEqualsWithParams1(getNumberOfMismatchesAgainstTheProfileOfLevel(solver, partition, Depth), 0, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStamp(), step * TimeStepFactor, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStepSizeOfLastStep(), expectedStepSize, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStepSize(), TimeStepFactor, step);
  }
  validate(requests.getRefinementRequests(partition).empty());
  solver.release();
}

/**
 * A refinement and the erase back return the coarse solution within rounding.
 */
void
exahype2::fv::tests::SolverTest::testARefinementAndTheEraseBackKeepTheSolution() {
  const auto& partition = buildGrid();

  LinearSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  finishStep(solver);
  solver.performTimeStep(partition, requests);
  finishStep(solver);
  const double coarseTimeStepSize = solver.getTimeStepping().getTimeStepSize();
  peano4::Spacetree::getInstance().markPersistentDataLive();

  regridWithLiveData(solver, {peano4::tests::getRefinementOfCentreCell(Depth)}, requests);
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSize(), coarseTimeStepSize / peano4::SubdivisionFactor);
  validate(peano4::tests::getGlobalSumOf(getNumberOfCellsOfLevel(partition, Depth + 1)) > 0);
  validateEquals(getNumberOfMismatchesAgainstTheProfileOfLevel(solver, partition, Depth), 0);
  validateNumericalEquals(
    solver.getTimeStepping().getMinMeshWidth(),
    CoarseWidth / (peano4::SubdivisionFactor * PatchSize)
  );
  validateNumericalEquals(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth / PatchSize);

  requests.clear(partition.getNumberOfSlots());
  solver.performTimeStep(partition, requests);
  finishStep(solver);
  validateNumericalEquals(
    solver.getTimeStepping().getTimeStepSizeOfLastStep(),
    coarseTimeStepSize / peano4::SubdivisionFactor
  );
  validateEquals(getNumberOfMismatchesAgainstTheProfileOfLevel(solver, partition, Depth), 0);

  regridWithLiveData(solver, peano4::tests::getErasureOfEveryCell(Depth), requests);
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSize(), coarseTimeStepSize);
  validateEquals(partition.getMaximumLevel(), Depth);
  validateEquals(getNumberOfMismatchesAgainstTheProfileOfLevel(solver, partition, Depth), 0);
  validateEquals(peano4::tests::getGlobalSumOf(partition.getCells().size), RegularCells);
  solver.release();
}

/**
 * A plot writes the .vtu file of every rank and allocates a host copy of the solution only in a
 * build with a GPU backend.
 */
void
exahype2::fv::tests::SolverTest::testAPlotAllocatesHostCopiesOnlyWithAGPUBackend() {
  const auto& partition = buildGrid();
  const auto& rank      = tarch::mpi::Rank::getInstance();

  LinearSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  finishStep(solver);

  const std::string outputDirectory =
    (tarch::tests::getWritableTestDirectory() / "FiniteVolumeSolverPlotTest").string() + "/";
  std::filesystem::create_directories(outputDirectory + "patch/");
  const tarch::AllocatedBytes bytesBeforePlot = solver.getAllocatedBytes();
  solver.plot(partition, tarch::plotter::Format::VTU, outputDirectory);
  const tarch::AllocatedBytes bytesAfterPlot = solver.getAllocatedBytes();

  const auto&           solution = solver.getSolution();
  LinearSolver::Patches solutionCopy(solution.getChunkSize(), tarch::MemoryLocation::Heap);
  static_cast<void>(solution.download(solutionCopy));
  validateEquals(bytesAfterPlot.host, bytesBeforePlot.host + solutionCopy.getAllocatedBytes().host);
  validateEquals(bytesAfterPlot.device, bytesBeforePlot.device);
  const std::string           vtuName = "LinearSolver-0_000000-rank-" + std::to_string(rank.getRank()) + ".vtu";
  const std::filesystem::path vtuPath = std::filesystem::path(outputDirectory) / "patch" / vtuName;
  validateWithParams1(std::filesystem::exists(vtuPath), vtuPath.string());
  solver.release();

  tarch::mpi::barrier(rank.getCommunicator());
  if (rank.isGlobalMaster()) {
    std::filesystem::remove_all(outputDirectory);
  }
}

/**
 * The allocated bytes of a solver cover its solution pool until release() frees every byte.
 */
void
exahype2::fv::tests::SolverTest::testReleaseFreesEveryPool() {
  const auto& partition = buildGrid();

  LinearSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
  solver.allocate(partition);
  const tarch::AllocatedBytes solutionBytes = solver.getSolution().getAllocatedBytes();
  validate(solutionBytes.getTotal() > 0);
  validate(solver.getAllocatedBytes().host >= solutionBytes.host);
  validate(solver.getAllocatedBytes().device >= solutionBytes.device);
  solver.release();
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
}
