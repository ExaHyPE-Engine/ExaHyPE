// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "ADERDGSolverState.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Matrix.h"
#include "tarch/la/SymmetricMatrix.h"
#include "tarch/la/Vector.h"

namespace tests::exahype2::aderdg::grmhd {
  using Unknowns  = tests::exahype2::aderdg::ADERDGSolverUnknowns<double>;
  using Vector3   = tarch::la::Vector<3, double>;
  using Matrix3x3 = tarch::la::Matrix<3, 3, double>;

  INLINE_METHOD GPU_CALLABLE_METHOD double getScalarProductOf(const Vector3& left, const Vector3& right);

  INLINE_METHOD GPU_CALLABLE_METHOD Vector3 getScaledCrossProductOf(
    const Vector3& first,
    const Vector3& second,
    double         scale
  );

  INLINE_METHOD GPU_CALLABLE_METHOD Vector3 multiplyMatrixVector(const Matrix3x3& matrix, const Vector3& vector);

  INLINE_METHOD GPU_CALLABLE_METHOD double invertMatrix3x3(const Matrix3x3& matrix, Matrix3x3& inverse);

  INLINE_METHOD GPU_CALLABLE_METHOD Matrix3x3 getSpatialMetricOf(const tarch::la::SymmetricMatrix<3, double>& gij);

  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateRecoveryFunction(
    double  squaredVelocity,
    double& residual,
    double& residualDerivative,
    double  gam,
    double  d,
    double  e,
    double  s2,
    double  b2,
    double  sb2,
    double& w
  );

  INLINE_METHOD GPU_CALLABLE_METHOD double findSquaredVelocity(
    double  lowerStart,
    double  upperStart,
    double  tolerance,
    double& w,
    double  gam,
    double  d,
    double  e,
    double  s2,
    double  b2,
    double  sb2,
    bool&   failed
  );

  INLINE_METHOD GPU_CALLABLE_METHOD Unknowns getConservedVariablesOf(const Unknowns& V);

  INLINE_METHOD GPU_CALLABLE_METHOD Unknowns getPrimitiveVariablesOf(
    const Unknowns& Q,
    Matrix3x3&      gContr,
    double&         determinant
  );
} // namespace tests::exahype2::aderdg::grmhd

#include "PDE.cpph"
