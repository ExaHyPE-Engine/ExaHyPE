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

  if (tarch::la::greater(Q[s::h], 0.0)) {
    const double u = Q[s::hu + normal] / Q[s::h];
    const double c = std::sqrt(GRAV * std::cos(Zeta) * Q[s::h]);
    result = std::fmax(std::fabs(u - c), std::fabs(u + c));
  }

  return result;
"""

flux = friction_coefficients + r"""
  if (tarch::la::greater(Q[s::h], 0.0)) {
    const double ih = 1.0 / Q[s::h];
    F[s::h] = Q[s::hu + normal];
    switch (normal) {
    case 0:
      F[s::hu] = ih * Q[s::hu] * Q[s::hu] + 0.5 * GRAV * std::cos(Zeta) * Q[s::h] * Q[s::h];
      F[s::hv] = ih * Q[s::hu] * Q[s::hv];
      break;
    case 1:
      F[s::hu] = ih * Q[s::hv] * Q[s::hu];
      F[s::hv] = ih * Q[s::hv] * Q[s::hv] + 0.5 * GRAV * std::cos(Zeta) * Q[s::h] * Q[s::h];
      break;
    }
  } else {
    F[s::h]  = 0.0;
    F[s::hu] = 0.0;
    F[s::hv] = 0.0;
  }
"""

nonconservative_product = friction_coefficients + r"""
  BTimesDeltaQ[s::h] = 0.0;
  BTimesDeltaQ[s::hu] = 0.0;
  BTimesDeltaQ[s::hv] = 0.0;
  BTimesDeltaQ[s::b] = 0.0;

  const double hx = (Q[s::hx] - 0.5 * deltaQ[s::hx]) / (1.0 * h(0));
  const double hy = (Q[s::hy] - 0.5 * deltaQ[s::hy]) / (1.0 * h(1));

  // u
  const double ux  = (Q[s::ux] - 0.5 * deltaQ[s::ux]) / (1.0 * h(0));
  const double uy  = (Q[s::uy] - 0.5 * deltaQ[s::uy]) / (1.0 * h(1));
  const double uxx = (Q[s::uxx] - 0.5 * deltaQ[s::uxx]) / (h(0) * h(0));
  const double uyy = (Q[s::uyy] - 0.5 * deltaQ[s::uyy]) / (h(1) * h(1));
  const double uxy = (Q[s::uxy] - 0.5 * deltaQ[s::uxy]) / (4.0 * h(0) * h(1));

  // v
  const double vx  = (Q[s::vx] - 0.5 * deltaQ[s::vx]) / (1.0 * h(0));
  const double vy  = (Q[s::vy] - 0.5 * deltaQ[s::vy]) / (1.0 * h(1));
  const double vxx = (Q[s::vxx] - 0.5 * deltaQ[s::vxx]) / (h(0) * h(0));
  const double vyy = (Q[s::vyy] - 0.5 * deltaQ[s::vyy]) / (h(1) * h(1));
  const double vxy = (Q[s::vxy] - 0.5 * deltaQ[s::vxy]) / (4.0 * h(0) * h(1));

  switch (normal) {
  case 0: // x
    BTimesDeltaQ[s::hu] = -(h(0) * Nu * std::sqrt(Q[s::h]) * (1.5 * hx * ux + Q[s::h] * uxx));
    BTimesDeltaQ[s::hv] = -(h(0) * 0.5 * Nu * std::sqrt(Q[s::h]) * (1.5 * hx * (uy + vx) + Q[s::h] * (uxy + vxx)));
    break;
  case 1: // y
    BTimesDeltaQ[s::hu] = -(h(1) * 0.5 * Nu * std::sqrt(Q[s::h]) * (1.5 * hy * (uy + vx) + Q[s::h] * (uyy + vxy)));
    BTimesDeltaQ[s::hv] = -(h(1) * Nu * std::sqrt(Q[s::h]) * (1.5 * hy * vy + Q[s::h] * vyy));
    break;
  }
"""

coulomb_friction = friction_coefficients + r"""
  S[s::h] = 0.0;
  S[s::hu] = 0.0;
  S[s::hv] = 0.0;

  if (tarch::la::greater(Q[s::h], 0.0)) {
    const double u{Q[s::hu + 0] / Q[s::h]};
    const double v{Q[s::hu + 1] / Q[s::h]};
    const double velMagnitudeSqr{u * u + v * v};
    if (tarch::la::greater(velMagnitudeSqr, 0.0)) {
      const double zx{Q[s::bx] / h(0)};
      const double zy{Q[s::by] / h(1)};
      const double c{1.0 / std::sqrt(1.0 + zx * zx + zy * zy)};
      const double friction{-GRAV * (Mu1 * Q[s::h] * c)};
      const double velMagnitude{std::sqrt(velMagnitudeSqr)};
      S[s::hu] += u / velMagnitude * friction;
      S[s::hv] += v / velMagnitude * friction;
    }
  }
"""

material_dependent_basal_friction = friction_coefficients + r"""
  if (tarch::la::greater(Q[s::h], 0.0)) {
    const double ih    = 1.0 / Q[s::h];
    const double u     = ih * Q[s::hu];
    const double v     = ih * Q[s::hv];
    const double ubar  = std::sqrt(Q[s::hu] * Q[s::hu] + Q[s::hv] * Q[s::hv]) * ih;
    const double Fr    = ubar / std::sqrt(GRAV * Q[s::h] * std::cos(Zeta));
    const double Hstar = (BetaStar + Gamma) * Hstop / Beta;

    double Mu = 1.0;
    if (Fr > BetaStar) {
      const double vstop    = (Q[s::h] * Beta) / (Fr + Gamma);
      const double Mustop   = Mu1 + (Mu1 - Mu2) / (1.0 + vstop / FrictionLengthScale);
      Mu = Mustop;
    } else if (Fr > 0.0 && Fr <= BetaStar) {
      const double vstart   = Q[s::h];
      const double vstop    = Q[s::h] * Beta / (BetaStar + Gamma);
      const double Mustart  = Mu3 + (Mu1 - Mu2) / (1.0 + vstart / FrictionLengthScale);
      const double Mustop   = Mu1 + (Mu1 - Mu2) / (1.0 + vstop / FrictionLengthScale);
      const double T = Mustart + (Mustop - Mustart) * std::pow((Fr / BetaStar), Kappa);
      Mu = T;
    } else if (tarch::la::equals(Fr, 0.0)) {
      const double vstart   = Q[s::h];
      const double Mustart  = Mu3 + (Mu1 - Mu2) / (1.0 + vstart / FrictionLengthScale);
      Mu = Mustart;
    }

    if (tarch::la::equals(ubar, 0.0)) {
      const double hx = Q[s::hx] / h(0);
      const double hy = Q[s::hy] / h(1);
      const tarch::la::Vector<Dimensions, double> frictionVector = {std::tan(Zeta) - hx, -hy};
      const double frictionVectorMagnitude = tarch::la::norm2(frictionVector);
      if (tarch::la::greater(Mu, frictionVectorMagnitude)) {
        Mu = frictionVectorMagnitude;
      }

      if (tarch::la::equals(Mu, 0.0)) {
        S[s::h]  = 0.0;
        S[s::hu] = Q[s::h] * GRAV * std::sin(Zeta);
        S[s::hv] = 0.0;
      } else {
        S[s::h]  = 0.0;
        S[s::hu] = Q[s::h] * GRAV * std::sin(Zeta) - Mu * (frictionVector(0) / frictionVectorMagnitude) * Q[s::h] * GRAV * std::cos(Zeta);
        S[s::hv] = -Mu * (frictionVector(1) / frictionVectorMagnitude) * Q[s::h] * GRAV * std::cos(Zeta);
      }
    } else if (tarch::la::greater(ubar, 0.0)) {
      S[s::h]  = 0.0;
      S[s::hu] = Q[s::h] * GRAV * std::sin(Zeta) - Mu * (u / ubar) * Q[s::h] * GRAV * std::cos(Zeta);
      S[s::hv] = -Mu * (v / ubar) * Q[s::h] * GRAV * std::cos(Zeta);
    }
  } else {
    S[s::h]  = 0.0;
    S[s::hu] = 0.0;
    S[s::hv] = 0.0;
  }
"""
