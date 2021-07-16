#include "MHD.h"

#include "PDE.h"
#include "InitialData.h"

tarch::logging::Log   examples::exahype2::finitevolumes::MHD::_log( "examples::exahype2::finitevolumes::MHD" );



void examples::exahype2::finitevolumes::MHD::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );

  double xd[Dimensions];
  for (int d=0; d< Dimensions; d++) xd[d] = 0.0;//x[d];
  double t = 0;
  alfenwave_(&xd[0], &Q[0], &t);
  for(int i=0; i<9; i++){
    assert(std::isfinite(Q[i]));
    assert(!std::isnan(Q[i]));
  }

  logTraceOut( "initialCondition(...)" );
}



double examples::exahype2::finitevolumes::MHD::maxEigenvalue(
  const double * __restrict__ Q, // Q[9+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );
  double nv[3] = {0.};
  double lambda[9];
  nv[normal] = 1;
  pdeeigenvalues_(lambda, Q, nv);
  
  double result = 0;
  for (int i=0; i<9; i++) {
    result = std::max( result, std::abs(lambda[i]) );
  }
  
  logTraceOut( "eigenvalues(...)" );
  
  return result;
}


void examples::exahype2::finitevolumes::MHD::flux(
  const double * __restrict__ Q, // Q[9+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[9]
) {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  pdeflux_(F, Q, &normal);
  logTraceOut( "flux(...)" );
}


void examples::exahype2::finitevolumes::MHD::boundaryConditions(
  const double * __restrict__ Qinside, // Qinside[9+0]
  double * __restrict__ Qoutside, // Qoutside[9+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  for(int i=0; i< 9; i++)
      Qoutside[i]=Qinside[i];
  logTraceOut( "boundaryConditions(...)" );
}

