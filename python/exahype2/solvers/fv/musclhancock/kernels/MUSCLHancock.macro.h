// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'ApplyTerms.macro.h' as terms with context %}{# get template macros #}

{% macro addSourceAndNCP() %}
  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" or NCP_IMPLEMENTATION!="<none>" %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) { // extrapolate QInGathered in time
    QInGathered[unknown] += 0.5 * timeStepSize * QInExtrapolationUpdate[unknown];
  }
  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  // apply source term to extrapolated QIn and add to evolved values in QOut
  {{ terms.applySource('QInGathered', 'sourceGathered', 'volumeCentre', 'timeStamp + 0.5 * timeStepSize') }}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
    QOutGathered[unknown] += timeStepSize * sourceGathered[unknown];
  }
  {% endif %} {# source term implementation #}
  {% if NCP_IMPLEMENTATION!="<none>" %}
  for (int normal = 0; normal < {{DIMENSIONS}}; normal ++) {
    // apply ncp to extrapolated QIn and subtract from evolved values in QOut
    {{ terms.applyNCP('QInGathered', 'slopeGathered[normal]', 'cellCenteredNCPGathered[normal]', 'volumeCentre', 'timeStamp + 0.5 * timeStepSize') }}
    #if defined(SharedOMP)
    #pragma omp simd
    #else
    #pragma unroll
    #endif
    for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
      QOutGathered[unknown] -= timeStepSize * cellCenteredNCPGathered[normal][unknown];
    }
  }
  {% endif %} {# ncp implementation #}
  {% endif %} {# source term or ncp implementation #}
{% endmacro %}

{% macro gatherSlopes() %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    {{ terms.applyLimiter('QInGatheredRight[unknown] - QInGathered[unknown]', 'QInGathered[unknown] - QInGatheredLeft[unknown]', 'slopeGathered[normal][unknown]') }}
    {{ terms.applyLimiter('QInGathered[unknown] - QInGatheredLeft[unknown]', 'QInGatheredLeft[unknown] - QInGatheredLeftLeft[unknown]', 'slopeGatheredLeft[normal][unknown]') }}
    {{ terms.applyLimiter('QInGatheredRightRight[unknown] - QInGatheredRight[unknown]', 'QInGatheredRight[unknown] - QInGathered[unknown]', 'slopeGatheredRight[normal][unknown]') }}

    #ifdef GPUOffloadingOff
    assertion(std::isfinite(slopeGathered[normal][unknown]));
    assertion(std::isfinite(slopeGatheredLeft[normal][unknown]));
    assertion(std::isfinite(slopeGatheredRight[normal][unknown]));
    #endif
  } // unknown
{% endmacro %}

{% macro boundaryExtrapolation() %}
  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
    extrapolatedBoundaryLeft[unknown] = QInGathered[unknown] - 0.5 * volumeSize(normal) * slopeGathered[normal][unknown];
    extrapolatedBoundaryRight[unknown] = QInGathered[unknown] + 0.5 * volumeSize(normal) * slopeGathered[normal][unknown];
    extrapolatedBoundaryLeftNeighborLeftBoundary[unknown] = QInGatheredLeft[unknown] - 0.5 * volumeSize(normal) * slopeGatheredLeft[normal][unknown];
    extrapolatedBoundaryLeftNeighborRightBoundary[unknown] = QInGatheredLeft[unknown] + 0.5 * volumeSize(normal) * slopeGatheredLeft[normal][unknown];
    extrapolatedBoundaryRightNeighborLeftBoundary[unknown] = QInGatheredRight[unknown] - 0.5 * volumeSize(normal) * slopeGatheredRight[normal][unknown];
    extrapolatedBoundaryRightNeighborRightBoundary[unknown] = QInGatheredRight[unknown] + 0.5 * volumeSize(normal) * slopeGatheredRight[normal][unknown];
  } // unknown
{% endmacro %}

{% macro boundaryEvolutionHalfStep() %}
  {% if FLUX_IMPLEMENTATION!="<none>" %}
  {{ terms.applyFlux('extrapolatedBoundaryLeft', 'boundaryFluxLeftBoundary', 'volumeCentre', 'timeStamp') }}
  {{ terms.applyFlux('extrapolatedBoundaryRight', 'boundaryFluxRightBoundary', 'volumeCentre', 'timeStamp') }}
  {{ terms.applyFlux('extrapolatedBoundaryLeftNeighborLeftBoundary', 'boundaryFluxLeftNeighborLeftBoundary', 'volumeCentreLeft', 'timeStamp') }}
  {{ terms.applyFlux('extrapolatedBoundaryLeftNeighborRightBoundary', 'boundaryFluxLeftNeighborRightBoundary', 'volumeCentreLeft', 'timeStamp') }}
  {{ terms.applyFlux('extrapolatedBoundaryRightNeighborLeftBoundary', 'boundaryFluxRightNeighborLeftBoundary', 'volumeCentreRight', 'timeStamp') }}
  {{ terms.applyFlux('extrapolatedBoundaryRightNeighborRightBoundary', 'boundaryFluxRightNeighborRightBoundary', 'volumeCentreRight', 'timeStamp') }}
  {% endif %}

  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  {{ terms.applySource('QInGatheredLeft', 'sourceGatheredLeft', 'volumeCentreLeft', 'timeStamp') }}
  {{ terms.applySource('QInGatheredRight', 'sourceGatheredRight', 'volumeCentreRight', 'timeStamp') }}
  {% endif %}

  {% if NCP_IMPLEMENTATION!="<none>" %}
  {{ terms.applyNCP('QInGathered', 'slopeGathered[normal]', 'cellCenteredNCPGathered[normal]', 'volumeCentre', 'timeStamp') }}
  {{ terms.applyNCP('QInGatheredLeft', 'slopeGatheredLeft[normal]', 'cellCenteredNCPGatheredLeft', 'volumeCentreLeft', 'timeStamp') }}
  {{ terms.applyNCP('QInGatheredRight', 'slopeGatheredRight[normal]', 'cellCenteredNCPGatheredRight', 'volumeCentreRight', 'timeStamp') }}
  {% endif %}

  #if defined(SharedOMP)
  #pragma omp simd
  #else
  #pragma unroll
  #endif
  for (int unknown = 0; unknown < {{NUMBER_OF_UNKNOWNS}}; unknown++) {
    {{COMPUTE_PRECISION}} updateLeftBoundary = 0.0;
    {{COMPUTE_PRECISION}} updateRightBoundary = 0.0;
    {{COMPUTE_PRECISION}} updateLeftNeighborRightBoundary = 0.0;
    {{COMPUTE_PRECISION}} updateRightNeighborLeftBoundary = 0.0;

    {% if FLUX_IMPLEMENTATION!="<none>" %}
    const {{COMPUTE_PRECISION}} volumeFluxDifference = 1 / volumeSize(normal) * (boundaryFluxLeftBoundary[unknown] - boundaryFluxRightBoundary[unknown]);
    QInExtrapolationUpdate[unknown] += volumeFluxDifference;
    updateLeftBoundary += volumeFluxDifference;
    updateRightBoundary += volumeFluxDifference;
    updateLeftNeighborRightBoundary += 1 / volumeSize(normal) * (boundaryFluxLeftNeighborLeftBoundary[unknown] - boundaryFluxLeftNeighborRightBoundary[unknown]);
    updateRightNeighborLeftBoundary += 1 / volumeSize(normal) * (boundaryFluxRightNeighborLeftBoundary[unknown] - boundaryFluxRightNeighborRightBoundary[unknown]);
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    updateLeftBoundary += sourceGathered[unknown];
    updateRightBoundary += sourceGathered[unknown];
    updateLeftNeighborRightBoundary += sourceGatheredLeft[unknown];
    updateRightNeighborLeftBoundary += sourceGatheredRight[unknown];
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" %}
    QInExtrapolationUpdate[unknown] -= cellCenteredNCPGathered[normal][unknown];
    updateLeftBoundary -= cellCenteredNCPGathered[normal][unknown];
    updateRightBoundary -= cellCenteredNCPGathered[normal][unknown];
    updateLeftNeighborRightBoundary -= cellCenteredNCPGatheredLeft[unknown];
    updateRightNeighborLeftBoundary -= cellCenteredNCPGatheredRight[unknown];
    {% endif %}

    // Evolve only directly adjacent boundaries
    extrapolatedBoundaryLeft[unknown] += 0.5 * timeStepSize * updateLeftBoundary;
    extrapolatedBoundaryRight[unknown] += 0.5 * timeStepSize * updateRightBoundary;
    extrapolatedBoundaryLeftNeighborRightBoundary[unknown] += 0.5 * timeStepSize * updateLeftNeighborRightBoundary;
    extrapolatedBoundaryRightNeighborLeftBoundary[unknown] += 0.5 * timeStepSize * updateRightNeighborLeftBoundary;

    #ifdef GPUOffloadingOff
    assertion(std::isfinite(extrapolatedBoundaryLeft[unknown]));
    assertion(std::isfinite(extrapolatedBoundaryRight[unknown]));
    assertion(std::isfinite(extrapolatedBoundaryLeftNeighborRightBoundary[unknown]));
    assertion(std::isfinite(extrapolatedBoundaryRightNeighborLeftBoundary[unknown]));
    #endif
  } // unknown
{% endmacro %}

{% macro createMemoryOnStackMUSCL() %}
  {{COMPUTE_PRECISION}} QInGatheredLeftLeft[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} QInGatheredRightRight[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {{COMPUTE_PRECISION}} slopeGathered[{{DIMENSIONS}}][{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} slopeGatheredLeft[{{DIMENSIONS}}][{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} slopeGatheredRight[{{DIMENSIONS}}][{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {{COMPUTE_PRECISION}} QInExtrapolationUpdate[{{NUMBER_OF_UNKNOWNS}}];

  {{COMPUTE_PRECISION}} extrapolatedBoundaryLeft[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} extrapolatedBoundaryRight[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} extrapolatedBoundaryLeftNeighborLeftBoundary[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} extrapolatedBoundaryLeftNeighborRightBoundary[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} extrapolatedBoundaryRightNeighborLeftBoundary[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];
  {{COMPUTE_PRECISION}} extrapolatedBoundaryRightNeighborRightBoundary[{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}];

  {% if FLUX_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} boundaryFluxLeftBoundary[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} boundaryFluxRightBoundary[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} boundaryFluxLeftNeighborLeftBoundary[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} boundaryFluxLeftNeighborRightBoundary[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} boundaryFluxRightNeighborLeftBoundary[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} boundaryFluxRightNeighborRightBoundary[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}

  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  {{COMPUTE_PRECISION}} sourceGatheredLeft[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} sourceGatheredRight[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}

  {% if NCP_IMPLEMENTATION!="<none>" %}
  static_assert(false, "NCP is currently not supported with MUSCL-Hancock");
  {{COMPUTE_PRECISION}} cellCenteredNCPGathered[{{DIMENSIONS}}][{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} cellCenteredNCPGatheredLeft[{{NUMBER_OF_UNKNOWNS}}];
  {{COMPUTE_PRECISION}} cellCenteredNCPGatheredRight[{{NUMBER_OF_UNKNOWNS}}];
  {% endif %}
{% endmacro %}
