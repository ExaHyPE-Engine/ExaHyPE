# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.solvers.PDETerms import PDETerms
from ..SubcellScheme import SubcellScheme


class Weighting:
    """Lists the available nonlinear-weighting policy type names."""

    jiangshu = "::exahype2::fv::weno::JiangShu"


def validate_overlap(overlap):
    if isinstance(overlap, bool) or not isinstance(overlap, int) or overlap != 3:
        raise ValueError("WENO requires exactly three halo volumes.")


def subcell_scheme(weighting=Weighting.jiangshu, riemann_solver=None):
    """Returns the WENO scheme used by standalone and limited solvers."""
    return SubcellScheme(
        reconstruction="::exahype2::fv::reconstruction::WENO<" + weighting + ">",
        time_stepping="::exahype2::fv::timestepping::SingleStep",
        halo_width=3,
        includes=[
            "exahype2/fv/FiniteVolume.h",
            "exahype2/fv/reconstruction/WENO.h",
            "exahype2/fv/timestepping/SingleStep.h",
            "exahype2/fv/weno/NonlinearWeights.h",
        ],
        riemann_solver=riemann_solver,
    )


def validate_configuration(solver):
    validate_overlap(solver.overlap)
    if solver._diffusive_source_term_implementation != PDETerms.None_Implementation:
        raise ValueError("WENO does not support diffusive source terms.")
    if solver._limiter_implementation != PDETerms.None_Implementation:
        raise ValueError(
            "WENO selects its stencils through the nonlinear weighting and takes no slope limiter."
        )


def configure_compute_kernel_call(solver, output, weighting):
    """Configures the patch update to call ``exahype2::fv::applyFiniteVolume``."""
    validate_configuration(solver)

    scheme = subcell_scheme(weighting=weighting)

    replacement_dictionary = output.makefile.dictionary()
    solver._init_dictionary_with_default_parameters(replacement_dictionary)
    solver.add_entries_to_text_replacement_dictionary(replacement_dictionary)

    for header in scheme.includes:
        solver._user_action_set_includes += '#include "' + header + '"\n'

    solver._compute_kernel_call = jinja2.Template(
        """
    ::exahype2::fv::applyFiniteVolume<
      {{RECONSTRUCTION}},
      {{TIME_STEPPING}},
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{ "true" if USE_FLUX else "false" }},
      {{ "true" if USE_NCP else "false" }},
      {{ "true" if USE_SOURCE else "false" }},
      {{ "true" if USE_DIFFUSIVE_SOURCE else "false" }},
      {{ "true" if USE_RIEMANN_SOLVER else "false" }},
      {{ "true" if COMPUTE_MAX_EIGENVALUE else "false" }},
      {{ "true" if (COMPUTE_MAX_EIGENVALUE and USE_MAX_EIGENVALUE) else "false" }},
      {{STORAGE_PRECISION}},
      {{COMPUTE_PRECISION}}>(
      QIn, QOut, faceQIn, faceQOut, marker.getX(), marker.getH(), timeStamp, timeStepSize
    );
""",
        undefined=jinja2.DebugUndefined,
    ).render(
        RECONSTRUCTION=scheme.reconstruction,
        TIME_STEPPING=scheme.time_stepping,
        **replacement_dictionary,
    )


from .GlobalFixedTimeStep import GlobalFixedTimeStep
from .GlobalAdaptiveTimeStep import GlobalAdaptiveTimeStep
