// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::reconstruction {
  template <exahype2::fv::limiter::SlopeLimiter Limiter>
  struct LimitedLinear;
} // namespace exahype2::fv::reconstruction

TARGET_DECLARE
/**
 * MUSCL-Hancock reconstruction with the slope limiter Limiter. The half-step predictor reads
 * neighbours along one axis only. A face stores the halo volumes along its normal only. Both volumes
 * at a face pass the centre of the lower volume plus half a volume width along the normal to the
 * solver as the face centre.
 */
template <exahype2::fv::limiter::SlopeLimiter Limiter>
struct exahype2::fv::reconstruction::LimitedLinear final {
  static constexpr int HaloSize = 2;

  template <
    int  NumberOfUnknowns,
    bool EvaluateNonconservativeProduct,
    bool EvaluateCustomRiemann,
    typename ComputePrecision
  >
  struct FaceTerms final {
    ComputePrecision flux[EvaluateCustomRiemann ? 1 : NumberOfUnknowns]{};
    ComputePrecision
      nonconservativeProduct[(EvaluateNonconservativeProduct and not EvaluateCustomRiemann) ? NumberOfUnknowns : 1]{};
    ComputePrecision leftGoingFlux[EvaluateCustomRiemann ? NumberOfUnknowns : 1]{};
    ComputePrecision rightGoingFlux[EvaluateCustomRiemann ? NumberOfUnknowns : 1]{};
    ComputePrecision maxWaveSpeed{0};
  };

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
  struct StateTerms final {
    using Face = FaceTerms<NumberOfUnknowns, EvaluateNonconservativeProduct, EvaluateCustomRiemann, ComputePrecision>;

    ComputePrecision minus[DIMENSIONS][NumberOfUnknowns + NumberOfAuxiliaryVariables]{};
    ComputePrecision plus[DIMENSIONS][NumberOfUnknowns + NumberOfAuxiliaryVariables]{};
    Face             upperFace[DIMENSIONS]{};

    INLINE_METHOD void evaluate(
      const StoragePrecision* NOALIAS               patch,
      const StoragePrecision* NOALIAS const         faces[peano4::FacesPerCell],
      const StoragePrecision* NOALIAS const         auxVarsFaces[peano4::FacesPerCell],
      const tarch::la::Vector<DIMENSIONS, int>&     volumeIndex,
      int                                           centreOffset,
      const tarch::la::Vector<DIMENSIONS, double>&  patchCentre,
      const tarch::la::Vector<DIMENSIONS, double>&  patchSize,
      const tarch::la::Vector<DIMENSIONS, double>&  volumeCentre,
      const tarch::la::Vector<DIMENSIONS, double>&  volumeSize,
      double                                        timeStamp,
      double                                        timeStepSize,
      const typename SolverType::RuntimeParameters& parameters
    );
  };

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
  using SweepTerms = exahype2::fv::TermRing<
    StateTerms<
      SolverType,
      NumberOfVolumesPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      EvaluateFlux,
      EvaluateNonconservativeProduct,
      EvaluateSource,
      EvaluateCustomRiemann,
      StoragePrecision,
      ComputePrecision
    >,
    peano4::power(NumberOfVolumesPerAxis, DIMENSIONS - 1)
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
    typename ComputePrecision
  >
  using TeamTerms = exahype2::fv::NoTermRing;

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
    const StoragePrecision* NOALIAS               inputPatch,
    const StoragePrecision* NOALIAS const         faceInputPatches[peano4::FacesPerCell],
    const StoragePrecision* NOALIAS const         faceAuxVarsPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>&     volumeIndex,
    int                                           centreOffset,
    const StoragePrecision* NOALIAS               centreState,
    const tarch::la::Vector<DIMENSIONS, double>&  patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  patchSize,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeSize,
    double                                        timeStamp,
    double                                        timeStepSize,
    int                                           boundaryFaces,
    StoragePrecision* NOALIAS                     outputState,
    double&                                       maxEigenvalueAfterTimeStep,
    Terms&                                        terms,
    const typename SolverType::RuntimeParameters& parameters
  );

private:
  template <int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static const StoragePrecision* getNeighbourAlongNormal(
    const StoragePrecision* NOALIAS           inputPatch,
    const StoragePrecision* NOALIAS const     faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    int                                       centreOffset,
    int                                       strideAlongNormal,
    int                                       normal,
    int                                       offset
  );

  template <
    typename SolverType,
    int  NumberOfUnknowns,
    int  NumberOfData,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    typename StoragePrecision,
    typename ComputePrecision
  >
  INLINE_METHOD GPU_CALLABLE_METHOD static void evolveFaceStates(
    const StoragePrecision* NOALIAS               leftState,
    const StoragePrecision* NOALIAS               centreState,
    const StoragePrecision* NOALIAS               rightState,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeSize,
    int                                           normal,
    double                                        timeStamp,
    double                                        timeStepSize,
    ComputePrecision* NOALIAS                     evolvedMinusState,
    ComputePrecision* NOALIAS                     evolvedPlusState,
    const typename SolverType::RuntimeParameters& parameters
  );

  template <typename SolverType, int NumberOfUnknowns, bool EvaluateFlux, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision rusanovFaceFlux(
    const ComputePrecision* NOALIAS               leftState,
    const ComputePrecision* NOALIAS               rightState,
    const tarch::la::Vector<DIMENSIONS, double>&  faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  volumeSize,
    int                                           normal,
    double                                        timeStamp,
    double                                        timeStepSize,
    ComputePrecision* NOALIAS                     faceFlux,
    const typename SolverType::RuntimeParameters& parameters
  );

  template <
    typename SolverType,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfData,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateCustomRiemann,
    typename ComputePrecision
  >
  INLINE_METHOD GPU_CALLABLE_METHOD static void evaluateFaceTerms(
    const ComputePrecision* NOALIAS                                                                       lowerState,
    const ComputePrecision* NOALIAS                                                                       upperState,
    const tarch::la::Vector<DIMENSIONS, double>&                                                          faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>&                                                          volumeSize,
    double                                                                                                timeStamp,
    double                                                                                                timeStepSize,
    int                                                                                                   normal,
    const tarch::la::Vector<DIMENSIONS, int>&                                                             volumeIndex,
    int                                                                                                   side,
    int                                                                                                   boundaryFaces,
    FaceTerms<NumberOfUnknowns, EvaluateNonconservativeProduct, EvaluateCustomRiemann, ComputePrecision>& face,
    const typename SolverType::RuntimeParameters&                                                         parameters
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/LimitedLinear.cpph"
