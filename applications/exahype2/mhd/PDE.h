#ifndef __EXAHYPE_USER_PDE__
#define __EXAHYPE_USER_PDE__

// Fortran functions:
extern "C" {
    void pdeflux_(double* F,  const double * NOALIAS Q, int* normal);
    void pdeeigenvalues_(double* lambda, const double * NOALIAS Q, double* nv);
}/* extern "C" */


#endif /* __EXAHYPE_USER_PDE__ */
