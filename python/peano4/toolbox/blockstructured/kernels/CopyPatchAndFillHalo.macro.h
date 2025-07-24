// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro copyPatchAndFillHalo() %}
  //
  // Copy inner patch.
  //
  {% if DIMENSIONS == 3 %}
  const tarch::la::Vector<Dimensions, int> sourceCell({x, y, z});
  {% else %}
  const tarch::la::Vector<Dimensions, int> sourceCell({x, y});
  {% endif %}
  const tarch::la::Vector<Dimensions, int> destinationCell = sourceCell + tarch::la::Vector<Dimensions, int>({{OVERLAP}});
  const int sourceCellSerialised      = peano4::utils::dLinearised(sourceCell, {{DOFS_PER_AXIS}});
  const int destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int j = 0; j < {{UNKNOWNS}}; j++) {
    QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = QIn[sourceCellSerialised * {{UNKNOWNS}} + j];
    #ifdef GPUOffloadingOff
    assertion2(QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, j);
    #endif
  }

  auto serialisePatchIndex = [](const tarch::la::Vector<Dimensions, int>& overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d = 0; d < Dimensions; d++) {
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
      {% if DIMENSIONS == 3 %}
      tarch::la::Vector<Dimensions, int> sourceCell({x, y, z});
      {% else %}
      tarch::la::Vector<Dimensions, int> sourceCell({x, y});
      {% endif %}
      tarch::la::Vector<Dimensions, int> destinationCell = sourceCell + tarch::la::Vector<Dimensions, int>({{OVERLAP}});

      sourceCell(d)                 = i;
      destinationCell(d)            = i;
      int sourceCellSerialised      = serialisePatchIndex(sourceCell, d);
      int destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

      #if defined(SharedOMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = faces[d][sourceCellSerialised * {{UNKNOWNS}} + j];
        #ifdef GPUOffloadingOff
        assertion4({{ASSERTION_PREFIX_FOR_HALO}} or QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, destinationCell, j, d);
        #endif
      }

      sourceCell(d)             = i + {{OVERLAP}};
      destinationCell(d)        = i + {{DOFS_PER_AXIS}} + {{OVERLAP}};
      sourceCellSerialised      = serialisePatchIndex(sourceCell, d);
      destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});

      #if defined(SharedOMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int j = 0; j < {{UNKNOWNS}}; j++) {
        QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = faces[d + Dimensions][sourceCellSerialised * {{UNKNOWNS}} + j];
        #ifdef GPUOffloadingOff
        assertion4({{ASSERTION_PREFIX_FOR_HALO}} or QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j], sourceCell, destinationCell, j, d);
        #endif
      }
    }
  }
{% endmacro %}
