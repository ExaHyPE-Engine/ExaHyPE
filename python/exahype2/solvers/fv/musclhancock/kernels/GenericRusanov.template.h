// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'ApplyTerms.macro.h' as terms with context %}{# get template macros #}
{% import 'MUSCLHancock.macro.h' as muscl with context %}{# get template macros #}

{% macro createMemoryOnStack() %}
  {{COMPUTE_PRECISION}} QInGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QInGatheredLeft[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QInGatheredRight[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {{COMPUTE_PRECISION}} QOutGathered[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} sourceGathered[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}

  {% if NCP_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} QAverageGatheredRightInterface[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QAverageGatheredLeftInterface[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} DeltaQGatheredRightInterface[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} DeltaQGatheredLeftInterface[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} ncpGatheredRightInterface[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} ncpGatheredLeftInterface[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}

  {{ muscl.createMemoryOnStackMUSCL() }}
{% endmacro %}

{% macro computeFluxesAndPerformTimeStep() %}
  {{ muscl.gatherSlopes() }}
  {{ muscl.boundaryExtrapolation() }}
  {{ muscl.boundaryEvolutionHalfStep() }}

  {{ terms.getMaxEigenvalue('extrapolatedBoundaryLeft', 'maxEigenvalueGatheredLeftBoundary', 'volumeCentre', 'timeStamp + 0.5 * timeStepSize') }}
  {{ terms.getMaxEigenvalue('extrapolatedBoundaryRight', 'maxEigenvalueGatheredRightBoundary', 'volumeCentre', 'timeStamp + 0.5 * timeStepSize') }}
  {{ terms.getMaxEigenvalue('extrapolatedBoundaryLeftNeighborRightBoundary', 'maxEigenvalueGatheredLeftNeighborRightBoundary', 'volumeCentreLeft', 'timeStamp + 0.5 * timeStepSize') }}
  {{ terms.getMaxEigenvalue('extrapolatedBoundaryRightNeighborLeftBoundary', 'maxEigenvalueGatheredRightNeighborLeftBoundary', 'volumeCentreRight', 'timeStamp + 0.5 * timeStepSize') }}

  {% if FLUX_IMPLEMENTATION!="<none>" %}
  {{ terms.applyFlux('extrapolatedBoundaryLeft', 'boundaryFluxLeftBoundary', 'volumeCentre', 'timeStamp + 0.5 * timeStepSize') }}
  {{ terms.applyFlux('extrapolatedBoundaryRight', 'boundaryFluxRightBoundary', 'volumeCentre', 'timeStamp + 0.5 * timeStepSize') }}
  {{ terms.applyFlux('extrapolatedBoundaryLeftNeighborRightBoundary', 'boundaryFluxLeftNeighborRightBoundary', 'volumeCentreLeft', 'timeStamp + 0.5 * timeStepSize') }}
  {{ terms.applyFlux('extrapolatedBoundaryRightNeighborLeftBoundary', 'boundaryFluxRightNeighborLeftBoundary', 'volumeCentreRight', 'timeStamp + 0.5 * timeStepSize') }}
  {% endif %}

  {% if NCP_IMPLEMENTATION!="<none>" %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    QAverageGatheredRightInterface[unknown] = 0.5 * extrapolatedBoundaryRight[unknown] + 0.5 * extrapolatedBoundaryRightNeighborLeftBoundary[unknown];
    QAverageGatheredLeftInterface[unknown] = 0.5 * extrapolatedBoundaryLeftNeighborRightBoundary[unknown] + 0.5 * extrapolatedBoundaryLeft[unknown];
    DeltaQGatheredRightInterface[unknown] = extrapolatedBoundaryRightNeighborLeftBoundary[unknown] - extrapolatedBoundaryRight[unknown];
    DeltaQGatheredLeftInterface[unknown] = extrapolatedBoundaryLeft[unknown] - extrapolatedBoundaryLeftNeighborRightBoundary[unknown];
  } // unknown

  {{ terms.applyNCP('QAverageGatheredRightInterface', 'DeltaQGatheredRightInterface', 'ncpGatheredRightInterface', 'volumeCentre', 'timeStamp') }}
  {{ terms.applyNCP('QAverageGatheredLeftInterface', 'DeltaQGatheredLeftInterface', 'ncpGatheredLeftInterface', 'volumeCentreLeft', 'timeStamp') }}
  {% endif %}

  #ifdef GPUOffloadingOff
  assertion(std::isfinite(maxEigenvalueGatheredLeftBoundary));
  assertion(std::isfinite(maxEigenvalueGatheredRightBoundary));
  assertion(std::isfinite(maxEigenvalueGatheredRightNeighborLeftBoundary));
  assertion(std::isfinite(maxEigenvalueGatheredLeftNeighborRightBoundary));
  #endif

  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) { // Compute fluxes and perform time step
    #ifdef GPUOffloadingOff
    assertion(std::isfinite(extrapolatedBoundaryLeft[unknown]));
    assertion(std::isfinite(extrapolatedBoundaryRight[unknown]));
    assertion(std::isfinite(extrapolatedBoundaryLeftNeighborRightBoundary[unknown]));
    assertion(std::isfinite(extrapolatedBoundaryRightNeighborLeftBoundary[unknown]));

    {% if FLUX_IMPLEMENTATION!="<none>" %}
    assertion(std::isfinite(boundaryFluxLeftBoundary[unknown]));
    assertion(std::isfinite(boundaryFluxRightBoundary[unknown]));
    assertion(std::isfinite(boundaryFluxLeftNeighborRightBoundary[unknown]));
    assertion(std::isfinite(boundaryFluxRightNeighborLeftBoundary[unknown]));
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" %}
    assertion(std::isfinite(ncpGatheredRightInterface[unknown]));
    assertion(std::isfinite(ncpGatheredLeftInterface[unknown]));
    {% endif %}
    #endif

    const {{COMPUTE_PRECISION}} maxEigenvalueLeftInterface  = maxEigenvalueGatheredLeftBoundary   > maxEigenvalueGatheredLeftNeighborRightBoundary ? maxEigenvalueGatheredLeftBoundary  : maxEigenvalueGatheredLeftNeighborRightBoundary;
    const {{COMPUTE_PRECISION}} maxEigenvalueRightInterface = maxEigenvalueGatheredRightBoundary  > maxEigenvalueGatheredRightNeighborLeftBoundary ? maxEigenvalueGatheredRightBoundary : maxEigenvalueGatheredRightNeighborLeftBoundary;
    {{COMPUTE_PRECISION}} fluxLeft  = -maxEigenvalueLeftInterface  * (extrapolatedBoundaryLeft[unknown] - extrapolatedBoundaryLeftNeighborRightBoundary[unknown]);
    {{COMPUTE_PRECISION}} fluxRight = -maxEigenvalueRightInterface * (extrapolatedBoundaryRightNeighborLeftBoundary[unknown] - extrapolatedBoundaryRight[unknown]);
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    fluxLeft += boundaryFluxLeftBoundary[unknown] + boundaryFluxLeftNeighborRightBoundary[unknown];
    fluxRight += boundaryFluxRightBoundary[unknown] + boundaryFluxRightNeighborLeftBoundary[unknown];
    {% endif %}
    {% if NCP_IMPLEMENTATION!="<none>" %}
    fluxLeft -= ncpGatheredLeftInterface[unknown];
    fluxRight += ncpGatheredRightInterface[unknown];
    {% endif %}

    QOutGathered[unknown] += timeStepSize / patchSize(normal) * {{NUMBER_OF_VOLUMES_PER_AXIS}} * 0.5 * (fluxLeft - fluxRight);

    #ifdef GPUOffloadingOff
    assertion(std::isfinite(maxEigenvalueLeftInterface));
    assertion(std::isfinite(maxEigenvalueRightInterface));
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
