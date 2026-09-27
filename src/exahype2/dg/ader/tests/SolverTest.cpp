// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include "exahype2/Limits.h"
#include "exahype2/RefinementCommand.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/Solver.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Partition.h"
#include "peano4/RefinementRequest.h"
#include "peano4/Regrid.h"
#include "peano4/Spacetree.h"
#include "peano4/tests/TestGrids.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  TARGET_DECLARE
  constexpr int    SubdivisionFactor  = peano4::SubdivisionFactor;
  constexpr int    Depth              = peano4::tests::DepthWithCoarseCellsBesideTheCentreBlock;
  constexpr int    CellsPerAxis       = peano4::power(SubdivisionFactor, Depth);
  constexpr int    RegularCells       = peano4::power(CellsPerAxis, DIMENSIONS);
  constexpr double CoarseWidth        = 1.0 / CellsPerAxis;
  constexpr int    Order              = 1;
  constexpr int    NodesPerAxis       = Order + 1;
  constexpr int    NodesInCell        = exahype2::dg::getNodesPerCell(NodesPerAxis);
  constexpr auto   EntriesPerOperator = static_cast<std::size_t>(NodesPerAxis) * NodesPerAxis;
  constexpr double TimeStepFactor     = 0.5;
  constexpr double Tolerance          = 1.0e-12;
  TARGET_DECLARE_END

  const double QuadratureNodes[NodesPerAxis]   = {0.5 - 0.5 / std::sqrt(3.0), 0.5 + 0.5 / std::sqrt(3.0)};
  const double QuadratureWeights[NodesPerAxis] = {0.5, 0.5};

  /** Returns the Lagrange basis function of node at x. */
  double getBasisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NodesPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

  /** Returns the derivative of the Lagrange basis function of node at x. */
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

  /** Returns a new table in device memory, the memory the kernels read, with the values converted to Precision. */
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
   * The quadrature tables of the Gauss-Legendre basis of order one, in the layout of the generated tables. Every
   * Instance holds its own tables, so two solvers that are allocated at the same time use two Instance values.
   */
  template <typename Precision, int Instance>
  class QuadratureTables final {
  public:
    static void initialise() {
      assertion(QuadraturePoints1D == nullptr, Instance);
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
   * The reference operators of the Gauss-Legendre basis of order one, in the layout of the generated tables. Every
   * Instance holds its own tables.
   */
  template <typename Precision, int Instance>
  class DGMatrixTables final {
  public:
    static void initialise() {
      assertion(StiffnessOperator1D == nullptr, Instance);
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

  struct LinearADERDGSolver;
  struct AdaptiveADERDGSolver;
  template <int Side>
  struct HalfSolver;

  /** The PDE terms of a linear initial condition without flux and wave speed, so a step changes nothing. */
  struct LinearTerms {
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

  /** A solver with one unknown and no point source, which reads the tables of side 0. */
  struct LinearADERDGSolver final : exahype2::dg::ader::Solver<LinearADERDGSolver, double, double, void>, LinearTerms {
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

    using Solver::getCells;
    using Solver::getRiemannFaces;
    using Solver::projectSolutionOntoFaces;
    using Solver::projectSolutionOntoPlane;

    LinearADERDGSolver() :
      Solver(TimeStepFactor, "LinearADERDGSolver", {{"q", 1}}) {}
  };

  /** The linear solver with an adaptive rule and a unit wave speed. */
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
      [[maybe_unused]] const int                                    normal
    ) {
      return 1.0;
    }
  };

  /** The plane between the cells the two coupled solvers own. */
  constexpr double Interface = CoarseWidth;

  /** Owns the cells left of the interface for Side 0 and the cells right of it for Side 1. */
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
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      return Side == 0 ? x(0) < Interface : x(0) > Interface;
    }
  };

  /** Builds the regular grid of Depth levels partitioned over the ranks and returns the partition of this rank. */
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

  TARGET_DECLARE
  /** Copies one entry of a list in the memory of the kernels into a buffer the test reads back. */
  struct CopyListEntries final {
    tarch::Span<const int> source{};
    int*                   target{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int entry) const { target[entry] = source[entry]; }
  };
  TARGET_DECLARE_END

  /** Returns the entries of a list in the memory of the kernels. */
  std::vector<int> readList(const tarch::Span<const int> list) {
    if (list.isEmpty()) {
      return {};
    }
    tarch::accelerator::Buffer<int> target(list.size, tarch::MemoryLocation::Device);
    tarch::accelerator::parallelFor(tarch::accelerator::Range(list.size), CopyListEntries{list, target.getData()});
    return target.getValues();
  }

  /** Returns for every face whether it lies in a group that depends on the data of another rank. */
  std::vector<bool> getRemoteFaceMarks(const peano4::Partition& partition) {
    std::vector<bool> remote(static_cast<std::size_t>(partition.getNumberOfFaces()), false);
    for (int kind = 0; kind < peano4::NumberOfFaceKinds; kind++) {
      for (const int face : partition.getFaces(peano4::Dependence::Remote, static_cast<peano4::FaceKind>(kind))) {
        remote[static_cast<std::size_t>(face)] = true;
      }
    }
    return remote;
  }

  bool cellDependsOnRemoteData(const peano4::Partition& partition, const std::vector<bool>& remote, const int cell) {
    bool depends = false;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      depends = depends or remote[static_cast<std::size_t>(partition.getCellFace(cell, faceNumber))];
    }
    return depends;
  }

  /** Returns the number of interior, boundary and hanging faces of the partition. */
  int getNumberOfRiemannFacesOf(const peano4::Partition& partition) {
    int riemannFaces = 0;
    for (const auto dependence : {peano4::Dependence::Local, peano4::Dependence::Remote}) {
      for (const auto kind : {peano4::FaceKind::Interior, peano4::FaceKind::Boundary, peano4::FaceKind::Hanging}) {
        riemannFaces += partition.getFaces(dependence, kind).size;
      }
    }
    return riemannFaces;
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
  /** Increments the accumulator for every node of one cell whose value differs from the profile there. */
  template <typename Pool>
  struct CountMismatchesAgainstTheProfile final {
    peano4::Partition        partition{};
    typename Pool::ConstView solution{};
    const double*            quadraturePoints{nullptr};

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
        if (std::abs(values[peano4::dLinearised(node, NodesPerAxis)] - LinearTerms::getProfile(position)) > Tolerance) {
          accumulator++;
        }
      });
    }
  };
  TARGET_DECLARE_END

  /** Counts the nodes of every cell whose value differs from the profile at the node. */
  template <typename Solver>
  int getNumberOfMismatchesAgainstTheProfile(const Solver& solver, const peano4::Partition& partition) {
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getCells().size),
      CountMismatchesAgainstTheProfile<typename Solver::CellPool>{
        partition.onDevice(),
        solver.getSolution().getView(),
        Quadrature<double>::QuadraturePoints1D
      },
      tarch::accelerator::Sum<int>{mismatches}
    );
    return mismatches;
  }

  /** Returns whether the face is an interior face with normal 0 on the interface plane. */
  GPU_CALLABLE_METHOD bool isInterfaceFace(const peano4::Partition& partition, const int face) {
    return partition.getFaceNormal(face) == 0 and std::abs(partition.getFaceCentre(face, 0) - Interface) < Tolerance
      and partition.getFaceKind(face) == peano4::FaceKind::Interior;
  }

  TARGET_DECLARE
  /** Increments the accumulator when the face is an interface face that holds a cell on its lower side. */
  struct CountCouplingFace final {
    peano4::Partition partition{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (isInterfaceFace(partition, face) and partition.getFaceCell(face, 0) >= 0) {
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

    peano4::Partition                   partition{};
    HalfSolver<0>::StatePool::ConstView leftStates{};
    HalfSolver<1>::StatePool::ConstView rightStates{};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (not isInterfaceFace(partition, face)) {
        return;
      }
      tarch::la::Vector<DIMENSIONS, double> centre;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        centre(axis) = partition.getFaceCentre(face, axis);
      }
      for (int side = 0; side < 2; side++) {
        if (partition.getFaceCell(face, side) >= 0) {
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
          if (not equal or std::abs(mean - LinearTerms::getProfile(centre)) > Tolerance) {
            accumulator++;
          }
        }
      }
    }
  };

  /**
   * Increments the accumulator for every node of both halves of an interior face, of an interface
   * face only with interfaceOnly, whose face state differs from the profile.
   */
  struct CountTraceMismatchesAgainstTheProfile final {
    static constexpr int NodesPerFace = exahype2::dg::getNodesPerFace(NodesPerAxis);

    peano4::Partition                        partition{};
    LinearADERDGSolver::StatePool::ConstView faceStates{};
    const double*                            quadraturePoints{nullptr};
    bool                                     interfaceOnly{false};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (
        partition.getFaceKind(face) != peano4::FaceKind::Interior
        or (interfaceOnly and not isInterfaceFace(partition, face))
      ) {
        return;
      }
      const int           normal = partition.getFaceNormal(face);
      const double* const halves = faceStates.getChunk(face);
      for (int node = 0; node < NodesPerFace; node++) {
        const auto                            nodeIndex = peano4::dDelinearisedExclusive(node, NodesPerAxis, normal);
        tarch::la::Vector<DIMENSIONS, double> position;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          position(axis) = partition.getFaceCentre(face, axis);
          if (axis != normal) {
            position(axis) += (quadraturePoints[nodeIndex(axis)] - 0.5) * partition.getFaceWidth(face, axis);
          }
        }
        for (int half = 0; half < 2; half++) {
          if (std::abs(halves[half * NodesPerFace + node] - LinearTerms::getProfile(position)) > Tolerance) {
            accumulator++;
          }
        }
      }
    }
  };

  /** Increments the accumulator for every interior face, for every interface face only with interfaceOnly. */
  struct CountInteriorFace final {
    peano4::Partition partition{};
    bool              interfaceOnly{false};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int face, int& accumulator) const {
      if (
        partition.getFaceKind(face) == peano4::FaceKind::Interior
        and (not interfaceOnly or isInterfaceFace(partition, face))
      ) {
        accumulator++;
      }
    }
  };
  TARGET_DECLARE_END

  /**
   * Counts the interface faces with a cell on the lower side and the halves of the interface whose
   * copy differs from the partner's half or from the profile.
   */
  std::pair<int, int> getCouplingFaceMismatches(
    const HalfSolver<0>&     left,
    const HalfSolver<1>&     right,
    const peano4::Partition& partition
  ) {
    int couplingFaces = 0;
    int mismatches    = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getNumberOfFaces()),
      CountCouplingFace{partition.onDevice()},
      tarch::accelerator::Sum<int>{couplingFaces}
    );
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getNumberOfFaces()),
      CountTraceMismatchesOfFace{partition.onDevice(), left.getFaceStates().getView(), right.getFaceStates().getView()},
      tarch::accelerator::Sum<int>{mismatches}
    );
    return {couplingFaces, mismatches};
  }

  /**
   * Counts the interior faces, the interface faces only with interfaceOnly, and the nodes of their
   * halves whose face state differs from the profile.
   */
  std::pair<int, int> getTraceMismatchesAgainstTheProfile(
    const LinearADERDGSolver& solver,
    const peano4::Partition&  partition,
    const bool                interfaceOnly
  ) {
    int faces      = 0;
    int mismatches = 0;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getNumberOfFaces()),
      CountInteriorFace{partition.onDevice(), interfaceOnly},
      tarch::accelerator::Sum<int>{faces}
    );
    tarch::accelerator::parallelReduce(
      tarch::accelerator::Range(partition.getNumberOfFaces()),
      CountTraceMismatchesAgainstTheProfile{
        partition.onDevice(),
        solver.getFaceStates().getView(),
        Quadrature<double>::QuadraturePoints1D,
        interfaceOnly
      },
      tarch::accelerator::Sum<int>{mismatches}
    );
    return {faces, mismatches};
  }
} // namespace

exahype2::dg::ader::tests::SolverTest::SolverTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::SolverTest") {}

void
exahype2::dg::ader::tests::SolverTest::run() {
  testMethod(testInitialisationWritesTheInitialConditionIntoEveryNode);
  testMethod(testATimeStepWithoutFluxKeepsTheSolutionAndAdvancesTheTimeStamp);
  testMethod(testARefinementAndTheEraseBackKeepTheSolution);
  testMethod(testRefinementReducesTheNextAdaptiveTimeStep);
  testMethod(testTwoCoupledSolversExchangeTheirTracesAndAdvanceTogether);
  testMethod(testTheCellAndFaceGroupsSplitAtTheRankBoundary);
  testMethod(testTheProjectionWritesTheTracesOfTheSolutionIntoBothHalvesOfEveryInteriorFace);
  testMethod(testTheProjectionOntoAPlaneWritesTheTracesOfTheSolutionIntoItsFaces);
  testMethod(testReleaseFreesEveryPool);
}

/**
 * Both halves of every interior face hold the values of the solution after the projection, also
 * across rank boundaries.
 */
void
exahype2::dg::ader::tests::SolverTest::
  testTheProjectionWritesTheTracesOfTheSolutionIntoBothHalvesOfEveryInteriorFace() {
  const auto& partition = buildGrid();

  LinearADERDGSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  solver.projectSolutionOntoFaces(partition);

  const auto [interiorFaces, mismatches] = getTraceMismatchesAgainstTheProfile(solver, partition, false);
  validateEquals(peano4::tests::getGlobalSumOf(mismatches), 0);
  validate(peano4::tests::getGlobalSumOf(interiorFaces) > 0);
  solver.release();
}

/**
 * The local cells and the local Riemann faces have no face on a rank boundary, the remote groups
 * have one, and the groups together list every cell and every Riemann face.
 */
void
exahype2::dg::ader::tests::SolverTest::testTheCellAndFaceGroupsSplitAtTheRankBoundary() {
  const auto& partition = buildGrid();

  LinearADERDGSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  const std::vector<bool> remote = getRemoteFaceMarks(partition);

  const std::vector<int> localCells  = readList(solver.getCells(partition, peano4::Dependence::Local));
  const std::vector<int> remoteCells = readList(solver.getCells(partition, peano4::Dependence::Remote));
  validateEquals(static_cast<int>(localCells.size() + remoteCells.size()), partition.getCells().size);
  for (const int cell : localCells) {
    validateWithParams1(not cellDependsOnRemoteData(partition, remote, cell), cell);
  }
  for (const int cell : remoteCells) {
    validateWithParams1(cellDependsOnRemoteData(partition, remote, cell), cell);
  }

  const std::vector<int> localFaces  = readList(solver.getRiemannFaces(partition, peano4::Dependence::Local));
  const std::vector<int> remoteFaces = readList(solver.getRiemannFaces(partition, peano4::Dependence::Remote));
  validateEquals(static_cast<int>(localFaces.size() + remoteFaces.size()), getNumberOfRiemannFacesOf(partition));
  for (const int face : localFaces) {
    validateWithParams1(not remote[static_cast<std::size_t>(face)], face);
  }
  for (const int face : remoteFaces) {
    validateWithParams1(remote[static_cast<std::size_t>(face)], face);
  }
  const bool runsAlone      = tarch::mpi::Rank::getInstance().getNumberOfRanks() == 1;
  const bool hasRemoteFaces = peano4::tests::getGlobalSumOf(static_cast<int>(remoteFaces.size())) > 0;
  const bool hasRemoteCells = peano4::tests::getGlobalSumOf(static_cast<int>(remoteCells.size())) > 0;
  validateEquals(hasRemoteFaces, not runsAlone);
  validateEquals(hasRemoteCells, not runsAlone);
  solver.release();
}

/** Both halves of every face on the plane hold the values of the solution after the projection onto that plane. */
void
exahype2::dg::ader::tests::SolverTest::testTheProjectionOntoAPlaneWritesTheTracesOfTheSolutionIntoItsFaces() {
  const auto& partition = buildGrid();

  LinearADERDGSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  solver.allocate(partition);
  exahype2::RefinementRequests requests;
  requests.clear(partition.getNumberOfSlots());
  solver.initialise(partition, requests);
  solver.projectSolutionOntoPlane(partition, 0, Interface);

  const auto [interfaceFaces, mismatches] = getTraceMismatchesAgainstTheProfile(solver, partition, true);
  validateEquals(peano4::tests::getGlobalSumOf(mismatches), 0);
  validate(peano4::tests::getGlobalSumOf(interfaceFaces) > 0);
  solver.release();
}

/** The first time step after a refinement uses the stability bound of the finer grid. */
void
exahype2::dg::ader::tests::SolverTest::testRefinementReducesTheNextAdaptiveTimeStep() {
  const auto&          partition = buildGrid();
  AdaptiveADERDGSolver solver;
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

/**
 * The two solvers split the cells of the partition, the left solver drives both, every coupling
 * face of the interface plane carries the partner's halves in both pools, and both solvers keep
 * the profile and the same time stamp.
 */
void
exahype2::dg::ader::tests::SolverTest::testTwoCoupledSolversExchangeTheirTracesAndAdvanceTogether() {
  const auto& partition = buildGrid();

  std::vector<double> identity(EntriesPerOperator, 0.0);
  for (int node = 0; node < NodesPerAxis; node++) {
    identity[static_cast<std::size_t>(node) * NodesPerAxis + node] = 1.0;
  }
  HalfSolver<0> left;
  HalfSolver<1> right;
  left.coupleWith(right, identity, identity);
  exahype2::RefinementRequests requests;
  left.setLimits(getLimitsOfTheDomain());
  right.setLimits(getLimitsOfTheDomain());
  left.allocate(partition);
  right.allocate(partition);
  requests.clear(partition.getNumberOfSlots());
  left.initialise(partition, requests);
  right.initialise(partition, requests);
  validateEquals(left.getNumberOfCells(partition) + right.getNumberOfCells(partition), partition.getCells().size);
  validateEquals(peano4::tests::getGlobalSumOf(left.getNumberOfCells(partition)), RegularCells / CellsPerAxis);

  for (int step = 0; step < 3; step++) {
    requests.clear(partition.getNumberOfSlots());
    left.performTimeStep(partition, requests);
    right.performTimeStep(partition, requests);
    validateEqualsWithParams1(getNumberOfMismatchesAgainstTheProfile(left, partition), 0, step);
    validateEqualsWithParams1(getNumberOfMismatchesAgainstTheProfile(right, partition), 0, step);
    validateNumericalEqualsWithParams1(left.getTimeStepping().getTimeStamp(), step * TimeStepFactor, step);
    validateNumericalEqualsWithParams1(right.getTimeStepping().getTimeStamp(), step * TimeStepFactor, step);
    const auto [couplingFaces, mismatches] = getCouplingFaceMismatches(left, right, partition);
    validateEqualsWithParams1(mismatches, 0, step);
    validateEqualsWithParams1(
      peano4::tests::getGlobalSumOf(couplingFaces),
      peano4::power(CellsPerAxis, DIMENSIONS - 1),
      step
    );
  }
  validate(requests.getRefinementRequests(partition).empty());
  left.release();
  right.release();
}

void
exahype2::dg::ader::tests::SolverTest::testInitialisationWritesTheInitialConditionIntoEveryNode() {
  const auto& partition = buildGrid();

  LinearADERDGSolver solver;
  auto               constructionLimits     = getLimitsOfTheDomain();
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
  validate(solver.getAllocatedBytes().getTotal() > 0);
  validate(requests.getRefinementRequests(partition).empty());
  solver.release();
}

/**
 * The first step runs with a zero step size; every later one advances by the factor times the
 * mesh width over the admissible width.
 */
void
exahype2::dg::ader::tests::SolverTest::testATimeStepWithoutFluxKeepsTheSolutionAndAdvancesTheTimeStamp() {
  const auto& partition = buildGrid();

  LinearADERDGSolver solver;
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
    validateEqualsWithParams1(getNumberOfMismatchesAgainstTheProfile(solver, partition), 0, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStamp(), step * TimeStepFactor, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStepSizeOfLastStep(), expectedStepSize, step);
    validateNumericalEqualsWithParams1(solver.getTimeStepping().getTimeStepSize(), TimeStepFactor, step);
  }
  validate(requests.getRefinementRequests(partition).empty());
  solver.release();
}

/**
 * The prolongation reproduces the linear profile on every child, and the restriction of the
 * children returns it on the coarse cell within rounding.
 */
void
exahype2::dg::ader::tests::SolverTest::testARefinementAndTheEraseBackKeepTheSolution() {
  const auto& partition = buildGrid();

  LinearADERDGSolver solver;
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

void
exahype2::dg::ader::tests::SolverTest::testReleaseFreesEveryPool() {
  const auto& partition = buildGrid();

  LinearADERDGSolver solver;
  solver.setLimits(getLimitsOfTheDomain());
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
  solver.allocate(partition);
  validate(solver.getAllocatedBytes().getTotal() > 0);
  solver.release();
  validateEquals(solver.getAllocatedBytes().getTotal(), std::size_t{0});
}
