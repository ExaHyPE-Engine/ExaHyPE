# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.solvers.PDETerms import PDETerms
from ..SubcellScheme import SubcellScheme


class Limiter:
    """Lists the available slope-limiter policy type names."""

    superbee = "::exahype2::fv::limiter::Superbee"
    minmod = "::exahype2::fv::limiter::Minmod"
    zero = "::exahype2::fv::limiter::Zero"
    average = "::exahype2::fv::limiter::Average"
    vanalbada = "::exahype2::fv::limiter::VanAlbada"
    mclim = "::exahype2::fv::limiter::MinmodCentral"


def validate_overlap(overlap):
    if isinstance(overlap, bool) or not isinstance(overlap, int) or overlap != 2:
        raise ValueError("MUSCL-Hancock requires exactly two halo volumes.")


def subcell_scheme(slope_limiter=Limiter.minmod, riemann_solver=None):
    """Returns the MUSCL-Hancock scheme used by standalone and limited solvers."""
    return SubcellScheme(
        reconstruction="::exahype2::fv::reconstruction::LimitedLinear<"
        + slope_limiter
        + ">",
        time_stepping="::exahype2::fv::timestepping::SingleStep",
        halo_width=2,
        fallback_reconstruction=(
            "::exahype2::fv::reconstruction::LimitedLinear<"
            "::exahype2::fv::limiter::Zero>"
        ),
        includes=[
            "exahype2/fv/FiniteVolume.h",
            "exahype2/fv/limiter/SlopeLimiter.h",
            "exahype2/fv/reconstruction/LimitedLinear.h",
            "exahype2/fv/timestepping/SingleStep.h",
        ],
        riemann_solver=riemann_solver,
    )


def validate_configuration(solver):
    validate_overlap(solver.overlap)
    if solver._diffusive_source_term_implementation != PDETerms.None_Implementation:
        raise ValueError("MUSCL-Hancock does not support diffusive source terms.")
    if solver._limiter_implementation == PDETerms.None_Implementation:
        raise ValueError("MUSCL-Hancock requires a slope limiter.")
    if solver._limiter_implementation == PDETerms.User_Defined_Implementation:
        raise ValueError(
            "The limiter must be a slope-limiter policy type name or a C++ "
            "snippet computing the limited difference, not the user-defined "
            "marker."
        )


# The kernel picks up a solver's own limitedDifference and reconstruct methods
# through a C++ requires clause, so a scenario limits and extrapolates a derived
# quantity such as a hydrostatic pressure without a dedicated policy type.
_LIMITED_DIFFERENCE_SIGNATURE = """
    static INLINE_METHOD GPU_CALLABLE_METHOD {{COMPUTE_PRECISION}} limitedDifference(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QL,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QR,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const int                                    unknown
    ) {
"""

_RECONSTRUCT_SIGNATURE = """
    static INLINE_METHOD GPU_CALLABLE_METHOD void reconstruct(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QL,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QR,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   limitedDifferences,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const int                                    unknown,
      [[maybe_unused]] {{COMPUTE_PRECISION}}&                       reconstructedMinus,
      [[maybe_unused]] {{COMPUTE_PRECISION}}&                       reconstructedPlus
    ) {
"""


def configure_compute_kernel_call(solver, output, reconstruction):
    """Configures the patch update to call ``exahype2::fv::applyFiniteVolume``."""
    validate_configuration(solver)

    limiter = solver._limiter_implementation
    if limiter.startswith("::"):
        limiter_policy = limiter
    else:
        limiter_policy = "::exahype2::fv::limiter::Minmod"
        solver._solver_user_declarations += (
            _LIMITED_DIFFERENCE_SIGNATURE + limiter + "\n    }\n"
        )

    if reconstruction is not None:
        solver._solver_user_declarations += (
            _RECONSTRUCT_SIGNATURE + reconstruction + "\n    }\n"
        )

    scheme = subcell_scheme(slope_limiter=limiter_policy)

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
