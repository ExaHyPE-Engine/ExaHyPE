#ifndef __EXAHYPE_USER_PDE__
#define __EXAHYPE_USER_PDE__

// Fortran functions:
extern "C" {
    void pdeflux_(double* F,  double* Q, int* normal);
    void pdeeigenvalues_(double* lambda, double* Q, double* nv);
}/* extern "C" */

#endif /* __EXAHYPE_USER_PDE__ */
