#include "ADERDGCCZ4.h"
#include "InitialValue.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/NonCriticalAssertions.h"


/** it is for aderdg solver with signatures changed from the one with fv.
 * I manually include this header which in turn declared all the
 * routines written in Fortran.
 */
#include "PDE.h"
#include "Properties.h"

/**
 * This file is automatically created by Peano. I need it to interact with
 * the Python API, i.e. to read out data set there.
 */
#include "Constants.h"

#include <limits>

#include <stdio.h>
#include <string.h>

tarch::logging::Log   examples::exahype2::ccz4::ADERDGCCZ4::_log( "examples::exahype2::ADERDGCCZ4::CCZ4" );



examples::exahype2::ccz4::ADERDGCCZ4::ADERDGCCZ4() {
  if ( Scenario=="gaugewave-c++" ) {
    const char* name = "GaugeWave";
    int length = strlen(name);
    initparameters_(&length, name);
  }
  else {
    std::cerr << "initial scenario " << Scenario << " is not supported" << std::endl << std::endl << std::endl;
  }
}



void examples::exahype2::ccz4::ADERDGCCZ4::algebraicSources(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double * __restrict__ S
) {
  constexpr int nVars = 59;
  for(int i=0; i<nVars; i++){
    assertion3( std::isfinite(Q[i]), i, x, t );
  }
  memset(S, 0, nVars*sizeof(double));
  pdesource_(S,Q);    //  S(Q)
  for(int i=0; i<nVars; i++){
    nonCriticalAssertion3( std::isfinite(S[i]), i, x, t );
  }
  // enforceccz4constraints_(Q); // "cleans" Q, but knows nothing about S
}


void examples::exahype2::ccz4::ADERDGCCZ4::adjustSolution(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t
) {
  logTraceInWith2Arguments( "adjustSolution(...)", x, t);
  if (tarch::la::equals(t,0.0) ) {
    if ( Scenario=="gaugewave-c++" ) {
      gaugeWave(Q, x, t);
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




double examples::exahype2::ccz4::ADERDGCCZ4::maxEigenvalue(
  const double * __restrict__ Q, // Q[64+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "eigenvalues(...)", x, t, normal );
  // helper data structure
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

  // @todo Raus
  assertion3( std::isfinite(result), x, t, normal );
  //nonCriticalAssertion3( std::isfinite(result), x, t, normal );

  logTraceOut( "eigenvalues(...)" );
  return result;
}


void examples::exahype2::ccz4::ADERDGCCZ4::nonconservativeProduct(
  const double * __restrict__                  Q, // Q[64+0],
  const double * __restrict__                  deltaQ, // [64+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[64]
) {
  logTraceInWith3Arguments( "nonconservativeProduct(...)", x, t, normal );

  assertion( normal>=0 );
  assertion( normal<Dimensions );
  constexpr int nVars = 59;
  double gradQSerialised[nVars*3];
  for (int i=0; i<nVars; i++) {
    gradQSerialised[i+0*nVars] = 0.0;
    gradQSerialised[i+1*nVars] = 0.0;
    gradQSerialised[i+2*nVars] = 0.0;

    gradQSerialised[i+normal*nVars] = deltaQ[i];
  }
  pdencp_(BgradQ, Q, gradQSerialised);

  for (int i=0; i<nVars; i++) {
    nonCriticalAssertion4( std::isfinite(BgradQ[i]), i, x, t, normal );
  }

  logTraceOut( "nonconservativeProduct(...)" );
}



void examples::exahype2::ccz4::ADERDGCCZ4::boundaryConditions(
  const double * __restrict__                  Qinside,
  double * __restrict__                        Qoutside,
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );
  for(int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    assertion4( Qinside[i]==Qinside[i], x, t, normal, i );
    Qoutside[i]=Qinside[i];
  }
  logTraceOut( "boundaryConditions(...)" );
}

