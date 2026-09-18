// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "peano4/Constants.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

template <typename T>
struct PredictorScratchBuffer{{NAME_SUFFIX}} final {
  T* data{nullptr};

  PredictorScratchBuffer{{NAME_SUFFIX}}() = default;
  PredictorScratchBuffer{{NAME_SUFFIX}}(const PredictorScratchBuffer{{NAME_SUFFIX}}&) = delete;
  PredictorScratchBuffer{{NAME_SUFFIX}}(PredictorScratchBuffer{{NAME_SUFFIX}}&&) = delete;
  PredictorScratchBuffer{{NAME_SUFFIX}}& operator=(const PredictorScratchBuffer{{NAME_SUFFIX}}&) = delete;
  PredictorScratchBuffer{{NAME_SUFFIX}}& operator=(PredictorScratchBuffer{{NAME_SUFFIX}}&&) = delete;
  ~PredictorScratchBuffer{{NAME_SUFFIX}}() {
    tarch::accelerator::freeHeapMemory(data);
  }

  T* get(const std::size_t entries) {
    if (data == nullptr and entries > 0) {
      // Bypass the cache because thread-local scratch can outlive its cache state.
      data = reinterpret_cast<T*>(tarch::accelerator::allocateHeapMemory(entries * sizeof(T)));
    }

    return data;
  }
};

{% if not IS_LINEAR %}
template <typename SolutionStorageType, typename PredictorComputationType>
double picardIteration(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  PredictorComputationType* NOALIAS             lQi,
  PredictorComputationType* NOALIAS             rhs,
  PredictorComputationType* NOALIAS             lFi,
  PredictorComputationType* NOALIAS             lSi,
  PredictorComputationType* NOALIAS             gradQ,
  PredictorComputationType* NOALIAS             newLQiSlice,
  const SolutionStorageType* const NOALIAS      luh,
  const tarch::la::Vector<DIMENSIONS, double>&  cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&  cellSize,
  double                                        timeStamp,
  double                                        timeStepSize
);
{% endif %}

{% if IS_LINEAR %}
template <typename SolutionStorageType, typename PredictorComputationType, typename PredictorStorageType>
{% else %}
template <typename SolutionStorageType, typename PredictorComputationType, typename PicardComputationType, typename PredictorStorageType>
{% endif %}
int fusedSpaceTimePredictorVolumeIntegral{{NAME_SUFFIX}}(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  PredictorStorageType* const NOALIAS           lQhbnd[peano4::FacesPerCell],
  PredictorStorageType* const NOALIAS           lFhbnd[peano4::FacesPerCell],
  SolutionStorageType* const NOALIAS            luh,
  const tarch::la::Vector<DIMENSIONS, double>&  cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&  cellSize,
  double                                        timeStamp,
  double                                        timeStepSize
{% if USE_POINT_SOURCE %}
  , const std::vector<int>&                     pointSources
{% endif %}
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
