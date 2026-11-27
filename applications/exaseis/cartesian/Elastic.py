# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

POINT_SOURCE_LOCATION = (0.0, 2.0, 0.0)


def initial():
    return """
  // LOH1 places a slower layer between y = 0 and y = 1.
  const bool upperLayer = x(1) <= 1.0;
  Q                     = {};
  Q.rho                 = upperLayer ? 2.6 : 2.7;
  Q.cp                  = upperLayer ? 4.0 : 6.0;
  Q.cs                  = upperLayer ? 2.0 : 3.464;
"""


def boundary():
    return """
  switch (normal) {
  case 0:
    Qoutside.sigma(0, 0) = 0.0;
    Qoutside.sigma(0, 1) = 0.0;
    Qoutside.sigma(0, 2) = 0.0;
    Qoutside.v           = 0.0;
    break;
  case 1:
    // A free surface negates the traction on the face and copies the velocities.
    Qoutside.sigma(1, 1) = -Qinside.sigma(1, 1);
    Qoutside.sigma(0, 1) = -Qinside.sigma(0, 1);
    Qoutside.sigma(1, 2) = -Qinside.sigma(1, 2);
    Qoutside.v           = Qinside.v;
    break;
  default:
    Qoutside.sigma(2, 2) = 0.0;
    Qoutside.sigma(0, 2) = 0.0;
    Qoutside.sigma(1, 2) = 0.0;
    Qoutside.v           = 0.0;
    break;
  }
  Qoutside.rho = Qinside.rho;
  Qoutside.cp  = Qinside.cp;
  Qoutside.cs  = Qinside.cs;
"""


def eigenvalue():
    return """
  return static_cast<double>(Q.cp);
"""


def flux():
    return """
  const auto shearModulus           = Q.rho * Q.cs * Q.cs;
  const auto firstLameParameter     = Q.rho * Q.cp * Q.cp - 2 * shearModulus;
  const auto negativeInverseDensity = -1 / Q.rho;

  F = {};
  for (int axis = 0; axis < 3; axis++) {
    F.v(axis)           = negativeInverseDensity * Q.sigma(normal, axis);
    F.sigma(axis, axis) = -firstLameParameter * Q.v(normal);
    if (axis != normal) {
      F.sigma(normal, axis) = -shearModulus * Q.v(axis);
    }
  }
  F.sigma(normal, normal) = -(firstLameParameter + 2 * shearModulus) * Q.v(normal);
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
  forceVector                      = {};
  forceVector.sigma(0, 2)          = MomentMagnitude * t / (OnsetTime * OnsetTime) * std::exp(-t / OnsetTime);
"""


def init_point_source():
    return """
  location[0] = {{{}, {}, {}}};
""".format(*POINT_SOURCE_LOCATION)


def riemann_solver():
    return """
  return applications::exahype2::exaseis::solveElasticRiemannProblem<{{COMPUTE_PRECISION}}, 1>(
    QL,
    QR,
    normal,
    isBoundaryFace,
    faceIndex,
    FL,
    FR
  );
"""
