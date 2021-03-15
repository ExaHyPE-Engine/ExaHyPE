/**
 *
 * This code is taken from the original ExaHyPE project written by
 * colleagues from the University of Trento.
 *
 */

#ifndef __EXAHYPE_USER_PDE__
#define __EXAHYPE_USER_PDE__



// Fortran functions:
extern "C" {
void adjustedsolutionvalues_(const double* const x,const double* w,const double* t,const double* dt,double* Q);
void pdeflux_(double* Fx, double* Fy, double* Fz, const double* const Q);
void pdesource_(double* S, const double* const Q);
void pdencp_(double* BgradQ, const double* const Q, const double* const gradQ);
void pdefusedsrcncp_(double* S, const double* const Q, const double* const gradQ);
void pdeeigenvalues_(double* lambda, const double* const Q, double* nv);

void hllemfluxfv_(double* lambda, double* FL, double* FR, const double* const  QL, const double* const  QR, const int* normalNonZeroIndex);
void hllemriemannsolver_(const int* basisSize, const int* normalNonZeroIndex, double* FL, double* FR, const double* const  QL, const double* const  QR, const double* const  QavL, const double* const  QavR);

/**
 * @param constraints out Double vector of size 6
 * @param Q           in  Double vector of size 59
 * @param gradQ       in  Double matrix of size 59x3. Our Fortran codes works 
 *                        with a plain double array. Note that the argument is
 *                        thus linearised and it is linearised Fortran-style.
 */
void admconstraints_(double* constraints, const double* const Q, const double* const gradQ);

void pdecons2prim_(double* V,const double* Q);
void enforceccz4constraints_(double* Q); 


}/* extern "C" */

#endif /* __EXAHYPE_USER_PDE__ */
