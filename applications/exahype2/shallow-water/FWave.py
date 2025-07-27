# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

fWave = r"""
  /**
   ****
   **** F-Wave Riemann Solver for the Shallow Water Equation
   ****
   * Note:
   * This solver does not support inundation. For dry cells, we assume an infinitely high solid wall
   * at the shore-line, which is equivalent to reflecting boundary conditions. Thus no water is
   * able to pass the shoreline, and we have to assure that the particle velocity at the boundary is
   * zero.
   *
   * Literature:
   *
   *   @article{bale2002wave,
   *            title={A wave propagation method for conservation laws and balance laws with spatially varying flux functions},
   *            author={Bale, D.S. and LeVeque, R.J. and Mitran, S. and Rossmanith, J.A.}, journal={SIAM Journal on Scientific Computing},
   *            volume={24},
   *            number={3},
   *            pages={955--978},
   *            year={2002},
   *            publisher={Citeseer}}
   *
   *   @book{leveque2002finite,
   *         Author = {LeVeque, R. J.},
   *         Date-Added = {2011-09-13 14:09:31 +0000},
   *         Date-Modified = {2011-10-31 09:46:40 +0000},
   *         Publisher = {Cambridge University Press},
   *         Title = {Finite Volume Methods for Hyperbolic Problems},
   *         Volume = {31},
   *         Year = {2002}}
   *
   *   @webpage{levequeclawpack,
   *            Author = {LeVeque, R. J.},
   *            Lastchecked = {January, 05, 2011},
   *            Title = {Clawpack Sofware},
   *            Url = {https://github.com/clawpack/clawpack-4.x/blob/master/geoclaw/2d/lib}}
   ****
   */

  enum class WetDryState {
    DryDry,               /**< Both cells are dry. */
    WetWet,               /**< Both cells are wet. */
    WetDryInundation,     /**< 1st cell: wet, 2nd cell: dry. 1st cell lies higher than the 2nd one. */
    WetDryWall,           /**< 1st cell: wet, 2nd cell: dry. 1st cell lies lower than the 2nd one. Momentum is not large enough to overcome the difference. */
    WetDryWallInundation, /**< 1st cell: wet, 2nd cell: dry. 1st cell lies lower than the 2nd one. Momentum is large enough to overcome the difference. */
    DryWetInundation,     /**< 1st cell: dry, 2nd cell: wet. 1st cell lies lower than the 2nd one. */
    DryWetWall,           /**< 1st cell: dry, 2nd cell: wet. 1st cell lies higher than the 2nd one. Momentum is not large enough to overcome the difference. */
    DryWetWallInundation  /**< 1st cell: dry, 2nd cell: wet. 1st cell lies higher than the 2nd one. Momentum is large enough to overcome the difference. */
  };

  // Set speeds to zero (will be determined later)
  double uLeft = 0.0;
  double uRight = 0.0;

  // Not const here as they may get reassigned due to boundary conditions
  double hLeft = QL[Shortcuts::h];
  double hRight = QR[Shortcuts::h];
  double huLeft = QL[Shortcuts::hu + normal];
  double huRight = QR[Shortcuts::hu + normal];
  double bLeft = QL[Shortcuts::z];
  double bRight = QR[Shortcuts::z];

  //! Wet/dry state of our Riemann-problem
  WetDryState wetDryState = WetDryState::WetWet; // Assume everything is wet

  // Determine the wet/dry state
  if (hLeft <= hThreshold && hRight <= hThreshold) { // Both cells are dry
    wetDryState = WetDryState::DryDry;
  } else if (hLeft <= hThreshold) { // Left cell dry, right cell wet
    uRight = huRight / hRight;
    // Set wall boundary conditions (inundation is not supported!)
    hLeft = hRight;
    bLeft = bRight;
    huLeft = -huRight;
    uLeft = -uRight;
    wetDryState = WetDryState::DryWetWall;
  } else if (hRight <= hThreshold) { // Left cell wet, right cell dry
    uLeft = huLeft / hLeft;
    hRight = hLeft;
    bRight = bLeft;
    huRight = -huLeft;
    uRight = -uLeft;
    wetDryState = WetDryState::WetDryWall;
  } else {
    uLeft  = huLeft / hLeft;
    uRight = huRight / hRight;
    wetDryState = WetDryState::WetWet;
  }

  // Zero updates in the beginning
  for (int i = 0; i < 3; i++) {
    FL[i] = 0.0;
    FR[i] = 0.0;
  }

  // Return in the case of dry cells
  if (wetDryState == WetDryState::DryDry) {
    return 0.0;
  }

  // Compute eigenvalues of the Jacobian matrices in states Q_{i-1} and Q_{i}
  const double hRoe = 0.5 * (hRight + hLeft);
  const double uRoe = (uLeft * std::sqrt(hLeft) + uRight * std::sqrt(hRight)) / (std::sqrt(hLeft) + std::sqrt(hRight));
  const double roeSpeeds[2] = {uRoe - std::sqrt(g * hRoe), uRoe + std::sqrt(g * hRoe)};
  const double characteristicSpeeds[2] = {uLeft - std::sqrt(g * hLeft), uRight + std::sqrt(g * hRight)};
  const double einfeldtSpeeds[2] = {std::fmin(characteristicSpeeds[0], roeSpeeds[0]), std::fmax(characteristicSpeeds[1], roeSpeeds[1])};

  // Set wave speeds (lambda Roe)
  //! Wave speeds of the f-waves
  const double waveSpeeds[2] {einfeldtSpeeds[0], einfeldtSpeeds[1]};
  const double deltaLambda = waveSpeeds[1] - waveSpeeds[0];

#ifdef GPUOffloadingOff
  // Wave speed of the 1st wave family should be less than the speed of the 2nd wave family.
  assertion(waveSpeeds[0] < waveSpeeds[1]);
  assertion(std::fabs(deltaLambda) > 0); // Make sure we do not divide by 0
#endif

  const double inverseDeltaLambda = 1.0 / deltaLambda;

  // Compute the inverse matrix R^{-1} to obtain Eigencoefficients (alpha)
  //
  // 1                   1             ^-1
  //
  // \lambda_{1}         \lambda_{2}
  //
  // Inverse: https://mathworld.wolfram.com/MatrixInverse.html
  //
  //       1          \lambda_{2}       -1
  //  ____________
  //   deltaLambda   -\lambda_{1}        1
  const double Rinv[2][2] {
    { inverseDeltaLambda *  waveSpeeds[1], -inverseDeltaLambda },
    { inverseDeltaLambda * -waveSpeeds[0],  inverseDeltaLambda }
  };

  // Right hand side
  const double fluxLeft[2] {huLeft, huLeft * uLeft + 0.5 * g * hLeft * hLeft};
  const double fluxRight[2] {huRight, huRight * uRight + 0.5 * g * hRight * hRight};

  // Add source term (bathymetry) into the decomposition of the jump in the flux function
  const double deltaxPsi = -g * (bRight - bLeft) * hRoe;
  // Calculate flux difference f(Q_i) - f(Q_{i-1})
  const double deltaFlux[2] {fluxRight[0] - fluxLeft[0], fluxRight[1] - fluxLeft[1] - deltaxPsi};

  // Solve linear equations
  const double alpha[2] {
    Rinv[0][0] * deltaFlux[0] + Rinv[0][1] * deltaFlux[1],
    Rinv[1][0] * deltaFlux[0] + Rinv[1][1] * deltaFlux[1]
  };

  // Z
  const double fWaves[2][2] {
    { alpha[0], alpha[0] * waveSpeeds[0] },
    { alpha[1], alpha[1] * waveSpeeds[1] }
  };

  // Compute the net-updates
  double hUpdateLeft = 0.0;
  double hUpdateRight = 0.0;
  double huUpdateLeft = 0.0;
  double huUpdateRight = 0.0;

  constexpr double ZERO_TOL = std::numeric_limits<double>::epsilon();
  // 1st wave family
  if (waveSpeeds[0] < ZERO_TOL) { // Left going
    hUpdateLeft += fWaves[0][0];
    huUpdateLeft += fWaves[0][1];
  } else if (waveSpeeds[0] > ZERO_TOL) { // Right going
    hUpdateRight -= fWaves[0][0];
    huUpdateRight -= fWaves[0][1];
  } else { // Split waves
    hUpdateLeft += 0.5 * fWaves[0][0];
    huUpdateLeft += 0.5 * fWaves[0][1];
    hUpdateRight -= 0.5 * fWaves[0][0];
    huUpdateRight -= 0.5 * fWaves[0][1];
  }

  // 2nd wave family
  if (waveSpeeds[1] < ZERO_TOL) { // Left going
    hUpdateLeft += fWaves[1][0];
    huUpdateLeft += fWaves[1][1];
  } else if (waveSpeeds[1] > ZERO_TOL) { // Right going
    hUpdateRight -= fWaves[1][0];
    huUpdateRight -= fWaves[1][1];
  } else { // Split waves
    hUpdateLeft += 0.5 * fWaves[1][0];
    huUpdateLeft += 0.5 * fWaves[1][1];
    hUpdateRight -= 0.5 * fWaves[1][0];
    huUpdateRight -= 0.5 * fWaves[1][1];
  }

  // Zero ghost updates (wall boundary)
  if (wetDryState == WetDryState::WetDryWall) {
    hUpdateRight = 0;
    huUpdateRight = 0;
  } else if (wetDryState == WetDryState::DryWetWall) {
    hUpdateLeft = 0;
    huUpdateLeft = 0;
  }

  // Compute maximum wave speed (-> CFL-condition)
  const double maxWaveSpeed = std::fmax(std::fabs(waveSpeeds[0]), std::fabs(waveSpeeds[1]));

#ifdef GPUOffloadingOff
  assertion(!std::isnan(hUpdateLeft));
  assertion(!std::isnan(hUpdateRight));
  assertion(!std::isnan(huUpdateLeft));
  assertion(!std::isnan(huUpdateRight));
  assertion(!std::isnan(maxWaveSpeed));
#endif

  // Set net-updates
  FL[Shortcuts::h] = hUpdateLeft;
  FR[Shortcuts::h] = hUpdateRight;
  FL[normal + 1] = huUpdateLeft;
  FR[normal + 1] = huUpdateRight;

  return maxWaveSpeed;
"""
