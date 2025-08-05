// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'CopyPatchAndFillHalo.macro.h' as cp with context %}{# get template macros #}

{% macro loopOverCells() %}
  auto ids = std::views::cartesian_product(
    std::views::iota(0, numberOfPatches),
    std::views::iota(0, {{DOFS_PER_AXIS}}),
    std::views::iota(0, {{DOFS_PER_AXIS}}),
    std::views::iota(0, {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %})
  );

  std::for_each_n(std::execution::par_unseq, ids.begin(), std::ranges::size(ids),
    [QInFlattened, facesFlattened, QOutFlattened](auto idx) {
      auto [patchIndex, x, y, z] = idx;

      using FaceType          = const {{PRECISION}}(*)[{{FACES_PER_DOF*UNKNOWNS}}];
      const auto* const QIn   = &QInFlattened[patchIndex  * {{DOFS*UNKNOWNS}}];
      const auto* const faces = reinterpret_cast<FaceType>(&facesFlattened[patchIndex * TWO_TIMES_D * {{FACES_PER_DOF*UNKNOWNS}}]);
      auto* const QOut        = &QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];

      {{ cp.copyPatchAndFillHalo() | indent(4) }}
    }
  );
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches                 = patchData.numberOfCells;
  const {{PRECISION}}* const QInFlattened   = patchData.QInFlattened;
  const {{PRECISION}}* const facesFlattened = patchData.facesFlattened;
  {{PRECISION}}* const QOutFlattened        = patchData.QOutFlattened;

  {{ loopOverCells() }}
{% endmacro %}
