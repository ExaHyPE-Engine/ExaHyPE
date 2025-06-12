// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'SerialisePatchIndex.template.h' as helper with context %}{# get template macros #}

{% macro loopOverCells() %}
  {{ helper.serialisePatchIndex() }}

  //
  // Loop over original patch (k) and copy stuff over.
  //
  dfor(sourceCell, {{DOFS_PER_AXIS}}) {
    tarch::la::Vector<Dimensions, int> destinationCell = sourceCell + tarch::la::Vector<Dimensions, int>({{OVERLAP}});
    int sourceCellSerialised      = peano4::utils::dLinearised(sourceCell, {{DOFS_PER_AXIS}});
    int destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});
    for (int j = 0; j < {{UNKNOWNS}}; j++) {
      QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = QIn[sourceCellSerialised * {{UNKNOWNS}} + j];
      assertion2(
        QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j],
        sourceCell, j
      );
    }
  }

  //
  // Bring in the auxiliary patches, i.e. befill halo
  //
  for (int d = 0; d < Dimensions; d++) {
    //
    // d-loop over all dimensions except d. The vector k's entry d is set
    // to 0. We start with the left/bottom face, i.e. the one closer to the
    // coordinate system's origin.
    //
    dfore(k, {{DOFS_PER_AXIS}}, d, 0) {
      for (int i = 0; i < {{OVERLAP}}; i++) {
        tarch::la::Vector<Dimensions, int> destinationCell = k + tarch::la::Vector<Dimensions, int>({{OVERLAP}});
        tarch::la::Vector<Dimensions, int> sourceCell      = k;

        destinationCell(d) = i;
        sourceCell(d)      = i;

        int destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});
        int sourceCellSerialised      = serialisePatchIndex(sourceCell, d);

        for (int j = 0; j < {{UNKNOWNS}}; j++) {
          QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = faces[d][sourceCellSerialised * {{UNKNOWNS}} + j];
          assertion4(
            {{ASSERTION_PREFIX_FOR_HALO}} or QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j],
            sourceCell, destinationCell, j, d
          );
        }

        destinationCell(d) = i + {{DOFS_PER_AXIS}} + {{OVERLAP}};
        sourceCell(d)      = i + {{OVERLAP}};

        destinationCellSerialised = peano4::utils::dLinearised(destinationCell, {{DOFS_PER_AXIS}} + 2 * {{OVERLAP}});
        sourceCellSerialised      = serialisePatchIndex(sourceCell, d);

        for (int j = 0; j < {{UNKNOWNS}}; j++) {
          QOut[destinationCellSerialised * {{UNKNOWNS}} + j] = faces[d + Dimensions][sourceCellSerialised * {{UNKNOWNS}} + j];
          assertion4(
            {{ASSERTION_PREFIX_FOR_HALO}} or QOut[destinationCellSerialised * {{UNKNOWNS}} + j] == QOut[destinationCellSerialised * {{UNKNOWNS}} + j],
            sourceCell, destinationCell, j, d
          );
        }
      }
    }
  }
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches                 = patchData.numberOfCells;
  const {{PRECISION}}* const QInFlattened   = patchData.QInFlattened;
  const {{PRECISION}}* const facesFlattened = patchData.facesFlattened;
  {{PRECISION}}* const QOutFlattened        = patchData.QOutFlattened;

  using FaceType = const {{PRECISION}}(*)[{{FACES_PER_DOF*UNKNOWNS}}];

  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    const auto* const QIn   = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
    const auto* const faces = reinterpret_cast<FaceType>(&facesFlattened[patchIndex * TwoTimesD * {{FACES_PER_DOF*UNKNOWNS}}]);
    auto* const QOut        = &QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];

    {{ loopOverCells() | indent(2) }}
  }
{% endmacro %}
