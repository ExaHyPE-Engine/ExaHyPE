#include "CCZ4.h"
#include "exahype2/RefinementControl.h"

/**
 * I manually include this header which in turn declared all the
 * routines written in Fortran.
 */
#include "PDE.h"
#include "InitialData.h"



/**
 * This file is automatically created by Peano. I need it to interact with
 * the Python API, i.e. to read out data set there.
 */
#include "Constants.h"


#include <limits>




#include <stdio.h>
#include <string.h>


tarch::logging::Log   examples::exahype2::ccz4::CCZ4::_log( "examples::exahype2::ccz4::CCZ4" );


/**
 * Manually inserted to initialise CCZ4 component. The clean way
 * how to implement the selection process would be to define it
 * in the
 */
examples::exahype2::ccz4::CCZ4::CCZ4() {
  // int length = Scenario.size();
  // initparameters_(&length, Scenario.data());
}


/**
 * I leave this one as it is for the time being.
 */
::exahype2::RefinementCommand examples::exahype2::ccz4::CCZ4::refinementCriterion(
  double * __restrict__ Q, // Q[63+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", volumeX, volumeH, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::smallerEquals(_maxH,_NumberOfFiniteVolumesPerAxisPerPatch*tarch::la::max(volumeH)) ) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}


void examples::exahype2::ccz4::CCZ4::gaugeWave(
  double * __restrict__ Q, // Q[63+0],
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double t
) {
  constexpr double pi = M_PI;
  constexpr double ICA = 0.1; ///< Amplitude of the wave
  constexpr int nVars = 63;
  double HH     = 1.0 - ICA*sin( 2.0*pi*( volumeX[0] - t));
  double dxH    = -2.0*pi*ICA*cos( 2.0 * pi*(volumeX[0] - t));
  double dxphi  = - pow(HH,(-7.0/6.0))*dxH/6.0;
  double phi    = pow(( 1.0 / HH),(1.0/6.0));
  double Kxx    = - pi*ICA*cos( 2.0 * pi*(volumeX[0] - t))/sqrt( 1.0 - ICA*sin( 2.0*pi*( volumeX[0] - t))  );
  double traceK = Kxx/HH;
  memset(Q, 0, nVars*sizeof(double));
  Q[0]  = phi*phi*HH                         ;
  Q[3]  = phi*phi                            ;
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


/**
 * I don't adjust the solution, but I adjust the solution in the very
 * first time step, i.e. I impose initial conditions. For this, I forward
 * the initialisation request to the Fortran routines.
 */
void examples::exahype2::ccz4::CCZ4::adjustSolution(
  double * __restrict__ Q, // Q[63+0],
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "adjustSolution(...)", volumeX, volumeH, t );
  if (tarch::la::equals(t,0.0) ) {
    if ( Scenario=="gaugewave-c++" ) {
      gaugeWave(Q, volumeX, volumeH, t);
    }
    else {

      double width = volumeH(0);
      double t     = 0.0;
      double dt    = 0.0001;

  /*
    initialdata_(
      volumeX.data(), // const double* const x,const double* w,const double* t,const double* dt,double* Q
      &t,
      Q
    );
*/
      logError( "adjustSolution(...)", "initial scenario " << Scenario << " is not supported" );
    }
  }
  else {
    // other stuff
  }
  logTraceOut( "adjustSolution(...)" );
}




double examples::exahype2::ccz4::CCZ4::maxEigenvalue(
  double * __restrict__ Q, // Q[63+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );

  // helper data structure
  constexpr int Unknowns = 63;
  double lambda[Unknowns];
  for (int i=0; i<Unknowns; i++) lambda[i] = 0.0;

  // routine requires explicit normal vector
  double normalVector[3];

/*
  normalVector[0] = 0;
  normalVector[1] = 0;
  normalVector[2] = 0;
  if (normal==0) normalVector[0] = -1.0;
  if (normal==1) normalVector[1] = -1.0;
  if (normal==2) normalVector[2] = -1.0;
  if (normal==3) normalVector[0] =  1.0;
  if (normal==4) normalVector[1] =  1.0;
  if (normal==5) normalVector[2] =  1.0;
*/

  normalVector[0] = normal % 3 == 0 ? 1.0 : 0.0;
  normalVector[1] = normal % 3 == 1 ? 1.0 : 0.0;
  normalVector[2] = normal % 3 == 2 ? 1.0 : 0.0;


  // actual method invocation
  pdeeigenvalues_(lambda, Q, normalVector);

  // we are only interested in the maximum eigenvalue
  double result = 0.0;
  for (int i=0; i<Unknowns; i++) {
    result = std::max(result,lambda[i]);
  }

  logTraceOutWith1Argument( "eigenvalues(...)", result );
  return result;
}


void examples::exahype2::ccz4::CCZ4::nonconservativeProduct(
  double * __restrict__ Q, // Q[63+0],
  double                                       gradQ[63+0][Dimensions],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[63]
) {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );

  double gradQSerialised[63*3];
  for (int i=0; i<63; i++) {
    gradQSerialised[i+0*63] = gradQ[i][0];
    gradQSerialised[i+1*63] = gradQ[i][1];
    gradQSerialised[i+2*63] = gradQ[i][2];
  }

  pdencp_(BgradQ, Q, gradQSerialised);

  logTraceOut( "nonconservativeProduct(...)" );
}
