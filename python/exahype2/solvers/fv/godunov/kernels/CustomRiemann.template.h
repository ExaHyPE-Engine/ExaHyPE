// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'Index.template.h' as id with context %}{# get template macros #}

{% macro createMemoryOnStack() %}
  double QInGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  double QInGatheredLeft[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  double QInGatheredRight[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  double QOutGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  double leftGoingFluxGathered[{{NUMBER_OF_UNKNOWNS}}];
  double rightGoingFluxGathered[{{NUMBER_OF_UNKNOWNS}}];

  double leftGoingFluxGatheredLeft[{{NUMBER_OF_UNKNOWNS}}];
  double rightGoingFluxGatheredLeft[{{NUMBER_OF_UNKNOWNS}}];
{% endmacro %}

{% macro gatherQ() %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) { // Gather data
    const int indexWithHalo = {{ id.indexWithHalo('volume', 'unknown') | indent(4) }}{##}
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(QIn[indexWithHalo]));
    #endif
    QInGathered[unknown] = QIn[indexWithHalo];
    QOutGathered[unknown] = QIn[indexWithHalo];
  } // unknown
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
  } // unknown

  // Retrieve user data
  {% if STATELESS_PDE_TERMS==True %}
  const double maxEigenvalueGatheredLeft  = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QInGatheredLeft, QInGathered, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxGatheredLeft, rightGoingFluxGatheredLeft, ::exahype2::Solver::Offloadable::Yes);
  const double maxEigenvalueGatheredRight = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(QInGathered, QInGatheredRight, volumeCentre, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxGathered, rightGoingFluxGathered, ::exahype2::Solver::Offloadable::Yes);
  {% else %}
  const double maxEigenvalueGatheredLeft  = solver.solveRiemannProblem(QInGatheredLeft, QInGathered, volumeCentreLeft, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxGatheredLeft, rightGoingFluxGatheredLeft);
  const double maxEigenvalueGatheredRight = solver.solveRiemannProblem(QInGathered, QInGatheredRight, volumeCentre, volumeSize, timeStamp, timeStepSize, normal, leftGoingFluxGathered, rightGoingFluxGathered);
  {% endif %}

  #ifdef GPUOffloadingOff
  assertion(std::isfinite(maxEigenvalueGatheredLeft));
  assertion(std::isfinite(maxEigenvalueGatheredRight));
  #endif

  // Compute max eigenvalue (wave speed) after time step
  maxEigenvalueAfterTimeStep = std::fmax(maxEigenvalueAfterTimeStep, std::fmax(maxEigenvalueGatheredLeft, maxEigenvalueGatheredRight));

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

    assertion(std::isfinite(leftGoingFluxGathered[unknown]));
    assertion(std::isfinite(rightGoingFluxGathered[unknown]));

    assertion(std::isfinite(leftGoingFluxGatheredLeft[unknown]));
    assertion(std::isfinite(rightGoingFluxGatheredLeft[unknown]));
    #endif

    const double fluxLeft  = rightGoingFluxGatheredLeft[unknown];
    const double fluxRight = leftGoingFluxGathered[unknown];

    QOutGathered[unknown] += timeStepSize / patchSize(normal) * {{NUMBER_OF_VOLUMES_PER_AXIS}} * (fluxLeft - fluxRight);

    #ifdef GPUOffloadingOff
    assertion(std::isfinite(fluxLeft));
    assertion(std::isfinite(fluxRight));
    assertion(std::isfinite(QOutGathered[unknown]));
    nonCriticalAssertion(QOutGathered[unknown] == QOutGathered[unknown]);
    #endif
  } // unknown
{% endmacro %}

{% macro computeMaxEigenvalueAfterTimeStep() %}
{% endmacro %}
