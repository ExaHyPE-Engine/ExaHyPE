#include "ADERDGEuler.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   examples::exahype2::euler::ADERDGEuler::_log( "examples::exahype2::euler::ADERDGEuler" );


void examples::exahype2::euler::ADERDGEuler::adjustSolution(
  double * __restrict__                       Q,  // Q[5+0],
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t
) {
  logTraceInWith2Arguments( "adjustSolution(...)", x, t );
  if (tarch::la::equals(t,0.0) ) {
    logDebug( "adjustSolution(...)", "init volume at " << x << "x" << t );

/*
    #if Dimensions==2
    tarch::la::Vector<Dimensions,double> circleCentre = {0.2,0.5};
    #else
    tarch::la::Vector<Dimensions,double> circleCentre = {0.2,0.5,0.5};
    #endif

    // initial conditions
    bool isInTheCentre = ( tarch::la::norm2( x-circleCentre ) < 0.05 );
    //bool isInTheCentre = x(0)<=0.5;
    //bool isInTheCentre = x(1)<=0.5;
    Q[0] = 0.1;  // rho
    Q[1] = 0;    // velocities
    Q[2] = 0;
    Q[3] = 0;
    Q[4] = isInTheCentre ? 1.0 : 0.0; // inner energy
*/
    constexpr double gamma = 1.4;
    constexpr double width = 0.25;
    constexpr double x0[3] = { 0.5, 0.5, 0.5 };

    Q[0] = 1.;
    Q[1] = 0.;
    Q[2] = 0.;
    Q[3] = 0.;
    #if Dimensions==2
    const double norm2Squared = (x[0]-x0[0])*(x[0]-x0[0]) + (x[1]-x0[1])*(x[1]-x0[1]);
    #else
    const double norm2Squared = (x[0]-x0[0])*(x[0]-x0[0]) + (x[1]-x0[1])*(x[1]-x0[1]) + (x[2]-x0[2])*(x[2]-x0[2]);
    #endif
    Q[4] = 1. / (gamma - 1) + // pressure is set to one
        exp(-std::sqrt(norm2Squared) / pow(width, Dimensions)) * 2;  }
  else {
    // other stuff
  }
  logTraceOut( "adjustSolution(...)" );
}




double examples::exahype2::euler::ADERDGEuler::maxEigenvalue(
  const double * __restrict__                 Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t,
  int                                         normal
)  {
  logTraceInWith3Arguments( "maxEigenvalue(...)", x, t, normal );

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  const double u_n = Q[normal + 1] * irho;
  const double c   = std::sqrt(gamma * p * irho);

  double result = std::max( std::abs(u_n - c), std::abs(u_n + c ));

  logTraceOutWith1Argument( "maxEigenvalue(...)", result );
  return result;
}




void examples::exahype2::euler::ADERDGEuler::flux(
  const double * __restrict__                 Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t,
  int                                         normal,
  double * __restrict__                       F // F[5]
)  {
  logTraceInWith3Arguments( "flux(...)", x, t, normal );

  assertion3( normal>=0, x, t, normal );
  assertion3( normal<Dimensions, x, t, normal);
  nonCriticalAssertion8( Q[0]==Q[0], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  nonCriticalAssertion8( Q[1]==Q[1], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  nonCriticalAssertion8( Q[2]==Q[2], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  nonCriticalAssertion8( Q[3]==Q[3], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  nonCriticalAssertion8( Q[4]==Q[4], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );

  //nonCriticalAssertion8( Q[0]>1e-12, Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  // @todo Has to be removed once we are sure that the ADER-DG kernels do work
  assertion8( Q[0]>1e-12, Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  const double u_n = irho*Q[normal+1];
  F[0] = u_n*Q[0];
  F[1] = u_n*Q[1];
  F[2] = u_n*Q[2];
  F[3] = u_n*Q[3];
  F[normal+1] += p;
  F[4] = u_n*(Q[4]+p);  

  nonCriticalAssertion( F[0]==F[0] );
  nonCriticalAssertion( F[1]==F[1] );
  nonCriticalAssertion( F[2]==F[2] );
  nonCriticalAssertion( F[3]==F[3] );
  nonCriticalAssertion( F[4]==F[4] );

  logTraceOut( "flux(...)" );
}



void examples::exahype2::euler::ADERDGEuler::boundaryConditions(
  const double * __restrict__                 Qinside, // Qinside[5+0]
  double * __restrict__                       Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t,
  int                                         normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );

  nonCriticalAssertion3( Qinside[0]==Qinside[0], x, t, normal );
  nonCriticalAssertion3( Qinside[1]==Qinside[1], x, t, normal );
  nonCriticalAssertion3( Qinside[2]==Qinside[2], x, t, normal );
  nonCriticalAssertion3( Qinside[3]==Qinside[3], x, t, normal );
  nonCriticalAssertion3( Qinside[4]==Qinside[4], x, t, normal );

  nonCriticalAssertion3( Qinside[0]>1e-12, x, t, normal );

  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];

  logTraceOut( "boundaryConditions(...)" );
}

