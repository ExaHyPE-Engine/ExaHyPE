// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::exaskred::meteotsunami::numerics {
  using ScalarField3D = double (*)(double, double, double);

  double partialX(ScalarField3D f, double x, double y, double t);
  double partialY(ScalarField3D f, double x, double y, double t);
  double partialT(ScalarField3D f, double x, double y, double t);
} // namespace applications::exahype2::exaskred::meteotsunami::numerics
