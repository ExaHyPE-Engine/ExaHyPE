
#ifndef _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_DGMATRICES_H_
#define _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_DGMATRICES_H_

#include <set>

namespace generated {
namespace kernels {
namespace AderDG {
namespace nonlinear {
// All matrices are stored as column major array, _T denote a transposed

void initDGMatrices();
void freeDGMatrices();

extern double *Kxi;
extern double *Kxi_T;
extern double *iK1_T; //note: the generic version of iK1 is actually transposed
extern double *dudx;
extern double *dudx_T;
extern double *FLCoeff;
extern double *FRCoeff;
extern double ** fineGridProjector1d;
extern double ** fineGridProjector1d_T_weighted; // [k][i*nDof+j] = fineGridProjector1d[k][j*nDof+i] * weight[j] / weight[i] / 3.0

}
}
}
}
#endif /* _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_DGMATRICES_H_ */