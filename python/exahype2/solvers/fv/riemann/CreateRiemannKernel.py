# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from .SolverVariant import SolverVariant

def create_Riemann_kernel(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    riemann_solver_implementation,
    source_term_implementation,
    evaluate_max_eigenvalue_of_next_time_step,
    solver_variant: SolverVariant,
):
  if solver_variant == SolverVariant.WithVirtualFunctions:
   template = """timeStepWithRiemann<
      {{SOLVER_NAME}},
      {{HALO_SIZE}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if EIGENVALUES_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if EVALUATE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
      >(repositories::{{SOLVER_INSTANCE}}, patchData);
  """
  elif solver_variant == SolverVariant.Stateless:
   template = """timeStepWithRiemann<
      {{SOLVER_NAME}},
      {{HALO_SIZE}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if EIGENVALUES_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if EVALUATE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
      >(patchData, peano4::utils::LoopPlacement::Serial);
  """
  elif solver_variant == SolverVariant.Multicore:
   template = """timeStepWithRiemann<
      {{SOLVER_NAME}},
      {{HALO_SIZE}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if EIGENVALUES_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if EVALUATE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
      >(patchData, peano4::utils::LoopPlacement::SpreadOut);
  """
  elif solver_variant == SolverVariant.Accelerator:
   template = """timeStepWithRiemann<
      {{SOLVER_NAME}},
      {{HALO_SIZE}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if EIGENVALUES_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if EVALUATE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
      >(patchData, targetDevice);
  """

  result = jinja2.Template(template, undefined=jinja2.DebugUndefined)
  d = {}
  d["FLUX_IMPLEMENTATION"] = flux_implementation
  d["NCP_IMPLEMENTATION"] = ncp_implementation
  d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
  d["RIEMANN_SOLVER_IMPLEMENTATION"] = riemann_solver_implementation
  d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
  d["EVALUATE_MAX_EIGENVALUE"] = evaluate_max_eigenvalue_of_next_time_step
  return result.render(**d)
