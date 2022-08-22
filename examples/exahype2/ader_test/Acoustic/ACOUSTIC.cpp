#include "ACOUSTIC.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::acoustic::ACOUSTIC::_log( "examples::exahype2::acoustic::ACOUSTIC" );




::exahype2::RefinementCommand examples::exahype2::acoustic::ACOUSTIC::refinementCriterion(
  const double * __restrict__ Q,               // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
	const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
	logTraceInWith3Arguments( "refinementCriterion(...)", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}




void examples::exahype2::acoustic::ACOUSTIC::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

#if Dimensions==2
  tarch::la::Vector<Dimensions,double> centre = {0.5,0.5,0.5};
#else
  tarch::la::Vector<Dimensions,double> centre = {0.5,0.5,0.5};
#endif

  bool nearCentre = tarch::la::norm2(x-centre) < 0.05;

  Q[0] = (nearCentre ? 0.1 : 0.0);
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::acoustic::ACOUSTIC::boundaryValues(
    const double* const x,
    const double        t,
    const double        dt,
    const int           faceIndex,
    const int           direction,
    const double* const fluxIn,
    const double* const stateIn,
    const double* const gradStateIn,
    double* const       fluxOut,
    double* const       stateOut
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, direction );

  stateOut[0] = 0.0;
  stateOut[1] = 0.0;
  stateOut[2] = 0.0;
  stateOut[3] = 0.0;

  fluxOut[0] = fluxIn[0];
  fluxOut[1] = fluxIn[1];
  fluxOut[2] = fluxIn[2];
  fluxOut[3] = fluxIn[3];

  logTraceOut( "boundaryConditions(...)" );
}




  

template<typename T>
void ::examples::exahype2::acoustic::ACOUSTIC::eigenvalues(
    const T* const Q, // Q[4+0]
    const int direction,
    T* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );

  lambda[0] = 1.0;
  lambda[1] = -1.0;
  lambda[2] = 0.0;
  lambda[3] = 0.0;

  logTraceOut( "eigenvalue(...)" );
}




template<typename T>
void ::examples::exahype2::acoustic::ACOUSTIC::flux(
  const T* const Q, // Q[4+0]
  T** const F // F[Dimensions*4]
)  {
  logTraceIn( "flux(...)");

  double rho = 2.7;
  double c = 1.0;
  double K_0    = rho*c*c;

  F[0][0] = K_0 * Q[1];
  F[0][1] = (1/rho) * Q[0];
  F[0][2] = 0.0;
  F[0][3] = 0.0;

  F[1][0] = K_0 * Q[2];
  F[1][1] = 0.0;
  F[1][2] = (1/rho) * Q[0];
  F[1][3] = 0.0;

#if Dimensions==3
  F[2][0] = K_0 * Q[3];
  F[2][1] = 0.0;
  F[2][2] = 0.0;
  F[2][3] = (1/rho) * Q[0];
#endif

  logTraceOut( "flux(...)" );
}








