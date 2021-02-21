#include "CCZ4.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/NonCriticalAssertions.h"

/**
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

tarch::logging::Log   examples::exahype2::ccz4::CCZ4::_log( "examples::exahype2::ccz4::CCZ4" );

void gaugeWave(
  double * __restrict__ Q, // Q[64+0],
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double t
) {
  constexpr int nVars = 59;
  constexpr double pi = M_PI;
  constexpr double peak_number = 2.0;
  constexpr double ICA = 0.1; ///< Amplitude of the wave
  double HH     = 1.0 - ICA*sin( peak_number*pi*( volumeX[0] - t));
  double dxH    = -peak_number*pi*ICA*cos( peak_number * pi*(volumeX[0] - t));
  double dxphi  = - pow(HH,(-7.0/6.0))*dxH/6.0;
  double phi    = pow(( 1.0 / HH),(1.0/6.0));
  double Kxx    = - 0.5*peak_number*pi*ICA*cos( peak_number * pi*(volumeX[0] - t))/sqrt( 1.0 - ICA*sin( peak_number*pi*( volumeX[0] - t))  );
  double traceK = Kxx/HH;
  memset(Q, .0, nVars*sizeof(double));
  Q[0]  = phi*phi*HH ;
  Q[3]  = phi*phi  ;
  Q[5]  = phi*phi                            ;
  Q[6]  = phi*phi*(Kxx - 1.0/3.0*traceK*HH ) ;
  Q[9] =  phi*phi*(0.0 - 1.0/3.0*traceK*1.0) ;
  Q[11] = phi*phi*(0.0 - 1.0/3.0*traceK*1.0)  ;
  Q[16] = log(sqrt(HH));
  Q[13] = 2.0/(3.0*pow(HH,(5.0/3.0)))*dxH        ;
  Q[23] = 1.0/(2.0*HH)*dxH               ;
  Q[35] = pow(HH,(-1.0/3.0))*dxH/3.0         ;
  Q[38] = phi*dxphi                     ;
  Q[40] = phi*dxphi                    ;
  Q[53] = traceK;
  Q[54] = log(phi);
  Q[55] = dxphi/phi;
}


examples::exahype2::ccz4::CCZ4::CCZ4() {
  if ( Scenario=="gaugewave-c++" ) {
    const char* name = "GaugeWave";
    int length = strlen(name);
    initparameters_(&length, name);
  }
  else {
    std::cerr << "initial scenario " << Scenario << " is not supported" << std::endl << std::endl << std::endl;
  }
}



void examples::exahype2::ccz4::CCZ4::sourceTerm(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__ S
) {
  constexpr int nVars = 59;
  for(int i=0; i<nVars; i++){
    assertion4( std::isfinite(Q[i]), i, volumeCentre, volumeH, t );
  }
  memset(S, 0, nVars*sizeof(double));
  pdesource_(S,Q);    //  S(Q)
  for(int i=0; i<nVars; i++){
    nonCriticalAssertion4( std::isfinite(S[i]), i, volumeCentre, volumeH, t );
  }
  // enforceccz4constraints_(Q); // "cleans" Q, but knows nothing about S
}


void examples::exahype2::ccz4::CCZ4::adjustSolution(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt
) {
  logTraceInWith4Arguments( "adjustSolution(...)", volumeX, volumeH, t, dt );
  if (tarch::la::equals(t,0.0) ) {
    if ( Scenario=="gaugewave-c++" ) {
      gaugeWave(Q, volumeX, volumeH, t);
    }
    else {
      logError( "adjustSolution(...)", "initial scenario " << Scenario << " is not supported" );
    }

    for (int i=0; i<NumberOfUnknowns; i++) {
      assertion5( std::isfinite(Q[i]), volumeX, volumeH, t, dt, i );
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




double examples::exahype2::ccz4::CCZ4::maxEigenvalue(
  const double * __restrict__ Q, // Q[64+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );
  // helper data structure
  constexpr int Unknowns = 59;
  double lambda[Unknowns];
  for (int i=0; i<Unknowns; i++) {
    nonCriticalAssertion5( std::isfinite(Q[i]), i, faceCentre, volumeH, t, normal );
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
  assertion4( std::isfinite(result), faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( std::isfinite(result), faceCentre, volumeH, t, normal );

  logTraceOut( "eigenvalues(...)" );
  return result;
}


void examples::exahype2::ccz4::CCZ4::nonconservativeProduct(
  const double * __restrict__                  Q, // Q[64+0],
  const double * __restrict__                  deltaQ, // [64+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[64]
) {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );

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
    nonCriticalAssertion5( std::isfinite(BgradQ[i]), i, faceCentre, volumeH, t, normal );
  }

  logTraceOut( "nonconservativeProduct(...)" );
}



void examples::exahype2::ccz4::CCZ4::boundaryConditions(
  const double * __restrict__                  Qinside,
  double * __restrict__                        Qoutside,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  for(int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    assertion5( Qinside[i]==Qinside[i], faceCentre, volumeH, t, normal, i );
    Qoutside[i]=Qinside[i];
  }
  logTraceOut( "boundaryConditions(...)" );
}

