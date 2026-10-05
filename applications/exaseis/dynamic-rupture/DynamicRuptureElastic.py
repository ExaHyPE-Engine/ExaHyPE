# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import CurvilinearElastic

RUNTIME_PARAMETERS = "applications::exahype2::exaseis::RuntimeParameters"
FAULT_PLANE = (
    "applications::exahype2::exaseis::FaultPlane{"
    + RUNTIME_PARAMETERS
    + "::faultNormal(), "
    + RUNTIME_PARAMETERS
    + "::faultPosition()}"
)


def add_runtime_parameters(project):
    """Registers the runtime parameters faultNormal and faultPosition that the grid initialisation
    sets."""
    project.add_runtime_parameter("faultNormal", "int", 0)
    project.add_runtime_parameter("faultPosition", "double", 0.0)


def algebraic_source():
    return """
  std::fill_n(S, NumberOfUnknowns, 0);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    S[Shortcuts::u + axis] = Q[Shortcuts::v + axis];
  }
"""


def riemann_solver():
    return (
        """
  const bool isFaultFace = """
        + fault_face_predicate()
        + """;
  return applications::exahype2::exaseis::solveDynamicRuptureRiemannProblem<
    Shortcuts,
    {{COMPUTE_PRECISION}},
    DGOrder + 1,
    NumberOfUnknowns,
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    1>(QL, QR, t, normal, isBoundaryFace, isFaultFace, faceIndex, FL, FR);
"""
    )


def userIncludes():
    return [
        "applications/exaseis/riemannSolvers/DynamicRuptureRiemannSolver.h",
        "exahype2/Curvi.h",
        "exahype2/dg/Basis.h",
        "peano4/sfc/ActiveCurve.h",
        "tarch/reader/NetCDFFieldSampler.h",
    ]


def init_grid_step_implementation(scenario_perturbations):
    """Returns the curvilinear grid initialisation, in which scenario_perturbations writes the
    locals faultNormal and faultPosition. The initialisation copies them into the runtime
    parameters of the same names.
    """
    return CurvilinearElastic.init_grid_step_implementation(
        """
  int    faultNormal   = 0;
  double faultPosition = 0.0;
"""
        + scenario_perturbations
        + """
  """
        + RUNTIME_PARAMETERS
        + """::setFaultNormal(faultNormal);
  """
        + RUNTIME_PARAMETERS
        + """::setFaultPosition(faultPosition);
"""
    )


def fault_normal():
    """Returns the C++ expression of the axis that the fault plane is orthogonal to."""
    return RUNTIME_PARAMETERS + "::faultNormal()"


def fault_position():
    """Returns the C++ expression of the position of the fault plane along its normal."""
    return RUNTIME_PARAMETERS + "::faultPosition()"


def fault_face_predicate():
    """Returns the C++ expression that is true for a face on the fault plane."""
    return FAULT_PLANE + ".containsFace(x, h, normal)"
