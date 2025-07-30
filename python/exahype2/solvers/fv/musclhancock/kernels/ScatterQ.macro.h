// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'MUSCLHancock.macro.h' as muscl with context %}{# get template macros #}
{% import 'Index.macro.h' as id with context %}

{% macro scatterQ() %}
  {{ muscl.addSourceAndNCP() }}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Scatter data into QOut
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QOutGathered[unknown]));
    #endif
    const int index = {{ id.index('volume', 'unknown') | indent(4) }}{##}
    QOut[index] = QOutGathered[unknown];
  } // unknown
{% endmacro %}
