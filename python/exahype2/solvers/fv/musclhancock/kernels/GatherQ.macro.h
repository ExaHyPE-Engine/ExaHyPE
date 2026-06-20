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
    const int centerIndex = {{ id.index('volume', 'unknown') | indent(10) }}{##}
    assertion(std::isfinite(QIn[centerIndex]));
    QInGathered[unknown] = QIn[centerIndex];
    QOutGathered[unknown] = QIn[centerIndex];
  }
  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  {{ terms.applySource('QInGathered', 'sourceGathered', 'volumeCentre', 'timeStamp') }}
  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
    assertion(std::isfinite(sourceGathered[unknown]));
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
