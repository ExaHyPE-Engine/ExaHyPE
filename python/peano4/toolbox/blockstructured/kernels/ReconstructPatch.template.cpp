// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReconstructPatch.h"

#include "peano4/utils/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"

{% from 'CopyPatchAndFillHalo.macro.h' import CopyPatchAndFillHalo with context %}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatch(
  const {{PRECISION}}* NOALIAS const  QIn,
  {{PRECISION}}* NOALIAS const        faces[TWO_TIMES_D],
  {{PRECISION}}* NOALIAS const        QOut
) {
  {% if DIMENSIONS==3 %}
  for (int z = 0; z < {{DOFS_PER_AXIS}}; ++z)
  {% endif %}
  for (int y = 0; y < {{DOFS_PER_AXIS}}; ++y)
  for (int x = 0; x < {{DOFS_PER_AXIS}}; ++x) {
    //
    // Copy inner patch.
    //
    const tarch::la::Vector<DIMENSIONS, int> sourceCell = ::exahype2::fv::toVolumeIndex(x, y {% if DIMENSIONS==3 %}, z {% endif %});
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
        tarch::la::Vector<DIMENSIONS, int> sourceCell = ::exahype2::fv::toVolumeIndex(x, y {% if DIMENSIONS==3 %}, z {% endif %});
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
  }
}
