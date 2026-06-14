// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'ApplyTerms.macro.h' as terms with context %}{# get template macros #}
{% import 'MUSCLHancock.macro.h' as muscl with context %}{# get template macros #}

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

  {{ muscl.createMemoryOnStackMUSCL() }}
{% endmacro %}

{% macro computeFluxesAndPerformTimeStep() %}
  {{ muscl.gatherSlopes() }}
  {{ muscl.boundaryExtrapolation() }}
  {{ muscl.boundaryEvolutionHalfStep() }}

  auto faceCentreLeft(volumeCentre);
  faceCentreLeft(normal) -= 0.5 * volumeSize(normal);
  auto faceCentreRight(volumeCentre);
  faceCentreRight(normal) += 0.5 * volumeSize(normal);

  // Retrieve user data
  {% if STATELESS_RIEMANN_SOLVER==True %}
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredLeftInterface  = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(extrapolatedBoundaryLeftNeighborRightBoundary, extrapolatedBoundaryLeft, faceCentreLeft, volumeSize, timeStamp + 0.5 * timeStepSize, timeStepSize, normal, leftGoingFluxGatheredLeft, rightGoingFluxGatheredLeft);
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredRightInterface = {{FULL_QUALIFIED_SOLVER_NAME}}::solveRiemannProblem(extrapolatedBoundaryRight, extrapolatedBoundaryRightNeighborLeftBoundary, faceCentreRight, volumeSize, timeStamp + 0.5 * timeStepSize, timeStepSize, normal, leftGoingFluxGathered, rightGoingFluxGathered);
  {% else %}
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredLeftInterface  = solver.solveRiemannProblem(extrapolatedBoundaryLeftNeighborRightBoundary, extrapolatedBoundaryLeft, faceCentreLeft, volumeSize, timeStamp + 0.5 * timeStepSize, timeStepSize, normal, leftGoingFluxGatheredLeft, rightGoingFluxGatheredLeft);
  const {{COMPUTE_PRECISION}} maxEigenvalueGatheredRightInterface = solver.solveRiemannProblem(extrapolatedBoundaryRight, extrapolatedBoundaryRightNeighborLeftBoundary, faceCentreRight, volumeSize, timeStamp + 0.5 * timeStepSize, timeStepSize, normal, leftGoingFluxGathered, rightGoingFluxGathered);
  {% endif %}

  assertion(std::isfinite(maxEigenvalueGatheredLeftInterface));
  assertion(std::isfinite(maxEigenvalueGatheredRightInterface));

  // Compute max eigenvalue (wave speed) after time step
  maxEigenvalueAfterTimeStep = std::fmax(maxEigenvalueAfterTimeStep, std::fmax(maxEigenvalueGatheredLeftInterface, maxEigenvalueGatheredRightInterface));

  #if defined(WITH_OPENMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) { // Compute fluxes and perform time step
    assertion(std::isfinite(QInGathered[unknown]));
    assertion(std::isfinite(QInGatheredLeft[unknown]));
    assertion(std::isfinite(QInGatheredRight[unknown]));

    assertion(std::isfinite(leftGoingFluxGathered[unknown]));
    assertion(std::isfinite(rightGoingFluxGathered[unknown]));

    assertion(std::isfinite(leftGoingFluxGatheredLeft[unknown]));
    assertion(std::isfinite(rightGoingFluxGatheredLeft[unknown]));

    const {{COMPUTE_PRECISION}} fluxLeft  = rightGoingFluxGatheredLeft[unknown];
    const {{COMPUTE_PRECISION}} fluxRight = leftGoingFluxGathered[unknown];

    QOutGathered[unknown] += timeStepSize / patchSize(normal) * {{NUMBER_OF_VOLUMES_PER_AXIS}} * (fluxLeft - fluxRight);

    assertion(std::isfinite(fluxLeft));
    assertion(std::isfinite(fluxRight));
    assertion(std::isfinite(QOutGathered[unknown]));
  } // unknown
{% endmacro %}

{% macro computeMaxEigenvalueAfterTimeStep() %}
{% endmacro %}
