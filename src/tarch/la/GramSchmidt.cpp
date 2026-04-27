// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GramSchmidt.h"

#include "ScalarOperations.h"

static_assert(sizeof(tarch::la::DynamicMatrix) > 0, "GramSchmidt compile sentinel");

void tarch::la::modifiedGramSchmidt(DynamicMatrix A, DynamicMatrix& Q, DynamicMatrix& R) {
  for (int i = 0; i < A.rows(); i++) {
    Q(i, 0) = A(i, 0);
  }

  for (int k = 0; k < A.cols(); k++) {
    R(k, k) = 0.0;
    for (int i = 0; i < A.rows(); i++) {
      R(k, k) += std::pow(A(i, k), 2);
    }
    if (tarch::la::greater(R(k, k), 0.0)) {
      R(k, k) = std::sqrt(R(k, k));
      for (int i = 0; i < A.rows(); i++) {
        Q(i, k) = A(i, k) / R(k, k);
      }
    }

    for (int j = k + 1; j < A.cols(); j++) {
      for (int i = 0; i < A.rows(); i++) {
        R(k, j) += Q(i, k) * A(i, j);
      }
      for (int i = 0; i < A.rows(); i++) {
        A(i, j) -= Q(i, k) * R(k, j);
      }
    }
  }
}
