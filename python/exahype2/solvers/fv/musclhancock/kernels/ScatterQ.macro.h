// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'MUSCLHancock.macro.h' as muscl with context %}{# get template macros #}
{% import 'Index.macro.h' as id with context %}

{% macro projectOntoFaces() %}
  // Project QOut onto faces
  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) {
    if (volume(normal) < {{HALO_SIZE}}) {
      // Left boundary: write to faceQOut[normal]
      tarch::la::Vector<DIMENSIONS, int> overlapCell = volume;
      overlapCell(normal) = volume(normal) + {{HALO_SIZE}};
      const int overlapIdx = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
        overlapCell, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}, normal
      );
      #if defined(PEANO_WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
        faceQOut[normal][overlapIdx * {{NUMBER_OF_DATA}} + unknown] = QOutGathered[unknown];
      }
    }
    if (volume(normal) >= {{NUMBER_OF_VOLUMES_PER_AXIS}} - {{HALO_SIZE}}) {
      // Right boundary: write to faceQOut[normal + DIMENSIONS]
      tarch::la::Vector<DIMENSIONS, int> overlapCell = volume;
      overlapCell(normal) = volume(normal) - ({{NUMBER_OF_VOLUMES_PER_AXIS}} - {{HALO_SIZE}});
      const int overlapIdx = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
        overlapCell, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}, normal
      );
      #if defined(PEANO_WITH_OPENMP)
      #pragma omp simd
      #else
      #pragma unroll
      #endif
      for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
        faceQOut[normal + DIMENSIONS][overlapIdx * {{NUMBER_OF_DATA}} + unknown] = QOutGathered[unknown];
      }
    }
  }
{% endmacro %}

{% macro scatterQ() %}
  {{ muscl.addSourceAndNCP() }}
  #if defined(PEANO_WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Scatter data into QOut
    assertion(std::isfinite(QOutGathered[unknown]));
    const int index = {{ id.index('volume', 'unknown') | indent(4) }}{##}
    QOut[index] = QOutGathered[unknown];
  } // unknown
{% endmacro %}
