# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import CurvilinearElastic

FAULT_PLANE = "applications::exahype2::exaseis::FaultPlane::get()"


def algebraic_source():
    return """
  std::fill_n(S, NumberOfUnknowns, 0);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    S[Shortcuts::u + axis] = Q[Shortcuts::v + axis];
  }
"""


def riemann_solver():
    return """
  return applications::exahype2::exaseis::solveDynamicRuptureRiemannProblem<
    Shortcuts,
    {{COMPUTE_PRECISION}},
    DGOrder + 1,
    NumberOfUnknowns,
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    1>(QL, QR, x, h, t, normal, isBoundaryFace, faceIndex, FL, FR);
"""


def userIncludes():
    return [
        "applications/exaseis/riemannSolvers/DynamicRuptureRiemannSolver.h",
        "exahype2/Curvi.h",
        "exahype2/dg/Basis.h",
        "peano4/sfc/ActiveCurve.h",
        "tarch/reader/NetCDFFieldSampler.h",
    ]


def init_grid_step_implementation(scenario_perturbations):
    """Returns the grid initialisation of the curvilinear solver, in which the scenario sets the
    local faultNormal and faultPosition, followed by the copy of the fault plane to the devices.
    """
    return CurvilinearElastic.init_grid_step_implementation(
        """
  int    faultNormal   = 0;
  double faultPosition = 0.0;
"""
        + scenario_perturbations
        + """
  applications::exahype2::exaseis::FaultPlane::set(faultNormal, faultPosition);
"""
    )


def fault_normal():
    """Returns the C++ expression of the axis that the fault plane is orthogonal to."""
    return FAULT_PLANE + ".normal"


def fault_position():
    """Returns the C++ expression of the position of the fault plane along its normal."""
    return FAULT_PLANE + ".position"


def fault_face_predicate():
    """Returns the C++ expression that holds for a face on the fault plane."""
    return FAULT_PLANE + ".containsFace(x, h, normal)"
