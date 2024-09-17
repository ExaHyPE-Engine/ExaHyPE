#include "CCZ4.h"
#include "exahype2/RefinementControl.h"
#include "tarch/NonCriticalAssertions.h"

#include <algorithm>

#include "Constants.h"

#include <limits>
#include <stdio.h>
#include <string.h>


tarch::logging::Log   benchmarks::exahype2::ccz4::CCZ4::_log( "benchmarks::exahype2::ccz4::CCZ4" );


void benchmarks::exahype2::ccz4::CCZ4::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructed
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeX, volumeH, gridIsConstructed );

  constexpr int nVars = 59;
  constexpr double pi = M_PI;
  constexpr double peak_number = 2.0;
  constexpr double ICA = 0.1; ///< Amplitude of the wave 
  double HH     = 1.0 - ICA*sin( peak_number*pi*( volumeX[0] - 0));
  double dxH    = -peak_number*pi*ICA*cos( peak_number * pi*(volumeX[0] - 0));
  double dxphi  = - pow(HH,(-7.0/6.0))*dxH/6.0;
  double phi    = pow(( 1.0 / HH),(1.0/6.0));
  double Kxx    = - 0.5*peak_number*pi*ICA*cos( peak_number * pi*(volumeX[0] - 0))/sqrt( 1.0 - ICA*sin( peak_number*pi*( volumeX[0] - 0))  );
  double traceK = Kxx/HH;
  tarch::memset(Q, .0, nVars*sizeof(double));
  Q[0]  = phi*phi*HH ;          //\tilde(\gamma)_xx
  Q[3]  = phi*phi  ;          //\tilde(\gamma)_yy
  Q[5]  = phi*phi                            ;    //\tilde(\gamma)_zz
  Q[6]  = phi*phi*(Kxx - 1.0/3.0*traceK*HH ) ;    //\tilde(A)_xx
  Q[9] =  phi*phi*(0.0 - 1.0/3.0*traceK*1.0) ;    //\tilde(A)_yy
  Q[11] = phi*phi*(0.0 - 1.0/3.0*traceK*1.0) ;    //\tilde(A)_zz
  Q[16] = sqrt(HH);               //\alpha
  Q[13] = 2.0/(3.0*pow(HH,(5.0/3.0)))*dxH        ;  //\hat(\Gamma)^x
  Q[23] = 1.0/(2.0)*dxH*pow(HH,(-1.0/2.0))       ;    //A_x
  Q[35] = pow(HH,(-1.0/3.0))*dxH/3.0         ;    //D_xxx
  Q[38] = phi*dxphi                     ;   //D_xyy
  Q[40] = phi*dxphi                    ;    //D_xzz
  Q[53] = traceK;         //K
  Q[54] = phi;          //\phi
  Q[55] = dxphi;          //P_x

  for (int i=0; i<NumberOfUnknowns; i++) {
    assertion2( std::isfinite(Q[i]), volumeX, i );
  }

  logTraceOut( "initialCondition(...)" );
}


void benchmarks::exahype2::ccz4::CCZ4::sourceTerm(
  const double * __restrict__                  Q, // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[59
) {
#if !defined(GPUOffloadingOMP)
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  for(int i=0; i<NumberOfUnknowns; i++){
    assertion3( std::isfinite(Q[i]), i, volumeX, t );
  }
#endif
  benchmarks::exahype2::ccz4::CCZ4::sourceTerm(Q, volumeX, volumeH, t, dt, S, Offloadable::Yes);

#if !defined(GPUOffloadingOMP)
  for(int i=0; i<NumberOfUnknowns; i++){
    nonCriticalAssertion3( std::isfinite(S[i]), i, volumeX, t );
  }
  logTraceOut( "sourceTerm(...)" );
#endif
}


void benchmarks::exahype2::ccz4::CCZ4::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[59+0]
  double * __restrict__                        Qoutside, // Qoutside[59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  for(int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    assertion4( Qinside[i]==Qinside[i], faceCentre, t, normal, i );
    Qoutside[i]=Qinside[i];
  }
  logTraceOut( "boundaryConditions(...)" );
}


double benchmarks::exahype2::ccz4::CCZ4::maxEigenvalue(
  const double * __restrict__ Q, // Q[59+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)
{
   return maxEigenvalue(Q, faceCentre, volumeH, t, dt, normal, Offloadable::Yes);
}


void benchmarks::exahype2::ccz4::CCZ4::nonconservativeProduct(
  const double * __restrict__ Q, // Q[59+0],
  const double * __restrict__             deltaQ, // [59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[59]
)  {
#if !defined(GPUOffloadingOMP)
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  assertion( normal>=0 );
  assertion( normal<Dimensions );
#endif
  nonconservativeProduct(Q, deltaQ, faceCentre, volumeH, t, dt, normal, BgradQ, Offloadable::Yes);

#if !defined(GPUOffloadingOMP)
  for (int i=0; i<NumberOfUnknowns; i++) {
    nonCriticalAssertion4( std::isfinite(BgradQ[i]), i, faceCentre, t, normal );
  }
  logTraceOut( "nonconservativeProduct(...)" );
#endif
}

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void benchmarks::exahype2::ccz4::CCZ4::nonconservativeProduct(
  const double * __restrict__ Q, // Q[59+0],
  const double * __restrict__             deltaQ, // [59+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ BgradQ, // BgradQ[59]
  Offloadable
)
{
  double gradQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    gradQSerialised[i+0*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+1*NumberOfUnknowns] = 0.0;
    gradQSerialised[i+2*NumberOfUnknowns] = 0.0;

    gradQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  applications::exahype2::ccz4::ncp(BgradQ, Q, gradQSerialised, normal, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu, CCZ4SO);
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

::exahype2::RefinementCommand benchmarks::exahype2::ccz4::CCZ4::refinementCriterion(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>& volumeCentre,
  const tarch::la::Vector<Dimensions,double>& volumeH,
  double t
) {
  return ::exahype2::RefinementCommand::Keep;
}



