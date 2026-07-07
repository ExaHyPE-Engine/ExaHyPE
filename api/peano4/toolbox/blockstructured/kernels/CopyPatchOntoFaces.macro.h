// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro CopyPatchOntoFaces() %}
  for (int d = 0; d < DIMENSIONS; d++) {
    for (int i = 0; i < {{OVERLAP}}; i++) {
      const tarch::la::Vector<DIMENSIONS, int> sourceCell = ::exahype2::fv::toVolumeIndex(x, y {% if DIMENSIONS==3 %}, z {% endif %});
      tarch::la::Vector<DIMENSIONS, int> patchCell   = sourceCell;
      tarch::la::Vector<DIMENSIONS, int> overlapCell = sourceCell;
      patchCell(d)   = i;
      overlapCell(d) = i + {{OVERLAP}};

      int patchCellSerialised   = peano4::dLinearised(patchCell, {{DOFS_PER_AXIS}});
      int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell, {{DOFS_PER_AXIS}}, {{OVERLAP}}, d);

      auto faceIndex = faceIndices[patchIndex * TWO_TIMES_D + d];

      #if defined(PEANO_WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        assertion({{ASSERTION_PREFIX}} or QIn[patchCellSerialised * {{UNKNOWNS}} + j] == QIn[patchCellSerialised * {{UNKNOWNS}} + j], j, i, sourceCell, d, patchCell, overlapCell);
        auto index = faceIndex * {{UNKNOWNS * FACES_PER_DOF}} + overlapCellSerialised * {{UNKNOWNS}} + j;
        facePool[index] = QIn[patchCellSerialised * {{UNKNOWNS}} + j];
      }

      patchCell(d)   = i + {{DOFS_PER_AXIS}} - {{OVERLAP}};
      overlapCell(d) = i;

      patchCellSerialised   = peano4::dLinearised(patchCell, {{DOFS_PER_AXIS}});
      overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell, {{DOFS_PER_AXIS}}, {{OVERLAP}}, d);

      faceIndex = faceIndices[patchIndex * TWO_TIMES_D + d + DIMENSIONS];

      #if defined(PEANO_WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        assertion({{ASSERTION_PREFIX}} or QIn[patchCellSerialised * {{UNKNOWNS}} + j] == QIn[patchCellSerialised * {{UNKNOWNS}} + j], j, i,sourceCell, d, patchCell, overlapCell);
        auto index = faceIndex * {{UNKNOWNS * FACES_PER_DOF}} + overlapCellSerialised * {{UNKNOWNS}} + j;
        facePool[index] = QIn[patchCellSerialised * {{UNKNOWNS}} + j];
      }
    }
  }
{% endmacro %}
