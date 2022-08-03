#include "EulerFV.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::euler::EulerFV::_log( "examples::exahype2::euler::EulerFV" );


void examples::exahype2::euler::EulerFV::initialCondition(
  double * __restrict__                        Q,
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





void examples::exahype2::euler::EulerFV::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );

  ::applications::exahype2::euler::NeumannBoundaryConditions(
    Qinside, Qoutside, // Qoutside[5+0]
    faceCentre, volumeH, t, normal,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables
  );

  logTraceOut( "boundaryConditions(...)" );
}



double ::examples::exahype2::euler::EulerFV::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );

  double result = ::applications::exahype2::euler::maxEigenvalue(
    Q,
    faceCentre, volumeH, t, dt, normal,
    NumberOfUnknowns, NumberOfAuxiliaryVariables
  );

  logTraceOutWith1Argument( "maxEigenvalue(...)", result );
  return result;
}




void ::examples::exahype2::euler::EulerFV::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

  ::applications::exahype2::euler::flux(
    Q,
    faceCentre, volumeH, t, dt, normal,
    NumberOfUnknowns, NumberOfAuxiliaryVariables,
    F
  );

  logTraceOut( "flux(...)" );
}
