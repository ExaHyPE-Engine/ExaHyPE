// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AMRRoutines.h"

#include "DGMatrices.h"
#include "Quadrature.h"

{% if USE_LIBXSMM %}
#include "Gemms.h"
{% endif %}

#include "tarch/accelerator/accelerator.h"

{% import 'Gemms.template.h' as m with context %}{# get template macros #}

void {{FULL_QUALIFIED_NAMESPACE}}::faceUnknownsProlongation(
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS             lQhbndFine,
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS             lFhbndFine,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS lQhbndCoarse,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS lFhbndCoarse,
  const tarch::la::Vector<DIMENSIONS - 1, int>&            subfaceIndex
) {
{% if DIMENSIONS==3 %}
  double* tmpX = tarch::allocateMemory<double>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DATA}}, tarch::MemoryLocation::Heap);
{% endif %}

  {{CORRECTOR_COMPUTATION_PRECISION}}** projector = DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::fineGridProjector1d;

  // Apply the single level prolongation operator.
{% if DIMENSIONS==2 %}
  // Will overwrite outputs, no need to set to 0.
  {{ m.gemm('face_Q_x', 'lQhbndCoarse', 'projector[subfaceIndex[0]]', 'lQhbndFine', '0', '0', '0') | indent(4) }}{##}
  {{ m.gemm('face_F_x', 'lFhbndCoarse', 'projector[subfaceIndex[0]]', 'lFhbndFine', '0', '0', '0') | indent(4) }}{##}
{% else %}
  for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
    {{ m.gemm('face_Q_x', 'lQhbndCoarse', 'projector[subfaceIndex[0]]', 'tmpX', 'y*'~NUMBER_OF_DOF*NUMBER_OF_DATA, '0', 'y*'~NUMBER_OF_DOF*NUMBER_OF_DATA) | indent(6) }}{##}
  }
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('face_Q_y', 'tmpX', 'projector[subfaceIndex[1]]', 'lQhbndFine', 'x*'~NUMBER_OF_DATA, '0', 'x*'~NUMBER_OF_DATA) | indent(6) }}{##}
  }
  for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
    {{ m.gemm('face_F_x', 'lFhbndCoarse', 'projector[subfaceIndex[0]]', 'tmpX', 'y*'~NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS, '0', 'y*'~NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS) | indent(6) }}{##}
  }
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('face_F_y', 'tmpX', 'projector[subfaceIndex[1]]', 'lFhbndFine', 'x*'~NUMBER_OF_UNKNOWNS, '0', 'x*'~NUMBER_OF_UNKNOWNS) | indent(6) }}{##}
  }
{% endif %}

{% if DIMENSIONS==3 %}
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% endif %}
}

void {{FULL_QUALIFIED_NAMESPACE}}::faceFluxRestriction(
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS             lFhbndCoarse,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS lFhbndFine,
  const tarch::la::Vector<DIMENSIONS - 1, int>&            subfaceIndex
) {
{% if DIMENSIONS==3 %}
  double* tmpX = tarch::allocateMemory<double>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DATA}}, tarch::MemoryLocation::Heap);
{% endif %}

  {{CORRECTOR_COMPUTATION_PRECISION}}** restrictor = DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::fineGridProjector1d_T_weighted;

  // Apply the single level restriction operator.
  // Use the fine level unknowns as input in the first iteration.
{% if DIMENSIONS==2 %}
  {{ m.gemm('face_F_x_add', 'lFhbndFine', 'restrictor[subfaceIndex[0]]', 'lFhbndCoarse', '0', '0', '0') | indent(4) }}{##}
{% else %}
  for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
    {{ m.gemm('face_F_x', 'lFhbndFine', 'restrictor[subfaceIndex[0]]', 'tmpX', 'y*'~NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS, '0', 'y*'~NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS) | indent(6) }}{##}
  }
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('face_F_y_add', 'tmpX', 'restrictor[subfaceIndex[1]]', 'lFhbndCoarse', 'x*'~NUMBER_OF_UNKNOWNS, '0', 'x*'~NUMBER_OF_UNKNOWNS) | indent(6) }}{##}
  }
{% endif %}

{% if DIMENSIONS==3 %}
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% endif %}
}

void {{FULL_QUALIFIED_NAMESPACE}}::faceUnknownsRestriction(
  {{CORRECTOR_COMPUTATION_PRECISION}}* NOALIAS             lQhbndCoarse,
  const {{CORRECTOR_COMPUTATION_PRECISION}}* const NOALIAS lQhbndFine,
  const tarch::la::Vector<DIMENSIONS - 1, int>&            subfaceIndex
) {
{% if DIMENSIONS==3 %}
  double* tmpX = tarch::allocateMemory<double>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DATA}}, tarch::MemoryLocation::Heap);
{% endif %}

  {{CORRECTOR_COMPUTATION_PRECISION}}** restrictor = DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::fineGridProjector1d_T_weighted;

  // Apply the single level restriction operator.
  // Use the fine level unknowns as input in the first iteration.
{% if DIMENSIONS==2 %}
  {{ m.gemm('face_Q_x_add', 'lQhbndFine', 'restrictor[subfaceIndex[0]]', 'lQhbndCoarse', '0', '0', '0') | indent(4) }}{##}
{% else %}
  for (int y = 0; y < {{NUMBER_OF_DOF}}; y++) {
    {{ m.gemm('face_Q_x', 'lQhbndFine', 'restrictor[subfaceIndex[0]]', 'tmpX', 'y*'~NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS, '0', 'y*'~NUMBER_OF_DOF*NUMBER_OF_UNKNOWNS) | indent(6) }}{##}
  }
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('face_Q_y_add', 'tmpX', 'restrictor[subfaceIndex[1]]', 'lQhbndCoarse', 'x*'~NUMBER_OF_UNKNOWNS, '0', 'x*'~NUMBER_OF_UNKNOWNS) | indent(6) }}{##}
  }
{% endif %}

{% if DIMENSIONS==3 %}
  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% endif %}
}

void {{FULL_QUALIFIED_NAMESPACE}}::cellUnknownsProlongation(
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS             luhFine,
  const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS luhCoarse,
  const tarch::la::Vector<DIMENSIONS, int>&           subcellIndex
) {
  {{SOLUTION_STORAGE_PRECISION}}** projector = DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::fineGridProjector1d;

  double* tmpX = tarch::allocateMemory<double>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DATA}}, tarch::MemoryLocation::Heap);
{% if DIMENSIONS==3 %}
  double* tmpY = tarch::allocateMemory<double>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DATA}}, tarch::MemoryLocation::Heap);
{% endif %}

  // Will overwrite tmpX, no need to set to 0.
  for (int zy = 0; zy < {{NUMBER_OF_DOF*NUMBER_OF_DOF_3D}}; zy++) {
    {{ m.gemm('volume_x', 'luhCoarse', 'projector[subcellIndex[0]]', 'tmpX', 'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA, '0', 'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA) | indent(4) }}{##}
  }

{% if DIMENSIONS==2 %}
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('volume_y', 'tmpX', 'projector[subcellIndex[1]]', 'luhFine', 'x*'~NUMBER_OF_DATA, '0', 'x*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
{% else %}
  // Will overwrite tmpY, no need to set to 0.
  for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      {{ m.gemm('volume_y', 'tmpX', 'projector[subcellIndex[1]]', 'tmpY', '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA, '0', '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA) | indent(6) }}{##}
    }
  }

  for (int yx = 0; yx < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; yx++) {
    {{ m.gemm('volume_z', 'tmpY', 'projector[subcellIndex[2]]', 'luhFine', 'yx*'~NUMBER_OF_DATA, '0', 'yx*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
{% endif %}

  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% if DIMENSIONS==3 %}
  tarch::freeMemory(tmpY, tarch::MemoryLocation::Heap);
{% endif %}
}

void {{FULL_QUALIFIED_NAMESPACE}}::cellUnknownsRestriction(
  const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS luhFine,
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS             luhCoarse,
  const tarch::la::Vector<DIMENSIONS, int>&           subcellIndex
) {
  double* tmpX = tarch::allocateMemory<double>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DATA}}, tarch::MemoryLocation::Heap);
{% if DIMENSIONS==3 %}
  double* tmpY = tarch::allocateMemory<double>({{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF_3D}} * {{NUMBER_OF_DATA}}, tarch::MemoryLocation::Heap);
{% endif %}

  {{SOLUTION_STORAGE_PRECISION}}** restrictor = DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::fineGridProjector1d_T_weighted;

  // Will overwrite tmpX, no need to set to 0.
  for (int zy = 0; zy < {{NUMBER_OF_DOF_3D*NUMBER_OF_DOF}}; zy++) {
    {{ m.gemm('volume_x', 'luhFine', 'restrictor[subcellIndex[0]]', 'tmpX', 'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA, '0', 'zy*'~NUMBER_OF_DOF*NUMBER_OF_DATA) | indent(4) }}{##}
  }

{% if DIMENSIONS==2 %}
  // Add to the coarse output
  for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
    {{ m.gemm('volume_y_add', 'tmpX', 'restrictor[subcellIndex[1]]', 'luhCoarse', 'x*'~NUMBER_OF_DATA, '0', 'x*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
{% else %}
  // Will overwrite tmpY, no need to set to 0.
  for (int z = 0; z < {{NUMBER_OF_DOF}}; z++) {
    for (int x = 0; x < {{NUMBER_OF_DOF}}; x++) {
      {{ m.gemm('volume_y', 'tmpX', 'restrictor[subcellIndex[1]]', 'tmpY', '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA, '0', '(z*'~NUMBER_OF_DOF*NUMBER_OF_DOF~'+x)*'~NUMBER_OF_DATA) | indent(6) }}{##}
    }
  }

  // Add to the coarse output
  for (int yx = 0; yx < {{NUMBER_OF_DOF*NUMBER_OF_DOF}}; yx++) {
    {{ m.gemm('volume_z_add', 'tmpY', 'restrictor[subcellIndex[2]]', 'luhCoarse', 'yx*'~NUMBER_OF_DATA, '0', 'yx*'~NUMBER_OF_DATA) | indent(4) }}{##}
  }
{% endif %}

  tarch::freeMemory(tmpX, tarch::MemoryLocation::Heap);
{% if DIMENSIONS==3 %}
  tarch::freeMemory(tmpY, tarch::MemoryLocation::Heap);
{% endif %}
}
