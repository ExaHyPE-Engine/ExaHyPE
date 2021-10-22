#include "elastic2d.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::elastic2d::elastic2d::_log( "examples::exahype2::elastic2d::elastic2d" );

void examples::exahype2::elastic2d::elastic2d::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );

  //stresses
  Q[0] = 0.0; //xx
  Q[1] = 0.0; //yy
  Q[2] = 0.0; //xy

  //velocities
  tarch::la::Vector<Dimensions,double> circleCentre = {5.0,5.0};
  bool inCircle = ( tarch::la::norm2( volumeCentre-circleCentre ) < 1.5 );
  
  Q[3] = inCircle ? std::exp(-tarch::la::norm2( volumeCentre-circleCentre )/2) * 2 : 0; //x
  Q[4] = Q[3]; //y

  //auxiliary variables
  Q[5] = inCircle ? 4.0 : 6.0; //rho
  Q[6] = inCircle ? 2.0 : 3.464; //cp
  Q[7] = inCircle ? 2.6 : 2.7; //cs

  logTraceOut( "initialCondition(...)" );
}

void examples::exahype2::elastic2d::elastic2d::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+3]
  double * __restrict__                        Qoutside, // Qoutside[5+3]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );

  //stresses
  Qoutside[0] = 0;
  Qoutside[1] = 0;
  Qoutside[2] = 0;
  
  //velocities
  Qoutside[3] = 0;
  Qoutside[4] = 0;
  
  //auxiliary variables
  Qoutside[5] = Qinside[5]; //rho
  Qoutside[6] = Qinside[6]; //cp
  Qoutside[7] = Qinside[7]; //cs
  
  logTraceOut( "boundaryConditions(...)" );
}

::exahype2::RefinementCommand examples::exahype2::elastic2d::elastic2d::refinementCriterion(
  const double * __restrict__ Q, // Q[5+3]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", volumeX, volumeH, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::equals(t,0.0) ) {	
	  tarch::la::Vector<Dimensions,double> circleCentre = {5.0, 5.0};
	  double distance = tarch::la::norm2( volumeX-circleCentre );
	  
	  bool nearCircle = ( 0.8 < distance && distance < 1.0 );
	  if(nearCircle){
		  result = ::exahype2::RefinementCommand::Refine;
	  }
	  
  }
  
  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}

double ::examples::exahype2::elastic2d::elastic2d::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+3],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  
  //distinct eigenvalues are cp, -cp, cs, -cs, 0
  double result = std::max(std::abs(Q[6]), std::abs(Q[7]));
  
  logTraceOut( "maxEigenvalue(...)" );
  
  return result;
}

void ::examples::exahype2::elastic2d::elastic2d::flux(
  const double * __restrict__ Q, // Q[5+3],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

  //stresses
  F[0] = 0;
  F[1] = 0;
  F[2] = 0;
  
  //velocities
  switch(normal){
  case 0:
	  F[3] = Q[0];
	  F[4] = Q[2];
	  break;
  case 1:
	  F[3] = Q[2];
	  F[4] = Q[1];
	  break;
  }
  
  logTraceOut( "flux(...)" );
}




void ::examples::exahype2::elastic2d::elastic2d::nonconservativeProduct(
  const double * __restrict__ Q, // Q[5+3],
  const double * __restrict__             deltaQ, // [5+3]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[5]
)  {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  
  //Lamé parameters
  double mu = Q[5] * Q[7] * Q[7]; //rho*cs^2
  double lambda = Q[5] * Q[6] * Q[6] - 2 * mu; //rho*cp^2 - 2 mu
  
  //stresses
  switch(normal){
  case 0: //x
	  BgradQ[0] = (lambda + 2*mu) * deltaQ[3];
	  BgradQ[1] =  lambda * deltaQ[3];
	  BgradQ[2] =  mu * deltaQ[4];
	  break;
  case 1: //y
	  BgradQ[0] =  lambda * deltaQ[4];
	  BgradQ[1] = (lambda + 2*mu) * deltaQ[4];
	  BgradQ[2] =  mu * deltaQ[3];
	  break;
  }
  
  //velocities
  BgradQ[3] = 0;
  BgradQ[4] = 0;
  
  logTraceOut( "nonconservativeProduct(...)" );
}



















