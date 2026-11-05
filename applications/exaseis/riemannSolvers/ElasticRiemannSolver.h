// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaseis {
  template <typename Value>
  struct FaceNodeSide;

  template <typename Value>
  struct InterfaceValues;

  template <typename State>
  concept HasMetricDerivatives = requires(const State& state) { state.metricDerivative; };

  template <typename Unknowns>
  concept HasPerfectlyMatchedLayer = requires(const Unknowns& unknowns) { unknowns.pml; };

  template <typename Unknowns>
  concept HasDisplacement = requires(const Unknowns& unknowns) { unknowns.u; };

  template <typename Value, typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD FaceNodeSide<Value> getFaceNodeSide(const State& Q, int normal);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, Value> rotateIntoLocalBasis(
    const FaceNodeSide<Value>&         side,
    const tarch::la::Vector<3, Value>& physicalVector
  );

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveInteriorInterface(
    const FaceNodeSide<Value>& left,
    const FaceNodeSide<Value>& right,
    InterfaceValues<Value>&    leftInterface,
    InterfaceValues<Value>&    rightInterface
  );

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD InterfaceValues<Value> getBoundaryInterface(
    const FaceNodeSide<Value>& side,
    bool                       isLowerFace,
    Value                      reflection
  );

  template <typename Value, typename Flux>
  INLINE_METHOD GPU_CALLABLE_METHOD void writeFluxFluctuation(
    const FaceNodeSide<Value>&    side,
    const InterfaceValues<Value>& interface,
    bool                          isLeftSide,
    Flux&                         flux
  );

  template <typename Value, typename FaceStates, typename FaceFluxes, typename SolveInterface>
  INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblemOnFaceNodes(
    const FaceStates&     QL,
    const FaceStates&     QR,
    int                   normal,
    const SolveInterface& solveInterface,
    FaceFluxes&           FL,
    FaceFluxes&           FR
  );

  template <typename Value, int FreeSurfaceFace, typename FaceStates, typename FaceFluxes>
  INLINE_METHOD GPU_CALLABLE_METHOD double solveElasticRiemannProblem(
    const FaceStates& QL,
    const FaceStates& QR,
    int               normal,
    bool              isBoundaryFace,
    int               faceIndex,
    FaceFluxes&       FL,
    FaceFluxes&       FR
  );
} // namespace applications::exahype2::exaseis

/**
 * FaceNodeSide stores the material and the state on one side of a face node. The velocity and the
 * traction lie in the local basis of normal, firstTangent and secondTangent.
 */
template <typename Value>
struct applications::exahype2::exaseis::FaceNodeSide final {
  Value                       density{};
  Value                       pressureWaveSpeed{};
  Value                       shearWaveSpeed{};
  Value                       shearModulus{};
  Value                       firstLameParameter{};
  Value                       pressureImpedance{};
  Value                       shearImpedance{};
  Value                       metricNorm{};
  tarch::la::Vector<3, Value> normal{};
  tarch::la::Vector<3, Value> firstTangent{};
  tarch::la::Vector<3, Value> secondTangent{};
  tarch::la::Vector<3, Value> velocity{};
  tarch::la::Vector<3, Value> traction{};
  tarch::la::Vector<3, Value> pmlDamping{};
};

/**
 * InterfaceValues stores the velocity and the traction that the interface imposes on one side in the
 * local basis.
 */
template <typename Value>
struct applications::exahype2::exaseis::InterfaceValues final {
  tarch::la::Vector<3, Value> velocity{};
  tarch::la::Vector<3, Value> traction{};
};

#include "ElasticRiemannSolver.cpph"
