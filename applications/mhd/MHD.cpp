#include "MHD.h"

#include "PDE.h"
#include "InitialData.h"



void examples::exahype2::finitevolumes::MHD::initialCondition(
  double * NOALIAS Q,
  const tarch::la::Vector<DIMENSIONS,double>&  volumeCentre,
  const tarch::la::Vector<DIMENSIONS,double>&  volumeH
) {
  double xd[DIMENSIONS];
  for (int d=0; d< DIMENSIONS; d++) xd[d] = 0.0;//x[d];
  double t = 0;
  alfenwave_(&xd[0], &Q[0], &t);
  for(int i=0; i<9; i++){
    assert(std::isfinite(Q[i]));
    assert(!std::isnan(Q[i]));
  }
}



double examples::exahype2::finitevolumes::MHD::maxEigenvalue(
  const double * NOALIAS Q, // Q[9+0],
  const tarch::la::Vector<DIMENSIONS,double>&  faceCentre,
  const tarch::la::Vector<DIMENSIONS,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  double nv[3] = {0.};
  double lambda[9];
  nv[normal] = 1;
  pdeeigenvalues_(lambda, Q, nv);
  
  double result = 0;
  for (int i=0; i<9; i++) {
    result = std::max( result, std::abs(lambda[i]) );
  }
  
  
  return result;
}


void examples::exahype2::finitevolumes::MHD::flux(
  const double * NOALIAS Q, // Q[9+0],
  const tarch::la::Vector<DIMENSIONS,double>&  faceCentre,
  const tarch::la::Vector<DIMENSIONS,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * NOALIAS F // F[9]
) {
  pdeflux_(F, Q, &normal);
}


void examples::exahype2::finitevolumes::MHD::boundaryConditions(
  const double * NOALIAS Qinside, // Qinside[9+0]
  double * NOALIAS Qoutside, // Qoutside[9+0]
  const tarch::la::Vector<DIMENSIONS,double>&  faceCentre,
  const tarch::la::Vector<DIMENSIONS,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  for(int i=0; i< 9; i++)
      Qoutside[i]=Qinside[i];
}

