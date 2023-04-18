#include "swe.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::tutorial::swe::_log( "examples::exahype2::tutorial::swe" );



void examples::exahype2::tutorial::swe::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {

  bool nearCentre = tarch::la::norm2(volumeCentre)<0.40;
  // height of water
  Q[0] = 1.0;
  
  // height times velocities
  Q[1] = 0;
  Q[2] = 0;
  
  // bathimetry
  Q[3] = (nearCentre ? 0.0 : 0.1);

}





void examples::exahype2::tutorial::swe::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[3+1]
  double * __restrict__                        Qoutside, // Qoutside[3+1]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {

	  Qoutside[0] = Qinside[0];
	  Qoutside[1] = Qinside[1];
	  Qoutside[2] = Qinside[2];
	  Qoutside[3] = Qinside[3];

}





double ::examples::exahype2::tutorial::swe::maxEigenvalue(
  const double * __restrict__ Q, // Q[3+1],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {

	  double result = 1.0;
	  double ih = 1/Q[0];
	  double g = 9.81;
	  double c = std::sqrt( g * (Q[0]+Q[3]) );
	  double u = 0.0;
	  
	  switch(normal){
	  case 0: //x
		  u = ih * Q[1];
      result = std::max(std::abs(u-c), std::abs(u+c));
		  break;
	  case 1: //y
		  u = ih * Q[2];
      result = std::max(std::abs(u-c), std::abs(u+c));
		  break;
	  }
	  
	  return result;

}




void ::examples::exahype2::tutorial::swe::flux(
  const double * __restrict__ Q, // Q[3+1],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[3]
)  {

	  double ih = 1.0/Q[0];
	  
	  switch(normal){
	  case 0:
		  F[0] = Q[1];
		  F[1] = ih * Q[1] * Q[1];
		  F[2] = ih * Q[1] * Q[2];
		  break;
	  case 1:
		  F[0] = Q[2];
		  F[1] = ih * Q[2] * Q[1];
		  F[2] = ih * Q[2] * Q[2];
		  break;
	  }

}




void ::examples::exahype2::tutorial::swe::nonconservativeProduct(
  const double * __restrict__                  Q,      // Q[3+1],
  const double * __restrict__                  deltaQ, // deltaQ[3+1]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ  // BTimesDeltaQQ[3]
)  {

  double g = 9.81;
  
  switch(normal){
  case 0: //x
    BTimesDeltaQ[0] = 0.0;
    BTimesDeltaQ[1] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
    BTimesDeltaQ[2] = 0.0;
    break;
  case 1: //y
    BTimesDeltaQ[0] = 0.0;
    BTimesDeltaQ[1] = 0.0;
    BTimesDeltaQ[2] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
    break;
  }

}



::exahype2::RefinementCommand examples::exahype2::tutorial::swe::refinementCriterion(
  const double * __restrict__ Q, // Q[3+1]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t  
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  result = volumeX[0] < 0.0 ? ::exahype2::RefinementCommand::Refine : result;

  return result;
}















