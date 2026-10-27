// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/Limits.h"
#include "exahype2/RefinementRequests.h"
#include "exahype2/RefinementVotes.h"
#include "exahype2/TimeStepping.h"
#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "exahype2/plotter/FacePlotter.h"
#include "exahype2/plotter/PatchPlotter.h"
#include "exahype2/terms/DiffusiveFlux.h"
#include "exahype2/terms/DiffusiveSourceTerm.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/MaterialParameters.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/CellConnectivity.h"
#include "peano4/HaloExchange.h"
#include "peano4/MigrationExchange.h"
#include "peano4/Regrid.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/plotter/Format.h"

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

namespace exahype2::dg {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Solver;
} // namespace exahype2::dg

/**
 * The base of the discontinuous Galerkin solver families. A solver family defines
 * getTimeStepSizeFromCFL() and getFixedTimeStepSize(). StoragePrecision is the type of the nodal
 * values. ComputePrecision is the type of the face data and the kernels. A cell writes only its own
 * halves of its faces. The Riemann solver reads both halves.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::dg::Solver {
public:
  static constexpr int  NodesPerAxis             = Derived::DGOrder + 1;
  static constexpr int  NodesPerCell             = exahype2::dg::getNodesPerCell(NodesPerAxis);
  static constexpr int  NodesPerFace             = exahype2::dg::getNodesPerFace(NodesPerAxis);
  static constexpr int  PointsPerCell            = NodesPerCell;
  static constexpr int  NumberOfData             = Derived::NumberOfUnknowns + Derived::NumberOfAuxiliaryVariables;
  static constexpr int  ValuesPerCell            = NodesPerCell * NumberOfData;
  static constexpr int  StateValuesPerFace       = 2 * NodesPerFace * NumberOfData;
  static constexpr int  FluxValuesPerFace        = 2 * NodesPerFace * Derived::NumberOfUnknowns;
  static constexpr bool UsesAdaptiveTimeStepping = Derived::TimeSteppingRule == exahype2::TimeStepping::Rule::Adaptive;

  // The defaulted parameter defers the lookup in Derived to the use, where Derived is complete.
  template <typename Precision, typename Generated = Derived>
  using Quadrature = typename Generated::template Quadrature<Precision>;
  template <typename Precision, typename Generated = Derived>
  using DGMatrices = typename Generated::template DGMatrices<Precision>;

  using DerivedType = Derived;
  using CellPool    = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool   = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using FluxPool    = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  Solver()                         = delete;
  Solver(const Solver&)            = delete;
  Solver(Solver&&)                 = delete;
  Solver& operator=(const Solver&) = delete;
  Solver& operator=(Solver&&)      = delete;
  ~Solver()                        = default;

  INLINE_METHOD Solver(
    double                                           timeStepFactor,
    const std::string&                               name,
    const std::vector<exahype2::plotter::DataField>& plottedFields
  );

  INLINE_METHOD void requestRefinementForGridConstruction(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void allocate(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void initialise(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void regrid(
    const peano4::CellConnectivity& cellConnectivity,
    const peano4::Regrid&           regrid,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void plot(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::plotter::Format          format,
    const std::string&              outputDirectory
  );
  INLINE_METHOD void release();
  INLINE_METHOD void addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record);
  INLINE_METHOD void readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record);

  INLINE_METHOD void setLimits(const exahype2::Limits& limits);
  INLINE_METHOD void openErrorMeasurement(const std::string& fileName, double timeDeltaBetweenSnapshots);
  [[nodiscard]] INLINE_METHOD const exahype2::TimeStepping& getTimeStepping() const;
  [[nodiscard]] INLINE_METHOD const CellPool& getSolution() const;
  [[nodiscard]] INLINE_METHOD const StatePool& getFaceStates() const;
  [[nodiscard]] INLINE_METHOD const FluxPool& getFaceFluxes() const;
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] INLINE_METHOD int getNumberOfDoFs() const;
  [[nodiscard]] INLINE_METHOD int getNumberOfCells(const peano4::CellConnectivity& cellConnectivity) const;

protected:
  static constexpr bool EvaluateDiffusiveFlux = exahype2::terms::HasDiffusiveFlux<Derived, ComputePrecision>;
  static constexpr bool EvaluateFlux = exahype2::terms::HasFlux<Derived, ComputePrecision> or EvaluateDiffusiveFlux;
  static constexpr bool EvaluateNonconservativeProduct =
    exahype2::terms::HasNonconservativeProduct<Derived, ComputePrecision>;
  static constexpr bool EvaluateSource             = exahype2::terms::HasSourceTerm<Derived, ComputePrecision>;
  static constexpr bool EvaluateDiffusiveSource    = exahype2::terms::HasDiffusiveSourceTerm<Derived, ComputePrecision>;
  static constexpr bool EvaluateMaterialParameters = exahype2::terms::HasMaterialParameters<Derived, ComputePrecision>;

  using InterpolateHalves      = exahype2::dg::InterpolateFaceHalves<Derived, ComputePrecision>;
  using InterpolateStateHalves = exahype2::dg::InterpolateFaceStateHalves<Derived, ComputePrecision>;
  using RestrictFluxes         = exahype2::dg::RestrictFluxHalves<Derived, ComputePrecision>;
  using RestrictStates         = exahype2::dg::RestrictStateHalves<Derived, ComputePrecision>;

  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getCells(const peano4::CellConnectivity& cellConnectivity) const;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getCells(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency
  ) const;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getCellsOnHost(
    const peano4::CellConnectivity& cellConnectivity
  ) const;
  [[nodiscard]] INLINE_METHOD tarch::Span<const int> getRiemannFaces(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency
  ) const;
  [[nodiscard]] INLINE_METHOD const CellPool& downloadSolution();
  [[nodiscard]] INLINE_METHOD const StatePool& downloadFaceStates();
  INLINE_METHOD void projectSolutionOntoFaces(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void projectSolutionOntoPlane(
    const peano4::CellConnectivity& cellConnectivity,
    int                             normal,
    double                          position
  );
  INLINE_METHOD void solveRiemannProblems(
    const peano4::CellConnectivity& cellConnectivity,
    peano4::Adjacency               adjacency,
    double                          timeStamp,
    double                          timeStepSize,
    typename FluxPool::View         appliedFluxes = {}
  );
  INLINE_METHOD void closeTimeStep(double timeStepSize, const exahype2::dg::MaxEigenvalueStatistics& statistics);
  INLINE_METHOD void measureError(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void voteOnRefinement(
    const peano4::CellConnectivity& cellConnectivity,
    exahype2::RefinementRequests&   requests
  );
  INLINE_METHOD void plotFaces(
    const peano4::CellConnectivity& cellConnectivity,
    tarch::plotter::Format          format,
    const std::string&              outputDirectory
  );
  template <int PointsPerAxis, int SubcellsPerAxis = 1>
  INLINE_METHOD void plotPolynomial(
    const peano4::CellConnectivity&                   cellConnectivity,
    const double*                                     matrix1D,
    tarch::plotter::Format                            format,
    const std::string&                                outputDirectory,
    const CellPool*                                   subcells      = nullptr,
    const tarch::accelerator::ChunkedMemoryPool<int>* statuses      = nullptr,
    int                                               subcellStatus = 0,
    const char*                                       statusName    = nullptr
  );

  exahype2::TimeStepping _timeStepping;

  CellPool  _solution{ValuesPerCell};
  StatePool _faceStates{StateValuesPerFace};
  FluxPool  _faceFluxes{FluxValuesPerFace};

  /**
   * The MPI exchanges of the own face halves with the neighbour ranks. The restricted fluxes change
   * only on coarse faces.
   */
  peano4::HaloExchange<ComputePrecision> _stateExchange{_faceStates};
  peano4::HaloExchange<ComputePrecision> _fluxExchange{_faceFluxes};
  peano4::HaloExchange<ComputePrecision> _restrictedFluxExchange{_faceFluxes, peano4::FaceKind::Coarse};

  exahype2::plotter::PatchPlotter _plotter;

  /**
   * Host copies of the 1D node positions and quadrature weights for a host initial condition.
   */
  std::vector<double> _nodePositionsOnHost{};
  std::vector<double> _nodeWeightsOnHost{};

private:
  INLINE_METHOD void rebuildOwnedLists(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void measureMeshWidths(const peano4::CellConnectivity& cellConnectivity);

  peano4::MigrationExchange<StoragePrecision> _migration{_solution.getLocation()};

  exahype2::RefinementVotes _votes{};
  /**
   * regrid() sets it, and the next voteOnRefinement() clears it and casts no vote.
   */
  bool _meshChangedSinceLastTimeStep{false};

  /**
   * The cells the cell predicate of Derived owns, the local cells first. The list is empty without
   * a cell predicate.
   */
  tarch::accelerator::Buffer<int> _ownedCells{tarch::MemoryLocation::Device};
  std::vector<int>                _ownedCellsOnHost{};
  int                             _numberOfLocalOwnedCells{0};

  /**
   * The interior, boundary and hanging faces beside an owned cell, the local faces first. The list
   * is empty without a cell predicate.
   */
  tarch::accelerator::Buffer<int> _ownedRiemannFaces{tarch::MemoryLocation::Device};
  int                             _numberOfLocalOwnedRiemannFaces{0};

  exahype2::ErrorMeasurement _errorMeasurement{};

  CellPool                                      _solutionOnHost{ValuesPerCell, tarch::MemoryLocation::Heap};
  StatePool                                     _faceStatesOnHost{StateValuesPerFace, tarch::MemoryLocation::Heap};
  std::optional<exahype2::plotter::FacePlotter> _facePlotter{};
};

#include "exahype2/dg/Solver.cpph"
