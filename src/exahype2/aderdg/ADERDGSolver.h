// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "basis/GaussLegendreBasis.h"
#include "basis/GaussLobattoBasis.h"
#include "Solver.h"

namespace exahype2::aderdg {
  class ADERDGSolver;
} // namespace exahype2::aderdg

class exahype2::aderdg::ADERDGSolver: public exahype2::aderdg::Solver {
public:
  // [order][node i]
  static double** weights;
  static double** nodes;
  // [order][node i][node j]
  static double*** Kxi;
  static double*** dudx;
  static double*** iK1;
  static double*** equidistantGridProjector;
  // [order][ 0("left") or 1("right") ][node i]
  static double*** FCoeff;
  // [order][ subinterval ][fine grid node i][coarse grid node j]
  static double**** fineGridProjector;
  // [order][node i]
  static exahype2::aderdg::UnivariateFunction** basisFunction;
  static exahype2::aderdg::UnivariateFunction** basisFunctionFirstDerivative;
  static exahype2::aderdg::UnivariateFunction** basisFunctionSecondDerivative;

  int    pml_cell_width;
  double pml_alpha_const;
  double pml_alpha_scalar;
  double pml_rel_error;
  int    pml_power;

  /*
   * The following functions exist because the generic kernels require these to exist in order to compile
   * even when they are not being called.
   * These should however never be accessed, and if the kernels are called using any of these they should be
   * be replaced by an instance of the function in a derived class.
   */

  virtual void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  ) {
    // should not be called
    std::abort();
  }

  virtual void nonconservativeProduct(
    const double* __restrict__ Q,
    const double* __restrict__ deltaQ,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ BTimesDeltaQ
  ) {
    // should not be called
    std::abort();
  }

  virtual void algebraicSource(
    const double* const Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* S
  ) {
    // should not be called
    std::abort();
  }

  virtual void pointSource(const double* const Q, const double* const x, const double t, const double dt, double* const forceVector, int n) {
    // should not be called
    std::abort();
  }

  virtual void viscousFlux(const double* const Q, const double* const gradQ, double** const F) {
    // should not be called
    std::abort();
  }

  virtual void viscousEigenvalues(const double* const Q, const int direction, double* const lambda) {
    // should not be called
    std::abort();
  }

  virtual void multiplyMaterialParameterMatrix(const double* const Q, double** const rhs) {
    // should not be called
    std::abort();
  }
};
