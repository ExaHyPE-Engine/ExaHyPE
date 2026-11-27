# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

POINT_SOURCE_LOCATION = (0.0, 3.5, 0.0)


def initial():
    return """
  // LOH1 places a slower layer between y = 0 and y = 1.
  const bool upperLayer = x(1) <= 1.111111111;
  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    Q[unknown] = 0.0;
  }
  Q[Shortcuts::rho] = upperLayer ? 2.6 : 2.7;
  Q[Shortcuts::cp]  = upperLayer ? 2.1 : 4.0;
  Q[Shortcuts::cs]  = upperLayer ? 0.62 : 2.15;
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
    // A free surface negates the traction on the face and copies the velocities.
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

  constexpr double SourcePeakTime   = 0.1;
  constexpr double SourcePulseWidth = 0.03;
  if (t < SourcePeakTime + 5.0 * SourcePulseWidth
      //was 0.2
      and tarch::la::norm2(x - sourceLocation) < 0.07 + tarch::la::norm2(h)) {{
    return ::exahype2::RefinementCommand::Refine;
  }}
  // The detail is a relative residual of an affine fit of the indicator. VelocityReference sets the
  // velocity below which the field counts as quiet (km/s, scales with the source moment), and
  // DetailThreshold is the relative detail above which a cell refines.
  constexpr double VelocityReference = 1.0e-6;
  constexpr double DetailThreshold   = 0.05;

  return ::exahype2::refineOnDetail(
    Q,
    Quadrature<double>::getQuadraturePoints1D(),
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    DetailThreshold,
    ::exahype2::DefaultCoarsenSafetyFactor,
    [](const auto* const values) {{
      double velocitySquared = 0.0;
      for (int d = 0; d < DIMENSIONS; d++) {{
        velocitySquared += (values[Shortcuts::v + d]) * (values[Shortcuts::v + d]);
      }}
      return VelocityReference + std::sqrt(velocitySquared);
    }}
  );
""".format(*POINT_SOURCE_LOCATION)


def point_source():
    return """


  constexpr double M0 =  3.98e-5; 
  double tau = 0.05305164769729845; 
  double M0_time_function_value = M0 * (t / (tau * tau)) * std::exp(-t / tau); 

  constexpr double m_hat[3][3] = {
    {0.93686446, -0.22217437,  0.15047433},
    { -0.22217437, -0.84983167, -0.35248295},
    {0.15047433, -0.35248295, -0.08703279}};


  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    forceVector[unknown] = 0.0;
  }

  // stress order: xx, yy, zz, xy, xz, yz
  forceVector[Shortcuts::sigma + 0] = M0_time_function_value * m_hat[0][0];
  forceVector[Shortcuts::sigma + 1] = M0_time_function_value * m_hat[1][1];
  forceVector[Shortcuts::sigma + 2] = M0_time_function_value * m_hat[2][2];
  forceVector[Shortcuts::sigma + 3] = M0_time_function_value * m_hat[0][1];
  forceVector[Shortcuts::sigma + 4] = M0_time_function_value * m_hat[0][2];
  forceVector[Shortcuts::sigma + 5] = M0_time_function_value * m_hat[1][2];

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


def riemann_solver_fv():
    return """
  return applications::exahype2::exaseis::solveElasticRiemannProblem<
    Shortcuts,
    {{COMPUTE_PRECISION}},
    1,                                     // one state pair, not a DG face
    NumberOfUnknowns,
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    1>(QL, QR, normal, isBoundaryFace, faceIndex, FL, FR);
"""

def map_observerables(): 

  return (""" 
              observables[0] = Q[Shortcuts::v + 0];
              observables[1] = Q[Shortcuts::v + 1];
              observables[2] = Q[Shortcuts::v + 2];
              
  """)


def physical_admissibility_criterion():
  return """
  constexpr int stride = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  for (int i = 0; i < numberOfStates; i++) {
    const auto* q = Q + i * stride;

    for (int u = 0; u < stride; u++) {
      if (!std::isfinite(q[u])) return false;
    }

    const auto rho = q[Shortcuts::rho];
    if (rho <= 0.0 ) return false;
  }
  return true;
"""