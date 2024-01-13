// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractElasticWaveSolver.h"
#include "VariableShortcuts.h"

namespace applications::exahype2::exaseis {
  template <class T>
  class RiemannSolver {
  public:
    static GPUCallableInlineMethod void solve(
      T* const                                       FL,
      T* const                                       FR,
      const T* const                                 QL,
      const T* const                                 QR,
      const double                                   t,
      const double                                   dt,
      const ::tarch::la::Vector<Dimensions, double>& h,
      const int                                      direction,
      bool                                           isBoundaryFace,
      int                                            faceIndex
    ) InlineMethod;

  private:
    static GPUCallableInlineMethod void extractTransformation(
      const T* const Q, T& q_x, T& q_y, T& q_z, T& r_x, T& r_y, T& r_z, T& s_x, T& s_y, T& s_z
    ) InlineMethod;

    static GPUCallableInlineMethod void solveNodal(
      T v_p, T v_m, T sigma_p, T sigma_m, T z_p, T z_m, T& v_hat_p, T& v_hat_m, T& sigma_hat_p, T& sigma_hat_m
    ) InlineMethod;
    static GPUCallableInlineMethod void solveBoundary(
      int faceIndex, T r, T vn, T vm, T vl, T Tn, T Tm, T Tl, T zp, T zs, T& vn_hat, T& vm_hat, T& vl_hat, T& Tn_hat, T& Tm_hat, T& Tl_hat
    ) InlineMethod;

    static GPUCallableInlineMethod void solveBC0(T v, T sigma, T z, T r, T& v_hat, T& sigma_hat) InlineMethod;
    static GPUCallableInlineMethod void solveBCn(T v, T sigma, T z, T r, T& v_hat, T& sigma_hat) InlineMethod;

    static GPUCallableInlineMethod void localBasis(T* const n, T* const m, T* const l, int d) InlineMethod;
    static GPUCallableInlineMethod void gramSchmidt(T* const y, T* const z) InlineMethod;

    static GPUCallableInlineMethod void generateFluctuationsRight(T z, T myT, T T_hat, T v, T v_hat, T& F) InlineMethod;
    static GPUCallableInlineMethod void generateFluctuationsLeft(T z, T myT, T T_hat, T v, T v_hat, T& F) InlineMethod;
    static GPUCallableInlineMethod void rotateIntoPhysicalBasis(
      T* const n, T* const m, T* const l, T Fn, T Fm, T Fl, T& Fx, T& Fy, T& Fz
    ) InlineMethod;
    static GPUCallableInlineMethod void rotateIntoOrthogonalBasis(
      T* const n, T* const m, T* const l, T Tx, T Ty, T Tz, T& Tn, T& Tm, T& Tl
    ) InlineMethod;
    static GPUCallableInlineMethod void extractTractionsAndParticleVelocity(
      T* const n, const T* const Q, T& Tx, T& Ty, T& Tz, T& vx, T& vy, T& vz
    ) InlineMethod;
  };
} // namespace applications::exahype2::exaseis

#include "RiemannSolver.cpph"
