#include "{{CLASSNAME}}.h"


tarch::logging::Log   {FULL_QUALIFIED_CLASSNAME}::_log( "{FULL_QUALIFIED_CLASSNAME}" );



::exahype2::RefinementCommand {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}::refinementCriterion(
  double Q[{{NUMBER_OF_UNKNOWNS}}],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {{
  logTraceInWith3Arguments( "refinementCriterion(...)", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::smallerEquals(_maxH,_NumberOfFiniteVolumesPerAxisPerPatch*tarch::la::max(h)) ) {{
    result = ::exahype2::RefinementCommand::Refine;
  }}

  logTraceOutWith1Argument( "refinementCriterion(...)", toString(result) );
  return result;
}}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}::adjustSolution(
  double Q[{{NUMBER_OF_UNKNOWNS}}],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {{
  logTraceInWith3Arguments( "adjustSolution(...)", x, h, t );
  if (tarch::la::equals(t,0.0) ) {{
    // initial conditions
  }}
  else {{
    // other stuff
  }}
  logTraceOut( "adjustSolution(...)" );
}}



void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}::eigenvalues(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double                                       lambda[{{NUMBER_OF_UNKNOWNS}}]
) {{
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "eigenvalues(...)" );
}}


void {FULL_QUALIFIED_CLASSNAME}::boundaryConditions(
  double                                       Qinside[{{NUMBER_OF_UNKNOWNS}}],
  double                                       Qoutside[{{NUMBER_OF_UNKNOWNS}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {{
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "boundaryConditions(...)" );
}}

