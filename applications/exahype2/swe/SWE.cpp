#include "SWE.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::swe::SWE::_log( "examples::exahype2::swe::SWE" );



::exahype2::RefinementCommand examples::exahype2::swe::SWE::refinementCriterion(
  const double * __restrict__ Q, // Q[3+1]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", volumeX, volumeH, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::greaterEquals(Q[0] > 1e-8 && std::abs(Q[0] + Q[3]), 0.05)) { 
    result = ::exahype2::RefinementCommand::Refine;
  }

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}










