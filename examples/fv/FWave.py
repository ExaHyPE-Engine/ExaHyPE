# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

fWave = r"""
  // The f-wave solver splits the flux difference minus the source into Roe waves.
  Flux fluxRight{};
  Flux fluxLeft{};
  Flux sourceRight{};
  Flux sourceLeft{};

  flux(QL, x, h, t, dt, normal, fluxLeft, parameters);
  flux(QR, x, h, t, dt, normal, fluxRight, parameters);

  if (normal == 1) {
    sourceLeft.rhoU(1) = -GRAVITY * QL.rho;
    sourceLeft.rhoE    = -GRAVITY * QL.rhoU(1);

    sourceRight.rhoU(1) = -GRAVITY * QR.rho;
    sourceRight.rhoE    = -GRAVITY * QR.rhoU(1);
  }

  FL = {};
  FR = {};

  const auto rhoL     = QL.rho;
  const auto sqrtRhoL = sqrt(rhoL);
  const auto uL       = QL.rhoU(0) / rhoL;
  const auto vL       = QL.rhoU(1) / rhoL;
  #if DIMENSIONS == 3
  const auto wL       = QL.rhoU(2) / rhoL;
  const auto uSqL     = uL * uL + vL * vL + wL * wL;
  #else
  const auto uSqL     = uL * uL + vL * vL;
  #endif
  const auto EL       = QL.rhoE;
  const auto pL       = (GAMMA - 1.0) * (EL - 0.5 * rhoL * uSqL);

  ASSERTION(rhoL > 0, rhoL);
  ASSERTION(EL > 0, EL);
  ASSERTION(pL > 0, pL);

  const auto rhoR     = QR.rho;
  const auto sqrtRhoR = sqrt(rhoR);
  const auto uR       = QR.rhoU(0) / rhoR;
  const auto vR       = QR.rhoU(1) / rhoR;
  #if DIMENSIONS == 3
  const auto wR       = QR.rhoU(2) / rhoR;
  const auto uSqR     = uR * uR + vR * vR + wR * wR;
  #else
  const auto uSqR     = uR * uR + vR * vR;
  #endif
  const auto ER       = QR.rhoE;
  const auto pR       = (GAMMA - 1.0) * (ER - 0.5 * rhoR * uSqR);

  ASSERTION(rhoR > 0, rhoR);
  ASSERTION(ER > 0, ER);
  ASSERTION(pR > 0, pR);

  // Roe averages from LeVeque, chapter 15.3.4.
  const auto iSumSqrtRho = 1.0 / (sqrtRhoL + sqrtRhoR);

  const auto gammaAvg = GAMMA;
  const auto HRoe     = (
                          (pR + ER) / sqrtRhoR
                          +
                          (pL + EL) / sqrtRhoL
                        ) * iSumSqrtRho;

  tarch::la::Vector<DIMENSIONS, double> roeAveragedVelocities(0.0);
  roeAveragedVelocities(0)   = (sqrtRhoL * uL + sqrtRhoR * uR) * iSumSqrtRho;
  roeAveragedVelocities(1)   = (sqrtRhoL * vL + sqrtRhoR * vR) * iSumSqrtRho;
  #if DIMENSIONS == 3
  roeAveragedVelocities(2)   = (sqrtRhoL * wL + sqrtRhoR * wR) * iSumSqrtRho;
  #endif
  const auto qSquared        = 0.5 * (roeAveragedVelocities * roeAveragedVelocities);
  const auto speedOfSoundRoe = sqrt((gammaAvg - 1) * (HRoe - qSquared));

  // Roe eigenvalues from LeVeque, chapter 18.8, equation 18.47, one per wave.
  tarch::la::Vector<NumberOfUnknowns, double> roeEigenvalues(0.0);

  roeEigenvalues(0) = roeAveragedVelocities(normal) - speedOfSoundRoe;

  for (int eigenvalue = 1; eigenvalue <= DIMENSIONS; eigenvalue++) {
    roeEigenvalues(eigenvalue) = roeAveragedVelocities(normal);
  }

  roeEigenvalues(DIMENSIONS + 1) = roeAveragedVelocities(normal) + speedOfSoundRoe;

  // Delta values from Bale et al., equation 1.17.
  const double halfWidth = 0.5 * h(normal);
  const Flux   delta     = transform(
    fluxRight - fluxLeft,
    sourceLeft + sourceRight,
    [&](const double difference, const double sourceSum) { return difference - halfWidth * sourceSum; }
  );

  // Wave strengths following Bale et al., equation 1.14, with the inverted eigenvectors of
  // https://github.com/clawpack/riemann/blob/master/src/rpn3_euler.f90.
  const auto normalVelocity = roeAveragedVelocities(normal);

  auto entropyStrength = (HRoe - 2 * qSquared) * delta.rho + normalVelocity * delta.rhoU(normal);
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    const int transverse = (normal + axis) % DIMENSIONS;
    entropyStrength += roeAveragedVelocities(transverse) * delta.rhoU(transverse);
  }
  entropyStrength = (entropyStrength - delta.rhoE) / (HRoe - qSquared);

  const auto rightAcousticStrength = (
                                       delta.rhoU(normal)
                                       + (speedOfSoundRoe - normalVelocity) * delta.rho
                                       - speedOfSoundRoe * entropyStrength
                                     ) / (2 * speedOfSoundRoe);
  const auto leftAcousticStrength = delta.rho - entropyStrength - rightAcousticStrength;

  // The f-waves follow the right eigenvectors of LeVeque, chapter 18.8, equation 18.48, in the order
  // of roeEigenvalues: The left acoustic wave, one shear wave per transverse axis, the entropy wave
  // and the right acoustic wave.
  Flux fWaves[NumberOfUnknowns] = {};

  fWaves[0].rho              = leftAcousticStrength;
  fWaves[DIMENSIONS].rho     = entropyStrength;
  fWaves[DIMENSIONS + 1].rho = rightAcousticStrength;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    fWaves[0].rhoU(axis)              = leftAcousticStrength * roeAveragedVelocities(axis);
    fWaves[DIMENSIONS].rhoU(axis)     = entropyStrength * roeAveragedVelocities(axis);
    fWaves[DIMENSIONS + 1].rhoU(axis) = rightAcousticStrength * roeAveragedVelocities(axis);
  }
  fWaves[0].rhoU(normal)              = leftAcousticStrength * (normalVelocity - speedOfSoundRoe);
  fWaves[DIMENSIONS + 1].rhoU(normal) = rightAcousticStrength * (normalVelocity + speedOfSoundRoe);
  fWaves[0].rhoE                      = leftAcousticStrength * (HRoe - normalVelocity * speedOfSoundRoe);
  fWaves[DIMENSIONS].rhoE             = entropyStrength * qSquared;
  fWaves[DIMENSIONS + 1].rhoE         = rightAcousticStrength * (HRoe + normalVelocity * speedOfSoundRoe);

  for (int axis = 1; axis < DIMENSIONS; axis++) {
    const int  transverse         = (normal + axis) % DIMENSIONS;
    const auto shearStrength      = delta.rhoU(transverse) - roeAveragedVelocities(transverse) * delta.rho;
    fWaves[axis].rhoU(transverse) = shearStrength;
    fWaves[axis].rhoE             = shearStrength * roeAveragedVelocities(transverse);
  }

  for (int wave = 0; wave < NumberOfUnknowns; ++wave) {
    if (roeEigenvalues(wave) < 0.0) {
      FL = FL + fWaves[wave];
    } else {
      FR = FR - fWaves[wave];
    }
  }

  return tarch::la::maxAbs(roeEigenvalues);
"""
