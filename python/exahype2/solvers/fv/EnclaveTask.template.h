// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/EnclaveTask.h"

#include "peano4/datamanagement/CellMarker.h"

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
class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final : public ::exahype2::EnclaveTask<{{STORAGE_PRECISION}}, 1> {
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
      const double                                    timeStamp,
      const double                                    timeStepSize,
      {{STORAGE_PRECISION}}* NOALIAS                  QIn,
      {{STORAGE_PRECISION}}* NOALIAS const            faces[1][TwoTimesD],
      {{STORAGE_PRECISION}}* NOALIAS                  QOut
    );

  public:
    static int getEnclaveTaskTypeId();

    {{CLASSNAME}}(
      const peano4::datamanagement::CellMarker&       marker,
      const double                                    timeStamp,
      const double                                    timeStepSize,
      {{STORAGE_PRECISION}}* NOALIAS                  QIn,
      {{STORAGE_PRECISION}}* NOALIAS const            faces[1][TwoTimesD],
      {{STORAGE_PRECISION}}* NOALIAS                  QOut
    );

    ~{{CLASSNAME}}() override final = default;

    {% if CAN_FUSE %}
    virtual void fuse(const std::list<Task*>& otherTasks) override;
    {% endif %}
};

{# Empty line here #}
