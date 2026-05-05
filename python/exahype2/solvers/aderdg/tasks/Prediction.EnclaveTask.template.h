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

  static int _enclaveTaskTypeId;

public:
  static int getEnclaveTaskTypeId();

  {{CLASSNAME}}(
    const peano4::datamanagement::CellMarker&       marker,
    const double                                    timeStamp,
    const double                                    timeStepSize,
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QIn,
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS const   faces[2][TWO_TIMES_D],
    bool                                            isBoundary[TWO_TIMES_D],
    {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         QOut
  );

  ~{{CLASSNAME}}() override final = default;

  virtual void fuse(const std::list<Task*>& otherTasks) override;
};

{# Empty line here #}
