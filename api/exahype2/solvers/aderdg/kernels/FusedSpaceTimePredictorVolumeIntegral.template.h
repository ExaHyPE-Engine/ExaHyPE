// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

// TODO: This should be centralised somewhere (DataRepository) and count to the memory usage
template <typename T>
struct PredictorScratchBuffer{{NAME_SUFFIX}} {
  T* data = nullptr;

  PredictorScratchBuffer{{NAME_SUFFIX}}() = default;
  PredictorScratchBuffer{{NAME_SUFFIX}}(const PredictorScratchBuffer{{NAME_SUFFIX}}&) = delete;
  PredictorScratchBuffer{{NAME_SUFFIX}}& operator=(const PredictorScratchBuffer{{NAME_SUFFIX}}&) = delete;
  PredictorScratchBuffer{{NAME_SUFFIX}}(PredictorScratchBuffer{{NAME_SUFFIX}}&&) = delete;
  PredictorScratchBuffer{{NAME_SUFFIX}}& operator=(PredictorScratchBuffer{{NAME_SUFFIX}}&&) = delete;

  ~PredictorScratchBuffer{{NAME_SUFFIX}}() {
    tarch::accelerator::freeHeapMemory(data);
  }

  T* get(const std::size_t entries) {
    if (data == nullptr and entries > 0) {
      // This buffer is thread-local. Bypass the heap allocation cache here, as
      // thread-local scratch teardown can happen after the cache state for the
      // same thread has already been destroyed.
      data = reinterpret_cast<T*>(tarch::accelerator::allocateHeapMemory(entries * sizeof(T)));
    }

    return data;
  }
};

{% if IS_LINEAR %}
template <typename cStoreType, typename pCompType, typename pStoreType>
{% else %}
template <typename cStoreType, typename pCompType, typename picardCompType, typename pStoreType>
{% endif %}
int fusedSpaceTimePredictorVolumeIntegral{{NAME_SUFFIX}}(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  pStoreType* const NOALIAS                     lQhbnd[TWO_TIMES_D],
  pStoreType* const NOALIAS                     lFhbnd[TWO_TIMES_D],
  cStoreType* const NOALIAS                     luh,
  const tarch::la::Vector<DIMENSIONS, double>&  cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize
{% if USE_POINT_SOURCE %}
  , const std::vector<int>&                     pointSources
{% endif %}
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
