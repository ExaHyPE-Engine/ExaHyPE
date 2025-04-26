// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void faceUnknownsProlongation(
  {{CORRECTOR_COMPUTATION_PRECISION}}* noalias             lQhbndFine,
  {{CORRECTOR_COMPUTATION_PRECISION}}* noalias             lFhbndFine,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const noalias lQhbndCoarse,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const noalias lFhbndCoarse,
  const tarch::la::Vector<Dimensions - 1, int>&                 subfaceIndex
);

void faceFluxRestriction(
  {{CORRECTOR_COMPUTATION_PRECISION}}* noalias             lFhbndCoarse,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const noalias lFhbndFine,
  const tarch::la::Vector<Dimensions - 1, int>&                 subfaceIndex
);

void cellUnknownsProlongation(
  {{SOLUTION_STORAGE_PRECISION}}* noalias             luhFine,
  const {{SOLUTION_STORAGE_PRECISION}}* const noalias luhCoarse,
  const tarch::la::Vector<Dimensions, int>&                subcellIndex
);

void cellUnknownsRestriction(
  const {{SOLUTION_STORAGE_PRECISION}}* const noalias luhFine,
  {{SOLUTION_STORAGE_PRECISION}}* noalias             luhCoarse,
  const tarch::la::Vector<Dimensions, int>&                subcellIndex
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
