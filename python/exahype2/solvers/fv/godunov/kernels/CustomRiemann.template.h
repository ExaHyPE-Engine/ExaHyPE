// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'Index.template.h' as id with context %}{# get template macros #}

{% macro createMemoryOnStack() %}
  {{COMPUTE_PRECISION}} QInGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QInGatheredLeft[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QInGatheredRight[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {{COMPUTE_PRECISION}} QOutGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {{COMPUTE_PRECISION}} leftGoingFluxGathered[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} rightGoingFluxGathered[{{NUMBER_OF_UNKNOWNS}}];

  {{COMPUTE_PRECISION}} leftGoingFluxGatheredLeft[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} rightGoingFluxGatheredLeft[{{NUMBER_OF_UNKNOWNS}}];

  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} sourceGathered[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}

  {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} gradQGathered[{{DIMENSIONS}} * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})];
  {{COMPUTE_PRECISION}} sourceGathered[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}
{% endmacro %}

{% macro gatherQ() %}
  // Gather data
  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
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
  }
  {% if STATELESS_SOURCE_TERM==True %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::sourceTerm(QInGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, sourceGathered);
  {% else %}
  solver.sourceTerm(QInGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, sourceGathered);
  {% endif %}

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
    #if !defined(WITH_GPU)
    assertion(std::isfinite(sourceGathered[unknown]));
    #endif
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QOutGathered[unknown] = QIn[indexWithHalo] + timeStepSize * sourceGathered[unknown];
  } // unknown

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = {{NUMBER_OF_UNKNOWNS}}; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QOutGathered[unknown] = QIn[indexWithHalo];
  } // unknown
  {% endif %}

  {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int normal = 0; normal < {{DIMENSIONS}}; normal++) {
    {% if DIMENSIONS==2 %}
    const auto volumeLeft =
      normal == 0 ? ::exahype2::volumeIndex(volume(0) - {{HALO_SIZE}}, volume(1))
                  : ::exahype2::volumeIndex(volume(0),                 volume(1) - {{HALO_SIZE}});
    const auto volumeRight =
      normal == 0 ? ::exahype2::volumeIndex(volume(0) + {{HALO_SIZE}}, volume(1))
                  : ::exahype2::volumeIndex(volume(0),                 volume(1) + {{HALO_SIZE}});
    {% else %}
    const auto volumeLeft =
      normal == 0 ? ::exahype2::volumeIndex(volume(0) - {{HALO_SIZE}}, volume(1),                 volume(2)) :
      normal == 1 ? ::exahype2::volumeIndex(volume(0),                 volume(1) - {{HALO_SIZE}}, volume(2))
                  : ::exahype2::volumeIndex(volume(0),                 volume(1),                 volume(2) - {{HALO_SIZE}});
    const auto volumeRight =
      normal == 0 ? ::exahype2::volumeIndex(volume(0) + {{HALO_SIZE}}, volume(1),                 volume(2)) :
      normal == 1 ? ::exahype2::volumeIndex(volume(0),                 volume(1) + {{HALO_SIZE}}, volume(2))
                  : ::exahype2::volumeIndex(volume(0),                 volume(1),                 volume(2) + {{HALO_SIZE}});
    {% endif %}
    const auto volumeCentreLeft  = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeLeft);
    const auto volumeCentreRight = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeRight);

    #if defined(WITH_OPENMP)
    #pragma omp simd
    #else
    #pragma unroll
    #endif
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(10) }}{##}
      const int indexWithHaloLeft = {{ id.indexWithHalo('volumeLeft', 'unknown') | indent(6) }}{##}
      const int indexWithHaloRight = {{ id.indexWithHalo('volumeRight', 'unknown') | indent(6) }}{##}
      #if !defined(WITH_GPU)
      assertion(std::isfinite(QIn[indexWithHalo]));
      assertion(std::isfinite(QIn[indexWithHaloLeft]));
      assertion(std::isfinite(QIn[indexWithHaloRight]));
      #endif
      QInGathered[unknown] = QIn[indexWithHalo];
      QInGatheredLeft[unknown] = QIn[indexWithHaloLeft];
      QInGatheredRight[unknown] = QIn[indexWithHaloRight];
      gradQGathered[normal * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}) + unknown] = 0.5 * (QInGatheredRight[unknown] - QInGathered[unknown]) / patchSize(normal) * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    } //unknown
  } // normal

  {% if STATELESS_DIFFUSIVE_SOURCE_TERM==True %}
  {{FULL_QUALIFIED_SOLVER_NAME}}::diffusiveSourceTerm(QInGathered, gradQGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, sourceGathered);
  {% else %}
  solver.diffusiveSourceTerm(QInGathered, gradQGathered, volumeCentre, volumeSize, timeStamp, timeStepSize, sourceGathered);
  {% endif %}

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
    #if !defined(WITH_GPU)
    assertion(std::isfinite(sourceGathered[unknown]));
    #endif
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QOutGathered[unknown] = QIn[indexWithHalo] + timeStepSize * sourceGathered[unknown];
  } // unknown

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = {{NUMBER_OF_UNKNOWNS}}; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QOutGathered[unknown] = QIn[indexWithHalo];
  } // unknown
  {% endif %}

  {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION=="<none>" and SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QInGathered[unknown] = QIn[indexWithHalo];
    QOutGathered[unknown] = QIn[indexWithHalo];
  } // unknown
  {% endif %}
{% endmacro %}

{% macro scatterQ() %}
  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Scatter data into QOut
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QOutGathered[unknown]));
    #endif
    const int index = {{ id.index('volume', 'unknown') | indent(4) }}{##}
    QOut[index] = QOutGathered[unknown];
  } // unknown
{% endmacro %}

{% macro computeFluxesAndPerformTimeStep() %}
  const auto volumeCentreLeft  = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeLeft);
  const auto volumeCentreRight = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volumeRight);

  auto faceCentreLeft(volumeCentre);
  faceCentreLeft(normal) -= 0.5 * volumeSize(normal);
  auto faceCentreRight(volumeCentre);
  faceCentreRight(normal) += 0.5 * volumeSize(normal); 

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Gather neighbor data
    const int indexWithHaloLeft = {{ id.indexWithHalo('volumeLeft', 'unknown') | indent(6) }}{##}
    const int indexWithHaloRight = {{ id.indexWithHalo('volumeRight', 'unknown') | indent(6) }}{##}

    #if !defined(WITH_GPU)
    assertion(std::isfinite(QIn[indexWithHaloLeft]));
    assertion(std::isfinite(QIn[indexWithHaloRight]));
    #endif

    QInGatheredLeft[unknown] = QIn[indexWithHaloLeft];
    QInGatheredRight[unknown] = QIn[indexWithHaloRight];
  } // unknown

  // Retrieve user data
  {% if STATELESS_RIEMANN_SOLVER==True %}
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredLeft  = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QInGatheredLeft, QInGathered, faceCentreLeft, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxGatheredLeft, rightGoingFluxGatheredLeft);
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredRight = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QInGathered, QInGatheredRight, faceCentreRight, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxGathered, rightGoingFluxGathered);
  {% else %}
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredLeft  = solver.solveRiemannProblem(QInGatheredLeft, QInGathered, faceCentreLeft, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxGatheredLeft, rightGoingFluxGatheredLeft);
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredRight = solver.solveRiemannProblem(QInGathered, QInGatheredRight, faceCentreRight, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxGathered, rightGoingFluxGathered);
  {% endif %}

  #if !defined(WITH_GPU)
  assertion(std::isfinite(maxEigenvalueGatheredLeft));
  assertion(std::isfinite(maxEigenvalueGatheredRight));
  #endif

  // Compute max eigenvalue (wave speed) after time step
  maxEigenvalueAfterTimeStep = std::fmax(maxEigenvalueAfterTimeStep, std::fmax(maxEigenvalueGatheredLeft, maxEigenvalueGatheredRight));

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) { // Compute fluxes and perform time step
    #if !defined(WITH_GPU)
    assertion(std::isfinite(QInGathered[unknown]));
    assertion(std::isfinite(QInGatheredLeft[unknown]));
    assertion(std::isfinite(QInGatheredRight[unknown]));

    assertion(std::isfinite(leftGoingFluxGathered[unknown]));
    assertion(std::isfinite(rightGoingFluxGathered[unknown]));

    assertion(std::isfinite(leftGoingFluxGatheredLeft[unknown]));
    assertion(std::isfinite(rightGoingFluxGatheredLeft[unknown]));
    #endif

    const {{COMPUTE_PRECISION}} fluxLeft  = rightGoingFluxGatheredLeft[unknown];
    const {{COMPUTE_PRECISION}} fluxRight = leftGoingFluxGathered[unknown];

    QOutGathered[unknown] += timeStepSize / patchSize(normal) * {{NUMBER_OF_VOLUMES_PER_AXIS}} * (fluxLeft - fluxRight);

    #if !defined(WITH_GPU)
    assertion(std::isfinite(fluxLeft));
    assertion(std::isfinite(fluxRight));
    assertion(std::isfinite(QOutGathered[unknown]));
    nonCriticalAssertion(QOutGathered[unknown] == QOutGathered[unknown]);
    #endif
  } // unknown
{% endmacro %}

{% macro computeMaxEigenvalueAfterTimeStep() %}
{% endmacro %}
