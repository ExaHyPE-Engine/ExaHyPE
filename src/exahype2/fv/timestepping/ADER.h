// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/ader/NonlinearPredictor.h"
#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/ader/PredictorTerms.h"
#include "exahype2/fv/timestepping/ADERPredictorTables.h"

namespace exahype2::fv::timestepping {
  struct ADER;
}

TARGET_DECLARE
/**
 * ADER-WENO advances one volume from a polynomial space-time predictor. A worker owns
 * its predictor scratch and reads only the old patch and halos, so independent device
 * workers can update different volumes. At a patch face both sides restrict their
 * reconstruction to the same available stencil to preserve conservation.
 */
struct exahype2::fv::timestepping::ADER final {
  static constexpr int    NumberOfStages          = 1;
  static constexpr double PicardResidualTolerance = 1e-7;

  template <typename SolverType, typename Reconstruction, int Unknowns, int Auxiliary>
  using PredictorTerms = exahype2::fv::ader::PredictorTerms<SolverType, Reconstruction, Unknowns, Auxiliary>;

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
