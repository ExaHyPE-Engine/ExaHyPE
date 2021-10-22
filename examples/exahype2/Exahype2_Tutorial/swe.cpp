#include "swe.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::swe::swe::_log( "examples::exahype2::swe::swe" );

void examples::exahype2::swe::swe::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );

  // height of water
  bool isLeft = (volumeCentre[0] < 0.5);
  Q[0] = (isLeft ? 1.0 : 0.5);
  
  // height times velocities
  Q[1] = 0;
  Q[2] = 0;
  
  // bathimetry
  bool isExterior = (volumeCentre[1] < 0.25 || volumeCentre[1] > 0.75);
  Q[3] = (isExterior ? 1.0 : 0.5);

  logTraceOut( "initialCondition(...)" );
}


void examples::exahype2::swe::swe::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
	
  logTraceOut( "boundaryConditions(...)" );
}


double ::examples::exahype2::swe::swe::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );

  double result = 1.0;
  double g = 9.81;
  double c = std::sqrt( g * (Q[0]+Q[3]) );
  double u = 0.0;
  
  switch(normal){
  case 0: //x
	  u = Q[0] * Q[1];
	  result = std::max(u-c, u+c);
	  break;
  case 1: //y
	  u = Q[0] * Q[2];
	  result = std::max(u-c, u+c);
	  break;
  }
  
  return result;
  
  logTraceOut( "maxEigenvalue(...)" );
}


void ::examples::exahype2::swe::swe::flux(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[4]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  
  double irho = 1.0/Q[0];
  
  switch(normal){
  case 0:
	  F[0] = Q[1];
	  F[1] = irho * Q[1] * Q[1];
	  F[2] = irho * Q[1] * Q[2];
	  F[3] = 0;
	  break;
  case 1:
	  F[0] = Q[2];
	  F[1] = irho * Q[2] * Q[1];
	  F[2] = irho * Q[2] * Q[2];
	  F[3] = 0;
	  break;
  }

  logTraceOut( "flux(...)" );
}


void ::examples::exahype2::swe::swe::nonconservativeProduct(
  const double * __restrict__ Q, // Q[4+0],
  const double * __restrict__             deltaQ, // [4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[4]
)  {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  
  double g = 9.81;
  
  switch(normal){
  case 0: //x
	  BgradQ[0] = 0;
	  BgradQ[1] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
	  BgradQ[2] = 0;
	  BgradQ[3] = 0;
	  break;
  case 1: //y
	  BgradQ[0] = 0;
	  BgradQ[1] = 0;
	  BgradQ[2] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
	  BgradQ[3] = 0;
	  break;
  }
  
  logTraceOut( "nonconservativeProduct(...)" );
}

