// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SubcellLimiterTest.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/Solver.h"
#include "exahype2/dg/limiting/SubcellLimiter.h"
#include "exahype2/fv/singlestep/SubcellStep.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/CompilerSpecificSettings.h"
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

namespace {
  TARGET_DECLARE
  constexpr int SubdivisionFactor = peano4::SubdivisionFactor;
  /**
   * The smallest depth whose regular grid has at least three cells per axis.
   */
  constexpr int    Depth                = SubdivisionFactor >= 3 ? 1 : 2;
  constexpr int    CellsPerAxis         = peano4::power(SubdivisionFactor, Depth);
  constexpr int    RegularCells         = peano4::power(CellsPerAxis, DIMENSIONS);
  constexpr double CoarseWidth          = 1.0 / CellsPerAxis;
  constexpr int    Order                = 1;
  constexpr int    NodesPerAxis         = Order + 1;
  constexpr int    SubcellsPerAxis      = 2 * Order + 1;
  constexpr auto   EntriesPerOperator   = static_cast<std::size_t>(NodesPerAxis) * NodesPerAxis;
  constexpr auto   EntriesPerProjection = static_cast<std::size_t>(NodesPerAxis) * SubcellsPerAxis;
  constexpr double TimeStepFactor       = 0.5;
  constexpr double ConstantState        = 1.0;
  constexpr double Tolerance            = 1.0e-12;
  TARGET_DECLARE_END

  const double QuadratureNodes[NodesPerAxis]   = {0.5 - 0.5 / std::sqrt(3.0), 0.5 + 0.5 / std::sqrt(3.0)};
  const double QuadratureWeights[NodesPerAxis] = {0.5, 0.5};

  /**
   * Returns the Lagrange basis function of node at x.
   */
  double getBasisValue(const double x, const int node) {
    const int other = 1 - node;
    return (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
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
          const auto   entry                   = static_cast<std::size_t>(row) * NodesPerAxis + column;
          const auto   transposedEntry         = static_cast<std::size_t>(column) * NodesPerAxis + row;
          const double value                   = QuadratureWeights[column] * getBasisDerivative(row);
          stiffness[entry]                     = value;
          stiffnessTransposed[transposedEntry] = value;
          derivative[entry]                    = getBasisDerivative(column);
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

  /**
   * The projection tables of the limiter. A node takes the value of the subcell it lies in.
   */
  template <typename Precision>
  class SubcellProjection final {
  public:
    static void initialise() {
      std::vector<double> nodesToSubcells(EntriesPerProjection);
      std::vector<double> subcellsToNodes(EntriesPerProjection);
      std::vector<double> nodesToLobatto(EntriesPerOperator);
      for (int node = 0; node < NodesPerAxis; node++) {
        const int subcellOfNode = static_cast<int>(QuadratureNodes[node] * SubcellsPerAxis);
        for (int subcell = 0; subcell < SubcellsPerAxis; subcell++) {
          nodesToSubcells[static_cast<std::size_t>(node) * SubcellsPerAxis + subcell] =
            getBasisValue((subcell + 0.5) / SubcellsPerAxis, node);
          subcellsToNodes[static_cast<std::size_t>(subcell) * NodesPerAxis + node] =
            subcell == subcellOfNode ? 1.0 : 0.0;
        }
        nodesToLobatto[static_cast<std::size_t>(node) * NodesPerAxis]     = getBasisValue(0.0, node);
        nodesToLobatto[static_cast<std::size_t>(node) * NodesPerAxis + 1] = getBasisValue(1.0, node);
      }
      QuadraturePoints1D = getTableOf<Precision>({QuadratureNodes, QuadratureNodes + NodesPerAxis});
      NodesToSubcells1D  = getTableOf<Precision>(nodesToSubcells);
      SubcellsToNodes1D  = getTableOf<Precision>(subcellsToNodes);
      NodesToLobatto1D   = getTableOf<Precision>(nodesToLobatto);
    }

    static void deallocate() {
      dropTable(QuadraturePoints1D);
      dropTable(NodesToSubcells1D);
      dropTable(SubcellsToNodes1D);
      dropTable(NodesToLobatto1D);
    }

    inline static Precision* QuadraturePoints1D = nullptr;
    inline static Precision* NodesToSubcells1D  = nullptr;
    inline static Precision* SubcellsToNodes1D  = nullptr;
    inline static Precision* NodesToLobatto1D   = nullptr;
  };

  template <typename Solver>
  using SolverOf =
    exahype2::dg::ader::Solver<Solver, double, double, exahype2::dg::limiting::SubcellLimiter<Solver, double, double, exahype2::fv::singlestep::SubcellStep>>;

  /**
   * The PDE terms of every test solver, under which a constant state stays constant.
   */
  struct CommonTerms {
    static INLINE_METHOD GPU_CALLABLE_METHOD void initialCondition(
      double* const                                                 Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      Q[0] = ConstantState;
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

  struct AcceptingSolver final : SolverOf<AcceptingSolver>, CommonTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;
    static constexpr int  NumberOfObservables        = 0;
    static constexpr int  PatchSize                  = ::SubcellsPerAxis;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    using Reconstruction         = exahype2::fv::reconstruction::Constant;
    using FallbackReconstruction = void;

    template <typename Precision>
    using Quadrature = ::Quadrature<Precision>;
    template <typename Precision>
    using DGMatrices = ::DGMatrices<Precision>;
    template <typename Precision>
    using SubcellProjection = ::SubcellProjection<Precision>;

    AcceptingSolver() :
      Solver(TimeStepFactor, "AcceptingSolver", {{"q", 1}}, 0.0, 0.0) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const int                                    numberOfStates,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      return true;
    }
  };

  struct RejectingSolver final : SolverOf<RejectingSolver>, CommonTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;
    static constexpr int  NumberOfObservables        = 0;
    static constexpr int  PatchSize                  = ::SubcellsPerAxis;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    using Reconstruction         = exahype2::fv::reconstruction::Constant;
    using FallbackReconstruction = void;

    template <typename Precision>
    using Quadrature = ::Quadrature<Precision>;
    template <typename Precision>
    using DGMatrices = ::DGMatrices<Precision>;
    template <typename Precision>
    using SubcellProjection = ::SubcellProjection<Precision>;

    RejectingSolver() :
      SolverOf<RejectingSolver>(TimeStepFactor, "RejectingSolver", {{"q", 1}}, 0.0, 0.0) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const int                                    numberOfStates,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      return false;
    }
  };

  /**
   * A solver that rejects the candidate of the cell at the origin corner of the domain.
   */
  struct CornerSolver final : SolverOf<CornerSolver>, CommonTerms {
    static constexpr int  DGOrder                    = ::Order;
    static constexpr int  NumberOfUnknowns           = 1;
    static constexpr int  NumberOfAuxiliaryVariables = 0;
    static constexpr bool IsLinear                   = false;
    static constexpr int  NumberOfPointSources       = 0;
    static constexpr int  NumberOfObservables        = 0;
    static constexpr int  PatchSize                  = ::SubcellsPerAxis;

    static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

    using Reconstruction         = exahype2::fv::reconstruction::Constant;
    using FallbackReconstruction = void;

    template <typename Precision>
    using Quadrature = ::Quadrature<Precision>;
    template <typename Precision>
    using DGMatrices = ::DGMatrices<Precision>;
    template <typename Precision>
    using SubcellProjection = ::SubcellProjection<Precision>;

    CornerSolver() :
      Solver(TimeStepFactor, "CornerSolver", {{"q", 1}}, 0.0, 0.0) {}

    static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
      [[maybe_unused]] const double* const                          Q,
      [[maybe_unused]] const int                                    numberOfStates,
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (x(axis) > CoarseWidth) {
          return true;
        }
      }
      return false;
    }
  };

  const peano4::CellConnectivity& buildGrid() {
    peano4::tests::buildRegularGrid(Depth);
    return peano4::Spacetree::getInstance().getCellConnectivity();
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
   * Increments the accumulator for every node of one cell that differs from the constant state.
   */
  template <typename Pool>
  struct CountMismatchesAgainstTheConstantState final {
    static constexpr int NodesInCell = exahype2::dg::getNodesPerCell(NodesPerAxis);

    peano4::CellConnectivity cellConnectivity{};
    typename Pool::ConstView solution{};
    double                   tolerance{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry, int& accumulator) const {
      const double* const values = solution.getChunk(cellConnectivity.getCells()[entry]);
      for (int node = 0; node < NodesInCell; node++) {
        if (std::abs(values[node] - ConstantState) > tolerance) {
          accumulator++;
        }
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Counts the nodes of every cell whose value differs from the constant by more than tolerance.
   */
  template <typename Solver>
  int getNumberOfMismatches(
    const Solver&                   solver,
    const peano4::CellConnectivity& cellConnectivity,
    const double                    tolerance
  ) {
    int result = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(cellConnectivity.getCells().size),
      CountMismatchesAgainstTheConstantState<typename Solver::CellPool>{
        cellConnectivity.onDevice(),
        solver.getSolution().getView(),
        tolerance
      },
      tarch::accelerator::Sum<int>{result}
    );
    return result;
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
   * Initialises solver on the regular grid, runs steps time steps and returns the cell
   * connectivity of this rank.
   */
  template <typename Solver>
  const peano4::CellConnectivity& advance(Solver& solver, const int steps) {
    const auto& cellConnectivity = buildGrid();
    solver.setLimits(getLimitsOfTheDomain());
    solver.allocate(cellConnectivity);
    exahype2::RefinementRequests requests;
    requests.clear(cellConnectivity.getNumberOfSlots());
    solver.initialise(cellConnectivity, requests);
    finishStep(solver);
    for (int step = 0; step < steps; step++) {
      requests.clear(cellConnectivity.getNumberOfSlots());
      solver.performTimeStep(cellConnectivity, requests);
      finishStep(solver);
    }
    return cellConnectivity;
  }
} // namespace

exahype2::dg::limiting::tests::SubcellLimiterTest::SubcellLimiterTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::SubcellLimiterTest") {}

void
exahype2::dg::limiting::tests::SubcellLimiterTest::run() {
  testMethod(testAnAcceptedFreeStreamStaysOnTheGalerkinSchemeBitwise);
  testMethod(testARejectedFreeStreamRecomputesEveryCellAndKeepsTheConstant);
  testMethod(testOneTroubledCellSpreadsTwoRingsOfNeighbours);
  testMethod(testTheSubgridBoundaryConditionsSkipTheCellsOffTheScheme);
  testMethod(testAPlotAllocatesHostCopiesOnlyWithAGPUBackend);
  testMethod(testReleaseFreesEveryPool);
}

/**
 * The subgrid boundary conditions fill the outer half of a boundary face beside a cell on the
 * subcell scheme and skip the faces of Regular cells.
 */
void
exahype2::dg::limiting::tests::SubcellLimiterTest::testTheSubgridBoundaryConditionsSkipTheCellsOffTheScheme() {
  using Scheme                      = exahype2::dg::limiting::SubcellScheme<CornerSolver, double>;
  constexpr int    ValuesPerHalf    = Scheme::HaloValuesPerFace / 2;
  constexpr double LowerHalfValue   = 1.0;
  constexpr double UpperHalfValue   = 2.0;
  constexpr int    StatusEntryIndex = static_cast<int>(exahype2::dg::limiting::StatusEntry::Status);
  const auto&      cellConnectivity = buildGrid();

  tarch::accelerator::ChunkedMemoryPool<double> halo(Scheme::HaloValuesPerFace, tarch::MemoryLocation::Device);
  halo.reserve(cellConnectivity.getNumberOfFaces());
  std::vector<double> haloOnHost;
  halo.copyToHost(haloOnHost);
  const auto paddedFace = static_cast<std::size_t>(halo.getPaddedChunkSize());
  for (int face = 0; face < cellConnectivity.getNumberOfFaces(); face++) {
    for (int value = 0; value < Scheme::HaloValuesPerFace; value++) {
      haloOnHost[static_cast<std::size_t>(face) * paddedFace + static_cast<std::size_t>(value)] =
        value < ValuesPerHalf ? LowerHalfValue : UpperHalfValue;
    }
  }
  halo.copyFromHost(haloOnHost);

  tarch::accelerator::ChunkedMemoryPool<int> statuses(exahype2::dg::limiting::NumberOfStatusEntries);
  statuses.reserve(cellConnectivity.getNumberOfSlots());
  std::vector<int> statusesOnHost;
  statuses.copyToHost(statusesOnHost);
  const auto paddedStatus = static_cast<std::size_t>(statuses.getPaddedChunkSize());
  const auto cells        = cellConnectivity.getCells();
  for (int entry = 0; entry < cells.size; entry++) {
    statusesOnHost[static_cast<std::size_t>(cells[entry]) * paddedStatus + StatusEntryIndex] = static_cast<int>(
      entry % 2 == 0 ? exahype2::dg::limiting::LimiterStatus::Limited : exahype2::dg::limiting::LimiterStatus::Regular
    );
  }
  statuses.copyFromHost(statusesOnHost);

  const auto boundaryFaces = cellConnectivity.onDevice().getFaces(peano4::Adjacency::Local, peano4::FaceKind::Boundary);
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(boundaryFaces.size),
    exahype2::dg::limiting::ApplySubgridBoundaryConditions<CornerSolver, double, double>{
      {cellConnectivity.onDevice(), boundaryFaces, halo.getView(), 0.0},
      std::as_const(statuses).getView()
    }
  );
  halo.copyToHost(haloOnHost);

  for (const int face : cellConnectivity.getFaces(peano4::Adjacency::Local, peano4::FaceKind::Boundary)) {
    const int  lowerCell = cellConnectivity.getFaceCell(face, 0);
    const int  slot      = lowerCell >= 0 ? lowerCell : cellConnectivity.getFaceCell(face, 1);
    const bool onScheme  = statusesOnHost[static_cast<std::size_t>(slot) * paddedStatus + StatusEntryIndex]
      != static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular);
    const int  outerHalf = lowerCell >= 0 ? 1 : 0;
    const auto firstOuter =
      static_cast<std::size_t>(face) * paddedFace + static_cast<std::size_t>(outerHalf * ValuesPerHalf);
    const double innerValue = lowerCell >= 0 ? LowerHalfValue : UpperHalfValue;
    const double outerValue = lowerCell >= 0 ? UpperHalfValue : LowerHalfValue;
    validateEqualsWithParams2(haloOnHost[firstOuter], onScheme ? innerValue : outerValue, face, onScheme);
  }
}

void
exahype2::dg::limiting::tests::SubcellLimiterTest::testAnAcceptedFreeStreamStaysOnTheGalerkinSchemeBitwise() {
  AcceptingSolver solver;
  const auto&     cellConnectivity = advance(solver, 2);
  validateEquals(getNumberOfMismatches(solver, cellConnectivity, 0.0), 0);
  validateEquals(peano4::tests::getGlobalSumOf(cellConnectivity.getCells().size), RegularCells);
  validateEquals(solver.getLimiter().getNumberOfTroubledCells(), 0);
  validateEquals(solver.getLimiter().getGlobalNumberOfTroubledCells(), 0);
  validateEquals(solver.getLimiter().getNumberOfRecomputedCells(), 0);
  validateEquals(solver.getLimiter().getNumberOfCellsOnSubcellScheme(), 0);
  validateNumericalEquals(solver.getTimeStepping().getTimeStamp(), TimeStepFactor);
  solver.release();
}

/**
 * The subgrid of a constant keeps the constant, and a rejected cell stays on the subcell scheme in the next step.
 */
void
exahype2::dg::limiting::tests::SubcellLimiterTest::testARejectedFreeStreamRecomputesEveryCellAndKeepsTheConstant() {
  RejectingSolver solver;
  const auto&     cellConnectivity = advance(solver, 2);
  const int       numberOfCells    = cellConnectivity.getCells().size;
  validateEquals(getNumberOfMismatches(solver, cellConnectivity, Tolerance), 0);
  validateEquals(solver.getLimiter().getNumberOfTroubledCells(), numberOfCells);
  validateEquals(solver.getLimiter().getGlobalNumberOfTroubledCells(), peano4::tests::getGlobalSumOf(numberOfCells));
  validateEquals(solver.getLimiter().getNumberOfRecomputedCells(), numberOfCells);
  validateEquals(solver.getLimiter().getNumberOfCellsOnSubcellScheme(), numberOfCells);
  solver.release();
}

/**
 * A troubled corner cell makes its face neighbours Limited and the cells two faces away Halo.
 */
void
exahype2::dg::limiting::tests::SubcellLimiterTest::testOneTroubledCellSpreadsTwoRingsOfNeighbours() {
  static_assert(CellsPerAxis >= 3, "The test needs at least three cells per axis.");
  CornerSolver solver;
  const auto&  cellConnectivity = advance(solver, 1);
  validateEquals(getNumberOfMismatches(solver, cellConnectivity, Tolerance), 0);
  const int troubled   = peano4::tests::getGlobalSumOf(solver.getLimiter().getNumberOfTroubledCells());
  const int recomputed = peano4::tests::getGlobalSumOf(solver.getLimiter().getNumberOfRecomputedCells());
  const int subcell    = peano4::tests::getGlobalSumOf(solver.getLimiter().getNumberOfCellsOnSubcellScheme());
  validateEquals(troubled, 1);
  validateEquals(solver.getLimiter().getGlobalNumberOfTroubledCells(), 1);
  validateEquals(recomputed - troubled, DIMENSIONS);
  validateEquals(subcell - recomputed, DIMENSIONS * (DIMENSIONS + 1) / 2);
  solver.release();
}

/**
 * A plot of cells on the subcell scheme writes the .vtu file of every rank and allocates host copies
 * only in a build with a GPU backend.
 */
void
exahype2::dg::limiting::tests::SubcellLimiterTest::testAPlotAllocatesHostCopiesOnlyWithAGPUBackend() {
  RejectingSolver solver;
  const auto&     cellConnectivity = advance(solver, 1);
  const auto&     rank             = tarch::mpi::Rank::getInstance();
  validateEquals(solver.getLimiter().getNumberOfCellsOnSubcellScheme(), cellConnectivity.getCells().size);

  const std::string outputDirectory =
    (tarch::tests::getWritableTestDirectory() / "SubcellLimiterPlotTest").string() + "/";
  std::filesystem::create_directories(outputDirectory + "patch/");
  const tarch::AllocatedBytes bytesBeforePlot = solver.getAllocatedBytes();
  solver.plot(cellConnectivity, tarch::plotter::Format::VTU, outputDirectory);
  const tarch::AllocatedBytes bytesAfterPlot = solver.getAllocatedBytes();

  const auto& solution = solver.getSolution();
  if (solution.isHostAccessible()) {
    validateEquals(bytesAfterPlot.host, bytesBeforePlot.host);
  } else {
    validate(bytesAfterPlot.host > bytesBeforePlot.host + solution.getAllocatedBytes().device);
  }
  validateEquals(bytesAfterPlot.device, bytesBeforePlot.device);
  const std::string           vtuName = "RejectingSolver-0_000000-rank-" + std::to_string(rank.getRank()) + ".vtu";
  const std::filesystem::path vtuPath = std::filesystem::path(outputDirectory) / "patch" / vtuName;
  validateWithParams1(std::filesystem::exists(vtuPath), vtuPath.string());
  solver.release();

  tarch::mpi::barrier(rank.getCommunicator());
  if (rank.isGlobalMaster()) {
    std::filesystem::remove_all(outputDirectory);
  }
}

void
exahype2::dg::limiting::tests::SubcellLimiterTest::testReleaseFreesEveryPool() {
  const auto& cellConnectivity = buildGrid();

  AcceptingSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
  solver.allocate(cellConnectivity);
  validate(solver.getAllocatedBytes().getTotal() > 0);
  validate(solver.getLimiter().getAllocatedBytes().getTotal() > 0);
  solver.release();
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
}
