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
#include "peano4/HaloExchange.h"
#include "peano4/MigrationExchange.h"
#include "peano4/Partition.h"
#include "peano4/Regrid.h"
#include "tarch/Span.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/plotter/Format.h"

namespace exahype2::dg {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class Solver;
} // namespace exahype2::dg

/**
 * The solution pool, the two face pools, the exchanges, the refinement votes and the error
 * measurement of a discontinuous Galerkin solver family on the partition. Derived is the
 * generated solver class with the layout constants, the operator tables Quadrature and
 * DGMatrices and the PDE terms; StoragePrecision is the type of the nodal values,
 * ComputePrecision the type of the face states, the face fluxes and the kernels. A solver family
 * defines getTimeStepSizeFromCFL() and getFixedTimeStepSize(), which closeTimeStep() reads
 * through Derived.
 *
 * Between threads the cells meet only in the face pools: Every cell writes its own halves of its
 * faces, and the Riemann problems read both halves. Between ranks the own halves of every
 * exchanged face travel in _stateExchange and _fluxExchange. A solver with a cell predicate
 * advances only the cells the predicate owns and lists them and their Riemann faces grouped like
 * the spans of the partition.
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

  Solver(
    double                                           timeStepFactor,
    const std::string&                               name,
    const std::vector<exahype2::plotter::DataField>& plottedFields
  );

  void requestRefinementForGridConstruction(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void allocate(const peano4::Partition& partition);
  void initialise(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void regrid(const peano4::Partition& partition, const peano4::Regrid& regrid, exahype2::RefinementRequests& requests);
  void plot(const peano4::Partition& partition, tarch::plotter::Format format, const std::string& outputDirectory);
  void release();

  void setLimits(const exahype2::Limits& limits);
  void openErrorMeasurement(const std::string& fileName, double timeDeltaBetweenSnapshots);
  [[nodiscard]] const exahype2::TimeStepping& getTimeStepping() const;
  [[nodiscard]] const CellPool& getSolution() const;
  [[nodiscard]] const StatePool& getFaceStates() const;
  [[nodiscard]] const FluxPool& getFaceFluxes() const;
  [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const;
  [[nodiscard]] int getNumberOfDoFs() const;
  [[nodiscard]] int getNumberOfCells(const peano4::Partition& partition) const;

protected:
  static constexpr bool EvaluateDiffusiveFlux = exahype2::terms::HasDiffusiveFlux<Derived, ComputePrecision>;
  static constexpr bool EvaluateFlux = exahype2::terms::HasFlux<Derived, ComputePrecision> or EvaluateDiffusiveFlux;
  static constexpr bool EvaluateNonconservativeProduct =
    exahype2::terms::HasNonconservativeProduct<Derived, ComputePrecision>;
  static constexpr bool EvaluateSource             = exahype2::terms::HasSourceTerm<Derived, ComputePrecision>;
  static constexpr bool EvaluateDiffusiveSource    = exahype2::terms::HasDiffusiveSourceTerm<Derived, ComputePrecision>;
  static constexpr bool EvaluateMaterialParameters = exahype2::terms::HasMaterialParameters<Derived, ComputePrecision>;

  /**
   * Interpolates the face states and the face fluxes of a coarse face onto its fine faces.
   */
  using InterpolateHalves = exahype2::dg::InterpolateFaceHalves<Derived, ComputePrecision>;
  /**
   * Interpolates the face states of a coarse face onto its fine faces.
   */
  using InterpolateStateHalves = exahype2::dg::InterpolateFaceStateHalves<Derived, ComputePrecision>;
  /**
   * Restricts the fluxes of the fine faces of a level jump onto their coarse face.
   */
  using RestrictFluxes = exahype2::dg::RestrictFluxHalves<Derived, ComputePrecision>;
  /**
   * Restricts the face states of the fine faces of a level jump onto their coarse face.
   */
  using RestrictStates = exahype2::dg::RestrictStateHalves<Derived, ComputePrecision>;

  [[nodiscard]] tarch::Span<const int> getCells(const peano4::Partition& partition) const;
  [[nodiscard]] tarch::Span<const int> getCells(
    const peano4::Partition& partition,
    peano4::Dependence       dependence
  ) const;
  [[nodiscard]] tarch::Span<const int> getCellsOnHost(const peano4::Partition& partition) const;
  [[nodiscard]] tarch::Span<const int> getRiemannFaces(
    const peano4::Partition& partition,
    peano4::Dependence       dependence
  ) const;
  [[nodiscard]] const StoragePrecision* downloadSolution();
  [[nodiscard]] const ComputePrecision* downloadFaceStates();
  void projectSolutionOntoFaces(const peano4::Partition& partition);
  void projectSolutionOntoPlane(const peano4::Partition& partition, int normal, double position);
  void solveRiemannProblems(
    const peano4::Partition& partition,
    peano4::Dependence       dependence,
    double                   timeStamp,
    double                   timeStepSize
  );
  void closeTimeStep(
    const peano4::Partition&                     partition,
    double                                       timeStepSize,
    const exahype2::dg::MaxEigenvalueStatistics& statistics
  );
  void measureError(const peano4::Partition& partition);
  void voteOnRefinement(const peano4::Partition& partition, exahype2::RefinementRequests& requests);
  void plotFaces(const peano4::Partition& partition, tarch::plotter::Format format, const std::string& outputDirectory);

  exahype2::TimeStepping _timeStepping;

  CellPool  _solution{ValuesPerCell};
  StatePool _faceStates{StateValuesPerFace};
  FluxPool  _faceFluxes{FluxValuesPerFace};

  /**
   * The MPI exchanges of the own halves of every exchanged face with the neighbour ranks.
   */
  peano4::HaloExchange<ComputePrecision> _stateExchange{_faceStates};
  peano4::HaloExchange<ComputePrecision> _fluxExchange{_faceFluxes};

  exahype2::plotter::PatchPlotter _plotter;

  /**
   * The 1D node positions and quadrature weights on the host, which the plots and a host initial condition read.
   */
  std::vector<double> _nodePositionsOnHost{};
  std::vector<double> _nodeWeightsOnHost{};

private:
  void rebuildOwnedLists(const peano4::Partition& partition);
  void reduceMeshWidths(const peano4::Partition& partition);

  peano4::MigrationExchange<StoragePrecision> _migration{_solution.getLocation()};

  exahype2::RefinementVotes _votes{};
  /**
   * The regrid sets it and the next time step clears it, so that step casts no refinement vote.
   */
  bool _meshChangedSinceLastTimeStep{false};

  /**
   * The cells the cell predicate of Derived owns, the local cells first; empty without a predicate.
   */
  tarch::accelerator::Buffer<int> _ownedCells{tarch::MemoryLocation::Device};
  std::vector<int>                _ownedCellsOnHost{};
  int                             _numberOfLocalOwnedCells{0};

  /**
   * The interior, boundary and hanging faces with an owned cell, the local faces first; empty without a predicate.
   */
  tarch::accelerator::Buffer<int> _ownedRiemannFaces{tarch::MemoryLocation::Device};
  int                             _numberOfLocalOwnedRiemannFaces{0};

  exahype2::ErrorMeasurement _errorMeasurement{};

  std::vector<StoragePrecision>                 _solutionOnHost{};
  std::vector<ComputePrecision>                 _faceStatesOnHost{};
  std::optional<exahype2::plotter::FacePlotter> _facePlotter{};
};

#include "exahype2/dg/Solver.cpph"
