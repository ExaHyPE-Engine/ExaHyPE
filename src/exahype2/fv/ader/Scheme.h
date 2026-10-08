// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <array>

#include "exahype2/TimeStepping.h"
#include "exahype2/dg/ader/NonlinearPredictor.h"
#include "exahype2/dg/ader/TeamWorkers.h"
#include "exahype2/fv/Patch.h"
#include "exahype2/fv/ader/PredictorTerms.h"
#include "exahype2/fv/ader/PredictorTables.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::ader {
  template <typename Basis>
  struct FaceTransferTables;

  template <typename Derived, typename ComputePrecision>
  class Scheme;
} // namespace exahype2::fv::ader

TARGET_DECLARE
/**
 * The 1D operators between the face nodes of a coarse volume face and those of the
 * SubdivisionFactor fine volume faces along one tangential axis of it. The members are arrays of an
 * object rather than static arrays, so a kernel that declares a local constexpr instance can read
 * them in device code, too.
 */
template <typename Basis>
struct exahype2::fv::ader::FaceTransferTables final {
  static constexpr int Nodes  = Basis::NumberOfNodes;
  static constexpr int Thirds = peano4::SubdivisionFactor;

  /**
   * Entry [third][fineNode][coarseNode] evaluates the coarse nodal polynomial at a fine node of the
   * fine volume face third.
   */
  std::array<double, Thirds * Nodes * Nodes> prolongation = [] {
    constexpr auto                             Points = Basis::nodes();
    std::array<double, Thirds * Nodes * Nodes> matrix{};
    for (int third = 0; third < Thirds; third++) {
      for (int fineNode = 0; fineNode < Nodes; fineNode++) {
        for (int coarseNode = 0; coarseNode < Nodes; coarseNode++) {
          matrix[(third * Nodes + fineNode) * Nodes + coarseNode] =
            Basis::value(coarseNode, (third + Points[fineNode]) / Thirds);
        }
      }
    }
    return matrix;
  }();

  /**
   * Entry [third][coarseNode][fineNode] projects the fine nodal values of the fine volume face
   * third onto the coarse nodes. The fine faces together keep the integral of the coarse face.
   */
  std::array<double, Thirds * Nodes * Nodes> restriction = [] {
    constexpr auto                             Points  = Basis::nodes();
    constexpr auto                             Weights = Basis::weights();
    std::array<double, Thirds * Nodes * Nodes> matrix{};
    for (int third = 0; third < Thirds; third++) {
      for (int coarseNode = 0; coarseNode < Nodes; coarseNode++) {
        for (int fineNode = 0; fineNode < Nodes; fineNode++) {
          matrix[(third * Nodes + coarseNode) * Nodes + fineNode] =
            Weights[fineNode] / (Thirds * Weights[coarseNode])
            * Basis::value(coarseNode, (third + Points[fineNode]) / Thirds);
        }
      }
    }
    return matrix;
  }();
};

/**
 * The ADER-WENO scheme on the patch of one cell: Derived::Reconstruction turns the volume averages
 * into a nodal polynomial per volume, and the space-time predictor of ADER-DG evolves it.
 *
 * A face of the patch holds two halves, the lower side first, of VolumesPerLayer volume faces with
 * NodesPerFace nodes each. The first tangential axis of the volume faces and of the nodes runs
 * fastest. The state halves hold the predicted time-averaged states, the flux halves first the
 * predicted fluxes and after the Riemann solve the numerical flux that the cell on that side adds.
 *
 * The predictions inside a patch live in PatchScratchSize values: For every interface of two
 * volumes of the patch, the state and flux of the lower and then of the upper volume, the
 * interfaces of the first axis first, followed by the source update of every volume, the nodal
 * polynomial of every volume and the scratch of the reconstruction. The prediction of one volume
 * works in VolumeScratchSize values: the predictor block and the predicted states and fluxes of
 * its faces.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::fv::ader::Scheme final {
public:
  using Reconstruction = typename Derived::Reconstruction;
  using Basis          = typename Reconstruction::Basis;
  using Predictor      = exahype2::dg::ader::NonlinearPredictor<
         exahype2::fv::ader::PredictorTerms<
           Derived,
           Reconstruction,
           Derived::NumberOfUnknowns,
           Derived::NumberOfAuxiliaryVariables>,
         ComputePrecision>;
  using Tables         = exahype2::fv::ader::PredictorTables<Basis, ComputePrecision>;

  static constexpr int    PatchSize                  = Derived::PatchSize;
  static constexpr int    NumberOfUnknowns           = Derived::NumberOfUnknowns;
  static constexpr int    NumberOfAuxiliaryVariables = Derived::NumberOfAuxiliaryVariables;
  static constexpr int    NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  static constexpr int    VolumesPerPatch            = peano4::power(PatchSize, DIMENSIONS);
  static constexpr int    VolumesPerLayer            = peano4::power(PatchSize, DIMENSIONS - 1);
  static constexpr int    NodesPerAxis               = Basis::NumberOfNodes;
  static constexpr int    NodesPerFace               = peano4::power(NodesPerAxis, DIMENSIONS - 1);
  static constexpr int    StateEntries               = NodesPerFace * NumberOfData;
  static constexpr int    FluxEntries                = NodesPerFace * NumberOfUnknowns;
  static constexpr int    StateValuesPerHalf         = VolumesPerLayer * StateEntries;
  static constexpr int    FluxValuesPerHalf          = VolumesPerLayer * FluxEntries;
  static constexpr int    StateValuesPerFace         = 2 * StateValuesPerHalf;
  static constexpr int    FluxValuesPerFace          = 2 * FluxValuesPerHalf;
  static constexpr int    InteriorInterfacesPerAxis  = (PatchSize - 1) * VolumesPerLayer;
  static constexpr int    EntriesPerSide             = StateEntries + FluxEntries;
  static constexpr int    SourceOffset               = DIMENSIONS * InteriorInterfacesPerAxis * 2 * EntriesPerSide;
  static constexpr int    NodalStateSize             = Predictor::CellLayout::NodesPerCell * NumberOfData;
  static constexpr int    NodalStatesOffset          = SourceOffset + VolumesPerPatch * NumberOfUnknowns;
  static constexpr int    ReconstructionOffset       = NodalStatesOffset + VolumesPerPatch * NodalStateSize;
  static constexpr int    PatchScratchSize =
    ReconstructionOffset + Reconstruction::template PatchScratchSize<PatchSize, NumberOfData>;
  static constexpr int    FaceStatesOffset           = Predictor::Scratch::Size;
  static constexpr int    FaceFluxesOffset           = FaceStatesOffset + peano4::FacesPerCell * StateEntries;
  static constexpr int    VolumeScratchSize          = FaceFluxesOffset + peano4::FacesPerCell * FluxEntries;
  static constexpr double PicardResidualTolerance    = 1e-7;
  static constexpr bool ReportEigenvalues = Derived::TimeSteppingRule == exahype2::TimeStepping::Rule::Adaptive;

  static_assert(Predictor::CellLayout::NodesPerFace == NodesPerFace);

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void reconstructPatch(
    const StoragePrecision* NOALIAS        patch,
    const StoragePrecision* NOALIAS const  halo[peano4::FacesPerCell],
    ComputePrecision* NOALIAS              patchScratch,
    const exahype2::dg::ader::TeamWorkers& workers
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int predictVolume(
    int                                                volume,
    const tarch::la::Vector<DIMENSIONS, double>&       cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&       cellSize,
    double                                             timeStamp,
    double                                             timeStepSize,
    const exahype2::dg::BasisTables<ComputePrecision>& tables,
    ComputePrecision* NOALIAS                          patchScratch,
    ComputePrecision* NOALIAS                          volumeScratch,
    ComputePrecision* const                            stateHalves[peano4::FacesPerCell],
    ComputePrecision* const                            fluxHalves[peano4::FacesPerCell],
    const exahype2::dg::ader::TeamWorkers&             workers
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double updateVolumeInsidePatch(
    int                                          volume,
    StoragePrecision*                            patch,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    const ComputePrecision* NOALIAS              scratch
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblemsOnFace(
    ComputePrecision* NOALIAS                    states,
    ComputePrecision* NOALIAS                    fluxes,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& faceWidth,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    int                                          boundaryFaceNumber
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void correctVolume(
    int                                          volume,
    StoragePrecision*                            patch,
    const ComputePrecision* const                fluxHalves[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStepSize
  );

  template <int Values>
  static INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFaceHalf(
    const ComputePrecision* NOALIAS coarse,
    ComputePrecision* NOALIAS       fine,
    int                             subface
  );

  template <int Values>
  static INLINE_METHOD GPU_CALLABLE_METHOD void restrictFaceHalf(
    const ComputePrecision* const NOALIAS fine[peano4::SubfacesPerFace],
    ComputePrecision* NOALIAS             coarse
  );

private:
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getTangentialIndex(
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    int                                       normal
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isOnPatchFace(
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    int                                       face
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getInteriorSideOffset(
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    int                                       face
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getNodeWeight(int node);

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblemAtNode(
    const ComputePrecision* NOALIAS              left,
    const ComputePrecision* NOALIAS              right,
    const ComputePrecision* NOALIAS              leftFlux,
    const ComputePrecision* NOALIAS              rightFlux,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    bool                                         isBoundaryFace,
    int                                          faceIndex,
    ComputePrecision* NOALIAS                    fluxLeft,
    ComputePrecision* NOALIAS                    fluxRight
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void applyBoundaryConditionsAtNode(
    const ComputePrecision* NOALIAS              inside,
    ComputePrecision* NOALIAS                    outside,
    ComputePrecision* NOALIAS                    outsideFlux,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/ader/Scheme.cpph"
