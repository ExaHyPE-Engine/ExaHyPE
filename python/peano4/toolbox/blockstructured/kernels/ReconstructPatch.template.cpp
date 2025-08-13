// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReconstructPatch.h"

#include "peano4/utils/Loop.h"

{% import 'Loop.macro.h' as patch with context %}{# get template macros #}
{% import 'CopyPatchAndFillHalo.macro.h' as operation with context %}{# get template macros #}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatch(
  const {{PRECISION}}* NOALIAS const  QIn,
  {{PRECISION}}* NOALIAS const        faces[TWO_TIMES_D],
  {{PRECISION}}* NOALIAS const        QOut
) {
  {{ patch.loopOverCells(operation) }}
}
