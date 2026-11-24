# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

fWave = r"""
  /*******************************
   * Flux and source preparation *
   ******************************/

  double fluxRight[NumberOfUnknowns] {0.0};
  double fluxLeft[NumberOfUnknowns] {0.0};
  double sourceRight[NumberOfUnknowns] {0.0};
  double sourceLeft[NumberOfUnknowns] {0.0};

  flux(QL, x, h, t, dt, normal, fluxLeft, parameters);
  flux(QR, x, h, t, dt, normal, fluxRight, parameters);

  if (normal == 1) {
    sourceLeft[Shortcuts::rhoU + 1] = -GRAVITY * QL[Shortcuts::rho];
    sourceLeft[Shortcuts::rhoE] = -GRAVITY * QL[Shortcuts::rhoU + 1];

    sourceRight[Shortcuts::rhoU + 1] = -GRAVITY * QR[Shortcuts::rho];
    sourceRight[Shortcuts::rhoE] = -GRAVITY * QR[Shortcuts::rhoU + 1];
  }

  std::fill_n(FL, NumberOfUnknowns, 0.0);
  std::fill_n(FR, NumberOfUnknowns, 0.0);

  /********************
   * State extraction *
   *******************/

  const auto rhoL                  = QL[Shortcuts::rho];
  const auto sqrtRhoL               = sqrt(rhoL);
  const auto uL                     = QL[Shortcuts::rhoU + 0] / rhoL;
  const auto vL                     = QL[Shortcuts::rhoU + 1] / rhoL;
  #if DIMENSIONS == 3
  const auto wL                     = QL[Shortcuts::rhoU + 2] / rhoL;
  const auto uSqL                   = uL * uL + vL * vL + wL * wL;
  #else
  const auto uSqL                   = uL * uL + vL * vL;
  #endif
  const auto EL                     = QL[Shortcuts::rhoE];
  const auto pL                     = (GAMMA - 1.0) * (EL - 0.5 * rhoL * uSqL);

  ASSERTION(rhoL > 0, rhoL);
  ASSERTION(EL > 0, EL);
  ASSERTION(pL > 0, pL);

  const auto rhoR                   = QR[Shortcuts::rho];
  const auto sqrtRhoR               = sqrt(rhoR);
  const auto uR                     = QR[Shortcuts::rhoU + 0] / rhoR;
  const auto vR                     = QR[Shortcuts::rhoU + 1] / rhoR;
  #if DIMENSIONS == 3
  const auto wR                     = QR[Shortcuts::rhoU + 2] / rhoR;
  const auto uSqR                   = uR * uR + vR * vR + wR * wR;
  #else
  const auto uSqR                   = uR * uR + vR * vR;
  #endif
  const auto ER                     = QR[Shortcuts::rhoE];
  const auto pR                     = (GAMMA - 1.0) * (ER - 0.5 * rhoR * uSqR);

  ASSERTION(rhoR > 0, rhoR);
  ASSERTION(ER > 0, ER);
  ASSERTION(pR > 0, pR);

  /********************************************
   * Roe averages from LeVeque Chapter 15.3.4 *
   *******************************************/

  const auto iSumSqrtRho  = 1.0 / (sqrtRhoL + sqrtRhoR);

  const auto gammaAvg = GAMMA;
  const auto HRoe     = (
                          (pR + ER) / sqrtRhoR
                          +
                          (pL + EL) / sqrtRhoL
                        ) * iSumSqrtRho;

  tarch::la::Vector<DIMENSIONS, double> roeAveragedVelocities(0.0);
  roeAveragedVelocities(0)    = (sqrtRhoL * uL + sqrtRhoR * uR) * iSumSqrtRho;
  roeAveragedVelocities(1)    = (sqrtRhoL * vL + sqrtRhoR * vR) * iSumSqrtRho;
  #if DIMENSIONS == 3
  roeAveragedVelocities(2)    = (sqrtRhoL * wL + sqrtRhoR * wR) * iSumSqrtRho;
  #endif
  const auto qSquared         = 0.5 * (roeAveragedVelocities * roeAveragedVelocities);
  const auto speedOfSoundRoe  = sqrt((gammaAvg - 1) * (HRoe - qSquared));

  /*************************************************************
   * Roe Eigenvalues from LeVeque Chapter 18.8, Equation 18.47 *
   ************************************************************/

  tarch::la::Vector<NumberOfUnknowns, double> roeEigenvalues(0.0);

  roeEigenvalues(0) = roeAveragedVelocities(normal) - speedOfSoundRoe;

  for (int eigenvalue = 1; eigenvalue <= DIMENSIONS; eigenvalue++) {
    roeEigenvalues(eigenvalue) = roeAveragedVelocities(normal);
  }

  roeEigenvalues(DIMENSIONS + 1) = roeAveragedVelocities(normal) + speedOfSoundRoe;

  /***********************************************
   * Delta values from Bale et al. Equation 1.17 *
   **********************************************/

  tarch::la::Vector<NumberOfUnknowns, double> delta(0.0);

  for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
    delta(unknown) = fluxRight[unknown] - fluxLeft[unknown];
    delta(unknown) -= 0.5 * h(normal) * (sourceLeft[unknown] + sourceRight[unknown]);
  }

  /************************************************************************************
   * Wave strengths following Bale et al. Equation 1.14 with the inverted eigenvectors *
   * of https://github.com/clawpack/riemann/blob/master/src/rpn3_euler.f90             *
   ***********************************************************************************/

  constexpr int Energy = DIMENSIONS + 1;
  const auto normalVelocity = roeAveragedVelocities(normal);

  auto entropyStrength = (HRoe - 2 * qSquared) * delta(0) + normalVelocity * delta(1 + normal);
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    const int transverse = (normal + axis) % DIMENSIONS;
    entropyStrength += roeAveragedVelocities(transverse) * delta(1 + transverse);
  }
  entropyStrength = (entropyStrength - delta(Energy)) / (HRoe - qSquared);

  const auto rightAcousticStrength = (
                                       delta(1 + normal)
                                       + (speedOfSoundRoe - normalVelocity) * delta(0)
                                       - speedOfSoundRoe * entropyStrength
                                     ) / (2 * speedOfSoundRoe);
  const auto leftAcousticStrength = delta(0) - entropyStrength - rightAcousticStrength;

  /****************************************************************************************
   * fWaves from the right eigenvectors of LeVeque Chapter 18.8, Equation 18.48. The waves *
   * follow roeEigenvalues: The left acoustic wave, one shear wave per transverse axis, the *
   * entropy wave and the right acoustic wave.                                              *
   ***************************************************************************************/

  tarch::la::Vector<NumberOfUnknowns, double> fWaves[NumberOfUnknowns];
  for (int wave = 0; wave < NumberOfUnknowns; ++wave) {
    fWaves[wave] = tarch::la::Vector<NumberOfUnknowns, double>(0.0);
  }

  fWaves[0](0)              = leftAcousticStrength;
  fWaves[DIMENSIONS](0)     = entropyStrength;
  fWaves[DIMENSIONS + 1](0) = rightAcousticStrength;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    fWaves[0](1 + axis)              = leftAcousticStrength * roeAveragedVelocities(axis);
    fWaves[DIMENSIONS](1 + axis)     = entropyStrength * roeAveragedVelocities(axis);
    fWaves[DIMENSIONS + 1](1 + axis) = rightAcousticStrength * roeAveragedVelocities(axis);
  }
  fWaves[0](1 + normal)              = leftAcousticStrength * (normalVelocity - speedOfSoundRoe);
  fWaves[DIMENSIONS + 1](1 + normal) = rightAcousticStrength * (normalVelocity + speedOfSoundRoe);
  fWaves[0](Energy)                  = leftAcousticStrength * (HRoe - normalVelocity * speedOfSoundRoe);
  fWaves[DIMENSIONS](Energy)         = entropyStrength * qSquared;
  fWaves[DIMENSIONS + 1](Energy)     = rightAcousticStrength * (HRoe + normalVelocity * speedOfSoundRoe);

  for (int axis = 1; axis < DIMENSIONS; axis++) {
    const int  transverse    = (normal + axis) % DIMENSIONS;
    const auto shearStrength = delta(1 + transverse) - roeAveragedVelocities(transverse) * delta(0);
    fWaves[axis](1 + transverse) = shearStrength;
    fWaves[axis](Energy)         = shearStrength * roeAveragedVelocities(transverse);
  }

  for (int wave = 0; wave < NumberOfUnknowns; ++wave) {
    const auto eigenvalue = roeEigenvalues(wave);
    if (eigenvalue < 0.0) {
      for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
        FL[unknown] += fWaves[wave](unknown);
      }
    } else {
      for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
        FR[unknown] -= fWaves[wave](unknown);
      }
    }
  }

  return tarch::la::maxAbs(roeEigenvalues);
"""
