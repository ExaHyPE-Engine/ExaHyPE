// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "exahype2/dg/ader/PredictorScratch.h"
#include "exahype2/dg/ader/SpaceTimePredictor.h"
#include "exahype2/fv/EigenvalueStatistics.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/timestepping/ADERPredictorTables.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::timestepping {
  struct ADER;
} // namespace exahype2::fv::timestepping

TARGET_DECLARE
/**
 * The one-step ADER finite-volume scheme of Dumbser et al.: A reconstruction turns the volume
 * averages into a polynomial on every volume, the space-time predictor of the ADER-DG solvers
 * evolves it over the time step, and the corrector updates the averages with the time-averaged
 * fluxes through the faces.
 *
 * Every volume of the patch is predicted from the polynomial over all volumes the patch holds,
 * which yields its update and the fluxes through the interfaces inside the patch. A volume on
 * either side of a face of the patch is predicted once more from the volumes the neighbouring
 * patch holds as well, so both patches compute the same flux through that face and the scheme
 * stays conservative although the halo holds no volume beyond an edge or a corner. The
 * predictor adds the space-time average of the source and of the
 * volume part of the non-conservative product, and the flux volume term vanishes against the
 * constant test function of a finite volume. The Riemann solver acts on every quadrature node of
 * a face: Rusanov on the time-averaged states and fluxes like the ADER-DG solvers, a custom one on
 * the time-averaged states. The non-conservative product adds half the fluctuation across a face
 * to either side.
 *
 * The @c Reconstruction provides the nodal @c Basis of its polynomial, from which the predictor
 * tables and the face quadrature follow, its @c ScratchSize and @c reconstructPolynomial. The
 * step runs on a device as well, so it works in the @c ScratchSize entries the caller provides and
 * reports a diverged predictor by returning NaN instead of terminating.
 */
struct exahype2::fv::timestepping::ADER final {
  static constexpr int NumberOfStages = 1;

  static constexpr double PicardResidualTolerance = 1e-7;

  /**
   * Offsets of the blocks of the scratch of one patch. The interfaces along one axis are numbered
   * like the volumes whose lower face they are, with one more position along that axis. Each
   * interface keeps the time-averaged states and fluxes of its lower and its upper volume on the
   * quadrature nodes of the face.
   */
  template <
    typename Reconstruction,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource>
  struct ScratchLayout final {
    using Basis      = typename Reconstruction::Basis;
    using CellLayout = exahype2::dg::ader::Layout<Basis::NumberOfNodes, NumberOfUnknowns, NumberOfAuxiliaryVariables>;
    using Predictor  = exahype2::dg::ader::NonlinearPredictorScratch<
      Basis::NumberOfNodes,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      EvaluateFlux,
      EvaluateNonconservativeProduct,
      EvaluateSource,
      false,
      false>;

    static constexpr int NumberOfData        = NumberOfUnknowns + NumberOfAuxiliaryVariables;
    static constexpr int StateEntriesPerSide = CellLayout::NodesPerFace * NumberOfData;
    static constexpr int FluxEntriesPerSide  = CellLayout::NodesPerFace * NumberOfUnknowns;
    static constexpr int EntriesPerInterface = 2 * (StateEntriesPerSide + FluxEntriesPerSide);
    static constexpr int InterfacesPerAxis =
      (NumberOfVolumesPerAxis + 1) * peano4::power(NumberOfVolumesPerAxis, DIMENSIONS - 1);

    static constexpr int InterfaceDataOffset   = 0;
    static constexpr int PredictorOffset       = InterfaceDataOffset + DIMENSIONS * InterfacesPerAxis * EntriesPerInterface;
    static constexpr int NodalStateOffset      = PredictorOffset + Predictor::Size;
    static constexpr int DiscardedStatesOffset = NodalStateOffset + CellLayout::NodesPerCell * NumberOfData;
    static constexpr int DiscardedFluxesOffset = DiscardedStatesOffset + peano4::FacesPerCell * StateEntriesPerSide;
    static constexpr int ReconstructionOffset  = DiscardedFluxesOffset + peano4::FacesPerCell * FluxEntriesPerSide;
    static constexpr int Size = ReconstructionOffset + Reconstruction::template ScratchSize<NumberOfData>;
  };

  template <
    typename Reconstruction,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource>
  static constexpr int ScratchSize = ScratchLayout<
    Reconstruction,
    NumberOfVolumesPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    EvaluateFlux,
    EvaluateNonconservativeProduct,
    EvaluateSource>::Size;

  template <
    typename Reconstruction,
    typename SolverType,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveSource,
    bool EvaluateCustomRiemann,
    bool EvaluateMaxEigenvalue,
    bool EvaluateMaxEigenvalueOfFinalState,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static double apply(
    const StoragePrecision* NOALIAS              inputPatch,
    StoragePrecision* NOALIAS                    outputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    StoragePrecision* NOALIAS const              faceOutputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    ComputePrecision* NOALIAS                    scratch
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/timestepping/ADER.cpph"
