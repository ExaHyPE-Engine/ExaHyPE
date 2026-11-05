# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

max_eigenvalue = r"""
  // The eigenvalues are the advection speed and the two fast magnetosonic speeds along normal. The
  // result is never smaller than the speed of light, 1.
  constexpr double AdiabaticIndex = 4.0 / 3.0;

  tests::exahype2::aderdg::grmhd::Matrix3x3 gContr;
  double                                    determinant;
  const auto primitives = tests::exahype2::aderdg::grmhd::getPrimitiveVariablesOf(Q, gContr, determinant);

  const double rho    = primitives.rho;
  const auto&  vCov   = primitives.vel;
  const double p      = primitives.E;
  const auto&  bContr = primitives.B;
  const auto&  shift  = primitives.shift;
  const double lapse  = primitives.lapse;
  const auto   gCov   = tests::exahype2::aderdg::grmhd::getSpatialMetricOf(primitives.gij);

  const auto vContr = tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, vCov);
  const auto bCov   = tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gCov, bContr);

  // The comoving magnetic field b has b^2 = B^2 (1 - v^2) + (v B)^2.
  const double v2                          = tests::exahype2::aderdg::grmhd::getScalarProductOf(vContr, vCov);
  const double inverseLorentzFactorSquared = 1.0 - v2;
  const double b2                          = tests::exahype2::aderdg::grmhd::getScalarProductOf(bContr, bCov);
  const double vDotB                       = tests::exahype2::aderdg::grmhd::getScalarProductOf(vContr, bCov);
  const double comovingB2                  = b2 * inverseLorentzFactorSquared + vDotB * vDotB;

  const double enthalpyFactor           = AdiabaticIndex / (AdiabaticIndex - 1.0);
  const double totalEnthalpy            = rho + enthalpyFactor * p + comovingB2;
  const double magnetosonicSpeedSquared = (AdiabaticIndex * p + comovingB2) / totalEnthalpy;

  const double normalVelocity     = vContr(normal);
  const double normalShift        = shift(normal);
  const double normalMetric       = gContr(normal, normal);
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
  ASSERTION(normal >= 0 and normal < 3, normal);
  constexpr double AdiabaticIndex = 4.0 / 3.0;
  const double     enthalpyFactor = AdiabaticIndex / (AdiabaticIndex - 1.0);

  F = {};

  tests::exahype2::aderdg::grmhd::Matrix3x3 gContr;
  double                                    determinant;
  const auto primitives = tests::exahype2::aderdg::grmhd::getPrimitiveVariablesOf(Q, gContr, determinant);

  const double rho                 = primitives.rho;
  const auto&  vCov                = primitives.vel;
  const double p                   = primitives.E;
  const auto&  bContr              = primitives.B;
  const auto&  densitisedBContr    = Q.B;
  const auto&  shift               = primitives.shift;
  const double lapse               = primitives.lapse;
  const auto   gCov                = tests::exahype2::aderdg::grmhd::getSpatialMetricOf(primitives.gij);

  const double rootDeterminant        = std::sqrt(determinant);
  const double inverseRootDeterminant = 1. / rootDeterminant;

  const auto vContr = tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, vCov);
  const auto sContr = tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, Q.vel);
  const auto bCov   = tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gCov, bContr);

  const auto vCrossBCov   = tests::exahype2::aderdg::grmhd::getScaledCrossProductOf(vContr, bContr, rootDeterminant);
  const auto vCrossBContr = tests::exahype2::aderdg::grmhd::getScaledCrossProductOf(vCov, bCov, inverseRootDeterminant);

  const double v2 = tests::exahype2::aderdg::grmhd::getScalarProductOf(vContr, vCov);
  const double e2 = tests::exahype2::aderdg::grmhd::getScalarProductOf(vCrossBContr, vCrossBCov);
  const double b2 = tests::exahype2::aderdg::grmhd::getScalarProductOf(bContr, bCov);

  const double electromagneticEnergy = 0.5 * (b2 + e2);
  const double lorentzFactor         = 1.0 / std::sqrt(1.0 - v2);

  const double enthalpy       = rho + enthalpyFactor * p;
  const double scaledEnthalpy = enthalpy * lorentzFactor * lorentzFactor;
  const auto   fluidMomentum  = scaledEnthalpy * vContr;

  tests::exahype2::aderdg::grmhd::Vector3 transportVelocity;
  for (int i = 0; i < 3; i++) {
    transportVelocity(i) = lapse * vContr(i) - shift(i);
  }

  // The momentum flux contains the pressure and the electromagnetic energy on the normal component.
  F.rho = vContr(normal) * Q.rho;
  for (int i = 0; i < 3; i++) {
    if (i == normal) {
      F.vel(i) = rootDeterminant * (fluidMomentum(normal) * vCov(i) - vCrossBContr(normal) * vCrossBCov(i) - bContr(normal) * bCov(i) + p + electromagneticEnergy);
    } else {
      F.vel(i) = rootDeterminant * (fluidMomentum(normal) * vCov(i) - vCrossBContr(normal) * vCrossBCov(i) - bContr(normal) * bCov(i));
    }
    F.B(i) = transportVelocity(normal) * densitisedBContr(i) - transportVelocity(i) * densitisedBContr(normal);
  }
  F.E = sContr(normal) - F.rho;

  // Q stores the fluid values multiplied by the root of the metric determinant.
  F.rho = lapse * F.rho - shift(normal) * Q.rho;
  for (int i = 0; i < 3; i++) {
    F.vel(i) = lapse * F.vel(i) - shift(normal) * Q.vel(i);
  }
  F.E = lapse * F.E - shift(normal) * Q.E;
"""

ncp = r"""
  constexpr double AdiabaticIndex          = 4.0 / 3.0;
  constexpr double DivergenceCleaningSpeed = 1.0;

  BTimesDeltaQ = {};

  const double lapse = Q.lapse;
  const auto&  shift = Q.shift;
  const auto   gCov  = tests::exahype2::aderdg::grmhd::getSpatialMetricOf(Q.gij);

  tests::exahype2::aderdg::grmhd::Matrix3x3 gContr;
  double                                    determinant;
  const auto primitives = tests::exahype2::aderdg::grmhd::getPrimitiveVariablesOf(Q, gContr, determinant);

  const double rootDeterminant        = std::sqrt(determinant);
  const double inverseRootDeterminant = 1. / rootDeterminant;

  const double enthalpyFactor = AdiabaticIndex / (AdiabaticIndex - 1.);

  const double rho    = primitives.rho;
  const auto&  vCov   = primitives.vel;
  const double p      = primitives.E;
  const auto&  bContr = primitives.B;

  const auto bCov   = tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gCov, bContr);
  const auto vContr = tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, vCov);
  const auto sContr = tests::exahype2::aderdg::grmhd::multiplyMatrixVector(gContr, Q.vel);

  const auto vCrossBCov   = tests::exahype2::aderdg::grmhd::getScaledCrossProductOf(vContr, bContr, rootDeterminant);
  const auto vCrossBContr = tests::exahype2::aderdg::grmhd::getScaledCrossProductOf(vCov, bCov, inverseRootDeterminant);

  const double v2 = tests::exahype2::aderdg::grmhd::getScalarProductOf(vContr, vCov);
  const double e2 = tests::exahype2::aderdg::grmhd::getScalarProductOf(vCrossBContr, vCrossBCov);
  const double b2 = tests::exahype2::aderdg::grmhd::getScalarProductOf(bContr, bCov);

  const double electromagneticEnergy = 0.5 * (b2 + e2);
  const double lorentzFactor         = 1.0 / std::sqrt(1.0 - v2);

  const double enthalpy       = rho + enthalpyFactor * p;
  const double scaledEnthalpy = enthalpy * lorentzFactor * lorentzFactor;

  // A permutation of the axes permutes the three shift terms, the three diagonal metric terms and the
  // three off-diagonal metric terms within their groups. Each group adds in order of magnitude. The
  // groups and the lapse term add in a fixed order. A permutation of the axes keeps both sums bitwise.
  double momentumShiftTerms[3];
  double energyShiftTerms[3];
  for (int i = 0; i < 3; i++) {
    const double kroneckerDelta = i == normal ? 1. : 0.;
    const double stressMixed    = scaledEnthalpy * vCov(i) * vContr(normal) - vCrossBCov(i) * vCrossBContr(normal)
                             - bCov(i) * bContr(normal) + (p + electromagneticEnergy) * kroneckerDelta;
    momentumShiftTerms[i] = -Q.vel(i) * deltaQ.shift(i);
    energyShiftTerms[i]   = -rootDeterminant * stressMixed * deltaQ.shift(i);
  }

  // The stress terms run over the six independent components of the symmetric metric.
  double momentumDiagonalTerms[3];
  double energyDiagonalTerms[3];
  double momentumOffDiagonalTerms[3];
  double energyOffDiagonalTerms[3];
  int    offDiagonal = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = i; j < 3; j++) {
      double stressContr = tarch::la::productInOrderOfMagnitude(scaledEnthalpy, vContr(i), vContr(j))
                         - vCrossBContr(i) * vCrossBContr(j) - bContr(i) * bContr(j)
                         + (p + electromagneticEnergy) * gContr(i, j);
      if (i != j) {
        stressContr = 2.0 * stressContr;
      }
      const double momentumTerm = -0.5 * rootDeterminant * lapse * stressContr * deltaQ.gij(i, j);
      const double energyTerm   = -0.5 * rootDeterminant * stressContr * shift(normal) * deltaQ.gij(i, j);
      if (i == j) {
        momentumDiagonalTerms[i] = momentumTerm;
        energyDiagonalTerms[i]   = energyTerm;
      } else {
        momentumOffDiagonalTerms[offDiagonal] = momentumTerm;
        energyOffDiagonalTerms[offDiagonal]   = energyTerm;
        offDiagonal++;
      }
    }
  }

  const auto sumOfGroup = [](const double (&terms)[3]) {
    return tarch::la::sumInOrderOfMagnitude(terms[0], terms[1], terms[2]);
  };
  BTimesDeltaQ.vel(normal) =
    ((sumOfGroup(momentumShiftTerms) + sumOfGroup(momentumDiagonalTerms)) + sumOfGroup(momentumOffDiagonalTerms))
    + (Q.E + Q.rho) * deltaQ.lapse;
  BTimesDeltaQ.E =
    ((sumOfGroup(energyShiftTerms) + sumOfGroup(energyDiagonalTerms)) + sumOfGroup(energyOffDiagonalTerms))
    + sContr(normal) * deltaQ.lapse;
  for (int i = 0; i < 3; i++) {
    BTimesDeltaQ.B(i) = lapse * rootDeterminant * gContr(i, normal) * deltaQ.psi - Q.shift(i) * deltaQ.B(normal);
  }
  BTimesDeltaQ.psi = inverseRootDeterminant * lapse * DivergenceCleaningSpeed * DivergenceCleaningSpeed
                       * deltaQ.B(normal)
                   - Q.shift(normal) * deltaQ.psi;
"""
