// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/terms/Flux.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/SourceTerm.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::ader {
  template <typename SolverType, typename Reconstruction, int Unknowns, int Auxiliary>
  struct PredictorTerms;
} // namespace exahype2::fv::ader

TARGET_DECLARE
/**
 * Presents the PDE terms of the finite-volume solver SolverType to the space-time predictor of
 * ADER-DG, which evolves the polynomial of degree DGOrder that Reconstruction yields per volume.
 */
template <typename SolverType, typename Reconstruction, int Unknowns, int Auxiliary>
struct exahype2::fv::ader::PredictorTerms {
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
TARGET_DECLARE_END
