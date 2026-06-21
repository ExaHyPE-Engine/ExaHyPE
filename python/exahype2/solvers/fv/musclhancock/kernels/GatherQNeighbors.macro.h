// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro resolveNeighborPointer(v, out) %}
    if ({{v}}(normal) >= 0 && {{v}}(normal) < {{NUMBER_OF_VOLUMES_PER_AXIS}}) {
      // Interior: read from the cell patch.
      {% if DIMENSIONS==2 %}
      {{out}} = &QIn[{{v}}(1) * {{PATCH_CARDINALITY_1D}} + {{v}}(0) * {{NUMBER_OF_DATA}}];
      {% else %}
      {{out}} = &QIn[{{v}}(2) * {{PATCH_CARDINALITY_2D}} + {{v}}(1) * {{PATCH_CARDINALITY_1D}} + {{v}}(0) * {{NUMBER_OF_DATA}}];
      {% endif %}
    } else {
      // Halo: read from the face overlap.
      tarch::la::Vector<DIMENSIONS, int> overlapCell = {{v}};
      int faceIdx;
      if ({{v}}(normal) < 0) {
        overlapCell(normal) = {{v}}(normal) + {{HALO_SIZE}};
        faceIdx = normal;
      } else {
        overlapCell(normal) = {{v}}(normal) + {{HALO_SIZE}} - {{NUMBER_OF_VOLUMES_PER_AXIS}};
        faceIdx = normal + DIMENSIONS;
      }
      const int overlapIdx = toolbox::blockstructured::serialiseVoxelIndexInOverlap(
        overlapCell, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}, normal
      );
      {{out}} = &faceQIn[faceIdx][overlapIdx * {{NUMBER_OF_DATA}}];
    }
{% endmacro %}

{% macro gatherQNeighbors() %}
  const auto volumeCentreLeft  = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeLeft);
  const auto volumeCentreRight = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeRight);

  const {{STORAGE_PRECISION}}* NOALIAS QLeftP       = nullptr;
  const {{STORAGE_PRECISION}}* NOALIAS QLeftLeftP   = nullptr;
  const {{STORAGE_PRECISION}}* NOALIAS QRightP      = nullptr;
  const {{STORAGE_PRECISION}}* NOALIAS QRightRightP = nullptr;

  {
    {{ resolveNeighborPointer('volumeLeft',       'QLeftP')       | indent(4) }}
  }
  {
    {{ resolveNeighborPointer('volumeLeftLeft',   'QLeftLeftP')   | indent(4) }}
  }
  {
    {{ resolveNeighborPointer('volumeRight',      'QRightP')      | indent(4) }}
  }
  {
    {{ resolveNeighborPointer('volumeRightRight', 'QRightRightP') | indent(4) }}
  }

  #if defined(PEANO_WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Gather neighbor data
    assertion(std::isfinite(QLeftLeftP[unknown]));
    assertion(std::isfinite(QLeftP[unknown]));
    assertion(std::isfinite(QRightP[unknown]));
    assertion(std::isfinite(QRightRightP[unknown]));

    QInGatheredLeft[unknown]       = QLeftP[unknown];
    QInGatheredLeftLeft[unknown]   = QLeftLeftP[unknown];
    QInGatheredRight[unknown]      = QRightP[unknown];
    QInGatheredRightRight[unknown] = QRightRightP[unknown];
  } // unknown
{% endmacro %}
