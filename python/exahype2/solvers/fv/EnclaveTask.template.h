// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/EnclaveTask.h"
#include "exahype2/CellData.h"

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
class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final : public ::exahype2::EnclaveTask<{{STORAGE_PRECISION}}, 3> {
  private:

    /**
     * This is a unique static id which we use for task fusion
     * to distinguish different task types.
     */
    static int _enclaveTaskTypeId;

  public:
    static int getEnclaveTaskTypeId();

    {{CLASSNAME}}(
      const peano4::datamanagement::CellMarker&       marker,
      const double                                    timeStamp,
      const double                                    timeStepSize,
      {{STORAGE_PRECISION}}* NOALIAS                  QIn,
      {{STORAGE_PRECISION}}* NOALIAS const            faces[3][TWO_TIMES_D],
      bool                                            isBoundary[TWO_TIMES_D],
      {{STORAGE_PRECISION}}* NOALIAS                  QOut
    );

    ~{{CLASSNAME}}() override final = default;

    virtual void fuse(const std::list<Task*>& otherTasks) override;
    static void fuse(::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData);
};

{# Empty line here #}
