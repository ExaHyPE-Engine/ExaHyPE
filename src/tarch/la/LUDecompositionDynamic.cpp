#include "LUDecomposition.h"
#include <cstring>
#include <limits>

void tarch::la::backSubstitution(
  const DynamicMatrix& R,
  const double* f,
  double* x
){
  for(int k = R.cols()-1; k >= 0; k--){
    x[k] = f[k];
    for(int i = k+1; i < R.cols(); i++){
      x[k] -= R(k, i)*x[i];
    }
    if (R(k, k) < -std::numeric_limits<float>::min() or R(k, k) > std::numeric_limits<float>::min()){
      x[k] = x[k]/R(k, k);
    }
  }

}

tarch::la::DynamicMatrix tarch::la::invertUpperTriangular(
  const tarch::la::DynamicMatrix& R
){
  tarch::la::DynamicMatrix inverseR(R.cols(), R.cols());
  double* e = new double[R.cols()];
  double* col = new double[R.cols()];

  for (int i = 0; i < R.cols(); i++){
    std::memset(e, 0.0, R.cols() * sizeof(double));
    e[i] = 1.0;
    
    backSubstitution(R, e, col);
    for (int j = 0; j <= i; j++){
      inverseR(j, i) = col[j];
    }
  }
  delete[] e;
  delete[] col;
  return inverseR;
}
