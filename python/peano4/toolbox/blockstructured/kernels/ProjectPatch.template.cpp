// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatch.h"

#include "peano4/utils/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"
#include "toolbox/blockstructured/Projection.h"

{% import 'Loop.macro.h' as patch with context %}{# get template macros #}
{% import 'CopyPatchOntoFaces.macro.h' as operation with context %}{# get template macros #}

void {{FULL_QUALIFIED_NAMESPACE}}::projectPatch(
  const {{PRECISION}}* NOALIAS const QIn,
  {{PRECISION}}* NOALIAS const faces[TWO_TIMES_D]
) {
  {{ patch.loopOverCells(operation) }}
}
