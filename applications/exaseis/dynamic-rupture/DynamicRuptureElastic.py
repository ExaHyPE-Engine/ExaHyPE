# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
def algebraic_source():
    return """
  std::fill_n(S, NumberOfUnknowns, 0);

  S[Shortcuts::u+0] = Q[Shortcuts::v+0];
  S[Shortcuts::u+1] = Q[Shortcuts::v+1];
  S[Shortcuts::u+2] = Q[Shortcuts::v+2];
"""


def riemann_solver():
    return """
  applications::exahype2::exaseis::riemannSolverDynamicRupture<VariableShortcuts{{SOLVER_NAME}}, {{CORRECTOR_COMPUTATION_PRECISION}}, (DGOrder + 1), NumberOfUnknowns, (NumberOfUnknowns+NumberOfAuxiliaryVariables), 1>(
    FL, FR, QL, QR, t, dt, x, h, direction, isBoundaryFace, faceIndex, faultNormal, faultPosition
  );
"""


def userIncludes():
    return [
        "tarch/reader/NetCDFFieldSampler.h",
        "DynamicRuptureParameters.h",
        "../riemannSolvers/riemannsolverDynamicRupture.h",
        "exahype2/Curvi.h",
        "exahype2/dg/DGUtils.h",
    ]


def abstractDeclarations():
    return """
protected:
  // Normal on which the fault lies, i.e. if this axis is x, then
  // the fault is on a y-z plane.
  static int faultNormal;

  // In reference space, i.e. this should correspond to the position
  // as viewed by ExaHyPE of the fault.
  static double faultPosition;

public:
  int getFaultNormal(){
    return faultNormal;
  }

  double getFaultPosition(){
    return faultPosition;
  }
"""


def abstractUserDefinitions():
    return """
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::faultNormal;
double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::faultPosition;
"""
