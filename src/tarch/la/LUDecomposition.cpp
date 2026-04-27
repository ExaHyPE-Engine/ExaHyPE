// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LUDecomposition.h"

#include <cstring>
#include <vector>

static_assert(sizeof(tarch::la::DynamicMatrix) > 0, "LUDecomposition compile sentinel");

void tarch::la::backSubstitution(const DynamicMatrix& R, const double* f, double* x) {
  for (int k = R.cols() - 1; k >= 0; k--) {
    x[k] = f[k];
    for (int i = k + 1; i < R.cols(); i++) {
      x[k] -= R(k, i) * x[i];
    }
    if (tarch::la::smaller(R(k, k), 0.0) or tarch::la::greater(R(k, k), 0.0)) {
      x[k] = x[k] / R(k, k);
    }
  }
}

tarch::la::DynamicMatrix tarch::la::invertUpperTriangular(const tarch::la::DynamicMatrix& R) {
  tarch::la::DynamicMatrix inverseR(R.cols(), R.cols());
  std::vector<double>      e(R.cols(), 0.0);
  std::vector<double>      col(R.cols(), 0.0);

  for (int i = 0; i < R.cols(); i++) {
    std::fill(e.begin(), e.end(), 0.0);
    e[i] = 1.0;

    backSubstitution(R, e.data(), col.data());
    for (int j = 0; j <= i; j++) {
      inverseR(j, i) = col[j];
    }
  }
  return inverseR;
}
