// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro CopyPatchAndFillHalo() %}
  //
  // Copy inner patch
  //
  const tarch::la::Vector<DIMENSIONS, int> sourceCell = ::exahype2::fv::toVolumeIndex(x, y {% if DIMENSIONS==3 %}, z {% endif %});
  const tarch::la::Vector<DIMENSIONS, int> destinationCell = sourceCell + tarch::la::Vector<DIMENSIONS, int>({{OVERLAP}});
  const int sourceCellSerialised      = peano4::utils::dLinearised(sourceCell, {{DOFS_PER_AXIS}});
  const int destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int j = 0; j < {{UNKNOWNS}}; j++) {
    QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = QIn[sourceCellSerialised * {{UNKNOWNS}} + j];
    #if !defined(WITH_GPU)
    assertion2(QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, j);
    #endif
  }

  const auto serialisePatchIndex = [](const tarch::la::Vector<DIMENSIONS, int>& overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d = 0; d < DIMENSIONS; d++) {
      result += overlapCell(d) * base;
      if (d == normal) {
        base *= {{OVERLAP}} * 2;
      } else {
        base *= {{DOFS_PER_AXIS}};
      }
    }
    return result;
  };

  //
  // Bring in the auxiliary patches, i.e. befill halo.
  //
  for (int d = 0; d < {{DIMENSIONS}}; d++) {
    for (int i = 0; i < {{OVERLAP}}; i++) {
      tarch::la::Vector<DIMENSIONS, int> sourceCell = ::exahype2::fv::toVolumeIndex(x, y {% if DIMENSIONS==3 %}, z {% endif %});
      tarch::la::Vector<DIMENSIONS, int> destinationCell = sourceCell + tarch::la::Vector<DIMENSIONS, int>({{OVERLAP}});

      sourceCell(d)                 = i;
      destinationCell(d)            = i;
      int sourceCellSerialised      = serialisePatchIndex(sourceCell, d);
      int destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

      auto faceIndex = faceIndices[patchIndex * TWO_TIMES_D + d];

      #if defined(WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = facePool[
          faceIndex * {{UNKNOWNS * FACES_PER_DOF}} + sourceCellSerialised * {{UNKNOWNS}} + j
        ];
        #if !defined(WITH_GPU)
        assertion4({{ASSERTION_PREFIX_FOR_HALO}} or QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, destinationCell, j, d);
        #endif
      }

      sourceCell(d)             = i + {{OVERLAP}};
      destinationCell(d)        = i + {{DOFS_PER_AXIS}} + {{OVERLAP}};
      sourceCellSerialised      = serialisePatchIndex(sourceCell, d);
      destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

      faceIndex = faceIndices[patchIndex * TWO_TIMES_D + d + DIMENSIONS];

      #if defined(WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = facePool[
          faceIndex * {{UNKNOWNS * FACES_PER_DOF}} + sourceCellSerialised * {{UNKNOWNS}} + j
        ];
        #if !defined(WITH_GPU)
        assertion4({{ASSERTION_PREFIX_FOR_HALO}} or QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, destinationCell, j, d);
        #endif
      }
    }
  }
{% endmacro %}
