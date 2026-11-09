// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/rk/Solver.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/dg/Basis.h"
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
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"
#include "tarch/plotter/Format.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/peano4/TestGrids.h"

namespace {
  TARGET_DECLARE
  constexpr int    SubdivisionFactor = peano4::SubdivisionFactor;
  constexpr int    Depth             = peano4::unittests::DepthWithCoarseCellsBesideTheCentreBlock;
  constexpr int    CellsPerAxis      = peano4::power(SubdivisionFactor, Depth);
  constexpr int    RegularCells      = peano4::power(CellsPerAxis, DIMENSIONS);
  constexpr double CoarseWidth       = 1.0 / CellsPerAxis;
  constexpr int    Order             = 1;
  constexpr int    RungeKuttaOrder   = 2;
  constexpr int    NodesPerAxis      = Order + 1;
  constexpr int    NodesInCell       = exahype2::dg::getNodesPerCell(NodesPerAxis);
  constexpr double TimeStepFactor    = 0.5;
  constexpr double FixedTimeStepSize = TimeStepFactor / (RungeKuttaOrder * RungeKuttaOrder);
  TARGET_DECLARE_END

  const double QuadratureNodes[NodesPerAxis]   = {0.5 - 0.5 / std::sqrt(3.0), 0.5 + 0.5 / std::sqrt(3.0)};
  const double QuadratureWeights[NodesPerAxis] = {0.5, 0.5};

  double getBasisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NodesPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

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

  struct RuntimeParameters final {};

  /**
   * The PDE terms of every test solver without flux and wave speed.
   */
  template <typename Solver>
  struct CommonTerms {
    using RuntimeParameters = ::RuntimeParameters;

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
      return 0.0;
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

  /**
   * The layout and the linear initial condition of the fixed-rule test solvers. A step of a solver
   * with these terms alone keeps its state.
   */
  struct FixedRuleTerms {
    static constexpr int                   DGOrder                    = ::Order;
    static constexpr int                   RungeKuttaOrder            = ::RungeKuttaOrder;
    static constexpr int                   NumberOfUnknowns           = 1;
    static constexpr int                   NumberOfAuxiliaryVariables = 0;
    [[maybe_unused]] static constexpr bool IsLinear                   = false;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;
    static constexpr std::array<int, 1>           MeasuredUnknowns{0};

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
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return true;
    }
  };

  /**
   * A solver with a constant state, an adaptive rule and a unit wave speed.
   */
  struct AdaptiveSolver final : exahype2::dg::rk::Solver<AdaptiveSolver, double, double>, CommonTerms<AdaptiveSolver> {
    static constexpr int                   DGOrder                    = ::Order;
    static constexpr int                   RungeKuttaOrder            = ::RungeKuttaOrder;
    static constexpr int                   NumberOfUnknowns           = 1;
    static constexpr int                   NumberOfAuxiliaryVariables = 0;
    [[maybe_unused]] static constexpr bool IsLinear                   = false;

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
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& width,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      values[0] = 1.0;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* const                          values,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& width,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return 1.0;
    }
  };

  /**
   * A solver with a constant initial condition and a unit source. A step adds the time step size.
   */
  struct SourceSolver final : exahype2::dg::rk::Solver<SourceSolver, double, double>, CommonTerms<SourceSolver> {
    static constexpr int                   DGOrder                    = ::Order;
    static constexpr int                   RungeKuttaOrder            = ::RungeKuttaOrder;
    static constexpr int                   NumberOfUnknowns           = 1;
    static constexpr int                   NumberOfAuxiliaryVariables = 0;
    [[maybe_unused]] static constexpr bool IsLinear                   = false;

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
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      Q[0] = InitialValue;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      double* const                                                 S,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      S[0] = 1.0;
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
    limits.maxAdmissibleMeshWidth = CoarseWidth;
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
   * Increments the accumulator for every node of one cell whose value differs from the expected
   * value there.
   */
  template <typename Pool, typename Expected>
  struct CountMismatchesAgainstExpected final {
    peano4::CellConnectivity cellConnectivity{};
    typename Pool::ConstView solution{};
    const double*            quadraturePoints{nullptr};
    Expected                 expected{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const int                             cell = cellConnectivity.getCells()[entry];
      tarch::la::Vector<DIMENSIONS, double> centre;
      tarch::la::Vector<DIMENSIONS, double> width;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        centre(axis) = cellConnectivity.getCellCentre(cell, axis);
        width(axis)  = cellConnectivity.getCellWidth(cell, axis);
      }
      const double* const values = solution.getChunk(cell);
      peano4::dForEach(NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
        const auto   position = exahype2::dg::getQuadraturePoint(centre, width, node, quadraturePoints);
        const double value    = values[peano4::dLinearised(node, NodesPerAxis)];
        if (std::abs(value - expected(position)) > tarch::testing::NumericalTolerance) {
          accumulator++;
        }
      });
    }
  };
  TARGET_DECLARE_END

  template <typename Solver, typename Expected>
  int getNumberOfMismatches(
    const Solver&                   solver,
    const peano4::CellConnectivity& cellConnectivity,
    const Expected                  expected
  ) {
    int result = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getCells().size),
      CountMismatchesAgainstExpected<typename Solver::CellPool, Expected>{
        cellConnectivity.onDevice(),
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

  int getNumberOfMismatchesAgainstTheProfile(
    const FixedRuleSolver&          solver,
    const peano4::CellConnectivity& cellConnectivity
  ) {
    return getNumberOfMismatches(solver, cellConnectivity, LinearProfile{});
  }

  void validateTheEstimatesCoverEverySlot(
    const FixedRuleSolver&          solver,
    const peano4::CellConnectivity& cellConnectivity
  ) {
    VALIDATE(solver.getEstimates().getCapacity() >= cellConnectivity.getNumberOfSlots());
  }
} // namespace

UNIT_TEST("The initialisation writes the initial condition into every node") {
  GIVEN("A solver without flux and a regular grid over every rank") {
    const auto&                  cellConnectivity = buildGrid();
    FixedRuleSolver              solver;
    exahype2::RefinementRequests requests;

    WHEN("The solver votes on the grid construction with a largest admissible size below the cell width") {
      auto constructionLimits                   = getLimitsOfTheDomain();
      constructionLimits.maxAdmissiblePatchSize = CoarseWidth / SubdivisionFactor;
      solver.setLimits(constructionLimits);
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.requestRefinementForGridConstruction(cellConnectivity, requests, RuntimeParameters{});
      const auto constructionRequests = requests.getRefinementRequests(cellConnectivity);

      THEN("Every cell requests its refinement") {
        VALIDATE_EQUALS(static_cast<int>(constructionRequests.size()), RegularCells);
      }

      AND_WHEN("The solver allocates with the domain limits and initialises every cell with an erase request") {
        solver.setLimits(getLimitsOfTheDomain());
        solver.allocate(cellConnectivity, RuntimeParameters{});
        requests.clear(cellConnectivity.getNumberOfSlots());
        requests.merge(exahype2::RefinementCommand::Erase, cellConnectivity.getCells());
        solver.initialise(cellConnectivity, requests, RuntimeParameters{});
        finishStep(solver);
        const int  cells              = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
        const auto refinementRequests = requests.getRefinementRequests(cellConnectivity);
        const int  mismatches =
          peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity));

        THEN("Every node stores the profile, the grid stays regular and the refinement requests stay empty") {
          VALIDATE_EQUALS(mismatches, 0);
          VALIDATE_EQUALS(solver.getNumberOfDoFs(), NodesInCell);
          VALIDATE_EQUALS(cells, RegularCells);
          VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMinMeshWidth(), CoarseWidth);
          VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth);
          VALIDATE(refinementRequests.empty());
          VALIDATE(solver.getEstimates().getCapacity() >= cellConnectivity.getNumberOfSlots());
          VALIDATE(solver.getAllocatedBytes().getTotal() > 0);
        }
      }
    }
    solver.release();
  }
}

UNIT_TEST("A time step without flux keeps the solution and advances the time stamp") {
  GIVEN("A solver without flux initialised on a regular grid") {
    const auto&     cellConnectivity = buildGrid();
    FixedRuleSolver solver;
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
          peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity))
        );
        timeStampOfStep.push_back(solver.getTimeStepping().getTimeStamp());
        sizeOfStep.push_back(solver.getTimeStepping().getTimeStepSizeOfLastStep());
        sizeOfNextStep.push_back(solver.getTimeStepping().getTimeStepSize());
      }
      const auto refinementRequests = requests.getRefinementRequests(cellConnectivity);

      THEN("Every node keeps its value and the refinement requests stay empty") {
        for (std::size_t step = 0; step < mismatchesOfStep.size(); step++) {
          VALIDATE_EQUALS(mismatchesOfStep[step], 0, step);
        }
        VALIDATE(refinementRequests.empty());
      }

      THEN("The time stamp advances by a first step of size zero and then by the fixed step size") {
        VALIDATE_NUMERICAL_EQUALS(firstStepSize, 0.0);
        for (std::size_t step = 0; step < timeStampOfStep.size(); step++) {
          VALIDATE_NUMERICAL_EQUALS(timeStampOfStep[step], static_cast<double>(step) * FixedTimeStepSize, step);
          VALIDATE_NUMERICAL_EQUALS(sizeOfStep[step], step == 0 ? 0.0 : FixedTimeStepSize, step);
          VALIDATE_NUMERICAL_EQUALS(sizeOfNextStep[step], FixedTimeStepSize, step);
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
    const std::string fileName       = (tarch::testing::getWritableTestDirectory() / "RKDGSolverTestError").string();
    const bool        isGlobalMaster = tarch::mpi::Rank::getInstance().isGlobalMaster();
    FixedRuleSolver   solver;
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

UNIT_TEST("A unit source advances every node by the time step size") {
  GIVEN("A solver with a unit source initialised to a constant on a regular grid") {
    const auto&  cellConnectivity = buildGrid();
    SourceSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);

    WHEN("The solver performs three time steps") {
      std::vector<int> mismatchesOfStep;
      for (int step = 0; step < 3; step++) {
        requests.clear(cellConnectivity.getNumberOfSlots());
        solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
        finishStep(solver);
        const double expectedValue = SourceSolver::InitialValue + solver.getTimeStepping().getTimeStamp();
        mismatchesOfStep.push_back(
          peano4::unittests::getGlobalSumOf(
            getNumberOfMismatches(solver, cellConnectivity, ConstantProfile{expectedValue})
          )
        );
      }

      THEN("Every node stores the initial value plus the time stamp and the time stamp ends at two step sizes") {
        for (std::size_t step = 0; step < mismatchesOfStep.size(); step++) {
          VALIDATE_EQUALS(mismatchesOfStep[step], 0, step);
        }
        VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getTimeStamp(), 2 * FixedTimeStepSize);
      }
    }
    solver.release();
  }
}

UNIT_TEST("The prolongation reproduces the linear profile on every child and the restriction returns it") {
  GIVEN("A solver without flux on a regular grid after one time step, with live solver data") {
    const auto&     cellConnectivity = buildGrid();
    FixedRuleSolver solver;
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
      const int fineCells = peano4::unittests::getGlobalSumOf(getNumberOfCellsOfLevel(cellConnectivity, Depth + 1));
      const int mismatches =
        peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity));

      THEN("Every fine node takes the profile, the estimates cover every slot and the step size shrinks") {
        VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getTimeStepSize(), coarseTimeStepSize / SubdivisionFactor);
        VALIDATE(fineCells > 0);
        VALIDATE_EQUALS(mismatches, 0);
        validateTheEstimatesCoverEverySlot(solver, cellConnectivity);
        VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMinMeshWidth(), CoarseWidth / SubdivisionFactor);
        VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth);
      }

      AND_WHEN("The solver performs a time step") {
        requests.clear(cellConnectivity.getNumberOfSlots());
        solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
        finishStep(solver);
        const int mismatchesAfterTheTimeStep =
          peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity));

        THEN("The step has the fine step size and keeps every value") {
          VALIDATE_NUMERICAL_EQUALS(
            solver.getTimeStepping().getTimeStepSizeOfLastStep(),
            coarseTimeStepSize / SubdivisionFactor
          );
          VALIDATE_EQUALS(mismatchesAfterTheTimeStep, 0);
        }

        AND_WHEN("Every fine cell erases back") {
          regridWithLiveData(solver, peano4::unittests::getErasureOfEveryCell(Depth), requests);
          const int cells = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
          const int mismatchesAfterTheErase =
            peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity));

          THEN("The restriction returns the coarse profile within rounding and the coarse step size") {
            VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getTimeStepSize(), coarseTimeStepSize);
            VALIDATE_EQUALS(mismatchesAfterTheErase, 0);
            VALIDATE_EQUALS(cells, RegularCells);
            VALIDATE_EQUALS(cellConnectivity.getMaximumLevel(), Depth);
          }
        }
      }
    }
    solver.release();
  }
}

UNIT_TEST("The first time step after a refinement takes the stability bound of the finer grid") {
  GIVEN("A solver with the adaptive rule and the wave speed one on a regular grid after two time steps") {
    const auto&    cellConnectivity = buildGrid();
    AdaptiveSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    const double coarseTimeStepSize = solver.getTimeStepping().getTimeStepSize();
    peano4::Spacetree::getInstance().markPersistentDataLive();

    WHEN("The cell in the centre refines and the solver performs the next time step") {
      regridWithLiveData(solver, {peano4::unittests::getRefinementOfCentreCell(Depth)}, requests);
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);

      THEN("The step size shrinks by the subdivision factor") {
        VALIDATE_NUMERICAL_EQUALS(
          solver.getTimeStepping().getTimeStepSizeOfLastStep(),
          coarseTimeStepSize / SubdivisionFactor
        );
      }
    }
    solver.release();
  }
}

UNIT_TEST("A plot writes the node and the face file and allocates host copies only with a GPU backend") {
  GIVEN("A solver without flux that plots every face, initialised on a regular grid over every rank") {
    const auto&                 cellConnectivity = buildGrid();
    const auto&                 rank             = tarch::mpi::Rank::getInstance();
    FacePlottingFixedRuleSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "DiscontinuousGalerkinSolverPlotTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "patch/");
    std::filesystem::create_directories(outputDirectory + "face/");

    WHEN("The solver plots its nodes and faces and every rank downloads the solution and the face states") {
      const tarch::AllocatedBytes bytesBeforeThePlot = solver.getAllocatedBytes();
      solver.plot(cellConnectivity, tarch::plotter::Format::VTU, outputDirectory, RuntimeParameters{});
      const tarch::AllocatedBytes bytesAfterThePlot = solver.getAllocatedBytes();

      const auto&                            solution   = solver.getSolution();
      const auto&                            faceStates = solver.getFaceStates();
      FacePlottingFixedRuleSolver::CellPool  solutionCopy(solution.getChunkSize(), tarch::MemoryLocation::Heap);
      FacePlottingFixedRuleSolver::StatePool faceStatesCopy(faceStates.getChunkSize(), tarch::MemoryLocation::Heap);
      static_cast<void>(solution.download(solutionCopy));
      const auto&                 faceStatesOnHost = faceStates.download(faceStatesCopy);
      const std::string           vtuSuffix        = "-0_000000-rank-" + std::to_string(rank.getRank()) + ".vtu";
      const std::filesystem::path nodeVtuPath =
        std::filesystem::path(outputDirectory) / "patch" / ("FixedRuleSolver" + vtuSuffix);
      const std::filesystem::path faceVtuPath =
        std::filesystem::path(outputDirectory) / "face" / ("FixedRuleSolver-face" + vtuSuffix);
      const bool nodeFileExists = std::filesystem::exists(nodeVtuPath);
      const bool faceFileExists = std::filesystem::exists(faceVtuPath);
      tarch::mpi::barrier(rank.getCommunicator());
      if (rank.isGlobalMaster()) {
        std::filesystem::remove_all(outputDirectory);
      }

      THEN("Both files exist, the host bytes grow by the host copies and both halves of a boundary face agree") {
        VALIDATE_EQUALS(
          bytesAfterThePlot.host,
          bytesBeforeThePlot.host + solutionCopy.getAllocatedBytes().host + faceStatesCopy.getAllocatedBytes().host
        );
        VALIDATE_EQUALS(bytesAfterThePlot.device, bytesBeforeThePlot.device);
        VALIDATE(nodeFileExists, nodeVtuPath.string());
        VALIDATE(faceFileExists, faceVtuPath.string());
        constexpr int NodesPerFace = exahype2::dg::getNodesPerFace(NodesPerAxis);
        for (const int face : cellConnectivity.getFaces(peano4::Adjacency::Local, peano4::FaceKind::Boundary)) {
          const double* const halves = faceStatesOnHost.getChunk(face);
          for (int node = 0; node < NodesPerFace; node++) {
            VALIDATE_EQUALS(halves[node], halves[NodesPerFace + node], face, node);
          }
        }
      }
    }
    solver.release();
  }
}

UNIT_TEST("A release frees every pool that the allocation reserved") {
  GIVEN("A solver and a regular grid") {
    const auto&     cellConnectivity = buildGrid();
    FixedRuleSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    const std::size_t bytesBeforeTheAllocation = solver.getAllocatedBytes().getTotal();

    WHEN("The solver allocates its pools on the grid and releases them") {
      solver.allocate(cellConnectivity, RuntimeParameters{});
      const std::size_t bytesAfterTheAllocation = solver.getAllocatedBytes().getTotal();
      solver.release();

      THEN("The pools occupy bytes between the allocation and the release only") {
        VALIDATE_EQUALS(bytesBeforeTheAllocation, std::size_t{0});
        VALIDATE(bytesAfterTheAllocation > 0);
        VALIDATE_EQUALS(solver.getAllocatedBytes().getTotal(), std::size_t{0});
        VALIDATE_EQUALS(solver.getEstimates().getCapacity(), 0);
      }
    }
  }
}
