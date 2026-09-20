# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from .codegen.helper import KeepUndefined
from .codegen.helper import render_jinja2_template

TEMPLATE_ACCUMULATE_PER_CELL = """
  ::exahype2::accumulateSolutionError<
    {{FULL_QUALIFIED_SOLVER_NAME}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}}
  >(
    MeasuredUnknowns,
    value,
    x,
    timeStamp,
    timeStepSize,
    weight,
    errors
  );
"""

TEMPLATE_ACCUMULATE_PER_DOF = """
  ::exahype2::accumulateAndRecordSolutionError<
    {{FULL_QUALIFIED_SOLVER_NAME}},
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}}
  >(
    MeasuredUnknowns,
    value,
    x,
    timeStamp,
    timeStepSize,
    weight,
    repositories::{{SOLVER_INSTANCE}}.errorMeasurement
  );
"""

_TEMPLATE_KERNEL = """
  if ({{ERROR_CONDITION}}) {
    [[maybe_unused]] const double timeStamp          = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    [[maybe_unused]] const double timeStepSize       = repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
    constexpr int                 MeasuredUnknowns[] = {{MEASURED_UNKNOWNS}};
{%- if ADDS_PER_CELL %}
    double errors[3] = {0.0, 0.0, 0.0};
{%- endif %}
    {{ERROR_LOOPS | indent(4)}}
{%- if ADDS_PER_CELL %}
    repositories::{{SOLVER_INSTANCE}}.errorMeasurement.add(errors);
{%- endif %}
  }
"""


def kernel(condition, loops, adds_per_cell=True):
    return render_jinja2_template(
        _TEMPLATE_KERNEL,
        {
            "ERROR_CONDITION": condition,
            "ERROR_LOOPS": textwrap.dedent(loops).strip(),
            "ADDS_PER_CELL": adds_per_cell,
        },
        KeepUndefined,
    )
