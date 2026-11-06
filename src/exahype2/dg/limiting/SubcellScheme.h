// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/TimeStepping.h"
#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/Kernels.h"
#include "exahype2/terms/DiffusiveSourceTerm.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/MaxEigenvalue.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/SourceTerm.h"
#include "exahype2/terms/SubcellRiemannSolver.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::limiting {
  template <typename Derived, typename ComputePrecision>
  class SubcellScheme;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ProjectSubgridsOntoFaces;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ApplySubgridBoundaryConditions;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct RecomputeSubcells;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ProjectNodesOntoSubgrids;
} // namespace exahype2::dg::limiting

namespace exahype2::dg::limiting::tests {
  class SubcellSchemeTest;
} // namespace exahype2::dg::limiting::tests

TARGET_DECLARE
/**
 * SubcellScheme runs the finite-volume scheme on the subgrid of one cell of Derived with PatchSize
 * subcells per axis.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::dg::limiting::SubcellScheme final {
public:
  using Subgrid = exahype2::fv::FiniteVolumeScheme<Derived, ComputePrecision>;

  static constexpr int NodesPerAxis               = Derived::DGOrder + 1;
  static constexpr int SubcellsPerAxis            = Derived::PatchSize;
  static constexpr int NumberOfUnknowns           = Derived::NumberOfUnknowns;
  static constexpr int NumberOfAuxiliaryVariables = Derived::NumberOfAuxiliaryVariables;
  static constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  static constexpr int HaloSize                   = Derived::Reconstruction::HaloSize;
  static constexpr int NodesPerCell               = exahype2::dg::getNodesPerCell(NodesPerAxis);
  static constexpr int SubcellsPerCell            = peano4::power(SubcellsPerAxis, DIMENSIONS);
  static constexpr int SubcellsPerFace            = peano4::power(SubcellsPerAxis, DIMENSIONS - 1);
  static constexpr int ValuesPerCell              = NodesPerCell * NumberOfData;
  static constexpr int SubcellValuesPerCell       = SubcellsPerCell * NumberOfData;
  static constexpr int HaloUnknownsPerFace        = 2 * HaloSize * SubcellsPerFace * NumberOfUnknowns;
  static constexpr int HaloAuxVarsPerFace         = 2 * HaloSize * SubcellsPerFace * NumberOfAuxiliaryVariables;
  static constexpr int NodeProjectionScratchValues =
    exahype2::dg::getScratchValuesOfTensorProduct<DIMENSIONS, SubcellsPerAxis, NodesPerAxis, NumberOfData>();
  static constexpr int SubcellProjectionScratchValues =
    exahype2::dg::getScratchValuesOfTensorProduct<DIMENSIONS, NodesPerAxis, SubcellsPerAxis, NumberOfData>();
  static constexpr int ProjectionScratchValues = NodeProjectionScratchValues > SubcellProjectionScratchValues
    ? NodeProjectionScratchValues
    : SubcellProjectionScratchValues;
  static constexpr int ScratchValuesPerRecomputedCell =
    SubcellValuesPerCell > ProjectionScratchValues ? SubcellValuesPerCell : ProjectionScratchValues;
  static constexpr bool ReportEigenvalues = Derived::TimeSteppingRule == exahype2::TimeStepping::Rule::Adaptive;

  [[nodiscard]] static INLINE_METHOD int getTeamThreads();

  template <typename StoragePrecision>
  static INLINE_METHOD void sampleInitialCondition(
    StoragePrecision* NOALIAS                    subcells,
    StoragePrecision* const                      auxVarsHaloHalves[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectNodesOntoSubcells(
    const StoragePrecision* NOALIAS nodes,
    StoragePrecision* NOALIAS       subcells,
    const StoragePrecision* NOALIAS nodesToSubcells
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectNodesOntoSubcellsInTeam(
    tarch::accelerator::TeamHandle  team,
    const StoragePrecision* NOALIAS nodes,
    StoragePrecision* NOALIAS       subcells,
    const StoragePrecision* NOALIAS nodesToSubcells,
    StoragePrecision* NOALIAS       scratch
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectSubgridOntoFaces(
    const int* NOALIAS              status,
    StoragePrecision* NOALIAS       subcells,
    const StoragePrecision* NOALIAS previousSolution,
    StoragePrecision* const         haloHalves[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS nodesToSubcells,
    bool                            subcellsStoreInitialCondition
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectSubgridOntoFacesInTeam(
    tarch::accelerator::TeamHandle  team,
    const int* NOALIAS              status,
    StoragePrecision* NOALIAS       subcells,
    const StoragePrecision* NOALIAS previousSolution,
    StoragePrecision* const         haloHalves[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS nodesToSubcells,
    bool                            subcellsStoreInitialCondition,
    StoragePrecision* NOALIAS       scratch
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics recompute(
    StoragePrecision* NOALIAS                    nodes,
    StoragePrecision* NOALIAS                    subcells,
    StoragePrecision* NOALIAS                    evolvedSubcells,
    const StoragePrecision* const                haloHalves[peano4::FacesPerCell],
    const StoragePrecision* const                auxVarsHaloHalves[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              subcellsToNodes,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          boundaryFaces,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics recomputeInTeam(
    tarch::accelerator::TeamHandle               team,
    StoragePrecision* NOALIAS                    nodes,
    StoragePrecision* NOALIAS                    subcells,
    StoragePrecision* NOALIAS                    evolvedSubcells,
    const StoragePrecision* const                haloHalves[peano4::FacesPerCell],
    const StoragePrecision* const                auxVarsHaloHalves[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              subcellsToNodes,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          boundaryFaces,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics
  reduceMaxEigenvaluesOfSubcell(
    const StoragePrecision* NOALIAS              subcells,
    int                                          subcell,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics reduceMaxEigenvalues(
    const StoragePrecision* NOALIAS              subcells,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

private:
  friend class exahype2::dg::limiting::tests::SubcellSchemeTest;

  template <typename Reconstruct>
  struct SubgridSolver;

  template <typename Reconstruct>
  using SubgridScheme = exahype2::fv::FiniteVolumeScheme<SubgridSolver<Reconstruct>, ComputePrecision>;

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectSubgridOntoFacesWithThreadsOfTeam(
    tarch::accelerator::TeamHandle  team,
    const int* NOALIAS              status,
    StoragePrecision* NOALIAS       subcells,
    const StoragePrecision* NOALIAS previousSolution,
    StoragePrecision* const         haloHalves[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS nodesToSubcells,
    bool                            subcellsStoreInitialCondition,
    StoragePrecision* NOALIAS       scratch
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics recomputeWithThreadsOfTeam(
    tarch::accelerator::TeamHandle               team,
    StoragePrecision* NOALIAS                    nodes,
    StoragePrecision* NOALIAS                    subcells,
    StoragePrecision* NOALIAS                    evolvedSubcells,
    const StoragePrecision* const                haloHalves[peano4::FacesPerCell],
    const StoragePrecision* const                auxVarsHaloHalves[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS              subcellsToNodes,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          boundaryFaces,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename StoragePrecision, typename Functor>
  static INLINE_METHOD constexpr void forEachSubcell(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    StoragePrecision*                            values,
    Functor&&                                    functor
  );

  template <typename Reconstruct, typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void advanceSubcells(
    const StoragePrecision* NOALIAS              subcells,
    StoragePrecision* NOALIAS                    evolvedSubcells,
    const StoragePrecision* const                haloHalves[peano4::FacesPerCell],
    const StoragePrecision* const                auxVarsHaloHalves[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          boundaryFaces,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectSubcellsOntoNodes(
    const StoragePrecision* NOALIAS subcells,
    StoragePrecision* NOALIAS       nodes,
    const StoragePrecision* NOALIAS subcellsToNodes
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectSubcellsOntoNodesInTeam(
    tarch::accelerator::TeamHandle  team,
    const StoragePrecision* NOALIAS subcells,
    StoragePrecision* NOALIAS       nodes,
    const StoragePrecision* NOALIAS subcellsToNodes,
    StoragePrecision* NOALIAS       scratch
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectSubcellsOntoNodes(
    const StoragePrecision* NOALIAS                                                                   subcells,
    StoragePrecision* NOALIAS                                                                         nodes,
    const StoragePrecision* NOALIAS                                                                   subcellsToNodes,
    const exahype2::dg::ProjectionWeightsOfDofPairs<SubcellsPerAxis, NodesPerAxis, StoragePrecision>& weights,
    StoragePrecision* NOALIAS                                                                         scratch,
    int                                                                                               stage,
    int                                                                                               firstItem,
    int                                                                                               itemStride
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectNodesOntoSubcells(
    const StoragePrecision* NOALIAS                                                                   nodes,
    StoragePrecision* NOALIAS                                                                         subcells,
    const StoragePrecision* NOALIAS                                                                   nodesToSubcells,
    const exahype2::dg::ProjectionWeightsOfDofPairs<NodesPerAxis, SubcellsPerAxis, StoragePrecision>& weights,
    StoragePrecision* NOALIAS                                                                         scratch,
    int                                                                                               stage,
    int                                                                                               firstItem,
    int                                                                                               itemStride
  );
};

/**
 * SubgridSolver wraps Derived for the finite-volume scheme of the subgrid. The fixed time step rule
 * skips the wave speeds in the update of the subgrid. solveRiemannProblem() calls the subcell
 * Riemann solver of Derived and every other term the term of Derived with the same name.
 */
template <typename Derived, typename ComputePrecision>
template <typename Reconstruct>
struct exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>::SubgridSolver final {
  static constexpr int PatchSize                  = Derived::PatchSize;
  static constexpr int NumberOfUnknowns           = Derived::NumberOfUnknowns;
  static constexpr int NumberOfAuxiliaryVariables = Derived::NumberOfAuxiliaryVariables;

  static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

  using Reconstruction  = Reconstruct;
  using TimeIntegration = typename Derived::TimeIntegration;

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const Precision*                             Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    Precision*                                   F
  )
    requires exahype2::terms::HasFlux<Derived, Precision>;

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const Precision*                             Q,
    const Precision*                             deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    Precision*                                   BTimesDeltaQ
  )
    requires exahype2::terms::HasNonconservativeProduct<Derived, Precision>;

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
    const Precision*                             Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    Precision*                                   S
  )
    requires exahype2::terms::HasSourceTerm<Derived, Precision>;

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const Precision*                             Q,
    const Precision*                             deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    Precision*                                   S
  )
    requires exahype2::terms::HasDiffusiveSourceTerm<Derived, Precision>;

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
    const Precision*                             Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal
  )
    requires exahype2::terms::HasMaxEigenvalue<Derived, Precision>;

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
    const Precision*                             QL,
    const Precision*                             QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    bool                                         isBoundaryFace,
    int                                          faceIndex,
    Precision*                                   FL,
    Precision*                                   FR
  )
    requires exahype2::terms::HasSubcellRiemannSolver<Derived, Precision>;
};

/**
 * Writes the subgrid of every listed cell onto its own halo halves. A cell that enters the
 * subcell scheme after the first step first takes the projection of its previous solution. The
 * projection stages lie in the scratch chunk of the work item.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::ProjectSubgridsOntoFaces final {
  using Kernel     = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;
  using CellPool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::CellConnectivity       cellConnectivity{};
  tarch::Span<const int>         cells{};
  typename CellPool::ConstView   previousSolution{};
  typename CellPool::View        subcells{};
  typename CellPool::View        halo{};
  typename StatusPool::ConstView statuses{};
  const StoragePrecision*        nodesToSubcells{nullptr};
  bool                           subcellsStoreInitialCondition{false};
  StoragePrecision*              scratch{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, tarch::accelerator::TeamHandle team) const;
};

/**
 * Fills the outer halo half of every listed boundary face beside a cell that is not Regular. Only
 * these cells write their subgrid onto their faces in this step. The launch runs one thread per
 * volume of a layer of every listed face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::ApplySubgridBoundaryConditions final {
  using Kernel     = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  exahype2::fv::ApplyBoundaryConditions<Derived, StoragePrecision, ComputePrecision> boundaryConditions{};
  typename StatusPool::ConstView                                                     statuses{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Advances every listed cell by one finite-volume step on its subgrid and projects the result onto
 * the nodes of the cell. Reduces the wave speeds of the subgrids. The scratch chunk of the work item
 * receives the evolved subgrid and then the stages of the projection.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::RecomputeSubcells final {
  using Kernel   = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::CellConnectivity     cellConnectivity{};
  tarch::Span<const int>       cells{};
  typename CellPool::View      nodes{};
  typename CellPool::View      subcells{};
  typename CellPool::ConstView halo{};
  typename CellPool::ConstView auxVarsHalo{};
  const StoragePrecision*      subcellsToNodes{nullptr};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};
  StoragePrecision*            scratch{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                                    entry,
    tarch::accelerator::TeamHandle         team,
    exahype2::dg::MaxEigenvalueStatistics& accumulator
  ) const;
};

/**
 * Writes the projection of the nodes of every listed cell on the Galerkin scheme into its subgrid.
 * Team entry projects the cells entry, entry + numberOfTeams and so on. The stages of the projection
 * lie in the scratch chunk of the team.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::dg::limiting::ProjectNodesOntoSubgrids final {
  using Kernel     = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;
  using CellPool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  tarch::Span<const int>         cells{};
  int                            numberOfTeams{1};
  typename CellPool::ConstView   nodes{};
  typename CellPool::View        subcells{};
  typename StatusPool::ConstView statuses{};
  const StoragePrecision*        nodesToSubcells{nullptr};
  StoragePrecision*              scratch{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, tarch::accelerator::TeamHandle team) const;
};
TARGET_DECLARE_END

#include "exahype2/dg/limiting/SubcellScheme.cpph"
