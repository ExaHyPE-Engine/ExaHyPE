#include "ADERDGAdvection.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::advection::ADERDGAdvection::_log( "examples::exahype2::advection::ADERDGAdvection" );







void examples::exahype2::advection::ADERDGAdvection::adjustSolution(
  double * __restrict__                       Q,
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t
) {
  logTraceInWith2Arguments( "adjustSolution(...)", x, t );
  if (tarch::la::equals(t,0.0) ) {
    // initial conditions
    Q[0] = 1.0;
    Q[1] = 0.1; // Q[1+i] are actually just parameters
    Q[2] = 0.2;
    Q[3] = 0.3;
  }
  else {
    // other stuff
  }
  logTraceOut( "adjustSolution(...)" );
}




void examples::exahype2::advection::ADERDGAdvection::boundaryConditions(
  const double * __restrict__ Qinside, // Qinside[4+0]
  double * __restrict__ Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t,
  int                                         normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  logTraceOut( "boundaryConditions(...)" );
}




double examples::exahype2::advection::ADERDGAdvection::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t,
  int                                         normal
)  {
  logTraceInWith2Arguments( "maxEigenvalue(...)", x, t );
  double result = std::abs(Q[normal+1]/Q[0]);
  logTraceOut( "maxEigenvalue(...)" );
  return result;
}




void examples::exahype2::advection::ADERDGAdvection::flux(
  const double * __restrict__                 Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t,
  int                                         normal,
  double * __restrict__                       F // F[4]
)  {
  logTraceInWith2Arguments( "flux(...)", x, t );
  F[0] = Q[normal+1];
  F[1] = 0.0;
  F[2] = 0.0;
  F[3] = 0.0;
  logTraceOut( "flux(...)" );
}






