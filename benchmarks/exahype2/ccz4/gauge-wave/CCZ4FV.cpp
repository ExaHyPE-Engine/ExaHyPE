#include "CCZ4FV.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   benchmarks::exahype2::ccz4::CCZ4FV::_log( "benchmarks::exahype2::ccz4::CCZ4FV" );







void benchmarks::exahype2::ccz4::CCZ4FV::initialCondition(
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









double ::benchmarks::exahype2::ccz4::CCZ4FV::maxEigenvalue(
  const double * __restrict__ Q, // Q[59+0],
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







void ::benchmarks::exahype2::ccz4::CCZ4FV::nonconservativeProduct(
  const double * __restrict__                  Q,      // Q[59+0],
  const double * __restrict__                  deltaQ, // deltaQ[59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ  // BTimesDeltaQQ[59]
)  {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "nonconservativeProduct(...)" );
}




void ::benchmarks::exahype2::ccz4::CCZ4FV::sourceTerm(
  const double * __restrict__                  Q, // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[59
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  // @todo implement
  logTraceOut( "sourceTerm(...)" );
}
















