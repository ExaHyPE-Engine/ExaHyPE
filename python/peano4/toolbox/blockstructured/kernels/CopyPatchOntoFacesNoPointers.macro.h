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

      std::uint64_t faceIndex = faceIndices[patchIndex * TWO_TIMES_D + d];

      #if defined(WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        #if !defined(WITH_GPU)
        assertion6({{ASSERTION_PREFIX}} or QIn[patchCellSerialised * {{UNKNOWNS}} + j] == QIn[patchCellSerialised * {{UNKNOWNS}} + j], j, i, sourceCell, d, patchCell, overlapCell);
        #endif
        uint64_t index = faceIndex * {{UNKNOWNS * FACES_PER_DOF}} + overlapCellSerialised * {{UNKNOWNS}} + j;
        facePool[index] = QIn[patchCellSerialised * {{UNKNOWNS}} + j];
      }

      patchCell(d)   = i + {{DOFS_PER_AXIS}} - {{OVERLAP}};
      overlapCell(d) = i;

      patchCellSerialised   = peano4::utils::dLinearised(patchCell, {{DOFS_PER_AXIS}});
      overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell, {{DOFS_PER_AXIS}}, {{OVERLAP}}, d);

      faceIndex = faceIndices[patchIndex * TWO_TIMES_D + d + DIMENSIONS];

      #if defined(WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        #if !defined(WITH_GPU)
        assertion6({{ASSERTION_PREFIX}} or QIn[patchCellSerialised * {{UNKNOWNS}} + j] == QIn[patchCellSerialised * {{UNKNOWNS}} + j], j, i,sourceCell, d, patchCell, overlapCell);
        #endif
        uint64_t index = faceIndex * {{UNKNOWNS * FACES_PER_DOF}} + overlapCellSerialised * {{UNKNOWNS}} + j;
        facePool[index] = QIn[patchCellSerialised * {{UNKNOWNS}} + j];
      }
    }
  }
{% endmacro %}

{% macro setup_patchdata_variables() %}
  // patchdata does not support this type of face indexing
{% endmacro %}

{% macro setup_patch_loop() %}
  const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
{% endmacro %}

{% macro cuda_kernel_parameters() %}
  const {{PRECISION}}* const NOALIAS QInFlattened,
  {{PRECISION}}*       const NOALIAS facePool,
  const std::uint64_t*               faceIndices
{% endmacro %}

{% macro parameters_passed_to_inner_loop() %}
  QInFlattened,
  facePool,
  faceIndices
{% endmacro %}
