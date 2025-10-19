// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro inner_loop_macro() %}
  for (int d = 0; d < DIMENSIONS; d++) {
    for (int i = 0; i < {{OVERLAP}}; i++) {
      {% if DIMENSIONS==3 %}
      const tarch::la::Vector<DIMENSIONS, int> sourceCell({x, y, z});
      {% else %}
      const tarch::la::Vector<DIMENSIONS, int> sourceCell({x, y});
      {% endif %}
      tarch::la::Vector<DIMENSIONS, int> patchCell   = sourceCell;
      tarch::la::Vector<DIMENSIONS, int> overlapCell = sourceCell;
      patchCell(d)   = i;
      overlapCell(d) = i + {{OVERLAP}};

      int patchCellSerialised   = peano4::utils::dLinearised(patchCell, {{DOFS_PER_AXIS}});
      int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell, {{DOFS_PER_AXIS}}, {{OVERLAP}}, d);
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        #if !defined(WITH_GPU)
        assertion6({{ASSERTION_PREFIX}} or QIn[patchCellSerialised * {{UNKNOWNS}} + j] == QIn[patchCellSerialised * {{UNKNOWNS}} + j], j, i, sourceCell, d, patchCell, overlapCell);
        #endif
        faces[d][overlapCellSerialised * {{UNKNOWNS}} + j] = QIn[patchCellSerialised * {{UNKNOWNS}} + j];
      }

      patchCell(d)   = i + {{DOFS_PER_AXIS}} - {{OVERLAP}};
      overlapCell(d) = i;

      patchCellSerialised   = peano4::utils::dLinearised(patchCell, {{DOFS_PER_AXIS}});
      overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell, {{DOFS_PER_AXIS}}, {{OVERLAP}}, d);
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        #if !defined(WITH_GPU)
        assertion6({{ASSERTION_PREFIX}} or QIn[patchCellSerialised * {{UNKNOWNS}} + j] == QIn[patchCellSerialised * {{UNKNOWNS}} + j], j, i, sourceCell, d, patchCell, overlapCell);
        #endif
        faces[d + DIMENSIONS][overlapCellSerialised * {{UNKNOWNS}} + j] = QIn[patchCellSerialised * {{UNKNOWNS}} + j];
      }
    }
  }
{% endmacro %}

{% macro setup_patchdata_variables() %}
  const int numberOfPatches                            = patchData.numberOfCells;
  const {{PRECISION}}* const QInFlattened              = patchData.QInFlattened;
  {{PRECISION}}* const (*facePointers)[3][TWO_TIMES_D] = patchData.faces;
{% endmacro %}

{% macro setup_patch_loop() %}
  const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
  auto* const faces     = facePointers[patchIndex][1];
{% endmacro %}

{% macro cuda_kernel_parameters() %}
  const {{PRECISION}}* const NOALIAS QInFlattened,
  {{PRECISION}}* const               (*facePointers)[3][TWO_TIMES_D]
{% endmacro %}

{% macro parameters_passed_to_inner_loop() %}
  QInFlattened,
  facePointers
{% endmacro %}
