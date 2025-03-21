// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void faceUnknownsProlongation(
  {{CORRECTOR_COMPUTATION_PRECISION}}* __restrict__             lQhbndFine,
  {{CORRECTOR_COMPUTATION_PRECISION}}* __restrict__             lFhbndFine,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const __restrict__ lQhbndCoarse,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const __restrict__ lFhbndCoarse,
  const tarch::la::Vector<Dimensions - 1, int>&                 subfaceIndex
);

void faceFluxRestriction(
  {{CORRECTOR_COMPUTATION_PRECISION}}* __restrict__             lFhbndCoarse,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const __restrict__ lFhbndFine,
  const tarch::la::Vector<Dimensions - 1, int>&                 subfaceIndex
);

void cellUnknownsProlongation(
  {{SOLUTION_STORAGE_PRECISION}}* __restrict__             luhFine,
  const {{SOLUTION_STORAGE_PRECISION}}* const __restrict__ luhCoarse,
  const tarch::la::Vector<Dimensions, int>&                subcellIndex
);

void cellUnknownsRestriction(
  const {{SOLUTION_STORAGE_PRECISION}}* const __restrict__ luhFine,
  {{SOLUTION_STORAGE_PRECISION}}* __restrict__             luhCoarse,
  const tarch::la::Vector<Dimensions, int>&                subcellIndex
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
