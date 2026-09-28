// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

namespace applications::exahype2::exaseis {
  template <typename Value>
  struct FaceNodeSide;

  template <typename Value>
  struct InterfaceValues;

  template <typename Shortcuts>
  concept HasMetricDerivatives = requires { Shortcuts::metricDerivative; };

  template <typename Shortcuts>
  concept HasPerfectlyMatchedLayer = requires {
    Shortcuts::pml;
    Shortcuts::pmlDamping;
  };

  template <typename Shortcuts>
  concept HasDisplacement = requires { Shortcuts::u; };

  template <typename Shortcuts, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD FaceNodeSide<Value> getFaceNodeSide(const Value* Q, int normal);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void rotateIntoLocalBasis(
    const FaceNodeSide<Value>& side,
    const Value*               physicalVector,
    Value*                     localVector
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

  template <typename Shortcuts, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void writeFluxFluctuation(
    const FaceNodeSide<Value>&    side,
    const InterfaceValues<Value>& interface,
    bool                          isLeftSide,
    Value*                        flux
  );

  template <
    typename Shortcuts,
    typename Value,
    int NodesPerAxis,
    int NumberOfUnknowns,
    int NumberOfData,
    typename SolveInterface
  >
  INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblemOnFaceNodes(
    const Value*          QL,
    const Value*          QR,
    int                   normal,
    const SolveInterface& solveInterface,
    Value*                FL,
    Value*                FR
  );

  template <
    typename Shortcuts,
    typename Value,
    int NodesPerAxis,
    int NumberOfUnknowns,
    int NumberOfData,
    int FreeSurfaceFace
  >
  INLINE_METHOD GPU_CALLABLE_METHOD double solveElasticRiemannProblem(
    const Value* QL,
    const Value* QR,
    int          normal,
    bool         isBoundaryFace,
    int          faceIndex,
    Value*       FL,
    Value*       FR
  );
} // namespace applications::exahype2::exaseis

/**
 * Holds the material and the state of one side of a face node. The velocity and the traction are
 * given in the local basis of normal, firstTangent and secondTangent.
 */
template <typename Value>
struct applications::exahype2::exaseis::FaceNodeSide final {
  Value density{};
  Value pressureWaveSpeed{};
  Value shearWaveSpeed{};
  Value shearModulus{};
  Value firstLameParameter{};
  Value pressureImpedance{};
  Value shearImpedance{};
  Value metricNorm{};
  Value normal[3]{};
  Value firstTangent[3]{};
  Value secondTangent[3]{};
  Value velocity[3]{};
  Value traction[3]{};
  Value pmlDamping[3]{};
};

/**
 * Holds the velocity and the traction that the interface imposes on one side, in the local basis.
 */
template <typename Value>
struct applications::exahype2::exaseis::InterfaceValues final {
  Value velocity[3]{};
  Value traction[3]{};
};

#include "ElasticRiemannSolver.cpph"
