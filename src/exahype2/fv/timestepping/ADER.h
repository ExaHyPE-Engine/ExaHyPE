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
 * ADER-WENO advances one volume from a polynomial space-time predictor. A worker owns
 * its predictor scratch and reads only the old patch and halos, so independent device
 * workers can update different volumes. At a patch face both sides restrict their
 * reconstruction to the same available stencil to preserve conservation.
 */
struct exahype2::fv::timestepping::ADER final {
  static constexpr int    NumberOfStages          = 1;
  static constexpr double PicardResidualTolerance = 1e-7;

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
};
TARGET_DECLARE_END

#include "exahype2/fv/timestepping/ADER.cpph"
