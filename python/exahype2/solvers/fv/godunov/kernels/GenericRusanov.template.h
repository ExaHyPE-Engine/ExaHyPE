// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'Index.template.h' as id with context %}{# get template macros #}

{% macro createMemoryOnStack() %}
  {{COMPUTE_PRECISION}} QInGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QInGatheredLeft[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QInGatheredRight[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {{COMPUTE_PRECISION}} QOutGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {% if FLUX_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} fluxGatheredLeft[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} fluxGatheredRight[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}

  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} sourceGathered[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}

  {% if NCP_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} QAverageGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QAverageGatheredLeft[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} DeltaQGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} DeltaQGatheredLeft[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {{COMPUTE_PRECISION}} ncpGathered[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} ncpGatheredLeft[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}
{% endmacro %}

{% macro gatherQ() %}
  // Gather data
  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(10) }}{##}
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QInGathered[unknown] = QIn[indexWithHalo];
  }
  {% if STATELESS_SOURCE_TERM==True %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::sourceTerm(QInGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, sourceGathered);
  {% else %}
  solver.sourceTerm(QInGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, sourceGathered);
  {% endif %}

  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(sourceGathered[unknown]));
    #endif
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QOutGathered[unknown] = QIn[indexWithHalo] + timeStepSize * sourceGathered[unknown];
  } // unknown

  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = {{NUMBER_OF_UNKNOWNS}}; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QOutGathered[unknown] = QIn[indexWithHalo];
  } // unknown
  {% else %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QInGathered[unknown] = QIn[indexWithHalo];
    QOutGathered[unknown] = QIn[indexWithHalo];
  } // unknown
  {% endif %}
{% endmacro %}

{% macro scatterQ() %}
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

{% macro computeFluxesAndPerformTimeStep() %}
  const auto volumeCentreLeft  = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeLeft);
  const auto volumeCentreRight = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeRight);

  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Gather neighbor data
    const int indexWithHaloLeft = {{ id.indexWithHalo('volumeLeft', 'unknown') | indent(6) }}{##}
    const int indexWithHaloRight = {{ id.indexWithHalo('volumeRight', 'unknown') | indent(6) }}{##}

    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QIn[indexWithHaloLeft]));
    assertion(std::isfinite(QIn[indexWithHaloRight]));
    #endif

    QInGatheredLeft[unknown] = QIn[indexWithHaloLeft];
    QInGatheredRight[unknown] = QIn[indexWithHaloRight];

    {% if NCP_IMPLEMENTATION!="<none>" %}
    /**
     * We count faces starting with zero: Let the leftmost volume have
     * the index i. i is typically 0 or -1 (if we have halos).
     * When we call this routine for the index i, we first take the volume
     * i and then we average it with i+1. So we look to the right. The
     * result is stored in i. That means, all the indices are kind of
     * shifted by 1/2 to the left. Index (4,3,2) is actually the face
     * located at (4.5, 3, 2).
     */
    QAverageGathered[unknown] = 0.5 * QInGathered[unknown] + 0.5 * QInGatheredRight[unknown];
    QAverageGatheredLeft[unknown] = 0.5 * QInGatheredLeft[unknown] + 0.5 * QInGathered[unknown];
    DeltaQGathered[unknown] = QInGatheredRight[unknown] - QInGathered[unknown];
    DeltaQGatheredLeft[unknown] = QInGathered[unknown] - QInGatheredLeft[unknown];
    {% endif %}
  } // unknown

  // Retrieve user data
  {% if STATELESS_MAX_EIGENVALUE==True %}
  const {{COMPUTE_PRECISION}} maxEigenvalueGathered      = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QInGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, normal);
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredLeft  = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QInGatheredLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal);
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredRight = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QInGatheredRight, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal);
  {% else %}
  const {{COMPUTE_PRECISION}} maxEigenvalueGathered      = solver.maxEigenvalue(QInGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, normal);
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredLeft  = solver.maxEigenvalue(QInGatheredLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal);
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredRight = solver.maxEigenvalue(QInGatheredRight, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal);
  {% endif %}

  {% if FLUX_IMPLEMENTATION!="<none>" %}
  {% if STATELESS_FLUX==True %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::flux(QInGatheredLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, fluxGatheredLeft);
  {{FULL_QUALIFIED_SOLVER_NAME}}::flux(QInGatheredRight, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal, fluxGatheredRight);
  {% else %}
  solver.flux(QInGatheredLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, fluxGatheredLeft);
  solver.flux(QInGatheredRight, volumeCentreRight, volumeSize, timeStamp, timeStepSize, normal, fluxGatheredRight);
  {% endif %}
  {% endif %}

  {% if NCP_IMPLEMENTATION!="<none>" %}
  {% if STATELESS_NCP==True %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(QAverageGathered, DeltaQGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, normal, ncpGathered);
  {{FULL_QUALIFIED_SOLVER_NAME}}::nonconservativeProduct(QAverageGatheredLeft, DeltaQGatheredLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, ncpGatheredLeft);
  {% else %}
  solver.nonconservativeProduct(QAverageGathered, DeltaQGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, normal, ncpGathered);
  solver.nonconservativeProduct(QAverageGatheredLeft, DeltaQGatheredLeft, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, ncpGatheredLeft);
  {% endif %}
  {% endif %}

  #ifdef GPUOffloadingOff
  assertion(std::isfinite(maxEigenvalueGathered));
  assertion(std::isfinite(maxEigenvalueGatheredLeft));
  assertion(std::isfinite(maxEigenvalueGatheredRight));
  #endif

  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) { // Compute fluxes and perform time step
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QInGathered[unknown]));
    assertion(std::isfinite(QInGatheredLeft[unknown]));
    assertion(std::isfinite(QInGatheredRight[unknown]));

    assertion(std::isfinite(fluxGatheredLeft[unknown]));
    assertion(std::isfinite(fluxGatheredRight[unknown]));
    #endif

    const {{COMPUTE_PRECISION}} maxEigenvalueLeft  = maxEigenvalueGatheredLeft   > maxEigenvalueGathered ? maxEigenvalueGatheredLeft  : maxEigenvalueGathered;
    const {{COMPUTE_PRECISION}} maxEigenvalueRight = maxEigenvalueGatheredRight  > maxEigenvalueGathered ? maxEigenvalueGatheredRight : maxEigenvalueGathered;
    {{COMPUTE_PRECISION}} fluxLeft  = -maxEigenvalueLeft  * (QInGathered     [unknown] - QInGatheredLeft[unknown]);
    {{COMPUTE_PRECISION}} fluxRight = -maxEigenvalueRight * (QInGatheredRight[unknown] - QInGathered    [unknown]);
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    fluxLeft += fluxGatheredLeft[unknown];
    fluxRight += fluxGatheredRight[unknown];
    {% endif %}
    {% if NCP_IMPLEMENTATION!="<none>" %}
    fluxLeft -= ncpGatheredLeft[unknown];
    fluxRight += ncpGathered[unknown];
    {% endif %}

    QOutGathered[unknown] += timeStepSize / patchSize(normal) * {{NUMBER_OF_VOLUMES_PER_AXIS}} * 0.5 * (fluxLeft - fluxRight);

    #ifdef GPUOffloadingOff
    assertion(std::isfinite(maxEigenvalueLeft));
    assertion(std::isfinite(maxEigenvalueRight));
    assertion(std::isfinite(fluxLeft));
    assertion(std::isfinite(fluxRight));
    assertion(std::isfinite(QOutGathered[unknown]));
    nonCriticalAssertion(QOutGathered[unknown] == QOutGathered[unknown]);
    #endif
  } // unknown
{% endmacro %}

{% macro computeMaxEigenvalueAfterTimeStep() %}
  // Compute max eigenvalue (wave speed) after time step
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) {
    maxEigenvalueAfterTimeStep = std::fmax(
      maxEigenvalueAfterTimeStep,
      {% if STATELESS_MAX_EIGENVALUE==True %}
      {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(QOutGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, normal)
      {% else %}
      solver.maxEigenvalue(QOutGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, normal)
      {% endif %}
    );
  } // normal
{% endmacro %}
