#include "EulerDG.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::euler::EulerDG::_log( "examples::exahype2::euler::EulerDG" );







void examples::exahype2::euler::EulerDG::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
  bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

  // @todo Implement your stuff here

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::euler::EulerDG::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );
  // @todo implement
  logTraceOut( "boundaryConditions(...)" );
}





double ::examples::exahype2::euler::EulerDG::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", x, t, dt, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}




void ::examples::exahype2::euler::EulerDG::flux(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[4]
)  {
  logTraceInWith4Arguments( "flux(...)", x, t, dt, normal );
  // @todo implement
  logTraceOut( "flux(...)" );
}
























