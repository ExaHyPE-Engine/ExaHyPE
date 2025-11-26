// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro inner_loop_macro() %}
  //
  // Copy inner patch.
  //
  {% if DIMENSIONS==3 %}
  const tarch::la::Vector<DIMENSIONS, int> sourceCell({x, y, z});
  {% else %}
  const tarch::la::Vector<DIMENSIONS, int> sourceCell({x, y});
  {% endif %}
  const tarch::la::Vector<DIMENSIONS, int> destinationCell = sourceCell + tarch::la::Vector<DIMENSIONS, int>({{OVERLAP}});
  const int sourceCellSerialised      = ::peano4::utils::dLinearised(sourceCell, {{DOFS_PER_AXIS}});
  const int destinationCellSerialised = ::peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int j = 0; j < {{UNKNOWNS}}; j++) {
    QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = QIn[sourceCellSerialised * {{UNKNOWNS}} + j];
    #if !defined(WITH_GPU)
    assertion3(QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, j);
    #endif
  }

  //
  // Bring in the auxiliary patches, i.e. befill halo.
  //
  for (int d = 0; d < {{DIMENSIONS}}; d++) {
    for (int i = 0; i < {{OVERLAP}}; i++) {
      {% if DIMENSIONS==3 %}
      tarch::la::Vector<DIMENSIONS, int> sourceCell({x, y, z});
      {% else %}
      tarch::la::Vector<DIMENSIONS, int> sourceCell({x, y});
      {% endif %}
      tarch::la::Vector<DIMENSIONS, int> destinationCell = sourceCell + tarch::la::Vector<DIMENSIONS, int>({{OVERLAP}});

      sourceCell(d)                 = i;
      destinationCell(d)            = i;
      int sourceCellSerialised      = toolbox::blockstructured::serialiseVoxelIndexInOverlap(sourceCell, {{DOFS_PER_AXIS}}, {{OVERLAP}}, d);
      int destinationCellSerialised = ::peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

      #if defined(WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = faces[d][sourceCellSerialised * {{UNKNOWNS}} + j];
        #if !defined(WITH_GPU)
        assertion5({{ASSERTION_PREFIX_FOR_HALO}} or QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, destinationCell, j, d);
        #endif
      }

      sourceCell(d)             = i + {{OVERLAP}};
      destinationCell(d)        = i + {{DOFS_PER_AXIS}} + {{OVERLAP}};
      sourceCellSerialised      = toolbox::blockstructured::serialiseVoxelIndexInOverlap(sourceCell, {{DOFS_PER_AXIS}}, {{OVERLAP}}, d);
      destinationCellSerialised = ::peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

      #if defined(WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = faces[d + DIMENSIONS][sourceCellSerialised * {{UNKNOWNS}} + j];
        #if !defined(WITH_GPU)
        assertion5({{ASSERTION_PREFIX_FOR_HALO}} or QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, destinationCell, j, d);
        #endif
      }
    }
  }
{% endmacro %}

{% macro setup_patchdata_variables() %}
  const int numberOfPatches                                   = patchData.numberOfCells;
  const {{PRECISION}}* const QInFlattened                     = patchData.QInFlattened;
  const {{PRECISION}}* const (*facePointers)[3][TWO_TIMES_D]  = patchData.faces;
  {{PRECISION}}* const QOutFlattened                          = patchData.QOutFlattened;
{% endmacro %}

{% macro setup_patch_loop() %}
  const auto* const QIn   = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
  const auto* const faces = facePointers[patchIndex][2];
  auto* const QOut        = &QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];
{% endmacro %}

{% macro cuda_kernel_parameters() %}
  const {{PRECISION}}* const NOALIAS QInFlattened,
  const {{PRECISION}}* const         (*facePointers)[3][TWO_TIMES_D],
  {{PRECISION}}* const NOALIAS       QOutFlattened
{% endmacro %}

{% macro parameters_passed_to_inner_loop() %}
  QInFlattened,
  facePointers,
  QOutFlattened
{% endmacro %}
