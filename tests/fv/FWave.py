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

  flux(QL, x, h, t, dt, normal, fluxLeft);
  flux(QR, x, h, t, dt, normal, fluxRight);

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

  // left state
  const auto rhoL                   = QL[Shortcuts::rho];
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

  assertion(rhoL > 0, rhoL);
  assertion(EL > 0, EL);
  assertion(pL > 0, pL);

  // right state
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

  assertion(rhoR > 0, rhoR);
  assertion(ER > 0, ER);
  assertion(pR > 0, pR);

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

  // in x-direction, these are uRoe - speedOfSoundRoe, DIMENSIONS x uRoe, and uRoe + speedOfSoundRoe
  tarch::la::Vector<NumberOfUnknowns, double> roeEigenvalues(0.0);

  // first eigenvalue is left uRoe - speedOfSoundRoe
  roeEigenvalues(0) = roeAveragedVelocities(normal) - speedOfSoundRoe;

  // DIMENSIONS x uRoe
  for (int eigenvalue = 1; eigenvalue <= DIMENSIONS; eigenvalue++) {
    roeEigenvalues(eigenvalue) = roeAveragedVelocities(normal);
  }

  // last eigenvalue is right uRoe + speedOfSoundRoe
  roeEigenvalues(DIMENSIONS + 1) = roeAveragedVelocities(normal) + speedOfSoundRoe;

  /***********************************************
   * Delta values from Bale et al. Equation 1.17 *
   **********************************************/

  tarch::la::Vector<NumberOfUnknowns, double> delta(0.0);

  for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
    delta(unknown) = fluxRight[unknown] - fluxLeft[unknown];
    delta(unknown) -= 0.5 * h(normal) * (sourceLeft[unknown] + sourceRight[unknown]);
  }

  /****************************************************************
   * Right Eigenvectors from LeVeque Chapter 18.8, Equation 18.48 *
   ***************************************************************/

  // @todo figure out for 3D
  // => github.com/clawpack/riemann/blob/master/src/rpn3_euler.f90

  const int transverse = normal == 0 ? 1 : 0;

  // first right Eigenvector
  tarch::la::Vector<NumberOfUnknowns, double> r1(0.0);
  r1(0)               = 1.0;
  r1(1 + normal)      = roeAveragedVelocities(normal) - speedOfSoundRoe;
  r1(1 + transverse)  = roeAveragedVelocities(transverse);
  r1(3)               = HRoe - roeAveragedVelocities(normal) * speedOfSoundRoe;

  // second right Eigenvector
  tarch::la::Vector<NumberOfUnknowns, double> r2(0.0);
  r2(0)               = 0.0;
  r2(1 + normal)      = 0.0;
  r2(1 + transverse)  = 1.0;
  r2(3)               = roeAveragedVelocities(transverse);

  // third right Eigenvector
  tarch::la::Vector<NumberOfUnknowns, double> r3(0.0);
  r3(0) = 1.0;
  r3(1) = roeAveragedVelocities(0);
  r3(2) = roeAveragedVelocities(1);
  r3(3) = qSquared;

  // fourth right Eigenvector
  tarch::la::Vector<NumberOfUnknowns, double> r4(0.0);
  r4(0)               = 1.0;
  r4(1 + normal)      = roeAveragedVelocities(normal) + speedOfSoundRoe;
  r4(1 + transverse)  = roeAveragedVelocities(transverse);
  r4(3)               = HRoe + roeAveragedVelocities(normal) * speedOfSoundRoe;

  /***************************************************
   * Beta values following Bale et al. Equation 1.14 *
   **************************************************/
  // inverted Eigenvectors taken from https://github.com/clawpack/riemann/blob/master/src/rpn2_euler_4wave.f90

  // @todo figure out for 3D
  tarch::la::Vector<NumberOfUnknowns, double> beta(0.0);

  beta(2) = (
              (HRoe - 2 * qSquared) * delta(0)
              + roeAveragedVelocities(normal) * delta(1 + normal)
              + roeAveragedVelocities(transverse) * delta(1 + transverse)
              - delta(3)
              ) / (HRoe - qSquared);

  beta(1) = delta(1 + transverse) - roeAveragedVelocities(transverse) * delta(0);

  beta(3) = (
              delta(1 + normal)
              + (speedOfSoundRoe - roeAveragedVelocities(normal)) * delta(0)
              - speedOfSoundRoe * beta(2)
              ) / (2 * speedOfSoundRoe);

  beta(0) = delta(0) - beta(2) - beta(3);

  /****************
   * fWave fluxes *
   ***************/

  const tarch::la::Vector<NumberOfUnknowns, double> fWaves[NumberOfUnknowns] = {
                                                                                  beta(0) * r1,
                                                                                  beta(1) * r2,
                                                                                  beta(2) * r3,
                                                                                  beta(3) * r4
                                                                                };

  for (int wave = 0; wave < NumberOfUnknowns; ++wave) {
    const auto eigenvalue = roeEigenvalues(wave);
    if (eigenvalue < 0.0) {
      // left going wave
      for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
        FL[unknown] += fWaves[wave](unknown);
      }
    } else {
      // right going wave
      for (int unknown = 0; unknown < NumberOfUnknowns; ++unknown) {
        FR[unknown] -= fWaves[wave](unknown);
      }
    }
  }

  return tarch::la::maxAbs(roeEigenvalues);
"""
