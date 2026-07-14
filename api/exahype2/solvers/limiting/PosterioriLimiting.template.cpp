// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

{% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %}
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isPhysicallyAdmissible(
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const Q,
  const tarch::la::Vector<DIMENSIONS, double>&      x,
  const tarch::la::Vector<DIMENSIONS, double>&      h,
  const double                                      timeStamp
) {
  return true;
}
{% endif %}
