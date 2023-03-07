#ifndef __EXAHYPE_USER_PDE__
#define __EXAHYPE_USER_PDE__

// Fortran functions:
extern "C" {
    void pdeflux_(double* F,  const double * __restrict__ Q, int* normal);
    void pdeeigenvalues_(double* lambda, const double * __restrict__ Q, double* nv);
}/* extern "C" */


#endif /* __EXAHYPE_USER_PDE__ */
