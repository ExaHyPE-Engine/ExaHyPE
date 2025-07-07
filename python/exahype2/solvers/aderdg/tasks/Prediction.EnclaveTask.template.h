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

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final : public ::exahype2::EnclaveTask<{{SOLUTION_STORAGE_PRECISION}}, 2> {
private:
  static tarch::logging::Log _log;

  static int _enclaveTaskTypeId;

  static void applyKernelToCell(
    const peano4::datamanagement::CellMarker&       marker,
    const double                                    timeStamp,
    const double                                    timeStepSize,
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QIn,
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS const   faces[2][TwoTimesD],
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QOut
  );

public:
  static int getEnclaveTaskTypeId();

  {{CLASSNAME}}(
    const peano4::datamanagement::CellMarker&       marker,
    const double                                    timeStamp,
    const double                                    timeStepSize,
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QIn,
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS const   faces[2][TwoTimesD],
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QOut
  );

  ~{{CLASSNAME}}() override final = default;

  {% if CAN_FUSE %}
  virtual void fuse(const std::list<Task*>& otherTasks) override;
  {% endif %}
};

{# Empty line here #}
