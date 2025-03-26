// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

tarch::logging::Log   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %}
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isPhysicallyAdmissible(
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const Q,
  const tarch::la::Vector<Dimensions,double>&       x,
  const tarch::la::Vector<Dimensions,double>&       h,
  const double timeStamp
){
  return true;
}
{% endif %}