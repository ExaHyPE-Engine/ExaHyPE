// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "toolbox/blockstructured/Projection.h"
#include "tarch/la/Vector.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/VolumeIndex.h"

#include "{{SOLVER_NAME}}.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void handleBoundary(
  {{FULL_QUALIFIED_SOLVER_NAME}}&             solver,
  const tarch::la::Vector<DIMENSIONS,double>  patchCentre,
  const tarch::la::Vector<DIMENSIONS,double>  patchSize,
  double                                      t,
  double                                      dt,
  int                                         faceNumber,
  {{STORAGE_PRECISION}}*                      Q
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
