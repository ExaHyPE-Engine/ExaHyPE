# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

friction_coefficients = r"""
  const double Zeta = 35.0 * tarch::la::PI / 180.0; // Conversion of degrees to radians
  const double Zeta1 = 28.95 * tarch::la::PI / 180.0;
  const double Zeta2 = 44.09 * tarch::la::PI / 180.0;
  const double Zeta3 = 31.81 * tarch::la::PI / 180.0;

  const double Mu1 = std::tan(Zeta1);
  const double Mu2 = std::tan(Zeta2);
  const double Mu3 = std::tan(Zeta3);

  constexpr double Beta = 1.07;
  constexpr double Gamma = 2.01;
  constexpr double BetaStar = 0.06;
  // Kappa in the friction law is assumed to be 1 and is just initialised as a
  // const double here using Kappa. The user can apply std::pow((Fr/BetaStar),
  // Kappa) for evaluating T here in case the value of Kappa is different
  // from 1.

  constexpr double Kappa = 1.0;
  constexpr double FrictionLengthScale = 0.00035;

  const double Nu = (2.0 / 9.0) * (FrictionLengthScale / Beta) * (GRAV * std::sin(Zeta) / std::sqrt(GRAV * std::cos(Zeta))) * (((Mu2 - Mu1) / (std::tan(Zeta) - Mu1)) - 1.0);
  const double Hstart = FrictionLengthScale * (((Mu2 - Mu1) / (std::tan(Zeta) - Mu3)) - 1.0);
  const double Hstop = FrictionLengthScale * (((Mu2 - Mu1) / (std::tan(Zeta) - Mu1)) - 1.0);
"""

max_eigenvalue = friction_coefficients + r"""
  double result{0.0};

  if (tarch::la::greater(Q[Shortcuts::h], 0.0)) {
    const double u = Q[Shortcuts::hu + normal] / Q[Shortcuts::h];
    const double c = std::sqrt(GRAV * std::cos(Zeta) * Q[Shortcuts::h]);
    result = std::fmax(std::fabs(u - c), std::fabs(u + c));
  }

  return result;
"""

flux = friction_coefficients + r"""
  if (tarch::la::greater(Q[Shortcuts::h], 0.0)) {
    const double ih = 1.0 / Q[Shortcuts::h];
    F[Shortcuts::h] = Q[Shortcuts::hu + normal];
    switch (normal) {
    case 0:
      F[Shortcuts::hu] = ih * Q[Shortcuts::hu] * Q[Shortcuts::hu] + 0.5 * GRAV * std::cos(Zeta) * Q[Shortcuts::h] * Q[Shortcuts::h];
      F[Shortcuts::hv] = ih * Q[Shortcuts::hu] * Q[Shortcuts::hv];
      break;
    case 1:
      F[Shortcuts::hu] = ih * Q[Shortcuts::hv] * Q[Shortcuts::hu];
      F[Shortcuts::hv] = ih * Q[Shortcuts::hv] * Q[Shortcuts::hv] + 0.5 * GRAV * std::cos(Zeta) * Q[Shortcuts::h] * Q[Shortcuts::h];
      break;
    }
  } else {
    F[Shortcuts::h]  = 0.0;
    F[Shortcuts::hu] = 0.0;
    F[Shortcuts::hv] = 0.0;
  }
"""

nonconservative_product = friction_coefficients + r"""
  BTimesDeltaQ[Shortcuts::h] = 0.0;
  BTimesDeltaQ[Shortcuts::hu] = 0.0;
  BTimesDeltaQ[Shortcuts::hv] = 0.0;
  BTimesDeltaQ[Shortcuts::b] = 0.0;

  const double hx = (Q[Shortcuts::hx] - 0.5 * deltaQ[Shortcuts::hx]) / (1.0 * h(0));
  const double hy = (Q[Shortcuts::hy] - 0.5 * deltaQ[Shortcuts::hy]) / (1.0 * h(1));

  // u
  const double ux  = (Q[Shortcuts::ux] - 0.5 * deltaQ[Shortcuts::ux]) / (1.0 * h(0));
  const double uy  = (Q[Shortcuts::uy] - 0.5 * deltaQ[Shortcuts::uy]) / (1.0 * h(1));
  const double uxx = (Q[Shortcuts::uxx] - 0.5 * deltaQ[Shortcuts::uxx]) / (h(0) * h(0));
  const double uyy = (Q[Shortcuts::uyy] - 0.5 * deltaQ[Shortcuts::uyy]) / (h(1) * h(1));
  const double uxy = (Q[Shortcuts::uxy] - 0.5 * deltaQ[Shortcuts::uxy]) / (4.0 * h(0) * h(1));

  // v
  const double vx  = (Q[Shortcuts::vx] - 0.5 * deltaQ[Shortcuts::vx]) / (1.0 * h(0));
  const double vy  = (Q[Shortcuts::vy] - 0.5 * deltaQ[Shortcuts::vy]) / (1.0 * h(1));
  const double vxx = (Q[Shortcuts::vxx] - 0.5 * deltaQ[Shortcuts::vxx]) / (h(0) * h(0));
  const double vyy = (Q[Shortcuts::vyy] - 0.5 * deltaQ[Shortcuts::vyy]) / (h(1) * h(1));
  const double vxy = (Q[Shortcuts::vxy] - 0.5 * deltaQ[Shortcuts::vxy]) / (4.0 * h(0) * h(1));

  switch (normal) {
  case 0: // x
    BTimesDeltaQ[Shortcuts::hu] = -(h(0) * Nu * std::sqrt(Q[Shortcuts::h]) * (1.5 * hx * ux + Q[Shortcuts::h] * uxx));
    BTimesDeltaQ[Shortcuts::hv] = -(h(0) * 0.5 * Nu * std::sqrt(Q[Shortcuts::h]) * (1.5 * hx * (uy + vx) + Q[Shortcuts::h] * (uxy + vxx)));
    break;
  case 1: // y
    BTimesDeltaQ[Shortcuts::hu] = -(h(1) * 0.5 * Nu * std::sqrt(Q[Shortcuts::h]) * (1.5 * hy * (uy + vx) + Q[Shortcuts::h] * (uyy + vxy)));
    BTimesDeltaQ[Shortcuts::hv] = -(h(1) * Nu * std::sqrt(Q[Shortcuts::h]) * (1.5 * hy * vy + Q[Shortcuts::h] * vyy));
    break;
  }
"""

coulomb_friction = friction_coefficients + r"""
  S[Shortcuts::h] = 0.0;
  S[Shortcuts::hu] = 0.0;
  S[Shortcuts::hv] = 0.0;

  if (tarch::la::greater(Q[Shortcuts::h], 0.0)) {
    const double u{Q[Shortcuts::hu + 0] / Q[Shortcuts::h]};
    const double v{Q[Shortcuts::hu + 1] / Q[Shortcuts::h]};
    const double velMagnitudeSqr{u * u + v * v};
    if (tarch::la::greater(velMagnitudeSqr, 0.0)) {
      const double zx{Q[Shortcuts::bx] / h(0)};
      const double zy{Q[Shortcuts::by] / h(1)};
      const double c{1.0 / std::sqrt(1.0 + zx * zx + zy * zy)};
      const double friction{-GRAV * (Mu1 * Q[Shortcuts::h] * c)};
      const double velMagnitude{std::sqrt(velMagnitudeSqr)};
      S[Shortcuts::hu] += u / velMagnitude * friction;
      S[Shortcuts::hv] += v / velMagnitude * friction;
    }
  }
"""

material_dependent_basal_friction = friction_coefficients + r"""
  if (tarch::la::greater(Q[Shortcuts::h], 0.0)) {
    const double ih    = 1.0 / Q[Shortcuts::h];
    const double u     = ih * Q[Shortcuts::hu];
    const double v     = ih * Q[Shortcuts::hv];
    const double ubar  = std::sqrt(Q[Shortcuts::hu] * Q[Shortcuts::hu] + Q[Shortcuts::hv] * Q[Shortcuts::hv]) * ih;
    const double Fr    = ubar / std::sqrt(GRAV * Q[Shortcuts::h] * std::cos(Zeta));
    const double Hstar = (BetaStar + Gamma) * Hstop / Beta;

    double Mu = 1.0;
    if (Fr > BetaStar) {
      const double vstop    = (Q[Shortcuts::h] * Beta) / (Fr + Gamma);
      const double Mustop   = Mu1 + (Mu1 - Mu2) / (1.0 + vstop / FrictionLengthScale);
      Mu = Mustop;
    } else if (Fr > 0.0 && Fr <= BetaStar) {
      const double vstart   = Q[Shortcuts::h];
      const double vstop    = Q[Shortcuts::h] * Beta / (BetaStar + Gamma);
      const double Mustart  = Mu3 + (Mu1 - Mu2) / (1.0 + vstart / FrictionLengthScale);
      const double Mustop   = Mu1 + (Mu1 - Mu2) / (1.0 + vstop / FrictionLengthScale);
      const double T = Mustart + (Mustop - Mustart) * std::pow((Fr / BetaStar), Kappa);
      Mu = T;
    } else if (tarch::la::equals(Fr, 0.0)) {
      const double vstart   = Q[Shortcuts::h];
      const double Mustart  = Mu3 + (Mu1 - Mu2) / (1.0 + vstart / FrictionLengthScale);
      Mu = Mustart;
    }

    if (tarch::la::equals(ubar, 0.0)) {
      const double hx = Q[Shortcuts::hx] / h(0);
      const double hy = Q[Shortcuts::hy] / h(1);
      const tarch::la::Vector<Dimensions, double> frictionVector = {std::tan(Zeta) - hx, -hy};
      const double frictionVectorMagnitude = tarch::la::norm2(frictionVector);
      if (tarch::la::greater(Mu, frictionVectorMagnitude)) {
        Mu = frictionVectorMagnitude;
      }

      if (tarch::la::equals(Mu, 0.0)) {
        S[Shortcuts::h]  = 0.0;
        S[Shortcuts::hu] = Q[Shortcuts::h] * GRAV * std::sin(Zeta);
        S[Shortcuts::hv] = 0.0;
      } else {
        S[Shortcuts::h]  = 0.0;
        S[Shortcuts::hu] = Q[Shortcuts::h] * GRAV * std::sin(Zeta) - Mu * (frictionVector(0) / frictionVectorMagnitude) * Q[Shortcuts::h] * GRAV * std::cos(Zeta);
        S[Shortcuts::hv] = -Mu * (frictionVector(1) / frictionVectorMagnitude) * Q[Shortcuts::h] * GRAV * std::cos(Zeta);
      }
    } else if (tarch::la::greater(ubar, 0.0)) {
      S[Shortcuts::h]  = 0.0;
      S[Shortcuts::hu] = Q[Shortcuts::h] * GRAV * std::sin(Zeta) - Mu * (u / ubar) * Q[Shortcuts::h] * GRAV * std::cos(Zeta);
      S[Shortcuts::hv] = -Mu * (v / ubar) * Q[Shortcuts::h] * GRAV * std::cos(Zeta);
    }
  } else {
    S[Shortcuts::h]  = 0.0;
    S[Shortcuts::hu] = 0.0;
    S[Shortcuts::hv] = 0.0;
  }
"""
