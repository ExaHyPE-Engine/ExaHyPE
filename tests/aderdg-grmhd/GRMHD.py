# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

max_eigenvalue = r"""
  // The eigenvalues are the advection speed and the two fast magnetosonic speeds along normal. The
  // result is never smaller than the speed of light, 1.
  constexpr double AdiabaticIndex = 4.0 / 3.0;

  double primitives[NumberOfUnknowns];
  double gContr[3][3];
  tests::exahype2::aderdg::grmhd::convertConservedToPrimitive(Q, primitives, gContr);

  const double rho       = primitives[Shortcuts::rho];
  const double vCov[3]   = {primitives[Shortcuts::vel + 0], primitives[Shortcuts::vel + 1], primitives[Shortcuts::vel + 2]};
  const double p         = primitives[Shortcuts::E];
  const double bContr[3] = {primitives[Shortcuts::B + 0], primitives[Shortcuts::B + 1], primitives[Shortcuts::B + 2]};
  const double shift[3]  = {primitives[Shortcuts::shift + 0], primitives[Shortcuts::shift + 1], primitives[Shortcuts::shift + 2]};
  const double lapse     = primitives[Shortcuts::lapse];

  const double gCov[3][3] = {
    {primitives[Shortcuts::gij + 0], primitives[Shortcuts::gij + 1], primitives[Shortcuts::gij + 2]},
    {primitives[Shortcuts::gij + 1], primitives[Shortcuts::gij + 3], primitives[Shortcuts::gij + 4]},
    {primitives[Shortcuts::gij + 2], primitives[Shortcuts::gij + 4], primitives[Shortcuts::gij + 5]}
  };

  double vContr[3];
  tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, vCov, vContr);

  double bCov[3];
  tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gCov, bContr, bCov);

  // The comoving magnetic field b has b^2 = B^2 (1 - v^2) + (v B)^2.
  const double v2                          = tests::exahype2::aderdg::grmhd::dotProduct(vContr, vCov);
  const double inverseLorentzFactorSquared = 1.0 - v2;
  const double b2                          = tests::exahype2::aderdg::grmhd::dotProduct(bContr, bCov);
  const double vDotB                       = tests::exahype2::aderdg::grmhd::dotProduct(vContr, bCov);
  const double comovingB2                  = b2 * inverseLorentzFactorSquared + vDotB * vDotB;

  const double enthalpyFactor           = AdiabaticIndex / (AdiabaticIndex - 1.0);
  const double totalEnthalpy            = rho + enthalpyFactor * p + comovingB2;
  const double magnetosonicSpeedSquared = (AdiabaticIndex * p + comovingB2) / totalEnthalpy;

  const double normalVelocity     = vContr[normal];
  const double normalShift        = shift[normal];
  const double normalMetric       = gContr[normal][normal];
  const double inverseDenominator = 1.0 / (1.0 - v2 * magnetosonicSpeedSquared);

  const double deltaU = std::sqrt(
    magnetosonicSpeedSquared * inverseLorentzFactorSquared
    * ((1.0 - v2 * magnetosonicSpeedSquared) * normalMetric - normalVelocity * normalVelocity * (1.0 - magnetosonicSpeedSquared))
  );

  double maxEigenvalue = std::abs(lapse * normalVelocity - normalShift);
  maxEigenvalue        = std::max(
    maxEigenvalue,
    std::abs(lapse * inverseDenominator * (normalVelocity * (1.0 - magnetosonicSpeedSquared) - deltaU) - normalShift)
  );
  maxEigenvalue = std::max(
    maxEigenvalue,
    std::abs(lapse * inverseDenominator * (normalVelocity * (1.0 - magnetosonicSpeedSquared) + deltaU) - normalShift)
  );
  maxEigenvalue = std::max(maxEigenvalue, 1.0);

  return maxEigenvalue;
"""

flux = r"""
  constexpr double AdiabaticIndex = 4.0 / 3.0;
  const double     enthalpyFactor = AdiabaticIndex / (AdiabaticIndex - 1.0);

  std::fill_n(F, NumberOfUnknowns, 0.0);

  double       primitives[NumberOfUnknowns];
  double       gContr[3][3];
  const double determinant = tests::exahype2::aderdg::grmhd::convertConservedToPrimitive(Q, primitives, gContr);

  const double rho                 = primitives[Shortcuts::rho];
  const double vCov[3]             = {primitives[Shortcuts::vel + 0], primitives[Shortcuts::vel + 1], primitives[Shortcuts::vel + 2]};
  const double p                   = primitives[Shortcuts::E];
  const double bContr[3]           = {primitives[Shortcuts::B + 0], primitives[Shortcuts::B + 1], primitives[Shortcuts::B + 2]};
  const double densitisedBContr[3] = {Q[Shortcuts::B + 0], Q[Shortcuts::B + 1], Q[Shortcuts::B + 2]};
  const double shift[3]            = {primitives[Shortcuts::shift + 0], primitives[Shortcuts::shift + 1], primitives[Shortcuts::shift + 2]};
  const double lapse               = primitives[Shortcuts::lapse];

  const double gCov[3][3] = {
    {primitives[Shortcuts::gij + 0], primitives[Shortcuts::gij + 1], primitives[Shortcuts::gij + 2]},
    {primitives[Shortcuts::gij + 1], primitives[Shortcuts::gij + 3], primitives[Shortcuts::gij + 4]},
    {primitives[Shortcuts::gij + 2], primitives[Shortcuts::gij + 4], primitives[Shortcuts::gij + 5]}
  };

  const double rootDeterminant        = std::sqrt(determinant);
  const double inverseRootDeterminant = 1. / rootDeterminant;

  double vContr[3];
  tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, vCov, vContr);

  double sContr[3];
  tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, &Q[Shortcuts::vel], sContr);

  double bCov[3];
  tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gCov, bContr, bCov);

  double vCrossBCov[3];
  double vCrossBContr[3];
  tests::exahype2::aderdg::grmhd::crossProduct(vContr, bContr, rootDeterminant, vCrossBCov);
  tests::exahype2::aderdg::grmhd::crossProduct(vCov, bCov, inverseRootDeterminant, vCrossBContr);

  const double v2 = tests::exahype2::aderdg::grmhd::dotProduct(vContr, vCov);
  const double e2 = tests::exahype2::aderdg::grmhd::dotProduct(vCrossBContr, vCrossBCov);
  const double b2 = tests::exahype2::aderdg::grmhd::dotProduct(bContr, bCov);

  const double electromagneticEnergy = 0.5 * (b2 + e2);
  const double lorentzFactor         = 1.0 / std::sqrt(1.0 - v2);

  const double enthalpy         = rho + enthalpyFactor * p;
  const double scaledEnthalpy   = enthalpy * lorentzFactor * lorentzFactor;
  const double fluidMomentum[3] = {scaledEnthalpy * vContr[0], scaledEnthalpy * vContr[1], scaledEnthalpy * vContr[2]};

  const double transportVelocity[3] = {
    lapse * vContr[0] - shift[0],
    lapse * vContr[1] - shift[1],
    lapse * vContr[2] - shift[2]
  };

  double inductionFlux[3][3];
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      inductionFlux[i][j] = transportVelocity[i] * densitisedBContr[j] - transportVelocity[j] * densitisedBContr[i];
    }
  }

  switch (normal) {
    case 0:
      F[Shortcuts::rho]     = vContr[0] * Q[Shortcuts::rho];
      F[Shortcuts::vel + 0] = rootDeterminant * (fluidMomentum[0] * vCov[0] - vCrossBContr[0] * vCrossBCov[0] - bContr[0] * bCov[0] + p + electromagneticEnergy);
      F[Shortcuts::vel + 1] = rootDeterminant * (fluidMomentum[0] * vCov[1] - vCrossBContr[0] * vCrossBCov[1] - bContr[0] * bCov[1]);
      F[Shortcuts::vel + 2] = rootDeterminant * (fluidMomentum[0] * vCov[2] - vCrossBContr[0] * vCrossBCov[2] - bContr[0] * bCov[2]);
      F[Shortcuts::E]       = sContr[0] - F[Shortcuts::rho];
      F[Shortcuts::B + 0]   = inductionFlux[0][0];
      F[Shortcuts::B + 1]   = inductionFlux[0][1];
      F[Shortcuts::B + 2]   = inductionFlux[0][2];
      break;
    case 1:
      F[Shortcuts::rho]     = vContr[1] * Q[Shortcuts::rho];
      F[Shortcuts::vel + 0] = rootDeterminant * (fluidMomentum[1] * vCov[0] - vCrossBContr[1] * vCrossBCov[0] - bContr[1] * bCov[0]);
      F[Shortcuts::vel + 1] = rootDeterminant * (fluidMomentum[1] * vCov[1] - vCrossBContr[1] * vCrossBCov[1] - bContr[1] * bCov[1] + p + electromagneticEnergy);
      F[Shortcuts::vel + 2] = rootDeterminant * (fluidMomentum[1] * vCov[2] - vCrossBContr[1] * vCrossBCov[2] - bContr[1] * bCov[2]);
      F[Shortcuts::E]       = sContr[1] - F[Shortcuts::rho];
      F[Shortcuts::B + 0]   = inductionFlux[1][0];
      F[Shortcuts::B + 1]   = inductionFlux[1][1];
      F[Shortcuts::B + 2]   = inductionFlux[1][2];
      break;
    case 2:
      F[Shortcuts::rho]     = vContr[2] * Q[Shortcuts::rho];
      F[Shortcuts::vel + 0] = rootDeterminant * (fluidMomentum[2] * vCov[0] - vCrossBContr[2] * vCrossBCov[0] - bContr[2] * bCov[0]);
      F[Shortcuts::vel + 1] = rootDeterminant * (fluidMomentum[2] * vCov[1] - vCrossBContr[2] * vCrossBCov[1] - bContr[2] * bCov[1]);
      F[Shortcuts::vel + 2] = rootDeterminant * (fluidMomentum[2] * vCov[2] - vCrossBContr[2] * vCrossBCov[2] - bContr[2] * bCov[2] + p + electromagneticEnergy);
      F[Shortcuts::E]       = sContr[2] - F[Shortcuts::rho];
      F[Shortcuts::B + 0]   = inductionFlux[2][0];
      F[Shortcuts::B + 1]   = inductionFlux[2][1];
      F[Shortcuts::B + 2]   = inductionFlux[2][2];
      break;
    default:
      ASSERTION(false, normal);
  }

  // Q stores the fluid values multiplied by the root of the metric determinant.
  for (int i = Shortcuts::rho; i <= Shortcuts::E; i++) {
    F[i] = lapse * F[i] - shift[normal] * Q[i];
  }
"""

ncp = r"""
  constexpr double AdiabaticIndex          = 4.0 / 3.0;
  constexpr double DivergenceCleaningSpeed = 1.0;

  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0.0);

  const double lapse    = Q[Shortcuts::lapse];
  const double shift[3] = {Q[Shortcuts::shift + 0], Q[Shortcuts::shift + 1], Q[Shortcuts::shift + 2]};

  const double gCov[3][3] = {
    {Q[Shortcuts::gij + 0], Q[Shortcuts::gij + 1], Q[Shortcuts::gij + 2]},
    {Q[Shortcuts::gij + 1], Q[Shortcuts::gij + 3], Q[Shortcuts::gij + 4]},
    {Q[Shortcuts::gij + 2], Q[Shortcuts::gij + 4], Q[Shortcuts::gij + 5]}
  };

  double       primitives[NumberOfUnknowns];
  double       gContr[3][3];
  const double determinant = tests::exahype2::aderdg::grmhd::convertConservedToPrimitive(Q, primitives, gContr);

  const double rootDeterminant        = std::sqrt(determinant);
  const double inverseRootDeterminant = 1. / rootDeterminant;

  const double enthalpyFactor = AdiabaticIndex / (AdiabaticIndex - 1.);

  const double rho       = primitives[Shortcuts::rho];
  const double vCov[3]   = {primitives[Shortcuts::vel + 0], primitives[Shortcuts::vel + 1], primitives[Shortcuts::vel + 2]};
  const double p         = primitives[Shortcuts::E];
  const double bContr[3] = {primitives[Shortcuts::B + 0], primitives[Shortcuts::B + 1], primitives[Shortcuts::B + 2]};

  double bCov[3];
  tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gCov, bContr, bCov);

  double vContr[3];
  tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, vCov, vContr);

  double sContr[3];
  tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, &Q[Shortcuts::vel], sContr);

  double vCrossBCov[3];
  double vCrossBContr[3];
  tests::exahype2::aderdg::grmhd::crossProduct(vContr, bContr, rootDeterminant, vCrossBCov);
  tests::exahype2::aderdg::grmhd::crossProduct(vCov, bCov, inverseRootDeterminant, vCrossBContr);

  const double v2 = tests::exahype2::aderdg::grmhd::dotProduct(vContr, vCov);
  const double e2 = tests::exahype2::aderdg::grmhd::dotProduct(vCrossBContr, vCrossBCov);
  const double b2 = tests::exahype2::aderdg::grmhd::dotProduct(bContr, bCov);

  const double electromagneticEnergy = 0.5 * (b2 + e2);
  const double lorentzFactor         = 1.0 / std::sqrt(1.0 - v2);

  const double enthalpy       = rho + enthalpyFactor * p;
  const double scaledEnthalpy = enthalpy * lorentzFactor * lorentzFactor;

  const double kroneckerDelta[3][3] = {
    {1., 0., 0.},
    {0., 1., 0.},
    {0., 0., 1.}
  };

  // A permutation of the axes permutes the three shift terms, the three diagonal metric terms and the
  // three off-diagonal metric terms within their groups. Each group adds in order of magnitude. The
  // groups and the lapse term add in a fixed order. A permutation of the axes keeps both sums bitwise.
  double momentumShiftTerms[3];
  double energyShiftTerms[3];
  for (int i = 0; i < 3; i++) {
    const double stressMixed = scaledEnthalpy * vCov[i] * vContr[normal] - vCrossBCov[i] * vCrossBContr[normal]
                             - bCov[i] * bContr[normal] + (p + electromagneticEnergy) * kroneckerDelta[i][normal];
    momentumShiftTerms[i] = -Q[Shortcuts::vel + i] * deltaQ[Shortcuts::shift + i];
    energyShiftTerms[i]   = -rootDeterminant * stressMixed * deltaQ[Shortcuts::shift + i];
  }

  // The stress terms run over the six components of the symmetric metric in the order of gij.
  double momentumDiagonalTerms[3];
  double energyDiagonalTerms[3];
  double momentumOffDiagonalTerms[3];
  double energyOffDiagonalTerms[3];
  int    metricIndex = 0;
  int    offDiagonal = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = i; j < 3; j++) {
      double stressContr = tarch::la::productInOrderOfMagnitude(scaledEnthalpy, vContr[i], vContr[j])
                         - vCrossBContr[i] * vCrossBContr[j] - bContr[i] * bContr[j]
                         + (p + electromagneticEnergy) * gContr[i][j];
      if (i != j) {
        stressContr = 2.0 * stressContr;
      }
      const double momentumTerm = -0.5 * rootDeterminant * lapse * stressContr * deltaQ[Shortcuts::gij + metricIndex];
      const double energyTerm   = -0.5 * rootDeterminant * stressContr * shift[normal] * deltaQ[Shortcuts::gij + metricIndex];
      if (i == j) {
        momentumDiagonalTerms[i] = momentumTerm;
        energyDiagonalTerms[i]   = energyTerm;
      } else {
        momentumOffDiagonalTerms[offDiagonal] = momentumTerm;
        energyOffDiagonalTerms[offDiagonal]   = energyTerm;
        offDiagonal++;
      }
      metricIndex++;
    }
  }

  const auto sumOfGroup = [](const double (&terms)[3]) {
    return tarch::la::sumInOrderOfMagnitude(terms[0], terms[1], terms[2]);
  };
  BTimesDeltaQ[Shortcuts::vel + normal] =
    ((sumOfGroup(momentumShiftTerms) + sumOfGroup(momentumDiagonalTerms)) + sumOfGroup(momentumOffDiagonalTerms))
    + (Q[Shortcuts::E] + Q[Shortcuts::rho]) * deltaQ[Shortcuts::lapse];
  BTimesDeltaQ[Shortcuts::E] =
    ((sumOfGroup(energyShiftTerms) + sumOfGroup(energyDiagonalTerms)) + sumOfGroup(energyOffDiagonalTerms))
    + sContr[normal] * deltaQ[Shortcuts::lapse];
  BTimesDeltaQ[Shortcuts::B + 0] = lapse * rootDeterminant * gContr[0][normal] * deltaQ[Shortcuts::psi]
                                 - Q[Shortcuts::shift + 0] * deltaQ[Shortcuts::B + normal];
  BTimesDeltaQ[Shortcuts::B + 1] = lapse * rootDeterminant * gContr[1][normal] * deltaQ[Shortcuts::psi]
                                 - Q[Shortcuts::shift + 1] * deltaQ[Shortcuts::B + normal];
  BTimesDeltaQ[Shortcuts::B + 2] = lapse * rootDeterminant * gContr[2][normal] * deltaQ[Shortcuts::psi]
                                 - Q[Shortcuts::shift + 2] * deltaQ[Shortcuts::B + normal];
  BTimesDeltaQ[Shortcuts::psi] = inverseRootDeterminant * lapse * DivergenceCleaningSpeed * DivergenceCleaningSpeed
                                   * deltaQ[Shortcuts::B + normal]
                               - Q[Shortcuts::shift + normal] * deltaQ[Shortcuts::psi];
"""
