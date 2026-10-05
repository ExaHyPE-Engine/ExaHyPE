// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/dg/Basis.h"
#include "exahype2/dg/rk/Solver.h"
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
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/Format.h"

namespace {
  TARGET_DECLARE
  constexpr int    SubdivisionFactor = peano4::SubdivisionFactor;
  constexpr int    Depth             = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;
  constexpr int    CellsPerAxis      = peano4::power(SubdivisionFactor, Depth);
  constexpr int    RegularCells      = peano4::power(CellsPerAxis, DIMENSIONS);
  constexpr double CoarseWidth       = 1.0 / CellsPerAxis;
  constexpr int    Order             = 1;
  constexpr int    RungeKuttaOrder   = 2;
  constexpr int    NodesPerAxis      = Order + 1;
  constexpr int    NodesInCell       = exahype2::dg::getNodesPerCell(NodesPerAxis);
  constexpr double TimeStepFactor    = 0.5;
  constexpr double FixedTimeStepSize = TimeStepFactor / (RungeKuttaOrder * RungeKuttaOrder);
  constexpr double Tolerance         = 1.0e-12;
  TARGET_DECLARE_END

  const double QuadratureNodes[NodesPerAxis]   = {0.5 - 0.5 / std::sqrt(3.0), 0.5 + 0.5 / std::sqrt(3.0)};
  const double QuadratureWeights[NodesPerAxis] = {0.5, 0.5};

  /**
   * Returns the Lagrange basis function of node at x.
   */
  double getBasisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NodesPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

  /**
   * Returns the constant derivative of the linear Lagrange basis function of node.
   */
  double getBasisDerivative(const int node) {
    const int other = 1 - node;
    return 1.0 / (QuadratureNodes[node] - QuadratureNodes[other]);
  }

  /**
   * Returns a new device table with the values converted to Precision.
   */
  template <typename Precision>
  Precision* getTableOf(const std::vector<double>& values) {
    std::vector<Precision> converted(values.size());
    for (std::size_t entry = 0; entry < values.size(); entry++) {
      converted[entry] = static_cast<Precision>(values[entry]);
    }
    auto* const table = tarch::allocateMemory<Precision>(values.size(), tarch::MemoryLocation::Device);
    tarch::copyTo(table, converted.data(), converted.size(), tarch::MemoryLocation::Device);
    return table;
  }

  template <typename Precision>
  void dropTable(Precision*& table) {
    tarch::freeMemory(table, tarch::MemoryLocation::Device);
    table = nullptr;
  }

  std::vector<double> getTensorProduct(const std::vector<double>& entries, const int dimensions) {
    std::vector<double> result{1.0};
    for (int dimension = 0; dimension < dimensions; dimension++) {
      std::vector<double> next;
      for (const double left : result) {
        for (const double right : entries) {
          next.push_back(left * right);
        }
      }
      result = next;
    }
    return result;
  }

  /**
   * The Gauss-Legendre quadrature tables of order one in the layout of the generated tables.
   */
  template <typename Precision>
  class Quadrature final {
  public:
    static void initialise() {
      const std::vector<double> weights(QuadratureWeights, QuadratureWeights + NodesPerAxis);
      std::vector<double>       inverseWeights;
      inverseWeights.reserve(weights.size());
      for (const double weight : weights) {
        inverseWeights.push_back(1.0 / weight);
      }
      std::vector<double> inverseCellWeights;
      for (const double weight : getTensorProduct(weights, DIMENSIONS)) {
        inverseCellWeights.push_back(1.0 / weight);
      }
      QuadraturePoints1D             = getTableOf<Precision>({QuadratureNodes, QuadratureNodes + NodesPerAxis});
      QuadratureWeights1D            = getTableOf<Precision>(weights);
      InverseQuadratureWeights1D     = getTableOf<Precision>(inverseWeights);
      QuadratureWeightsOnFace        = getTableOf<Precision>(getTensorProduct(weights, DIMENSIONS - 1));
      QuadratureWeightsInCell        = getTableOf<Precision>(getTensorProduct(weights, DIMENSIONS));
      InverseQuadratureWeightsInCell = getTableOf<Precision>(inverseCellWeights);
    }

    static void deallocate() {
      dropTable(QuadraturePoints1D);
      dropTable(QuadratureWeights1D);
      dropTable(InverseQuadratureWeights1D);
      dropTable(QuadratureWeightsOnFace);
      dropTable(QuadratureWeightsInCell);
      dropTable(InverseQuadratureWeightsInCell);
    }

    inline static Precision* QuadraturePoints1D             = nullptr;
    inline static Precision* QuadratureWeights1D            = nullptr;
    inline static Precision* InverseQuadratureWeights1D     = nullptr;
    inline static Precision* QuadratureWeightsOnFace        = nullptr;
    inline static Precision* QuadratureWeightsInCell        = nullptr;
    inline static Precision* InverseQuadratureWeightsInCell = nullptr;
  };

  /**
   * The Gauss-Legendre operators of order one in the layout of the generated tables.
   */
  template <typename Precision>
  class DGMatrices final {
  public:
    static void initialise() {
      std::vector<double> derivative(static_cast<std::size_t>(NodesPerAxis) * NodesPerAxis);
      std::vector<double> left(NodesPerAxis);
      std::vector<double> right(NodesPerAxis);
      for (int row = 0; row < NodesPerAxis; row++) {
        left[static_cast<std::size_t>(row)]  = getBasisValue(0.0, row);
        right[static_cast<std::size_t>(row)] = getBasisValue(1.0, row);
        for (int column = 0; column < NodesPerAxis; column++) {
          derivative[static_cast<std::size_t>(row) * NodesPerAxis + column] = getBasisDerivative(column);
        }
      }
      const auto prolongation = exahype2::dg::transposePerChild<NodesPerAxis>(
        exahype2::dg::getInterpolationMatrix1D<NodesPerAxis>(QuadratureNodes)
      );
      const auto restriction = exahype2::dg::transposePerChild<NodesPerAxis>(
        exahype2::dg::getWeightedRestrictionMatrix1D<NodesPerAxis>(QuadratureNodes, QuadratureWeights)
      );
      DerivativeOperator1D        = getTableOf<Precision>(derivative);
      BasisFunctionValuesLeft1D   = getTableOf<Precision>(left);
      BasisFunctionValuesRight1D  = getTableOf<Precision>(right);
      ProlongationMatrix1D        = getTableOf<Precision>({prolongation.begin(), prolongation.end()});
      WeightedRestrictionMatrix1D = getTableOf<Precision>({restriction.begin(), restriction.end()});
    }

    static void deallocate() {
      dropTable(DerivativeOperator1D);
      dropTable(BasisFunctionValuesLeft1D);
      dropTable(BasisFunctionValuesRight1D);
      dropTable(ProlongationMatrix1D);
      dropTable(WeightedRestrictionMatrix1D);
    }

    inline static Precision* DerivativeOperator1D        = nullptr;
    inline static Precision* BasisFunctionValuesLeft1D   = nullptr;
    inline static Precision* BasisFunctionValuesRight1D  = nullptr;
    inline static Precision* ProlongationMatrix1D        = nullptr;
    inline static Precision* WeightedRestrictionMatrix1D = nullptr;
  };

  /**
   * The PDE terms of every test solver without flux and wave speed.
   */
  template <typename Solver>
  struct CommonTerms {
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
      return 0.0;
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

  /**
   * The layout and the linear initial condition of the fixed-rule test solvers. Without flux and
   * wave speed a step changes nothing.
   */
  struct FixedRuleTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  RungeKuttaOrder            = ::RungeKuttaOrder;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

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
  };

  /**
   * A solver with the linear initial condition.
   */
  struct FixedRuleSolver final : exahype2::dg::rk::Solver<FixedRuleSolver, double, double>,
                                 CommonTerms<FixedRuleSolver>,
                                 FixedRuleTerms {
    template <typename Precision>
    using Quadrature = ::Quadrature<Precision>;
    template <typename Precision>
    using DGMatrices = ::DGMatrices<Precision>;

    FixedRuleSolver() :
      Solver(TimeStepFactor, "FixedRuleSolver", {{"q", 1}}) {}
  };

  /**
   * A solver with the linear initial condition whose plots write every face.
   */
  struct FacePlottingFixedRuleSolver final : exahype2::dg::rk::Solver<FacePlottingFixedRuleSolver, double, double>,
                                             CommonTerms<FacePlottingFixedRuleSolver>,
                                             FixedRuleTerms {
    template <typename Precision>
    using Quadrature = ::Quadrature<Precision>;
    template <typename Precision>
    using DGMatrices = ::DGMatrices<Precision>;

    FacePlottingFixedRuleSolver() :
      Solver(TimeStepFactor, "FixedRuleSolver", {{"q", 1}}) {}

    static bool plotsFace(
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const int                                    normal
    ) {
      return true;
    }
  };

  /**
   * A solver with a constant state, an adaptive rule and a unit wave speed.
   */
  struct AdaptiveSolver final : exahype2::dg::rk::Solver<AdaptiveSolver, double, double>, CommonTerms<AdaptiveSolver> {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  RungeKuttaOrder            = ::RungeKuttaOrder;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Adaptive;

    template <typename Precision>
    using Quadrature = ::Quadrature<Precision>;
    template <typename Precision>
    using DGMatrices = ::DGMatrices<Precision>;

    AdaptiveSolver() :
      Solver(TimeStepFactor, "AdaptiveSolver", {{"q", 1}}) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                                 values,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& width
    ) {
      values[0] = 1.0;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* const                          values,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& width,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal
    ) {
      return 1.0;
    }
  };

  /**
   * A solver with a constant initial condition and a unit source. A step adds the time step size.
   */
  struct SourceSolver final : exahype2::dg::rk::Solver<SourceSolver, double, double>, CommonTerms<SourceSolver> {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  RungeKuttaOrder            = ::RungeKuttaOrder;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    template <typename Precision>
    using Quadrature = ::Quadrature<Precision>;
    template <typename Precision>
    using DGMatrices = ::DGMatrices<Precision>;

    static constexpr double InitialValue = 1.0;

    SourceSolver() :
      Solver(TimeStepFactor, "SourceSolver", {{"q", 1}}) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                                 Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      Q[0] = InitialValue;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      double* const                                                 S
    ) {
      S[0] = 1.0;
    }
  };

  const peano4::Partition& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getPartition();
  }

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
    limits.maxAdmissibleMeshWidth = CoarseWidth;
    return limits;
  }

  TARGET_DECLARE
  /**
   * Increments the accumulator for every node of one cell whose value differs from the expected
   * value there.
   */
  template <typename Pool, typename Expected>
  struct CountMismatchesAgainstExpected final {
    peano4::Partition        partition{};
    typename Pool::ConstView solution{};
    const double*            quadraturePoints{nullptr};
    Expected                 expected{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const int                             cell = partition.getCells()[entry];
      tarch::la::Vector<DIMENSIONS, double> centre;
      tarch::la::Vector<DIMENSIONS, double> width;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        centre(axis) = partition.getCellCentre(cell, axis);
        width(axis)  = partition.getCellWidth(cell, axis);
      }
      const double* const values = solution.getChunk(cell);
      peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
        const auto position = exahype2::dg::getQuadraturePoint(centre, width, node, quadraturePoints);
        if (std::abs(values[peano4::dLinearised(node, NodesPerAxis)] - expected(position)) > Tolerance) {
          accumulator++;
        }
      });
    }
  };
  TARGET_DECLARE_END

  template <typename Solver, typename Expected>
  int getNumberOfMismatches(const Solver& solver, const peano4::Partition& partition, Expected expected) {
    int result = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getCells().size),
      CountMismatchesAgainstExpected<typename Solver::CellPool, Expected>{
        partition.onDevice(),
        solver.getSolution().getView(),
        Quadrature<double>::QuadraturePoints1D,
        expected
      },
      tarch::accelerator::Sum<int>{result}
    );
    return result;
  }

  struct LinearProfile final {
    GPU_CALLABLE_METHOD double operator()(const tarch::la::Vector<DIMENSIONS, double>& x) const {
      return FixedRuleSolver::getProfile(x);
    }
  };

  struct ConstantProfile final {
    double value{0.0};

    GPU_CALLABLE_METHOD double operator()([[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x) const {
      return value;
    }
  };

  int getNumberOfMismatchesAgainstTheProfile(const FixedRuleSolver& solver, const peano4::Partition& partition) {
    return getNumberOfMismatches(solver, partition, LinearProfile{});
  }
} // namespace

exahype2::dg::rk::tests::SolverTest::SolverTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::SolverTest") {}

void
exahype2::dg::rk::tests::SolverTest::run() {
  testMethod(testInitialisationWritesTheInitialConditionIntoEveryNode);
  testMethod(testATimeStepWithoutFluxKeepsTheSolutionAndAdvancesTheTimeStamp);
  testMethod(testASourceTermAdvancesEveryNodeByTheTimeStep);
  testMethod(testARefinementAndTheEraseBackKeepTheSolution);
  testMethod(testRefinementReducesTheNextAdaptiveTimeStep);
  testMethod(testAPlotAllocatesHostCopiesOnlyWithAGPUBackend);
  testMethod(testReleaseFreesEveryPool);
}

/**
 * The first time step after a refinement uses the stability bound of the finer grid.
 */
void
exahype2::dg::rk::tests::SolverTest::testRefinementReducesTheNextAdaptiveTimeStep() {
  const auto&    partition = buildGrid();
  AdaptiveSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  solver.performTimeStep(partition, requests);
  requests.clear(partition.getNumberOfSlots());
  solver.performTimeStep(partition, requests);
  const double coarseTimeStepSize = solver.getTimeStepping().getTimeStepSize();
  peano4::Spacetree::getInstance().markPersistentDataLive();
  regridWithLiveData(solver, {peano4::tests::getRefinementOfCentreCell(Depth)}, requests);
  requests.clear(partition.getNumberOfSlots());
  solver.performTimeStep(partition, requests);
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSizeOfLastStep(), coarseTimeStepSize / SubdivisionFactor);
  solver.release();
}

void
exahype2::dg::rk::tests::SolverTest::testInitialisationWritesTheInitialConditionIntoEveryNode() {
  const auto& partition = buildGrid();

  FixedRuleSolver solver;
  auto            constructionLimits        = getLimitsOfTheDomain();
  constructionLimits.maxAdmissiblePatchSize = CoarseWidth / SubdivisionFactor;
  solver.setLimits(constructionLimits);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.requestRefinementForGridConstruction(partition, requests);
  validateEquals(static_cast<int>(requests.getRefinementRequests(partition).size()), RegularCells);
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  requests.clear(partition.getNumberOfSlots());
  requests.merge(exahype2::RefinementCommand::Erase, partition.getCells());
  solver.initialise(partition, requests);

  validateEquals(getNumberOfMismatchesAgainstTheProfile(solver, partition), 0);
  validateEquals(solver.getNumberOfDoFs(), NodesInCell);
  validateEquals(peano4::tests::getGlobalSumOf(partition.getCells().size), RegularCells);
  validateNumericalEquals(solver.getTimeStepping().getMinMeshWidth(), CoarseWidth);
  validateNumericalEquals(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth);
  validate(solver.getEstimates().getCapacity() >= partition.getNumberOfSlots());
  validate(solver.getAllocatedBytes().getTotal() > 0);
  validate(requests.getRefinementRequests(partition).empty());
  solver.release();
}

/**
 * A time step without flux keeps the solution, and the time stamp advances by the fixed step size
 * after a first step of size zero.
 */
void
exahype2::dg::rk::tests::SolverTest::testATimeStepWithoutFluxKeepsTheSolutionAndAdvancesTheTimeStamp() {
  const auto& partition = buildGrid();

  FixedRuleSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSize(), 0.0);

  for (int step = 0; step < 3; step++) {
    requests.clear(partition.getNumberOfSlots());
    solver.performTimeStep(partition, requests);
    const double expectedStepSize = step == 0 ? 0.0 : FixedTimeStepSize;
    validateEqualsWithParams1(getNumberOfMismatchesAgainstTheProfile(solver, partition), 0, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStamp(), step * FixedTimeStepSize, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStepSizeOfLastStep(), expectedStepSize, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStepSize(), FixedTimeStepSize, step);
  }
  validate(requests.getRefinementRequests(partition).empty());
  solver.release();
}

/**
 * A unit source advances every node by the time step size.
 */
void
exahype2::dg::rk::tests::SolverTest::testASourceTermAdvancesEveryNodeByTheTimeStep() {
  const auto& partition = buildGrid();

  SourceSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);

  for (int step = 0; step < 3; step++) {
    requests.clear(partition.getNumberOfSlots());
    solver.performTimeStep(partition, requests);
    const double expectedValue = SourceSolver::InitialValue + solver.getTimeStepping().getTimeStamp();
    validateEqualsWithParams1(getNumberOfMismatches(solver, partition, ConstantProfile{expectedValue}), 0, step);
  }
  validateNumericalEquals(solver.getTimeStepping().getTimeStamp(), 2 * FixedTimeStepSize);
  solver.release();
}

/**
 * The prolongation reproduces the linear profile on every child, and the restriction of the
 * children returns it on the coarse cell within rounding.
 */
void
exahype2::dg::rk::tests::SolverTest::testARefinementAndTheEraseBackKeepTheSolution() {
  const auto& partition = buildGrid();

  FixedRuleSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  solver.performTimeStep(partition, requests);
  const double coarseTimeStepSize = solver.getTimeStepping().getTimeStepSize();
  peano4::Spacetree::getInstance().markPersistentDataLive();

  regridWithLiveData(solver, {peano4::tests::getRefinementOfCentreCell(Depth)}, requests);
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSize(), coarseTimeStepSize / SubdivisionFactor);
  validate(peano4::tests::getGlobalSumOf(getNumberOfCellsOfLevel(partition, Depth + 1)) > 0);
  validateEquals(getNumberOfMismatchesAgainstTheProfile(solver, partition), 0);
  validate(solver.getEstimates().getCapacity() >= partition.getNumberOfSlots());
  validateNumericalEquals(solver.getTimeStepping().getMinMeshWidth(), CoarseWidth / SubdivisionFactor);
  validateNumericalEquals(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth);

  requests.clear(partition.getNumberOfSlots());
  solver.performTimeStep(partition, requests);
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSizeOfLastStep(), coarseTimeStepSize / SubdivisionFactor);
  validateEquals(getNumberOfMismatchesAgainstTheProfile(solver, partition), 0);

  regridWithLiveData(solver, peano4::tests::getErasureOfEveryCell(Depth), requests);
  validateNumericalEquals(solver.getTimeStepping().getTimeStepSize(), coarseTimeStepSize);
  validateEquals(partition.getMaximumLevel(), Depth);
  validateEquals(getNumberOfMismatchesAgainstTheProfile(solver, partition), 0);
  validateEquals(peano4::tests::getGlobalSumOf(partition.getCells().size), RegularCells);
  solver.release();
}

/**
 * A plot writes the node file and the face file of every rank with the boundary state in the outer
 * half of every boundary face and allocates host copies only in a build with a GPU backend.
 */
void
exahype2::dg::rk::tests::SolverTest::testAPlotAllocatesHostCopiesOnlyWithAGPUBackend() {
  const auto& partition = buildGrid();
  const auto& rank      = tarch::mpi::Rank::getInstance();

  FacePlottingFixedRuleSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);

  const std::string outputDirectory =
    (tarch::tests::getWritableTestDirectory() / "DiscontinuousGalerkinSolverPlotTest").string() + "/";
  std::filesystem::create_directories(outputDirectory + "patch/");
  std::filesystem::create_directories(outputDirectory + "face/");
  const tarch::AllocatedBytes bytesBeforePlot = solver.getAllocatedBytes();
  solver.plot(partition, tarch::plotter::Format::VTU, outputDirectory);
  const tarch::AllocatedBytes bytesAfterPlot = solver.getAllocatedBytes();

  const auto&                         solution   = solver.getSolution();
  const auto&                         faceStates = solver.getFaceStates();
  FacePlottingFixedRuleSolver::CellPool  solutionCopy(solution.getChunkSize(), tarch::MemoryLocation::Heap);
  FacePlottingFixedRuleSolver::StatePool faceStatesCopy(faceStates.getChunkSize(), tarch::MemoryLocation::Heap);
  static_cast<void>(solution.download(solutionCopy));
  const auto& faceStatesOnHost = faceStates.download(faceStatesCopy);
  validateEquals(
    bytesAfterPlot.host,
    bytesBeforePlot.host + solutionCopy.getAllocatedBytes().host + faceStatesCopy.getAllocatedBytes().host
  );
  validateEquals(bytesAfterPlot.device, bytesBeforePlot.device);
  const std::string           vtuSuffix = "-0_000000-rank-" + std::to_string(rank.getRank()) + ".vtu";
  const std::filesystem::path nodeVtuPath =
    std::filesystem::path(outputDirectory) / "patch" / ("FixedRuleSolver" + vtuSuffix);
  const std::filesystem::path faceVtuPath =
    std::filesystem::path(outputDirectory) / "face" / ("FixedRuleSolver-face" + vtuSuffix);
  validateWithParams1(std::filesystem::exists(nodeVtuPath), nodeVtuPath.string());
  validateWithParams1(std::filesystem::exists(faceVtuPath), faceVtuPath.string());

  constexpr int NodesPerFace = exahype2::dg::getNodesPerFace(NodesPerAxis);
  for (const int face : partition.getFaces(peano4::Adjacency::Local, peano4::FaceKind::Boundary)) {
    const double* const halves = faceStatesOnHost.getChunk(face);
    for (int node = 0; node < NodesPerFace; node++) {
      validateEqualsWithParams2(halves[node], halves[NodesPerFace + node], face, node);
    }
  }
  solver.release();

  tarch::mpi::barrier(rank.getCommunicator());
  if (rank.isGlobalMaster()) {
    std::filesystem::remove_all(outputDirectory);
  }
}

void
exahype2::dg::rk::tests::SolverTest::testReleaseFreesEveryPool() {
  const auto& partition = buildGrid();

  FixedRuleSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
  solver.allocate(partition);
  validate(solver.getAllocatedBytes().getTotal() > 0);
  solver.release();
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
  validateEquals(solver.getEstimates().getCapacity(), 0);
}
