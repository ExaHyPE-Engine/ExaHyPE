// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::swe::frictionlaws {
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif

  const double Zeta = 35.0 * std::numbers::pi / 180.0; // Conversion of degrees to radians
  const double Zeta1 = 28.95 * std::numbers::pi / 180.0;
  const double Zeta2 = 44.09 * std::numbers::pi / 180.0;
  const double Zeta3 = 31.81 * std::numbers::pi / 180.0;

  const double Mu1 = std::tan(Zeta1);
  const double Mu2 = std::tan(Zeta2);
  const double Mu3 = std::tan(Zeta3);

  constexpr double Beta = 1.07;
  constexpr double Gamma = 2.01;
  constexpr double BetaStar = 0.06;
  // Kappa in the friction law is assumed to be 1 and is just initialised as a
  // const double here using Kappa. The user can apply std::pow((Fr/BetaStar),
  // Kappa) for evaluating T here in case the value of Kappa is different
  // from 1.

  constexpr double Kappa = 1.0;
  constexpr double FrictionLengthScale = 0.00035;

  const double Nu = (2.0 / 9.0) * (FrictionLengthScale / Beta) * (GRAV * std::sin(Zeta) / std::sqrt(GRAV * std::cos(Zeta))) * (((Mu2 - Mu1) / (std::tan(Zeta) - Mu1)) - 1.0);
  const double Hstart = FrictionLengthScale * (((Mu2 - Mu1) / (std::tan(Zeta) - Mu3)) - 1.0);
  const double Hstop = FrictionLengthScale * (((Mu2 - Mu1) / (std::tan(Zeta) - Mu1)) - 1.0);

  static inline GPUCallableMethod
  double maxEigenvalue(
    const double* const __restrict__             Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal
  ) InlineMethod;

  static inline GPUCallableMethod
  void maxEigenvalue(
    const double* const __restrict__             Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const __restrict__                   maxEigenvalue
  ) InlineMethod;

  static inline GPUCallableMethod
  void flux(
    const double* const __restrict__             Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const __restrict__                   F
  ) InlineMethod;

  static inline GPUCallableMethod
  void nonconservativeProduct(
    const double* const __restrict__             Q,
    const double* const __restrict__             deltaQ,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const __restrict__                   BTimesDeltaQ
  ) InlineMethod;

  static inline GPUCallableMethod
  void coulombFriction(
    const double* __restrict__                   Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    double* __restrict__                         S
  ) InlineMethod;

  static inline GPUCallableMethod
  void materialDependentBasalFriction(
    const double* __restrict__                   Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    double* __restrict__                         S
  ) InlineMethod;

  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace applications::exahype2::swe::frictionlaws

#include "FrictionLaws.cpph"
