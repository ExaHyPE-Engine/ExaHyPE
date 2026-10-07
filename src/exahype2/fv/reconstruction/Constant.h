// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/FiniteVolumeScheme.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::reconstruction {
  struct Constant;
} // namespace exahype2::fv::reconstruction

TARGET_DECLARE
/**
 * Piecewise-constant reconstruction, which gives the first-order Godunov scheme.
 */
struct exahype2::fv::reconstruction::Constant final {
  static constexpr int HaloSize = 1;

  /**
   * The flux and the largest wave speed of one state along every axis. The update of a volume
   * reads them for the volume and for its two neighbours along every axis.
   */
  template <
    typename SolverType,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    typename StoragePrecision,
    typename ComputePrecision
  >
  struct StateTerms final {
    ComputePrecision flux[DIMENSIONS][EvaluateFlux ? NumberOfUnknowns : 1]{};
    ComputePrecision maxEigenvalue[DIMENSIONS]{};

    INLINE_METHOD void evaluate(
      const StoragePrecision* NOALIAS              patch,
      const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
      const tarch::la::Vector<DIMENSIONS, int>&    volumeIndex,
      int                                          centreOffset,
      const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
      const tarch::la::Vector<DIMENSIONS, double>& patchSize,
      const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
      double                                       timeStamp,
      double                                       timeStepSize
    );
  };

  /**
   * The ring of StateTerms a sweep over a patch fills. A neighbour along the last axis lies one
   * layer of volumes away, so the ring covers one layer before and behind the current volume. A
   * solver with its own Riemann solver reads no StateTerms.
   */
  template <
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
    typename ComputePrecision
  >
  using SweepTerms = std::conditional_t<
    EvaluateCustomRiemann,
    exahype2::fv::NoTermRing,
    exahype2::fv::TermRing<
      StateTerms<
        SolverType,
        NumberOfUnknowns,
        NumberOfAuxiliaryVariables,
        EvaluateFlux,
        StoragePrecision,
        ComputePrecision
      >,
      peano4::power(NumberOfVolumesPerAxis, DIMENSIONS - 1)
    >
  >;

  template <
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
    typename Terms
  >
  INLINE_METHOD GPU_CALLABLE_METHOD static void accumulateFluxDivergence(
    const StoragePrecision* NOALIAS              inputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>&    volumeIndex,
    int                                          centreOffset,
    const StoragePrecision* NOALIAS              centreState,
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    StoragePrecision* NOALIAS                    outputState,
    double&                                      maxEigenvalueAfterTimeStep,
    const Terms&                                 terms
  );

  template <
    typename SolverType,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateCustomRiemann,
    typename StoragePrecision,
    typename ComputePrecision
  >
  INLINE_METHOD GPU_CALLABLE_METHOD static void patchFaceFlux(
    const StoragePrecision* NOALIAS              inputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>&    volumeIndex,
    int                                          patchFace,
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    ComputePrecision* NOALIAS                    flux
  );

  template <typename SolverType, int NumberOfUnknowns, bool EvaluateFlux, typename Terms, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static const ComputePrecision* getFluxAndMaxEigenvalue(
    const Terms&                                 terms,
    int                                          volume,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    ComputePrecision* NOALIAS                    fluxBuffer,
    ComputePrecision&                            maxEigenvalue
  );

  template <typename SolverType, typename Terms, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision getMaxEigenvalue(
    const Terms&                                 terms,
    int                                          volume,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/Constant.cpph"
