
#ifndef _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_QUADRATURE_H_
#define _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_QUADRATURE_H_

// Use Gauss-Legendre quadrature

namespace generated {
namespace kernels {
namespace AderDG {
namespace nonlinear {

void initQuadratureNodesAndWeights();
void freeQuadratureNodesAndWeights();

extern double *nodes;
extern double *weights1;
extern double *weights2;
extern double *weights3;
extern double *weights4;
extern double *iweights1;
extern double *iweights3;


}
}
}
}
#endif /* _EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_QUADRATURE_H_ */