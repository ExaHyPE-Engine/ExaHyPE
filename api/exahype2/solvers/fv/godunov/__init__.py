# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from ..SubcellScheme import SubcellScheme


def validate_overlap(overlap):
    if isinstance(overlap, bool) or not isinstance(overlap, int) or overlap != 1:
        raise ValueError("Godunov requires exactly one halo volume.")


def subcell_scheme(riemann_solver=None):
    """Returns the Godunov scheme used by standalone and limited solvers."""
    return SubcellScheme(
        reconstruction="::exahype2::fv::reconstruction::Constant",
        time_stepping="::exahype2::fv::timestepping::SingleStep",
        halo_width=1,
        includes=[
            "exahype2/fv/FiniteVolume.h",
            "exahype2/fv/reconstruction/Constant.h",
            "exahype2/fv/timestepping/SingleStep.h",
        ],
        riemann_solver=riemann_solver,
    )


def configure_compute_kernel_call(solver, output):
    """Configures the patch update to call ``exahype2::fv::applyFiniteVolume``."""
    scheme = subcell_scheme()
    validate_overlap(solver.overlap)

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
