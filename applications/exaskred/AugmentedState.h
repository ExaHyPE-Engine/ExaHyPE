// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::riemann {
  enum class WetDryState;
  enum class RiemannStructure;

  constexpr int    NewtonIterations     = 1;
  constexpr double NewtonTolerance      = 1e-5;
  constexpr double ConvergenceTolerance = 1e-5;
  constexpr double ZeroTolerance        = 1e-5;

  template <class T>
  INLINE_METHOD GPU_CALLABLE_METHOD RiemannStructure determineRiemannStructure(
    const T& hLeft,
    const T& hRight,
    const T& uLeft,
    const T& uRight,
    const T& g,
    const T& hThreshold
  );

  template <class T>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveLinearEquation(
    const T matrix[3][3],
    const T rightHandSide[3],
    T       solution[3]
  );

  template <class T>
  INLINE_METHOD GPU_CALLABLE_METHOD T computeMiddleState(
    const T&          hLeft,
    const T&          hRight,
    const T&          uLeft,
    const T&          uRight,
    RiemannStructure& riemannStructure,
    T*                middleSpeeds,
    const T&          g,
    const T&          hThreshold
  );

  template <class T>
  INLINE_METHOD GPU_CALLABLE_METHOD WetDryState determineWetDryState(
    T&                hLeft,
    T&                huLeft,
    T&                hvLeft,
    T&                uLeft,
    T&                vLeft,
    T&                bLeft,
    T&                hRight,
    T&                huRight,
    T&                hvRight,
    T&                uRight,
    T&                vRight,
    T&                bRight,
    T&                hMiddle,
    RiemannStructure& riemannStructure,
    T*                middleSpeeds,
    const T&          g,
    const T&          hThreshold
  );

  template <class T>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeWaveDecomposition(
    T&                hLeft,
    T&                huLeft,
    T&                uLeft,
    T&                vLeft,
    T&                bLeft,
    T&                hRight,
    T&                huRight,
    T&                uRight,
    T&                vRight,
    T&                bRight,
    T&                hMiddle,
    const T&          sqrtGhLeft,
    const T&          sqrtGhRight,
    const T&          sqrthLeft,
    const T&          sqrthRight,
    WetDryState       wetDryState,
    RiemannStructure& riemannStructure,
    T                 fWaves[3][3],
    T                 waveSpeeds[3],
    T*                middleSpeeds,
    const T&          g,
    const T&          hThreshold
  );

  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void capDryReceiverMomentum(
    bool dryReceiver,
    T    receiverHeightBeforeUpdate,
    T    incomingWater,
    T    velocityScale,
    T    cfl,
    T    minimumWaterHeight,
    T&   incomingMomentumNormal,
    T&   incomingMomentumTransverse
  );

  template <auto Bathymetry, typename State, typename Flux>
  INLINE_METHOD GPU_CALLABLE_METHOD double augmentedState(
    const State&                                 QL,
    const State&                                 QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       gravity,
    double                                       minimumWaterHeight,
    Flux&                                        FL,
    Flux&                                        FR
  );
} // namespace applications::exahype2::exaskred::riemann

#include "AugmentedState.cpph"
