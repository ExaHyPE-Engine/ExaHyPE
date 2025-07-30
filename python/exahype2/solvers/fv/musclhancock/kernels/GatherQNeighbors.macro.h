// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'Index.macro.h' as id with context %}{# get template macros #}

{% macro gatherQNeighbors() %}
  const auto volumeCentreLeft  = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeLeft);
  const auto volumeCentreRight = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeRight);

  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Gather neighbor data
    const int indexWithHaloLeft = {{ id.indexWithHalo('volumeLeft', 'unknown') | indent(6) }}{##}
    const int indexWithHaloLeftLeft = {{ id.indexWithHalo('volumeLeftLeft', 'unknown') | indent(6) }}{##}
    const int indexWithHaloRight = {{ id.indexWithHalo('volumeRight', 'unknown') | indent(6) }}{##}
    const int indexWithHaloRightRight = {{ id.indexWithHalo('volumeRightRight', 'unknown') | indent(6) }}{##}

    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QIn[indexWithHaloLeftLeft]));
    assertion(std::isfinite(QIn[indexWithHaloLeft]));
    assertion(std::isfinite(QIn[indexWithHaloRight]));
    assertion(std::isfinite(QIn[indexWithHaloRightRight]));
    #endif

    QInGatheredLeft[unknown] = QIn[indexWithHaloLeft];
    QInGatheredLeftLeft[unknown] = QIn[indexWithHaloLeftLeft];
    QInGatheredRight[unknown] = QIn[indexWithHaloRight];
    QInGatheredRightRight[unknown] = QIn[indexWithHaloRightRight];
  } // unknown
{% endmacro %}
