#include "GramSchmidt.h"

#include <limits>

void tarch::la::modifiedGramSchmidt (
  DynamicMatrix   A,
  DynamicMatrix&  Q,
  DynamicMatrix&  R
) {
  for (int i=0; i < A.rows(); i++){
    Q(i,0) = A(i,0);
  }

  for (int k=0; k < A.cols(); k++){
    // Compute norm of k-th column vector of (modified) A
    R(k,k) = 0.0;
    for (int i=0; i < A.rows(); i++){
      R(k,k) += std::pow(A(i,k), 2);
    }
    if (R(k, k) > std::numeric_limits<float>::min()){
      
      R(k,k) = std::sqrt(R(k,k));
      // Normalize k-th column of matrix Q
      for (int i=0; i < A.rows(); i++){
        Q(i,k) = A(i,k) / R(k,k);
      }
      
    }

    // Compute entries in R and next orthonormal vector
    for (int j=k+1; j < A.cols(); j++){
      // Compute entries of R from current Q and A
      for (int i=0; i < A.rows(); i++){
        R(k,j) += Q(i,k) * A(i,j);
      }
      // Subtract contributions from computed to open orthonormal vectors
      for (int i=0; i < A.rows(); i++){
        A(i,j) -= Q(i,k) * R(k,j);
      }
    }
  }
}

