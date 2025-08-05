// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'ApplyTerms.macro.h' as terms with context %}{# get template macros #}
{% import 'Index.macro.h' as id with context %}

{% macro gatherQ() %}
  // Gather data
  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(10) }}{##}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QInGathered[unknown] = QIn[indexWithHalo];
    QOutGathered[unknown] = QIn[indexWithHalo];
  }
  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  {{ terms.applySource('QInGathered', 'sourceGathered', 'volumeCentre', 'timeStamp') }}
  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
    #if !defined(WITH_GPU)
    assertion(std::isfinite(sourceGathered[unknown]));
    #endif
    QInExtrapolationUpdate[unknown] = sourceGathered[unknown];
  }
  {% else %}
  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
    QInExtrapolationUpdate[unknown] = 0.0;
  }
  {% endif %} {# source term implementation #}
{% endmacro %}
