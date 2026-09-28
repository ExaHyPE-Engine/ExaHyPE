// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/FiniteVolume.h"
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
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"

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
   * A solver with one unknown on constant subcells, a linear initial condition, no flux and, with
   * a fixed rule, no wave speed, so a step changes nothing.
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

  /**
   * Builds the regular grid of Depth levels partitioned over the ranks and returns the partition of this rank.
   */
  const peano4::Partition& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getPartition();
  }

  /**
   * Applies the refinement requests to the standing grid and moves the solver data through the
   * regrid, so every slot the regrid keeps holds its data afterwards.
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
      tarch::terminateWithError("The regrid of the test grid changed nothing.");
    }
    const auto& partition = spacetree.getPartition();
    requests.clear(partition.getNumberOfSlots());
    solver.regrid(partition, regrid, requests);
    spacetree.finishRegrid();
  }

  int getNumberOfCellsOfLevel(const peano4::Partition& partition, const int level) {
    return partition.getCells(peano4::Dependence::Local, level).size
      + partition.getCells(peano4::Dependence::Remote, level).size;
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
   * Raises the accumulator for every volume of one cell whose value differs from the profile at
   * the centre of the volume of level that covers it.
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
   * Raises the accumulator for every value of one cell that differs between the two solutions.
   */
  struct CountDifferencesBetweenSolutions final {
    peano4::Partition                partition{};
    LinearSolver::Patches::ConstView current{};
    LinearSolver::Patches::ConstView previous{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const int cell = partition.getCells()[entry];
      for (int value = 0; value < ValuesPerPatch; value++) {
        if (std::abs(current.getChunk(cell)[value] - previous.getChunk(cell)[value]) > Tolerance) {
          accumulator++;
        }
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Counts the volumes whose value differs from the profile at the centre of the volume of the
   * given level that covers them: The volume itself on its own level, the covering coarse volume
   * for a cell below that level.
   */
  int getNumberOfMismatchesAgainstTheProfileOfLevel(
    const LinearSolver&      solver,
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
   * Counts the values of every cell that differ between the current and the previous solution.
   */
  int getNumberOfDifferencesBetweenSolutions(const LinearSolver& solver, const peano4::Partition& partition) {
    int differences = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getCells().size),
      CountDifferencesBetweenSolutions{
        partition.onDevice(),
        solver.getSolution().getView(),
        solver.getPreviousSolution().getView()
      },
      tarch::accelerator::Sum<int>{differences}
    );
    return differences;
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
  testMethod(testReleaseFreesEveryPool);
}

/**
 * Checks the first time step on a refined grid against its smaller finite volumes.
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
  solver.performTimeStep(partition, requests);
  peano4::Spacetree::getInstance().markPersistentDataLive();
  regridWithLiveData(solver, {peano4::tests::getRefinementOfCentreCell(Depth)}, requests);
  requests.clear(partition.getNumberOfSlots());
  solver.performTimeStep(partition, requests);
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
 * The first step runs with a zero step size; every later one advances by the factor times the
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
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSize(), 0.0);

  for (int step = 0; step < 3; step++) {
    requests.clear(partition.getNumberOfSlots());
    solver.performTimeStep(partition, requests);
    const double expectedStepSize = step == 0 ? 0.0 : TimeStepFactor;
    validateEqualsWithParams1(getNumberOfMismatchesAgainstTheProfileOfLevel(solver, partition, Depth), 0, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStamp(), step * TimeStepFactor, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStepSizeOfLastStep(), expectedStepSize, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStepSize(), TimeStepFactor, step);
  }
  validateEquals(getNumberOfDifferencesBetweenSolutions(solver, partition), 0);
  validate(requests.getRefinementRequests(partition).empty());
  solver.release();
}

/**
 * The prolongation copies the covering coarse volume into every fine volume, and the restriction
 * averages them back, so the coarse solution returns within rounding.
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
  solver.performTimeStep(partition, requests);
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

void
exahype2::fv::tests::SolverTest::testReleaseFreesEveryPool() {
  const auto& partition = buildGrid();

  LinearSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
  solver.allocate(partition);
  validate(solver.getAllocatedBytes().getTotal() > 0);
  solver.release();
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
}
