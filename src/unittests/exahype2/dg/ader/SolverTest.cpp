// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/ader/Solver.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/dg/Basis.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "tarch/Assertions.h"
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
#include "unittests/tarch/accelerator/ChunkedMemoryPoolTestData.h"

namespace {
  TARGET_DECLARE
  constexpr int    SubdivisionFactor  = peano4::SubdivisionFactor;
  constexpr int    Depth              = peano4::unittests::DepthWithCoarseCellsBesideTheCentreBlock;
  constexpr int    CellsPerAxis       = peano4::power(SubdivisionFactor, Depth);
  constexpr int    RegularCells       = peano4::power(CellsPerAxis, DIMENSIONS);
  constexpr double CoarseWidth        = 1.0 / CellsPerAxis;
  constexpr int    Order              = 1;
  constexpr int    NodesPerAxis       = Order + 1;
  constexpr int    NodesInCell        = exahype2::dg::getNodesPerCell(NodesPerAxis);
  constexpr auto   EntriesPerOperator = static_cast<std::size_t>(NodesPerAxis) * NodesPerAxis;
  constexpr double TimeStepFactor     = 0.5;
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

  double getBasisDerivative(const double x, const int node) {
    double result = 0.0;
    for (int omitted = 0; omitted < NodesPerAxis; omitted++) {
      if (omitted != node) {
        double term = 1.0 / (QuadratureNodes[node] - QuadratureNodes[omitted]);
        for (int other = 0; other < NodesPerAxis; other++) {
          if (other != node and other != omitted) {
            term *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
          }
        }
        result += term;
      }
    }
    return result;
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
      result = std::move(next);
    }
    return result;
  }

  /**
   * The Gauss-Legendre quadrature tables of order one in the layout of the generated tables. Two
   * solvers whose allocations overlap in time need two Instance values.
   */
  template <typename Precision, int Instance>
  class QuadratureTables final {
  public:
    static void initialise() {
      ASSERTION(QuadraturePoints1D == nullptr, Instance);
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
   * The Gauss-Legendre operators of order one in the layout of the generated tables, one set per
   * Instance.
   */
  template <typename Precision, int Instance>
  class DGMatrixTables final {
  public:
    static auto getPlotMatrix1D() {
      return exahype2::dg::getEquidistantInterpolationMatrix1D<NodesPerAxis, std::max(2 * NodesPerAxis - 1, 2)>(
        QuadratureNodes
      );
    }

    static void initialise() {
      ASSERTION(StiffnessOperator1D == nullptr, Instance);
      std::vector<double> stiffness(EntriesPerOperator);
      std::vector<double> stiffnessTransposed(EntriesPerOperator);
      std::vector<double> derivative(EntriesPerOperator);
      std::vector<double> picard(EntriesPerOperator);
      std::vector<double> left(NodesPerAxis);
      std::vector<double> right(NodesPerAxis);
      for (int row = 0; row < NodesPerAxis; row++) {
        left[static_cast<std::size_t>(row)]  = getBasisValue(0.0, row);
        right[static_cast<std::size_t>(row)] = getBasisValue(1.0, row);
        for (int column = 0; column < NodesPerAxis; column++) {
          const auto   entry           = static_cast<std::size_t>(row) * NodesPerAxis + column;
          const auto   transposedEntry = static_cast<std::size_t>(column) * NodesPerAxis + row;
          const double value           = QuadratureWeights[column] * getBasisDerivative(QuadratureNodes[column], row);
          stiffness[entry]             = value;
          stiffnessTransposed[transposedEntry] = value;
          derivative[entry]                    = getBasisDerivative(QuadratureNodes[row], column);
          picard[entry]                        = getBasisValue(1.0, row) * getBasisValue(1.0, column) - value;
        }
      }
      // The Picard operator of two nodes inverts in closed form.
      const double              determinant = picard[0] * picard[3] - picard[1] * picard[2];
      const std::vector<double> inversePicard{
        picard[3] / determinant,
        -picard[1] / determinant,
        -picard[2] / determinant,
        picard[0] / determinant
      };
      const auto prolongation = exahype2::dg::transposePerChild<NodesPerAxis>(
        exahype2::dg::getInterpolationMatrix1D<NodesPerAxis>(QuadratureNodes)
      );
      const auto restriction = exahype2::dg::transposePerChild<NodesPerAxis>(
        exahype2::dg::getWeightedRestrictionMatrix1D<NodesPerAxis>(QuadratureNodes, QuadratureWeights)
      );
      StiffnessOperator1D           = getTableOf<Precision>(stiffness);
      StiffnessOperatorTransposed1D = getTableOf<Precision>(stiffnessTransposed);
      DerivativeOperator1D          = getTableOf<Precision>(derivative);
      InversePicardOperator1D       = getTableOf<Precision>(inversePicard);
      BasisFunctionValuesLeft1D     = getTableOf<Precision>(left);
      BasisFunctionValuesRight1D    = getTableOf<Precision>(right);
      ProlongationMatrix1D          = getTableOf<Precision>({prolongation.begin(), prolongation.end()});
      WeightedRestrictionMatrix1D   = getTableOf<Precision>({restriction.begin(), restriction.end()});
    }

    static void deallocate() {
      dropTable(StiffnessOperator1D);
      dropTable(StiffnessOperatorTransposed1D);
      dropTable(DerivativeOperator1D);
      dropTable(InversePicardOperator1D);
      dropTable(BasisFunctionValuesLeft1D);
      dropTable(BasisFunctionValuesRight1D);
      dropTable(ProlongationMatrix1D);
      dropTable(WeightedRestrictionMatrix1D);
    }

    inline static Precision* StiffnessOperator1D           = nullptr;
    inline static Precision* StiffnessOperatorTransposed1D = nullptr;
    inline static Precision* DerivativeOperator1D          = nullptr;
    inline static Precision* InversePicardOperator1D       = nullptr;
    inline static Precision* BasisFunctionValuesLeft1D     = nullptr;
    inline static Precision* BasisFunctionValuesRight1D    = nullptr;
    inline static Precision* ProlongationMatrix1D          = nullptr;
    inline static Precision* WeightedRestrictionMatrix1D   = nullptr;
  };

  template <typename Precision>
  using Quadrature = QuadratureTables<Precision, 0>;

  template <typename Precision>
  using DGMatrices = DGMatrixTables<Precision, 0>;

  struct FixedRuleADERDGSolver;
  struct AdaptiveADERDGSolver;
  template <int Side>
  struct HalfSolver;

  /**
   * The PDE terms of a linear profile without flux and wave speed. A time step keeps the profile.
   */
  struct LinearTerms {
    struct RuntimeParameters final {};

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

  using RuntimeParameters = LinearTerms::RuntimeParameters;

  /**
   * A solver with the linear profile of LinearTerms and a fixed time step rule.
   */
  struct FixedRuleADERDGSolver final : exahype2::dg::ader::Solver<FixedRuleADERDGSolver, double, double, void>,
                                       LinearTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    template <typename Precision>
    using Quadrature = QuadratureTables<Precision, 0>;
    template <typename Precision>
    using DGMatrices = DGMatrixTables<Precision, 0>;

    FixedRuleADERDGSolver() :
      Solver(TimeStepFactor, "FixedRuleADERDGSolver", {{"q", 1}}) {}
  };

  /**
   * A solver with the linear profile of LinearTerms and a fixed time step rule. Its plots write every
   * face.
   */
  struct FacePlottingFixedRuleADERDGSolver final
    : exahype2::dg::ader::Solver<FacePlottingFixedRuleADERDGSolver, double, double, void>,
      LinearTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    template <typename Precision>
    using Quadrature = QuadratureTables<Precision, 0>;
    template <typename Precision>
    using DGMatrices = DGMatrixTables<Precision, 0>;

    FacePlottingFixedRuleADERDGSolver() :
      Solver(TimeStepFactor, "FacePlottingFixedRuleADERDGSolver", {{"q", 1}}) {}

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
   * A solver with the linear profile of LinearTerms and a fixed time step rule. Its initialisation
   * projects the solution onto the plane of normal 0 at planePosition.
   */
  struct FaceTracingFixedRuleADERDGSolver final
    : exahype2::dg::ader::Solver<FaceTracingFixedRuleADERDGSolver, double, double, void>,
      LinearTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    template <typename Precision>
    using Quadrature = QuadratureTables<Precision, 0>;
    template <typename Precision>
    using DGMatrices = DGMatrixTables<Precision, 0>;

    explicit FaceTracingFixedRuleADERDGSolver(const double planePosition) :
      Solver(TimeStepFactor, "FaceTracingFixedRuleADERDGSolver", {{"q", 1}}),
      _planePosition(planePosition) {}

    void initialise(
      const peano4::CellConnectivity& cellConnectivity,
      exahype2::RefinementRequests&   requests,
      const RuntimeParameters&        parameters
    ) {
      Solver::initialise(cellConnectivity, requests, parameters);
      projectSolutionOntoPlane(cellConnectivity, 0, _planePosition);
    }

  private:
    double _planePosition{0.0};
  };

  /**
   * A solver with the linear profile of LinearTerms, a fixed time step rule and a linear predictor.
   * The Riemann solver of ExaHyPE reads its face states only. Its predictor writes face states only.
   */
  struct LinearFixedRuleADERDGSolver final
    : exahype2::dg::ader::Solver<LinearFixedRuleADERDGSolver, double, double, void>,
      LinearTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = true;
    static constexpr int  NumberOfPointSources       = 0;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    template <typename Precision>
    using Quadrature = QuadratureTables<Precision, 0>;
    template <typename Precision>
    using DGMatrices = DGMatrixTables<Precision, 0>;

    LinearFixedRuleADERDGSolver() :
      Solver(TimeStepFactor, "LinearFixedRuleADERDGSolver", {{"q", 1}}) {}

    /**
     * Overwrites every face flux with value.
     */
    void presetFaceFluxes(const double value) {
      const auto values = static_cast<std::size_t>(_faceFluxes.getCapacity())
        * static_cast<std::size_t>(_faceFluxes.getPaddedChunkSize());
      tarch::accelerator::unittests::setChunksOf(_faceFluxes, std::vector<double>(values, value));
    }
  };

  /**
   * A solver with the linear profile of LinearTerms, an adaptive time step rule and a unit wave
   * speed.
   */
  struct AdaptiveADERDGSolver final : exahype2::dg::ader::Solver<AdaptiveADERDGSolver, double, double, void>,
                                      LinearTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Adaptive;

    template <typename Precision>
    using Quadrature = QuadratureTables<Precision, 0>;
    template <typename Precision>
    using DGMatrices = DGMatrixTables<Precision, 0>;

    AdaptiveADERDGSolver() :
      Solver(TimeStepFactor, "AdaptiveADERDGSolver", {{"q", 1}}) {}

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
   * A solver with a fixed time step rule whose initial condition lies Increment below the linear
   * profile of LinearTerms. Its postprocessing adds Increment to every node of a cell.
   */
  struct PostprocessingSolver final : exahype2::dg::ader::Solver<PostprocessingSolver, double, double, void>,
                                      LinearTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    static constexpr double Increment = 0.5;

    template <typename Precision>
    using Quadrature = QuadratureTables<Precision, 0>;
    template <typename Precision>
    using DGMatrices = DGMatrixTables<Precision, 0>;

    PostprocessingSolver() :
      Solver(TimeStepFactor, "PostprocessingSolver", {{"q", 1}}) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                                 Q,
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      Q[0] = getProfile(x) - Increment;
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void postprocessUpdatedPatch(
      double* const                                                 Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int node = 0; node < NodesInCell; node++) {
        Q[node] += Increment;
      }
    }
  };

  /**
   * The plane between the cells the two coupled solvers own.
   */
  constexpr double Interface = CoarseWidth;

  /**
   * A solver that owns the cells left of the interface for Side 0 and the cells right of it for
   * Side 1.
   */
  template <int Side>
  struct HalfSolver final : exahype2::dg::ader::Solver<HalfSolver<Side>, double, double, void>, LinearTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    template <typename Precision>
    using Quadrature = QuadratureTables<Precision, Side>;
    template <typename Precision>
    using DGMatrices = DGMatrixTables<Precision, Side>;

    HalfSolver() :
      HalfSolver::Solver(TimeStepFactor, Side == 0 ? "LeftSolver" : "RightSolver", {{"q", 1}}) {}

    static bool ownsCell(
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return Side == 0 ? x(0) < Interface : x(0) > Interface;
    }

    /**
     * Overwrites every state and flux value of a face with the number of the face.
     */
    void writeFaceNumbersIntoFaces() {
      for (auto* const pool : {&this->_faceStates, &this->_faceFluxes}) {
        const auto          paddedChunkSize = static_cast<std::size_t>(pool->getPaddedChunkSize());
        std::vector<double> values(static_cast<std::size_t>(pool->getCapacity()) * paddedChunkSize);
        for (std::size_t value = 0; value < values.size(); value++) {
          values[value] = static_cast<double>(value / paddedChunkSize);
        }
        tarch::accelerator::unittests::setChunksOf(*pool, values);
      }
    }
  };

  /**
   * Calls release() of the solver when the guard leaves its scope, also after a failed check. The
   * next allocate() then finds the static tables empty.
   */
  template <typename AllocatedSolver>
  class ReleaseOnReturn final {
  public:
    explicit ReleaseOnReturn(AllocatedSolver& solver) :
      _solver(solver) {}
    ReleaseOnReturn(const ReleaseOnReturn& other)            = delete;
    ReleaseOnReturn(ReleaseOnReturn&& other)                 = delete;
    ReleaseOnReturn& operator=(const ReleaseOnReturn& other) = delete;
    ReleaseOnReturn& operator=(ReleaseOnReturn&& other)      = delete;
    ~ReleaseOnReturn() { _solver.release(); }

  private:
    AllocatedSolver& _solver;
  };

  const peano4::CellConnectivity& buildGrid() {
    peano4::unittests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getCellConnectivity();
  }

  /**
   * Returns the identity map of the nodes of one axis.
   */
  std::vector<double> getIdentityOperator() {
    std::vector<double> identity(EntriesPerOperator, 0.0);
    for (int node = 0; node < NodesPerAxis; node++) {
      identity[static_cast<std::size_t>(node) * NodesPerAxis + node] = 1.0;
    }
    return identity;
  }

  /**
   * Reduces the values of every solver over all ranks in the one allreduce that ends a step of a
   * run.
   */
  template <typename... Solvers>
  void finishStep(Solvers&... solvers) {
    tarch::mpi::ReductionRecord record;
    (solvers.addLocalValuesToGlobalReduction(record), ...);
    record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());
    (solvers.readValuesFromGlobalReduction(record), ...);
  }

  template <typename... Solvers>
  void regridWithLiveData(
    const std::vector<peano4::RefinementRequest>& refinementRequests,
    exahype2::RefinementRequests&                 requests,
    Solvers&... solvers
  ) {
    auto&          spacetree = peano4::Spacetree::getInstance();
    peano4::Regrid regrid;
    if (not spacetree.regrid(refinementRequests, regrid)) {
      tarch::terminateWithError("The regrid of the test grid must change the grid.");
    }
    const auto& cellConnectivity = spacetree.getCellConnectivity();
    requests.clear(cellConnectivity.getNumberOfSlots());
    (solvers.regrid(cellConnectivity, regrid, requests, RuntimeParameters{}), ...);
    spacetree.finishRegrid();
    finishStep(solvers...);
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

  TARGET_DECLARE
  /**
   * Increments the accumulator for every node of one cell whose value differs from the profile there.
   */
  template <typename Pool>
  struct CountMismatchesAgainstTheProfile final {
    peano4::CellConnectivity cellConnectivity{};
    typename Pool::ConstView solution{};
    const double*            quadraturePoints{nullptr};

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
        if (std::abs(value - LinearTerms::getProfile(position)) > tarch::testing::NumericalTolerance) {
          accumulator++;
        }
      });
    }
  };
  TARGET_DECLARE_END

  template <typename Solver>
  int getNumberOfMismatchesAgainstTheProfile(const Solver& solver, const peano4::CellConnectivity& cellConnectivity) {
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getCells().size),
      CountMismatchesAgainstTheProfile<typename Solver::CellPool>{
        cellConnectivity.onDevice(),
        solver.getSolution().getView(),
        Quadrature<double>::QuadraturePoints1D
      },
      tarch::accelerator::Sum<int>{mismatches}
    );
    return mismatches;
  }

  /**
   * Returns whether the face is an interior face with normal 0 on the interface plane.
   */
  GPU_CALLABLE_METHOD bool isInterfaceFace(const peano4::CellConnectivity& cellConnectivity, const int face) {
    return cellConnectivity.getFaceNormal(face) == 0
      and std::abs(cellConnectivity.getFaceCentre(face, 0) - Interface) <= tarch::testing::NumericalTolerance
      and cellConnectivity.getFaceKind(face) == peano4::FaceKind::Interior;
  }

  TARGET_DECLARE
  /**
   * Increments the accumulator for an interface face with a cell on its lower side.
   */
  struct CountCouplingFace final {
    peano4::CellConnectivity cellConnectivity{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (isInterfaceFace(cellConnectivity, face) and cellConnectivity.getFaceCell(face, 0) >= 0) {
        accumulator++;
      }
    }
  };

  /**
   * Increments the accumulator for every side of one interface face whose copied half differs from
   * the half of the partner or whose node mean differs from the profile at the face centre.
   */
  struct CountTraceMismatchesOfFace final {
    static constexpr int NodesPerFace = exahype2::dg::getNodesPerFace(NodesPerAxis);

    peano4::CellConnectivity            cellConnectivity{};
    HalfSolver<0>::StatePool::ConstView leftStates{};
    HalfSolver<1>::StatePool::ConstView rightStates{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (not isInterfaceFace(cellConnectivity, face)) {
        return;
      }
      tarch::la::Vector<DIMENSIONS, double> centre;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        centre(axis) = cellConnectivity.getFaceCentre(face, axis);
      }
      for (int side = 0; side < 2; side++) {
        if (cellConnectivity.getFaceCell(face, side) >= 0) {
          const int           partnerHalf   = 1 - side;
          const auto          partnerOffset = static_cast<std::ptrdiff_t>(partnerHalf) * NodesPerFace;
          const double* const copied =
            (side == 0 ? leftStates.getChunk(face) : rightStates.getChunk(face)) + partnerOffset;
          const double* const partner =
            (side == 0 ? rightStates.getChunk(face) : leftStates.getChunk(face)) + partnerOffset;
          double mean  = 0.0;
          bool   equal = true;
          for (int node = 0; node < NodesPerFace; node++) {
            equal = equal and copied[node] == partner[node];
            mean += copied[node] / NodesPerFace;
          }
          if (not equal or std::abs(mean - LinearTerms::getProfile(centre)) > tarch::testing::NumericalTolerance) {
            accumulator++;
          }
        }
      }
    }
  };

  /**
   * Returns the number of nodes of both halves of the face whose face state differs from the profile.
   */
  GPU_CALLABLE_METHOD int getProfileMismatchesOfFace(
    const peano4::CellConnectivity& cellConnectivity,
    const double* const             halves,
    const double* const             quadraturePoints,
    const int                       face
  ) {
    constexpr int NodesPerFace = exahype2::dg::getNodesPerFace(NodesPerAxis);
    ASSERTION(halves != nullptr);
    ASSERTION(quadraturePoints != nullptr);
    ASSERTION(face >= 0 and face < cellConnectivity.getNumberOfFaces(), face, cellConnectivity.getNumberOfFaces());
    const int normal     = cellConnectivity.getFaceNormal(face);
    int       mismatches = 0;
    for (int node = 0; node < NodesPerFace; node++) {
      const auto                            nodeIndex = peano4::dDelinearisedExclusive(node, NodesPerAxis, normal);
      tarch::la::Vector<DIMENSIONS, double> position;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        position(axis) = cellConnectivity.getFaceCentre(face, axis);
        if (axis != normal) {
          position(axis) += (quadraturePoints[nodeIndex(axis)] - 0.5) * cellConnectivity.getFaceWidth(face, axis);
        }
      }
      for (int half = 0; half < 2; half++) {
        const double value = halves[half * NodesPerFace + node];
        if (std::abs(value - LinearTerms::getProfile(position)) > tarch::testing::NumericalTolerance) {
          mismatches++;
        }
      }
    }
    return mismatches;
  }

  /**
   * Returns whether the face is a coarse face of a level jump with normal 0 on the plane at position.
   */
  GPU_CALLABLE_METHOD bool isCoarseFaceOnPlane(
    const peano4::CellConnectivity& cellConnectivity,
    const int                       face,
    const double                    position
  ) {
    return cellConnectivity.getFaceKind(face) == peano4::FaceKind::Coarse and cellConnectivity.getFaceNormal(face) == 0
      and std::abs(cellConnectivity.getFaceCentre(face, 0) - position) <= tarch::testing::NumericalTolerance;
  }

  /**
   * Adds the profile mismatches of an interior face to the accumulator. With interfaceOnly, only
   * interface faces count.
   */
  template <typename Pool>
  struct CountTraceMismatchesAgainstTheProfile final {
    peano4::CellConnectivity cellConnectivity{};
    typename Pool::ConstView faceStates{};
    const double*            quadraturePoints{nullptr};
    bool                     interfaceOnly{false};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (
        cellConnectivity.getFaceKind(face) == peano4::FaceKind::Interior
        and (not interfaceOnly or isInterfaceFace(cellConnectivity, face))
      ) {
        accumulator += getProfileMismatchesOfFace(cellConnectivity, faceStates.getChunk(face), quadraturePoints, face);
      }
    }
  };

  /**
   * Adds the profile mismatches of a coarse face on the plane at position to the accumulator.
   */
  template <typename Pool>
  struct CountCoarseFaceMismatchesOnPlane final {
    peano4::CellConnectivity cellConnectivity{};
    typename Pool::ConstView faceStates{};
    const double*            quadraturePoints{nullptr};
    double                   position{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (isCoarseFaceOnPlane(cellConnectivity, face, position)) {
        accumulator += getProfileMismatchesOfFace(cellConnectivity, faceStates.getChunk(face), quadraturePoints, face);
      }
    }
  };

  struct CountCoarseFacesOnPlane final {
    peano4::CellConnectivity cellConnectivity{};
    double                   position{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (isCoarseFaceOnPlane(cellConnectivity, face, position)) {
        accumulator++;
      }
    }
  };

  /**
   * Counts every interior face or with interfaceOnly the interface faces alone.
   */
  struct CountInteriorFace final {
    peano4::CellConnectivity cellConnectivity{};
    bool                     interfaceOnly{false};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (
        cellConnectivity.getFaceKind(face) == peano4::FaceKind::Interior
        and (not interfaceOnly or isInterfaceFace(cellConnectivity, face))
      ) {
        accumulator++;
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Returns the number of coupling faces and the number of their mismatching halves.
   */
  std::pair<int, int> getCouplingFaceMismatches(
    const HalfSolver<0>&            left,
    const HalfSolver<1>&            right,
    const peano4::CellConnectivity& cellConnectivity
  ) {
    int couplingFaces = 0;
    int mismatches    = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getNumberOfFaces()),
      CountCouplingFace{cellConnectivity.onDevice()},
      tarch::accelerator::Sum<int>{couplingFaces}
    );
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getNumberOfFaces()),
      CountTraceMismatchesOfFace{
        cellConnectivity.onDevice(),
        left.getFaceStates().getView(),
        right.getFaceStates().getView()
      },
      tarch::accelerator::Sum<int>{mismatches}
    );
    return {couplingFaces, mismatches};
  }

  /**
   * Returns the number of local level jumps and the number of values that differ from the number of
   * their face in the halves that the interpolation and the restriction of these level jumps write.
   */
  template <typename Solver>
  std::pair<int, int> getLevelJumpMismatchesAgainstTheFaceNumbers(
    const Solver&                   solver,
    const peano4::CellConnectivity& cellConnectivity
  ) {
    constexpr int NodesPerFace       = exahype2::dg::getNodesPerFace(NodesPerAxis);
    constexpr int StateValuesPerHalf = NodesPerFace * (Solver::NumberOfUnknowns + Solver::NumberOfAuxiliaryVariables);
    constexpr int FluxValuesPerHalf  = NodesPerFace * Solver::NumberOfUnknowns;
    const std::vector<double> states     = tarch::accelerator::unittests::getChunksOf(solver.getFaceStates());
    const std::vector<double> fluxes     = tarch::accelerator::unittests::getChunksOf(solver.getFaceFluxes());
    int                       mismatches = 0;
    const auto                countMismatchesOfHalf = [&mismatches](
                                                        const std::vector<double>& values,
                                                        const int                  paddedChunkSize,
                                                        const int                  valuesPerHalf,
                                                        const int                  face,
                                                        const int                  half
                                                      ) {
      const auto firstValue = static_cast<std::size_t>(face) * static_cast<std::size_t>(paddedChunkSize)
        + static_cast<std::size_t>(half) * static_cast<std::size_t>(valuesPerHalf);
      ASSERTION(firstValue + static_cast<std::size_t>(valuesPerHalf) <= values.size(), firstValue, values.size());
      for (std::size_t value = firstValue; value < firstValue + static_cast<std::size_t>(valuesPerHalf); value++) {
        if (values[value] != static_cast<double>(face)) {
          mismatches++;
        }
      }
    };
    const int  statePaddedChunkSize = solver.getFaceStates().getPaddedChunkSize();
    const int  fluxPaddedChunkSize  = solver.getFaceFluxes().getPaddedChunkSize();
    const auto levelJumps           = cellConnectivity.getLevelJumps(peano4::Adjacency::Local);
    for (const peano4::LevelJump& levelJump : levelJumps) {
      for (const int fineFace : levelJump.fineFaces) {
        countMismatchesOfHalf(states, statePaddedChunkSize, StateValuesPerHalf, fineFace, levelJump.coarseSide);
        countMismatchesOfHalf(fluxes, fluxPaddedChunkSize, FluxValuesPerHalf, fineFace, levelJump.coarseSide);
      }
      countMismatchesOfHalf(
        fluxes,
        fluxPaddedChunkSize,
        FluxValuesPerHalf,
        levelJump.coarseFace,
        1 - levelJump.coarseSide
      );
    }
    return {levelJumps.size, mismatches};
  }

  /**
   * Returns the number of counted faces and the number of their face nodes that differ from the
   * profile.
   */
  template <typename Solver>
  std::pair<int, int> getTraceMismatchesAgainstTheProfile(
    const Solver&                   solver,
    const peano4::CellConnectivity& cellConnectivity,
    const bool                      interfaceOnly
  ) {
    int faces      = 0;
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getNumberOfFaces()),
      CountInteriorFace{cellConnectivity.onDevice(), interfaceOnly},
      tarch::accelerator::Sum<int>{faces}
    );
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getNumberOfFaces()),
      CountTraceMismatchesAgainstTheProfile<typename Solver::StatePool>{
        cellConnectivity.onDevice(),
        solver.getFaceStates().getView(),
        Quadrature<double>::QuadraturePoints1D,
        interfaceOnly
      },
      tarch::accelerator::Sum<int>{mismatches}
    );
    return {faces, mismatches};
  }

  /**
   * Returns the number of coarse faces on the plane at position and the number of their face nodes
   * that differ from the profile.
   */
  template <typename Solver>
  std::pair<int, int> getCoarseFaceMismatchesOnPlane(
    const Solver&                   solver,
    const peano4::CellConnectivity& cellConnectivity,
    const double                    position
  ) {
    int faces      = 0;
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getNumberOfFaces()),
      CountCoarseFacesOnPlane{cellConnectivity.onDevice(), position},
      tarch::accelerator::Sum<int>{faces}
    );
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getNumberOfFaces()),
      CountCoarseFaceMismatchesOnPlane<typename Solver::StatePool>{
        cellConnectivity.onDevice(),
        solver.getFaceStates().getView(),
        Quadrature<double>::QuadraturePoints1D,
        position
      },
      tarch::accelerator::Sum<int>{mismatches}
    );
    return {faces, mismatches};
  }
} // namespace

UNIT_TEST("The initialisation writes the initial condition into every node") {
  GIVEN("A fixed-rule solver on a regular grid and limits whose largest admissible patch is one level below a cell") {
    const auto& cellConnectivity = buildGrid();

    FixedRuleADERDGSolver solver;
    auto                  constructionLimits  = getLimitsOfTheDomain();
    constructionLimits.maxAdmissiblePatchSize = CoarseWidth / SubdivisionFactor;
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());

    WHEN("The solver votes on the grid construction under these limits") {
      solver.setLimits(constructionLimits);
      solver.requestRefinementForGridConstruction(cellConnectivity, requests, RuntimeParameters{});
      const int refinementRequests = static_cast<int>(requests.getRefinementRequests(cellConnectivity).size());

      THEN("Every cell requests a refinement") { VALIDATE_EQUALS(refinementRequests, RegularCells); }

      AND_WHEN("The solver sets the domain limits, allocates its pools and initialises the cells with erase requests") {
        solver.setLimits(getLimitsOfTheDomain());
        solver.allocate(cellConnectivity, RuntimeParameters{});
        const ReleaseOnReturn release(solver);
        requests.clear(cellConnectivity.getNumberOfSlots());
        requests.merge(exahype2::RefinementCommand::Erase, cellConnectivity.getCells());
        solver.initialise(cellConnectivity, requests, RuntimeParameters{});
        finishStep(solver);
        const int  cells            = peano4::unittests::getGlobalSumOf(cellConnectivity.getCells().size);
        const bool noRequestRemains = requests.getRefinementRequests(cellConnectivity).empty();
        const int  mismatches =
          peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity));

        THEN("Every node stores the profile, the grid stays regular and the refinement requests stay empty") {
          VALIDATE_EQUALS(mismatches, 0);
          VALIDATE_EQUALS(solver.getNumberOfDoFs(), NodesInCell);
          VALIDATE_EQUALS(cells, RegularCells);
          VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMinMeshWidth(), CoarseWidth);
          VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getMaxMeshWidth(), CoarseWidth);
          VALIDATE(noRequestRemains);
          VALIDATE(solver.getAllocatedBytes().getTotal() > 0);
        }
      }
    }
  }
}

UNIT_TEST("A time step without flux keeps the solution and advances the time stamp") {
  GIVEN("A fixed-rule solver without flux initialised on a regular grid") {
    const auto& cellConnectivity = buildGrid();

    FixedRuleADERDGSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn        release(solver);
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
      const bool noRequestArises = requests.getRefinementRequests(cellConnectivity).empty();

      THEN("Every node keeps its value and the refinement requests stay empty") {
        for (std::size_t step = 0; step < mismatchesOfStep.size(); step++) {
          VALIDATE_EQUALS(mismatchesOfStep[step], 0, step);
        }
        VALIDATE(noRequestArises);
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
  }
}

UNIT_TEST("The corrector runs the postprocessing once for every cell of a time step") {
  GIVEN("A solver whose initial condition lies below the profile by the increment of its postprocessing") {
    const auto& cellConnectivity = buildGrid();

    PostprocessingSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn        release(solver);
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    const int nodes                   = cellConnectivity.getCells().size * NodesInCell;
    const int mismatchesBeforeTheStep = getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity);

    WHEN("The solver performs one time step") {
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);
      const int mismatchesAfterTheStep = getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity);

      THEN("Every node misses the profile before the step and matches it after the step") {
        VALIDATE_EQUALS(mismatchesBeforeTheStep, nodes);
        VALIDATE_EQUALS(mismatchesAfterTheStep, 0);
      }
    }
  }
}

UNIT_TEST("A refinement and the erase back keep the solution") {
  GIVEN("A fixed-rule solver on a regular grid after one time step, its coarse step size and live solver data") {
    const auto& cellConnectivity = buildGrid();

    FixedRuleADERDGSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn        release(solver);
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    const double coarseTimeStepSize = solver.getTimeStepping().getTimeStepSize();
    peano4::Spacetree::getInstance().markPersistentDataLive();

    WHEN("The cell in the centre refines") {
      regridWithLiveData({peano4::unittests::getRefinementOfCentreCell(Depth)}, requests, solver);
      const int fineCells = peano4::unittests::getGlobalSumOf(getNumberOfCellsOfLevel(cellConnectivity, Depth + 1));
      const int mismatches =
        peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity));

      THEN("The prolongation reproduces the profile on every child and the step shrinks by the subdivision factor") {
        VALIDATE_NUMERICAL_EQUALS(solver.getTimeStepping().getTimeStepSize(), coarseTimeStepSize / SubdivisionFactor);
        VALIDATE(fineCells > 0);
        VALIDATE_EQUALS(mismatches, 0);
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
          regridWithLiveData(peano4::unittests::getErasureOfEveryCell(Depth), requests, solver);
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
  }
}

UNIT_TEST("A linear time step reads every face flux across a level jump after the Riemann solvers write it") {
  GIVEN("A linear fixed-rule solver whose centre cell refined and whose face fluxes are all not a number") {
    const auto& cellConnectivity = buildGrid();

    LinearFixedRuleADERDGSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn        release(solver);
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    peano4::Spacetree::getInstance().markPersistentDataLive();
    regridWithLiveData({peano4::unittests::getRefinementOfCentreCell(Depth)}, requests, solver);
    const int fineCells = peano4::unittests::getGlobalSumOf(getNumberOfCellsOfLevel(cellConnectivity, Depth + 1));
    // The transfers of a level jump assert finite values in an Asserts build and trap a value that is
    // not a number in a build with floating-point exception traps.
    solver.presetFaceFluxes(std::numeric_limits<double>::quiet_NaN());

    WHEN("The solver performs one time step") {
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);
      const int mismatches = getNumberOfMismatchesAgainstTheProfile(solver, cellConnectivity);

      THEN("The grid has fine cells and every node keeps the profile") {
        VALIDATE(fineCells > 0);
        VALIDATE_EQUALS(mismatches, 0);
      }
    }
  }
}

UNIT_TEST("A refinement reduces the next adaptive time step to the stability bound of the finer grid") {
  GIVEN("An adaptive solver with the wave speed one on a regular grid after two time steps and its coarse step size") {
    const auto&          cellConnectivity = buildGrid();
    AdaptiveADERDGSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn        release(solver);
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
      regridWithLiveData({peano4::unittests::getRefinementOfCentreCell(Depth)}, requests, solver);
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);

      THEN("The step size is the coarse step size over the subdivision factor") {
        VALIDATE_NUMERICAL_EQUALS(
          solver.getTimeStepping().getTimeStepSizeOfLastStep(),
          coarseTimeStepSize / SubdivisionFactor
        );
      }
    }
  }
}

UNIT_TEST("Two coupled solvers split the cells, exchange their traces and advance together") {
  GIVEN("A left and a right solver coupled through identity matrices and initialised on a regular grid") {
    const auto& cellConnectivity = buildGrid();

    HalfSolver<0> left;
    HalfSolver<1> right;
    left.coupleWith(right, getIdentityOperator(), getIdentityOperator());
    exahype2::RefinementRequests requests;
    left.setLimits(getLimitsOfTheDomain());
    right.setLimits(getLimitsOfTheDomain());
    left.allocate(cellConnectivity, RuntimeParameters{});
    right.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn releaseLeft(left);
    const ReleaseOnReturn releaseRight(right);
    requests.clear(cellConnectivity.getNumberOfSlots());
    left.initialise(cellConnectivity, requests, RuntimeParameters{});
    right.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(left, right);

    WHEN("Both solvers perform three time steps and the left solver drives them") {
      const int           leftCells = peano4::unittests::getGlobalSumOf(left.getNumberOfCells(cellConnectivity));
      std::vector<int>    leftMismatchesOfStep;
      std::vector<int>    rightMismatchesOfStep;
      std::vector<double> leftTimeStampOfStep;
      std::vector<double> rightTimeStampOfStep;
      std::vector<int>    couplingMismatchesOfStep;
      std::vector<int>    couplingFacesOfStep;
      for (int step = 0; step < 3; step++) {
        requests.clear(cellConnectivity.getNumberOfSlots());
        left.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
        right.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
        finishStep(left, right);
        leftMismatchesOfStep.push_back(
          peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(left, cellConnectivity))
        );
        rightMismatchesOfStep.push_back(
          peano4::unittests::getGlobalSumOf(getNumberOfMismatchesAgainstTheProfile(right, cellConnectivity))
        );
        leftTimeStampOfStep.push_back(left.getTimeStepping().getTimeStamp());
        rightTimeStampOfStep.push_back(right.getTimeStepping().getTimeStamp());
        const auto [couplingFaces, mismatches] = getCouplingFaceMismatches(left, right, cellConnectivity);
        couplingMismatchesOfStep.push_back(peano4::unittests::getGlobalSumOf(mismatches));
        couplingFacesOfStep.push_back(peano4::unittests::getGlobalSumOf(couplingFaces));
      }
      const bool noRequestArises = requests.getRefinementRequests(cellConnectivity).empty();

      THEN("Both keep the profile at equal time stamps and the pools store the partner's half of every coupling face") {
        VALIDATE_EQUALS(leftCells, RegularCells / CellsPerAxis);
        for (std::size_t step = 0; step < leftMismatchesOfStep.size(); step++) {
          VALIDATE_EQUALS(leftMismatchesOfStep[step], 0, step);
          VALIDATE_EQUALS(rightMismatchesOfStep[step], 0, step);
          VALIDATE_NUMERICAL_EQUALS(leftTimeStampOfStep[step], static_cast<double>(step) * TimeStepFactor, step);
          VALIDATE_NUMERICAL_EQUALS(rightTimeStampOfStep[step], static_cast<double>(step) * TimeStepFactor, step);
          VALIDATE_EQUALS(couplingMismatchesOfStep[step], 0, step);
          VALIDATE_EQUALS(couplingFacesOfStep[step], peano4::power(CellsPerAxis, DIMENSIONS - 1), step);
        }
        VALIDATE(noRequestArises);
        VALIDATE_EQUALS(
          left.getNumberOfCells(cellConnectivity) + right.getNumberOfCells(cellConnectivity),
          cellConnectivity.getCells().size
        );
      }
    }
  }
}

UNIT_TEST("A coupled solver interpolates and restricts only the level jumps whose coarse side it owns") {
  GIVEN("Two coupled solvers that refined the centre cell and wrote the number of every face into its values") {
    const auto& cellConnectivity = buildGrid();

    HalfSolver<0> left;
    HalfSolver<1> right;
    left.coupleWith(right, getIdentityOperator(), getIdentityOperator());
    exahype2::RefinementRequests requests;
    left.setLimits(getLimitsOfTheDomain());
    right.setLimits(getLimitsOfTheDomain());
    left.allocate(cellConnectivity, RuntimeParameters{});
    right.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn releaseLeft(left);
    const ReleaseOnReturn releaseRight(right);
    requests.clear(cellConnectivity.getNumberOfSlots());
    left.initialise(cellConnectivity, requests, RuntimeParameters{});
    right.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(left, right);
    peano4::Spacetree::getInstance().markPersistentDataLive();
    regridWithLiveData({peano4::unittests::getRefinementOfCentreCell(Depth)}, requests, left, right);
    const int fineCells = peano4::unittests::getGlobalSumOf(getNumberOfCellsOfLevel(cellConnectivity, Depth + 1));
    left.writeFaceNumbersIntoFaces();
    right.writeFaceNumbersIntoFaces();

    WHEN("Both solvers perform one time step") {
      requests.clear(cellConnectivity.getNumberOfSlots());
      left.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      right.performTimeStep(cellConnectivity, requests, RuntimeParameters{});
      finishStep(left, right);
      const int leftMismatches            = getNumberOfMismatchesAgainstTheProfile(left, cellConnectivity);
      const int rightMismatches           = getNumberOfMismatchesAgainstTheProfile(right, cellConnectivity);
      const auto [levelJumps, mismatches] = getLevelJumpMismatchesAgainstTheFaceNumbers(left, cellConnectivity);
      const int globalLevelJumps          = peano4::unittests::getGlobalSumOf(levelJumps);
      const int globalMismatches          = peano4::unittests::getGlobalSumOf(mismatches);

      THEN("Both keep the profile and the face pools of the left solver keep the face numbers at every level jump") {
        VALIDATE(fineCells > 0);
        VALIDATE_EQUALS(leftMismatches, 0);
        VALIDATE_EQUALS(rightMismatches, 0);
        VALIDATE(globalLevelJumps > 0);
        VALIDATE_EQUALS(globalMismatches, 0);
      }
    }
  }
}

UNIT_TEST("A plot of every face writes the traces of the solution into both halves of every interior face") {
  GIVEN("A face-plotting fixed-rule solver initialised on a regular grid over every rank") {
    const auto& cellConnectivity = buildGrid();
    const auto& rank             = tarch::mpi::Rank::getInstance();

    FacePlottingFixedRuleADERDGSolver solver;
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn        release(solver);
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests, RuntimeParameters{});
    finishStep(solver);
    const std::string outputDirectory =
      (tarch::testing::getWritableTestDirectory() / "ADERDGSolverPlotTest").string() + "/";
    std::filesystem::create_directories(outputDirectory + "patch/");
    std::filesystem::create_directories(outputDirectory + "face/");

    WHEN("The solver plots its nodes and faces") {
      solver.plot(cellConnectivity, tarch::plotter::Format::VTU, outputDirectory, RuntimeParameters{});
      tarch::mpi::barrier(rank.getCommunicator());
      if (rank.isGlobalMaster()) {
        std::filesystem::remove_all(outputDirectory);
      }
      const auto [interiorFacesOfThisRank, mismatchesOfThisRank] =
        getTraceMismatchesAgainstTheProfile(solver, cellConnectivity, false);
      const int interiorFaces = peano4::unittests::getGlobalSumOf(interiorFacesOfThisRank);
      const int mismatches    = peano4::unittests::getGlobalSumOf(mismatchesOfThisRank);

      THEN("Both halves of every interior face store the profile, also across rank boundaries") {
        VALIDATE_EQUALS(mismatches, 0);
        VALIDATE(interiorFaces > 0);
      }
    }
  }
}

UNIT_TEST("The projection onto a plane writes the traces of the solution into its faces") {
  GIVEN("A fixed-rule solver that traces the faces of the interface plane on a regular grid over every rank") {
    const auto& cellConnectivity = buildGrid();

    FaceTracingFixedRuleADERDGSolver solver(Interface);
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn        release(solver);
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());

    WHEN("The initialisation of the solver projects its solution onto the interface plane") {
      solver.initialise(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);
      const auto [interfaceFacesOfThisRank, mismatchesOfThisRank] =
        getTraceMismatchesAgainstTheProfile(solver, cellConnectivity, true);
      const int interfaceFaces = peano4::unittests::getGlobalSumOf(interfaceFacesOfThisRank);
      const int mismatches     = peano4::unittests::getGlobalSumOf(mismatchesOfThisRank);

      THEN("Both halves of every face on the plane store the profile") {
        VALIDATE_EQUALS(mismatches, 0);
        VALIDATE(interfaceFaces > 0);
      }
    }
  }
}

UNIT_TEST("The projection onto a plane writes both halves of the coarse faces of the level jumps") {
  GIVEN("A fixed-rule solver that traces the faces of the lower boundary plane of the refined centre block") {
    const auto& cellConnectivity = buildGrid();
    peano4::unittests::refineGrid({peano4::unittests::getRefinementOfCentreCell(Depth)});
    const int    refinedCellsPerAxis = peano4::unittests::getNumberOfCellsPerAxisRefinedAroundTheCentre();
    const int    firstRefinedCell    = CellsPerAxis / 2 - refinedCellsPerAxis / 2;
    const double blockBoundary       = firstRefinedCell * CoarseWidth;

    FaceTracingFixedRuleADERDGSolver solver(blockBoundary);
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity, RuntimeParameters{});
    const ReleaseOnReturn        release(solver);
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());

    WHEN("The initialisation of the solver projects its solution onto the plane") {
      solver.initialise(cellConnectivity, requests, RuntimeParameters{});
      finishStep(solver);
      const auto [coarseFacesOfThisRank, mismatchesOfThisRank] =
        getCoarseFaceMismatchesOnPlane(solver, cellConnectivity, blockBoundary);
      const int coarseFaces = peano4::unittests::getGlobalSumOf(coarseFacesOfThisRank);
      const int mismatches  = peano4::unittests::getGlobalSumOf(mismatchesOfThisRank);

      THEN("Both halves of every coarse face on the plane store the profile") {
        VALIDATE_EQUALS(mismatches, 0);
        VALIDATE(coarseFaces > 0);
      }
    }
  }
}

UNIT_TEST("A release frees every pool that the allocation reserved") {
  GIVEN("A fixed-rule solver and a regular grid") {
    const auto& cellConnectivity = buildGrid();

    FixedRuleADERDGSolver solver;
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
      }
    }
  }
}
