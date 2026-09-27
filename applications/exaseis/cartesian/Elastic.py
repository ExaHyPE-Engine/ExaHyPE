# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

POINT_SOURCE_LOCATION = (0.0, 2.0, 0.0)


def initial():
    return """
  // The LOH1 scenario places a slower layer above y = 1.
  const bool upperLayer = x(1) <= 1.0;
  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    Q[unknown] = 0.0;
  }
  Q[Shortcuts::rho] = upperLayer ? 2.6 : 2.7;
  Q[Shortcuts::cp]  = upperLayer ? 4.0 : 6.0;
  Q[Shortcuts::cs]  = upperLayer ? 2.0 : 3.464;
"""


def boundary():
    return """
  // The stresses sigma + 0 to sigma + 5 are xx, yy, zz, xy, xz and yz.
  switch (normal) {
  case 0:
    Qoutside[Shortcuts::sigma + 0] = 0.0;
    Qoutside[Shortcuts::sigma + 3] = 0.0;
    Qoutside[Shortcuts::sigma + 4] = 0.0;
    Qoutside[Shortcuts::v + 0]     = 0.0;
    Qoutside[Shortcuts::v + 1]     = 0.0;
    Qoutside[Shortcuts::v + 2]     = 0.0;
    break;
  case 1:
    // The free surface mirrors the stresses on the face and keeps the velocities.
    Qoutside[Shortcuts::sigma + 1] = -Qinside[Shortcuts::sigma + 1];
    Qoutside[Shortcuts::sigma + 3] = -Qinside[Shortcuts::sigma + 3];
    Qoutside[Shortcuts::sigma + 5] = -Qinside[Shortcuts::sigma + 5];
    Qoutside[Shortcuts::v + 0]     = Qinside[Shortcuts::v + 0];
    Qoutside[Shortcuts::v + 1]     = Qinside[Shortcuts::v + 1];
    Qoutside[Shortcuts::v + 2]     = Qinside[Shortcuts::v + 2];
    break;
  default:
    Qoutside[Shortcuts::sigma + 2] = 0.0;
    Qoutside[Shortcuts::sigma + 4] = 0.0;
    Qoutside[Shortcuts::sigma + 5] = 0.0;
    Qoutside[Shortcuts::v + 0]     = 0.0;
    Qoutside[Shortcuts::v + 1]     = 0.0;
    Qoutside[Shortcuts::v + 2]     = 0.0;
    break;
  }
  Qoutside[Shortcuts::rho] = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::cp]  = Qinside[Shortcuts::cp];
  Qoutside[Shortcuts::cs]  = Qinside[Shortcuts::cs];
"""


def eigenvalue():
    return """
  return static_cast<double>(Q[Shortcuts::cp]);
"""


def flux():
    return """
  const auto shearModulus           = Q[Shortcuts::rho] * Q[Shortcuts::cs] * Q[Shortcuts::cs];
  const auto firstLameParameter     = Q[Shortcuts::rho] * Q[Shortcuts::cp] * Q[Shortcuts::cp] - 2 * shearModulus;
  const auto negativeInverseDensity = -1 / Q[Shortcuts::rho];

  // The stresses sigma + 0 to sigma + 5 are xx, yy, zz, xy, xz and yz.
  switch (normal) {
  case 0:
    F[Shortcuts::v + 0]     = negativeInverseDensity * Q[Shortcuts::sigma + 0];
    F[Shortcuts::v + 1]     = negativeInverseDensity * Q[Shortcuts::sigma + 3];
    F[Shortcuts::v + 2]     = negativeInverseDensity * Q[Shortcuts::sigma + 4];
    F[Shortcuts::sigma + 0] = -(firstLameParameter + 2 * shearModulus) * Q[Shortcuts::v + 0];
    F[Shortcuts::sigma + 1] = -firstLameParameter * Q[Shortcuts::v + 0];
    F[Shortcuts::sigma + 2] = -firstLameParameter * Q[Shortcuts::v + 0];
    F[Shortcuts::sigma + 3] = -shearModulus * Q[Shortcuts::v + 1];
    F[Shortcuts::sigma + 4] = -shearModulus * Q[Shortcuts::v + 2];
    F[Shortcuts::sigma + 5] = 0;
    break;
  case 1:
    F[Shortcuts::v + 0]     = negativeInverseDensity * Q[Shortcuts::sigma + 3];
    F[Shortcuts::v + 1]     = negativeInverseDensity * Q[Shortcuts::sigma + 1];
    F[Shortcuts::v + 2]     = negativeInverseDensity * Q[Shortcuts::sigma + 5];
    F[Shortcuts::sigma + 0] = -firstLameParameter * Q[Shortcuts::v + 1];
    F[Shortcuts::sigma + 1] = -(firstLameParameter + 2 * shearModulus) * Q[Shortcuts::v + 1];
    F[Shortcuts::sigma + 2] = -firstLameParameter * Q[Shortcuts::v + 1];
    F[Shortcuts::sigma + 3] = -shearModulus * Q[Shortcuts::v + 0];
    F[Shortcuts::sigma + 4] = 0;
    F[Shortcuts::sigma + 5] = -shearModulus * Q[Shortcuts::v + 2];
    break;
  default:
    F[Shortcuts::v + 0]     = negativeInverseDensity * Q[Shortcuts::sigma + 4];
    F[Shortcuts::v + 1]     = negativeInverseDensity * Q[Shortcuts::sigma + 5];
    F[Shortcuts::v + 2]     = negativeInverseDensity * Q[Shortcuts::sigma + 2];
    F[Shortcuts::sigma + 0] = -firstLameParameter * Q[Shortcuts::v + 2];
    F[Shortcuts::sigma + 1] = -firstLameParameter * Q[Shortcuts::v + 2];
    F[Shortcuts::sigma + 2] = -(firstLameParameter + 2 * shearModulus) * Q[Shortcuts::v + 2];
    F[Shortcuts::sigma + 3] = 0;
    F[Shortcuts::sigma + 4] = -shearModulus * Q[Shortcuts::v + 0];
    F[Shortcuts::sigma + 5] = -shearModulus * Q[Shortcuts::v + 1];
    break;
  }
"""


def refinement_criterion():
    return """
  const tarch::la::Vector<DIMENSIONS, double> sourceLocation{{{}, {}, {}}};
  if (tarch::la::equals(t, 0.0) and tarch::la::norm2(x - sourceLocation) < 1.0) {{
    return ::exahype2::RefinementCommand::Refine;
  }}
  return ::exahype2::RefinementCommand::Keep;
""".format(*POINT_SOURCE_LOCATION)


def point_source():
    return """
  constexpr double OnsetTime       = 0.1;
  constexpr double MomentMagnitude = 10.0;
  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    forceVector[unknown] = 0.0;
  }
  forceVector[Shortcuts::sigma + 4] = MomentMagnitude * t / (OnsetTime * OnsetTime) * std::exp(-t / OnsetTime);
"""


def init_point_source():
    return """
  location[0][0] = {};
  location[0][1] = {};
  location[0][2] = {};
""".format(*POINT_SOURCE_LOCATION)


def riemann_solver():
    return """
  return applications::exahype2::exaseis::solveElasticRiemannProblem<
    Shortcuts,
    {{COMPUTE_PRECISION}},
    DGOrder + 1,
    NumberOfUnknowns,
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    1>(QL, QR, normal, isBoundaryFace, faceIndex, FL, FR);
"""
