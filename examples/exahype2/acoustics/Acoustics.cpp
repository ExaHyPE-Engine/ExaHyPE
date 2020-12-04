#include "Acoustics.h"
#include "exahype2/RefinementControl.h"



tarch::logging::Log   examples::exahype2::acoustics::Acoustics::_log( "examples::exahype2::acoustics::Acoustics" );




::exahype2::RefinementCommand examples::exahype2::acoustics::Acoustics::refinementCriterion(
  double * __restrict__ Q, // Q[2+2]
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








