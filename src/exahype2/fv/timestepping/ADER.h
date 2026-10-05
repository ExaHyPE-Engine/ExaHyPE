// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/ader/NonlinearPredictor.h"
#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/timestepping/ADERPredictorTables.h"

namespace exahype2::fv::timestepping {
  struct ADER;
}

TARGET_DECLARE
/**
 * ADER-WENO advances one volume from a polynomial space-time predictor. At a patch face both
 * sides restrict their reconstruction to the same available stencil to preserve conservation.
 *
 * applyToVolume() predicts the volume and its neighbours on its own and reads only the old patch
 * and halos. A patch update predicts every volume once instead: predictForPatch() runs one
 * prediction of PatchPredictions and stores the face states, face fluxes and source integral it
 * yields, and applyPredictionsToVolume() then updates a volume from these predictions.
 */
struct exahype2::fv::timestepping::ADER final {
  static constexpr int    NumberOfStages          = 1;
  static constexpr double PicardResidualTolerance = 1e-7;
  static constexpr bool   PredictsPatchwise       = true;

  template <typename SolverType, typename Reconstruction, int Unknowns, int Auxiliary>
  struct PredictorTerms {
    static constexpr int DGOrder                    = Reconstruction::Basis::NumberOfNodes - 1;
    static constexpr int NumberOfUnknowns           = Unknowns;
    static constexpr int NumberOfAuxiliaryVariables = Auxiliary;

    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const P*                                     q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      P*                                           f
    )
      requires exahype2::terms::HasFlux<SolverType, P>
    {
      SolverType::flux(q, x, h, t, dt, normal, f);
    }

    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const P*                                     q,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      P*                                           s
    )
      requires exahype2::terms::HasSourceTerm<SolverType, P>
    {
      SolverType::sourceTerm(q, x, h, t, dt, s);
    }

    template <typename P>
    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      const P*                                     q,
      const P*                                     dq,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      P*                                           b
    )
      requires exahype2::terms::HasNonconservativeProduct<SolverType, P>
    {
      SolverType::nonconservativeProduct(q, dq, x, h, t, dt, normal, b);
    }
  };

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
    typename StoragePrecision,
    typename ComputePrecision,
    typename Terms>
  static INLINE_METHOD GPU_CALLABLE_METHOD double applyToVolume(
    int                                          volume,
    const StoragePrecision* NOALIAS              inputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    Terms&                                       terms,
    StoragePrecision* NOALIAS                    updatedState
  );

  /**
   * The predictions of a patch. The patch predicts each of its volumes once, and each volume at
   * a face of the patch and the halo volume beside it once more restricted to that face. The
   * predictions buffer holds, for every interface of two volumes, the state and flux of the lower
   * volume and then those of the upper volume, the interfaces of the first axis first. The
   * volume-integrated source update of every volume follows from SourceOffset on.
   */
  template <
    typename Reconstruction,
    typename SolverType,
    int NumberOfVolumesPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename ComputePrecision>
  struct PatchPredictions {
    using Predictor = exahype2::dg::ader::NonlinearPredictor<
      PredictorTerms<SolverType, Reconstruction, NumberOfUnknowns, NumberOfAuxiliaryVariables>,
      ComputePrecision>;
    using Layout = typename Predictor::CellLayout;

    static constexpr int Data              = NumberOfUnknowns + NumberOfAuxiliaryVariables;
    static constexpr int StateEntries      = Layout::NodesPerFace * Data;
    static constexpr int FluxEntries       = Layout::NodesPerFace * NumberOfUnknowns;
    static constexpr int EntriesPerSide    = StateEntries + FluxEntries;
    static constexpr int VolumesPerLayer   = peano4::power(NumberOfVolumesPerAxis, DIMENSIONS - 1);
    static constexpr int VolumesPerPatch   = VolumesPerLayer * NumberOfVolumesPerAxis;
    static constexpr int InterfacesPerAxis = (NumberOfVolumesPerAxis + 1) * VolumesPerLayer;
    static constexpr int SourceOffset      = DIMENSIONS * InterfacesPerAxis * 2 * EntriesPerSide;
    static constexpr int Size              = SourceOffset + VolumesPerPatch * NumberOfUnknowns;
    static constexpr int NumberOfJobs      = VolumesPerPatch + peano4::FacesPerCell * 2 * VolumesPerLayer;

    [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getSideOffset(
      const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
      int                                       face
    );
  };

  template <
    typename Reconstruction,
    typename SolverType,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void predictForPatch(
    int                                          job,
    const StoragePrecision* NOALIAS              inputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    ComputePrecision* NOALIAS                    predictions
  );

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
    typename StoragePrecision,
    typename ComputePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD double applyPredictionsToVolume(
    int                                          volume,
    const StoragePrecision*                      inputPatch,
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    const ComputePrecision* NOALIAS              predictions,
    StoragePrecision*                            updatedState
  );

private:
  template <
    typename Basis,
    typename SolverType,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    int  NodesPerFace,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateCustomRiemann,
    bool EvaluateMaxEigenvalue,
    typename ComputePrecision,
    typename UpdatePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void integrateFace(
    const tarch::la::Vector<DIMENSIONS, int>&    volumeIndex,
    int                                          face,
    const ComputePrecision*                      localState,
    const ComputePrecision*                      localFlux,
    const ComputePrecision*                      neighbourState,
    const ComputePrecision*                      neighbourFlux,
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    UpdatePrecision*                             update,
    double&                                      maximum
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/timestepping/ADER.cpph"
