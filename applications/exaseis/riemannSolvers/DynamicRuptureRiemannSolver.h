// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "ElasticRiemannSolver.h"

#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaseis {
  struct FaultPlane;

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value getBoxcar(double coordinate, Value halfWidth);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value getFrictionalStrength(
    Value         normalStress,
    Value         slip,
    const double* x,
    double        t
  );

  template <typename Shortcuts, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveFaultInterface(
    const Value*               leftData,
    const Value*               rightData,
    const FaceNodeSide<Value>& left,
    const FaceNodeSide<Value>& right,
    double                     t,
    InterfaceValues<Value>&    leftInterface,
    InterfaceValues<Value>&    rightInterface
  );

  template <
    typename Shortcuts,
    typename Value,
    int NodesPerAxis,
    int NumberOfUnknowns,
    int NumberOfData,
    int FreeSurfaceFace
  >
  INLINE_METHOD GPU_CALLABLE_METHOD double solveDynamicRuptureRiemannProblem(
    const Value*                                 QL,
    const Value*                                 QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    int                                          normal,
    bool                                         isBoundaryFace,
    int                                          faceIndex,
    Value*                                       FL,
    Value*                                       FR
  );
} // namespace applications::exahype2::exaseis

/**
 * The plane of the fault: It is orthogonal to the axis normal and lies at position along that axis,
 * in the coordinates of the mesh.
 */
struct applications::exahype2::exaseis::FaultPlane final {
  int    normal{0};
  double position{0.0};

  static INLINE_METHOD void set(int normal, double position);
  [[nodiscard]] static INLINE_METHOD const FaultPlane& getHostCopy();
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD FaultPlane get();
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool containsFace(
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          faceNormal
  ) const;

private:
  [[nodiscard]] static INLINE_METHOD FaultPlane& getStoredHostCopy();
};

#include "DynamicRuptureRiemannSolver.cpph"
