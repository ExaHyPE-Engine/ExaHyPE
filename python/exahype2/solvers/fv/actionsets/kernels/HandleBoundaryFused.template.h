// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "exahype2/CellData.h"
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

void handleBoundaryFused(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                                        solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData,
  const int                                                              targetDevice
);

void handleBoundaryFused(
  {{FULL_QUALIFIED_SOLVER_NAME}}&             solver,
  {{STORAGE_PRECISION}}*                      facesFlattened,
  const uint64_t*                             faceIndices,
  const uint8_t*                              faceNumbers,
  const uint64_t*                             cellIndices,
  const tarch::la::Vector<DIMENSIONS,double>* cellCentres,
  const tarch::la::Vector<DIMENSIONS,double>* cellSizes,
  const double                                t,
  const double                                dt,
  const int                                   numberOfFaces,
  const int                                   numberOfCells,
  const int                                   targetDevice
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
