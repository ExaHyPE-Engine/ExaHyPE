#include "FiniteVolumeCCZ4.h"
#include "exahype2/RefinementControl.h"


#include "exahype2/NonCriticalAssertions.h"

/** it is for aderdg solver with signatures changed from the one with fv.
 *  * I manually include this header which in turn declared all the
 *   * routines written in Fortran.
 *    */
#include "PDE.h"
#include "Properties.h"
#include "InitialValue.h"

/**
 *  * This file is automatically created by Peano. I need it to interact with
 *   * the Python API, i.e. to read out data set there.
 *    */
#include "Constants.h"

#include <limits>

#include <stdio.h>
#include <string.h>



tarch::logging::Log   examples::exahype2::ccz4::FiniteVolumeCCZ4::_log( "examples::exahype2::ccz4::FiniteVolumeCCZ4" );



examples::exahype2::ccz4::FiniteVolumeCCZ4::FiniteVolumeCCZ4() {
  if ( Scenario=="gaugewave-c++" ) {
    const char* name = "GaugeWave";
    int length = strlen(name);
    initparameters_(&length, name);
  }
  else {
    std::cerr << "initial scenario " << Scenario << " is not supported" << std::endl << std::endl << std::endl;
  }
}


void examples::exahype2::ccz4::FiniteVolumeCCZ4::adjustSolution(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt
) {
  logTraceInWith4Arguments( "adjustSolution(...)", volumeX, volumeH, t, dt );
  if (tarch::la::equals(t,0.0) ) {
    if ( Scenario=="gaugewave-c++" ) {
      gaugeWave(Q, volumeX, t);
    }
    else {
      logError( "adjustSolution(...)", "initial scenario " << Scenario << " is not supported" );
    }

    for (int i=0; i<NumberOfUnknowns; i++) {
      assertion3( std::isfinite(Q[i]), x, t, i );
    }

    for (int i=NumberOfUnknowns; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
      Q[i] = 0.0;
    }
  }
  else {
    enforceccz4constraints_(Q);
  }
  logTraceOut( "adjustSolution(...)" );
}


void examples::exahype2::ccz4::FiniteVolumeCCZ4::sourceTerm(
  const double * __restrict__                  Q, // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[59
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  for(int i=0; i<NumberOfUnknowns; i++){
    assertion3( std::isfinite(Q[i]), i, x, t );
  }
  memset(S, 0, NumberOfUnknowns*sizeof(double));
  pdesource_(S,Q);    //  S(Q)
  for(int i=0; i<NumberOfUnknowns; i++){
    nonCriticalAssertion3( std::isfinite(S[i]), i, x, t );
  }
  logTraceOut( "sourceTerm(...)" );
}



void examples::exahype2::ccz4::FiniteVolumeCCZ4::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[59+0]
  double * __restrict__                        Qoutside, // Qoutside[59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  for(int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    assertion4( Qinside[i]==Qinside[i], x, t, normal, i );
    Qoutside[i]=Qinside[i];
  }
  logTraceOut( "boundaryConditions(...)" );
}




double examples::exahype2::ccz4::FiniteVolumeCCZ4::maxEigenvalue(
  const double * __restrict__ Q, // Q[59+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  constexpr int Unknowns = 59;
  double lambda[Unknowns];
  for (int i=0; i<Unknowns; i++) {
    nonCriticalAssertion4( std::isfinite(Q[i]), i, x, t, normal );
    lambda[i] = 1.0;
  }

  // routine requires explicit normal vector
  double normalVector[3];
  normalVector[0] = normal % 3 == 0 ? 1.0 : 0.0;
  normalVector[1] = normal % 3 == 1 ? 1.0 : 0.0;
  normalVector[2] = normal % 3 == 2 ? 1.0 : 0.0;
  
  // actual method invocation
  pdeeigenvalues_(lambda, Q, normalVector);

  // we are only interested in the maximum eigenvalue
  double result = 0.0;
  for (int i=0; i<Unknowns; i++) {
    result = std::max(result,std::abs(lambda[i]));
  }
  logTraceOut( "maxEigenvalue(...)" );
}







void examples::exahype2::ccz4::FiniteVolumeCCZ4::nonconservativeProduct(
  const double * __restrict__ Q, // Q[59+0],
  const double * __restrict__             deltaQ, // [59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[59]
)  {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  assertion( normal>=0 );
  assertion( normal<Dimensions );
  double gradQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    gradQSerialised[i+0*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+1*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+2*NumberOfUnknowns] = 0.0;

    gradQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  pdencp_(BgradQ, Q, gradQSerialised);

  for (int i=0; i<NumberOfUnknowns; i++) {
    nonCriticalAssertion4( std::isfinite(BgradQ[i]), i, x, t, normal );
  }
  logTraceOut( "nonconservativeProduct(...)" );
}

