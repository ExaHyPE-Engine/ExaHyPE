#include "tarch/la/DynamicMatrixOperations.h"

tarch::la::DynamicMatrix tarch::la::transpose(const tarch::la::DynamicMatrix& matrix) {
  tarch::la::DynamicMatrix result(matrix.cols(), matrix.rows());
  #ifdef CompilerICC
  #pragma ivdep
  #endif
  for (int i=0; i < matrix.rows(); i++) {
    for (int j=0; j < matrix.cols(); j++) {
      result(j,i) += matrix(i,j);
    }
  }
  return result;
}

