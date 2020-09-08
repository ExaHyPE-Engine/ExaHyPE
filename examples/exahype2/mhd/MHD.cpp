#include "MHD.h"

#include "PDE.h"
#include "InitialData.h"

tarch::logging::Log   examples::exahype2::finitevolumes::MHD::_log( "examples::exahype2::finitevolumes::MHD" );



::exahype2::RefinementCommand examples::exahype2::finitevolumes::MHD::refinementCriterion(
  double Q[9],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  if (tarch::la::equals(t,0.0) and tarch::la::max(h)>1.0/3.0 ) {
    result = ::exahype2::RefinementCommand::Refine;
  }
  logTraceOutWith1Argument( "refinementCriterion(...)", toString(result) );
  return result;
}


void examples::exahype2::finitevolumes::MHD::adjustSolution(
  double Q[9],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  logTraceInWith3Arguments( "adjustSolution(...)", x, h, t );
  if (tarch::la::equals(t,0.0) ) {
    // initial conditions
    double xd[Dimensions];
    for (int d=0; d< Dimensions; d++) xd[d] = 0.0;//x[d];
    alfenwave_(&xd[0], &Q[0], &t);
    for(int i=0; i<9; i++){
        if(!std::isfinite(Q[i])) std::cout << "xd " << xd[0] << " " << xd[1] << std::endl;
        assert(std::isfinite(Q[i]));
        assert(!std::isnan(Q[i]));
    }
  }
  logTraceOut( "adjustSolution(...)" );
}



void examples::exahype2::finitevolumes::MHD::eigenvalues(
  double                                       Q[9],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double                                       lambda[9]
) {
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );
  double nv[3] = {0.};
  nv[normal] = 1;
  pdeeigenvalues_(lambda, Q, nv);
  logTraceOut( "eigenvalues(...)" );
}


void examples::exahype2::finitevolumes::MHD::flux(
  double                                       Q[9],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double                                       F[9]
) {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  pdeflux_(F, Q, &normal);
  logTraceOut( "flux(...)" );
}


void examples::exahype2::finitevolumes::MHD::boundaryConditions(
  double                                       Qinside[9],
  double                                       Qoutside[9],
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

