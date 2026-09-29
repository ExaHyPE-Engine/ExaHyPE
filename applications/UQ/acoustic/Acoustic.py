

def initial():
  return """

  //stresses
  Q[Shortcuts::p] = 0.;

  Q[Shortcuts::v+0] = 0;
  Q[Shortcuts::v+1] = 0;
#if DIMENSIONS==3
  Q[Shortcuts::v+2] = 0;
#endif
"""


def boundary():
  return """
  Qoutside[Shortcuts::p]   = 0;
  Qoutside[Shortcuts::v+0] = 0;
  Qoutside[Shortcuts::v+1] = 0;
#if DIMENSIONS==3
  Qoutside[Shortcuts::v+2] = 0;
#endif
"""

def eigenvalue():
  return """
  constexpr double K0 = 4;
  constexpr double rho = 1;
  const double c = std::sqrt(K0/rho);
  return c;
"""

def flux():
  return """
  constexpr auto K0 = 4;
  constexpr auto rho = 1;
  constexpr auto inv_rho = 1 / rho;

  F[Shortcuts::p] = K0 * Q[Shortcuts::v + normal];
  F[Shortcuts::v + 0] = 0;
  F[Shortcuts::v + 1] = 0;
#if DIMENSIONS==3
  F[Shortcuts::v + 2] = 0;
#endif

  F[Shortcuts::v + normal] = inv_rho * Q[Shortcuts::p];
"""

def refinement_criterion():
  return """
  auto result = ::exahype2::RefinementCommand::Keep;

  tarch::la::Vector<DIMENSIONS, double> sourceLocation = {
    pointSourceLocation[0][0],
    pointSourceLocation[0][1],
    #if DIMENSIONS == 3
    pointSourceLocation[0][2]
    #endif
  };

  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - sourceLocation) < 1.0) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
"""

def point_source():
  return """
  for (int i = 0; i < NumberOfUnknowns; i++) {
    forceVector[i] = 0;
  }

  constexpr auto t0 = 0.1;
  constexpr auto M0 = 10;//1000.0;
  auto           f  = M0 * t / (t0 * t0) * std::exp(-t / t0);

  forceVector[Shortcuts::p] = f;
"""