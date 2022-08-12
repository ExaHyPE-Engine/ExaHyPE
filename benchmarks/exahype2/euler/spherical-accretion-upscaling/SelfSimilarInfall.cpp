#include "SelfSimilarInfall.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::_log( "examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall" );







void examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );

  // @todo Implement your stuff here, but ensure all entries are
  //   initialised incl all auxiliary variables. The snippet below
  //   sets everything to zero
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "boundaryConditions(...)" );
}





double ::examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}




void ::examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "flux(...)" );
}







void ::examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[5
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  // @todo implement
  logTraceOut( "sourceTerm(...)" );
}
















