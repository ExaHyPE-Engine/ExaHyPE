#include "CCZ4.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/NonCriticalAssertions.h"

/**
 * I manually include this header which in turn declared all the
 * routines written in Fortran.
 */
#include "PDE.h"

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
  constexpr int nVars = 64;
  constexpr double pi = M_PI;
  constexpr double ICA = 0.1; ///< Amplitude of the wave
  double HH     = 1.0 - ICA*sin( 2.0*pi*( volumeX[0] - t));
  double dxH    = -2.0*pi*ICA*cos( 2.0 * pi*(volumeX[0] - t));
  double dxphi  = - pow(HH,(-7.0/6.0))*dxH/6.0;
  double phi    = pow(( 1.0 / HH),(1.0/6.0));
  double Kxx    = - pi*ICA*cos( 2.0 * pi*(volumeX[0] - t))/sqrt( 1.0 - ICA*sin( 2.0*pi*( volumeX[0] - t))  );
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
  Q[59] = 1.0;
}

/**
 * I don't adjust the solution, but I adjust the solution in the very
 * first time step, i.e. I impose initial conditions. For this, I forward
 * the initialisation request to the Fortran routines.
 */
void examples::exahype2::ccz4::CCZ4::adjustSolution(
  double * __restrict__ Q, // Q[64+0],
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "adjustSolution(...)", volumeX, volumeH, t );
  if (tarch::la::equals(t,0.0) ) {
    // initial conditions      
    if ( Scenario=="gaugewave-c++" ) {
          gaugeWave(Q, volumeX, volumeH, t);
    }
    else {

          double width = volumeH(0);
          double t     = 0.0;
          double dt    = 0.0001;
          logError( "adjustSolution(...)", "initial scenario " << Scenario << " is not supported" );
      }
  }
  else{
    for(int i=0; i<64; i++){
      assertion4( std::isfinite(Q[i]), i, volumeX, volumeH, t );
    }
    double S[64];
    memset(S, 0, 64*sizeof(double));
    pdesource_(S,Q);
    for(int i=0; i<64; i++){
      nonCriticalAssertion4( std::isfinite(S[i]), i, volumeX, volumeH, t );
    }
    enforceccz4constraints_(Q);
  }
  logTraceOut( "adjustSolution(...)" );
}




double examples::exahype2::ccz4::CCZ4::maxEigenvalue(
  double * __restrict__ Q, // Q[64+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );
  // helper data structure
  constexpr int Unknowns = 64;
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

  nonCriticalAssertion4( std::isfinite(result), faceCentre, volumeH, t, normal );

  logTraceOut( "eigenvalues(...)" );
  return result;
}


void examples::exahype2::ccz4::CCZ4::nonconservativeProduct(
  double * __restrict__ Q, // Q[64+0],
  double                                       gradQ[64+0][Dimensions],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[64]
) {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );

  double gradQSerialised[64*3];
  for (int i=0; i<64; i++) {
    gradQSerialised[i+0*64] = gradQ[i][0];
    gradQSerialised[i+1*64] = gradQ[i][1];
    gradQSerialised[i+2*64] = gradQ[i][2];
  }
  pdencp_(BgradQ, Q, gradQSerialised);

  for (int i=0; i<64; i++) {
    nonCriticalAssertion5( std::isfinite(BgradQ[i]), i, faceCentre, volumeH, t, normal );
  }

  logTraceOut( "nonconservativeProduct(...)" );
}



void examples::exahype2::ccz4::CCZ4::boundaryConditions(
  double * __restrict__ Qinside, // Qinside[64+0]
  double * __restrict__ Qoutside, // Qoutside[64+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  for(int i=0; i<64; i++)
      Qoutside[i]=Qinside[i];
  assertion(false);
  logTraceOut( "boundaryConditions(...)" );
}

