// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "repositories/SolverRepository.h"

#include "tarch/multicore/Task.h"

#include "peano4/datamanagement/CellMarker.h"

#include "exahype2/EnclaveTask.h"

{{SOLVER_INCLUDES}}

#include <vector>
#include <memory>

{% for item in NAMESPACE -%}
  namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

/**
 * Single task that can also take multiple tasks and deploy them to the GPU
 *
 * @author Tobias Weinzierl
 */
class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final : public ::exahype2::EnclaveTask<{{STORAGE_PRECISION}}> {
  private:
    static tarch::logging::Log _log;

    /**
     * This is a unique static id which we use for task fusion
     * to distinguish different task types.
     */
    static int _enclaveTaskTypeId;

    /**
     * Simple call to compute kernels.
     */
     static void applyKernelToPatch(
      const peano4::datamanagement::CellMarker&       marker,
      double                                          timeStamp,
      double                                          timeStepSize,
      /*const*/ {{STORAGE_PRECISION}}* NOALIAS const oldQWithHalo,
      {{STORAGE_PRECISION}}* NOALIAS const       newQ
    );

  public:
    static int getEnclaveTaskTypeId();

    {{CLASSNAME}}(
      const peano4::datamanagement::CellMarker&       marker,
      double                                          timeStamp,
      double                                          timeStepSize,
      /*const*/ {{STORAGE_PRECISION}}* NOALIAS const oldQWithHalo,
      {{STORAGE_PRECISION}}* NOALIAS const       newQ
    );

    ~{{CLASSNAME}}() override final = default;

    virtual bool canFuse() const override;

    {% if CAN_FUSE %}
    virtual bool fuse(const std::list<Task*>& otherTasks) override;
    {% endif %}
};

{# Empty line here #}
