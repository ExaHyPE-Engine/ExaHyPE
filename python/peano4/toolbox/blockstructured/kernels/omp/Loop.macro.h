// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'CopyPatchAndFillHalo.macro.h' as cp with context %}{# get template macros #}

{% macro loopOverCells() %}
  {% if GPU == "omp" %}
  #pragma omp parallel for collapse(3)
  {% endif %}
  for (int z = 0; z < {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %}; z++) {
    for (int y = 0; y < {{DOFS_PER_AXIS}}; y++) {
      for (int x = 0; x < {{DOFS_PER_AXIS}}; x++) {
        {{ cp.copyPatchAndFillHalo() | indent(6) }}
      } // x
    } // y
  } // z
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches                 = patchData.numberOfCells;
  const {{PRECISION}}* const QInFlattened   = patchData.QInFlattened;
  const {{PRECISION}}* const facesFlattened = patchData.facesFlattened;
  {{PRECISION}}* const QOutFlattened        = patchData.QOutFlattened;

  using FaceType = const {{PRECISION}}(*)[{{FACES_PER_DOF*UNKNOWNS}}];

  {% if GPU == "omp" %}
  #pragma omp target teams distribute device(targetDevice)
  {% else %}
  #pragma omp parallel for
  {% endif %}
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    const auto* const QIn   = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
    const auto* const faces = reinterpret_cast<FaceType>(&facesFlattened[patchIndex * TWO_TIMES_D * {{FACES_PER_DOF*UNKNOWNS}}]);
    auto* const QOut        = &QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];

    {{ loopOverCells() | indent(2) }}
  }
{% endmacro %}
