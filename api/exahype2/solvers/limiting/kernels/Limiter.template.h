// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void projectOnFVLimiterSpaceWithHalo(
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const   luh,
  {{LIMITER_SOLVER_STORAGE_PRECISION}}* const         lim
);

void projectOnFVLimiterSpaceWithoutHalo(
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const   luh,
  {{LIMITER_SOLVER_STORAGE_PRECISION}}* const         lim
);

void projectOnDGSpaceFromFVWithHalo(
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}*         lim,
  {{REGULAR_SOLVER_STORAGE_PRECISION}}* const         luh
);

void projectOnDGSpaceFromFVWithoutHalo(
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}*         lim,
  {{REGULAR_SOLVER_STORAGE_PRECISION}}* const         luh
);

void projectOnFVSpaceWithHaloFromFVWithoutHalo(
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}*         fvWithoutHalo,
  {{LIMITER_SOLVER_STORAGE_PRECISION}}* const         fvWithHalo
);

void projectOnFVSpaceWithoutHaloFromFVWithHalo(
  {{LIMITER_SOLVER_STORAGE_PRECISION}}* const         fvWithoutHalo,
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}*         fvWithHalo
);

bool isPhysicallyAdmissible(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                     solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const   luh,
  const tarch::la::Vector<DIMENSIONS, double>&        cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&        cellSize,
  const double                                        timeStamp
);

bool discreteMaximumPrincipleAndMinAndMaxSearch(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                     solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const   luh,
  const {{COMPUTE_PRECISION}}                         relaxationParameter,
  const {{COMPUTE_PRECISION}}                         differenceScaling,
  {{COMPUTE_PRECISION}}*                              boundaryMinPerObservable,
  {{COMPUTE_PRECISION}}*                              boundaryMaxPerObservable
);

void findCellLocalLimiterMinAndMax(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                     solver,
  const {{LIMITER_SOLVER_STORAGE_PRECISION}}*         lim,
  {{COMPUTE_PRECISION}}* const                        localMinPerObservable,
  {{COMPUTE_PRECISION}}* const                        localMaxPerObservable
);

void findCellLocalMinAndMax(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                     solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const   luh,
  {{COMPUTE_PRECISION}}* const                        localMinPerObservable,
  {{COMPUTE_PRECISION}}* const                        localMaxPerObservable
);

void compareWithADERDGSolutionAtGaussLobattoNodes(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                   solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  {{COMPUTE_PRECISION}}*                            min,
  {{COMPUTE_PRECISION}}*                            max
);

void compareWithADERDGSolutionAtFVSubcellCenters(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                   solver,
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const luh,
  {{COMPUTE_PRECISION}}*                            min,
  {{COMPUTE_PRECISION}}*                            max
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
