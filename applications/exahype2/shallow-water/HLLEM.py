# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

hllem = r"""
  /**
   ****
   **** An new formulation of the HLLEM Riemann solver for the Shallow Water Equation with support for inundation
   ****
   * Literature:
   *
   *   @article{DUMBSER2016275,
   *            title = {A new efficient formulation of the HLLEM Riemann solver for general conservative and non-conservative hyperbolic systems},
   *            journal = {Journal of Computational Physics},
   *            volume = {304},
   *            pages = {275-319},
   *            year = {2016},
   *            issn = {0021-9991},
   *            doi = {https://doi.org/10.1016/j.jcp.2015.10.014},
   *            url = {https://www.sciencedirect.com/science/article/pii/S0021999115006786},
   *            author = {Michael Dumbser and Dinshaw S. Balsara}}
   *
   *   @article{KURGANOV,
   *            title = {A Second-Order Well-Balanced Positivity Preserving Central-Upwind Scheme for the Saint-Venant System},
   *            year = {2007},
   *            month = {03},
   *            pages = {},
   *            volume = {5},
   *            journal = {Communications in mathematical sciences},
   *            author = {Kurganov, Alexander and Petrova, G.},
   *            doi = {10.4310/CMS.2007.v5.n1.a6}}
   ****
   */

  const double hLeft   = QL[s::h];
  const double hRight  = QR[s::h];
  const double huLeft  = QL[s::hu];
  const double huRight = QR[s::hu];
  const double hvLeft  = QL[s::hv];
  const double hvRight = QR[s::hv];
  const double bLeft   = QL[s::b];
  const double bRight  = QR[s::b];

  // Compute dry tolerance according to KURGANOV. Does not work!
  //const double dryTol = std::pow(h[0], 4);
  const double dryTol = 1e-3;

  const double cLeft  = std::sqrt(GRAV * hLeft);
  const double cRight = std::sqrt(GRAV * hRight);

  // Correction of particle velocities according to KURGANOV (modified version):
  const double uCorrectedLeft  = huLeft  * hLeft  * std::sqrt(2) / std::sqrt(std::pow(hLeft,  4) + std::pow(std::fmax(hLeft,  dryTol), 4));
  const double uCorrectedRight = huRight * hRight * std::sqrt(2) / std::sqrt(std::pow(hRight, 4) + std::pow(std::fmax(hRight, dryTol), 4));

  const double vCorrectedLeft  = hvLeft  * hLeft  * std::sqrt(2) / std::sqrt(std::pow(hLeft,  4) + std::pow(std::fmax(hLeft,  dryTol), 4));
  const double vCorrectedRight = hvRight * hRight * std::sqrt(2) / std::sqrt(std::pow(hRight, 4) + std::pow(std::fmax(hRight, dryTol), 4));

  // Implementation following Eq. (2.17) in KURGANOV does not work:
  // const double uCorrectedLeft  = huLeft  * hLeft  * std::sqrt(2) / std::sqrt(std::pow(hLeft,  4) + std::fmax(std::pow(hLeft,  4), dryTol));
  // const double uCorrectedRight = huRight * hRight * std::sqrt(2) / std::sqrt(std::pow(hRight, 4) + std::fmax(std::pow(hRight, 4), dryTol));

  // const double vCorrectedLeft  = hvLeft  * hLeft  * std::sqrt(2) / std::sqrt(std::pow(hLeft,  4) + std::fmax(std::pow(hLeft,  4), dryTol));
  // const double vCorrectedRight = hvRight * hRight * std::sqrt(2) / std::sqrt(std::pow(hRight, 4) + std::fmax(std::pow(hRight, 4), dryTol));

  const double eigenvaluesLeft[3] {
    normal == 0 ? uCorrectedLeft + cLeft : vCorrectedLeft + cLeft,
    normal == 0 ? uCorrectedLeft - cLeft : vCorrectedLeft - cLeft,
    normal == 0 ? uCorrectedLeft         : vCorrectedLeft
  };
  const double eigenvaluesRight[3] {
    normal == 0 ? uCorrectedRight + cRight : vCorrectedRight + cRight,
    normal == 0 ? uCorrectedRight - cRight : vCorrectedRight - cRight,
    normal == 0 ? uCorrectedRight          : vCorrectedRight
  };

  double maxWaveSpeed = 0.0;
  for (int i = 0; i < 3; i++) {
    const double absEigenvalueLeft = std::fabs(eigenvaluesLeft[i]);
    maxWaveSpeed = std::fmax(absEigenvalueLeft, maxWaveSpeed);
  }
  for (int i = 0; i < 3; i++) {
    const double absEigenvalueRight = std::fabs(eigenvaluesRight[i]);
    maxWaveSpeed = std::fmax(absEigenvalueRight, maxWaveSpeed);
  }

  const double fluxLeft[3] {
    normal == 0 ? hLeft * uCorrectedLeft                  : hLeft * vCorrectedLeft,
    normal == 0 ? hLeft * uCorrectedLeft * uCorrectedLeft : hLeft * vCorrectedLeft * uCorrectedLeft,
    normal == 0 ? hLeft * uCorrectedLeft * vCorrectedLeft : hLeft * vCorrectedLeft * vCorrectedLeft
  };

  const double fluxRight[3] {
    normal == 0 ? hRight * uCorrectedRight                   : hRight * vCorrectedRight,
    normal == 0 ? hRight * uCorrectedRight * uCorrectedRight : hRight * vCorrectedRight * uCorrectedRight,
    normal == 0 ? hRight * uCorrectedRight * vCorrectedRight : hRight * vCorrectedRight * vCorrectedRight
  };

  const double bMax     = std::fmax(bLeft, bRight);
  const double deltaEta = std::fmax(hRight + bRight - bMax, 0.0) - std::fmax(hLeft + bLeft - bMax, 0.0);
  const double hRoe     = 0.5 * (hLeft + hRight);

  double dJump[3] = {0.0};
  dJump[s::hu + normal] = 0.5 * GRAV * hRoe * deltaEta;

  const double netUpdates[3] {
    0.5 * (fluxLeft[s::h]  + fluxRight[s::h])  - 0.5 * maxWaveSpeed * deltaEta,
    0.5 * (fluxLeft[s::hu] + fluxRight[s::hu]) - 0.5 * maxWaveSpeed * (huRight - huLeft),
    0.5 * (fluxLeft[s::hv] + fluxRight[s::hv]) - 0.5 * maxWaveSpeed * (hvRight - hvLeft)
  };

  for (int i = 0; i < 3; i++){
    FL[i] = netUpdates[i] + dJump[i];
    FR[i] = netUpdates[i] - dJump[i];
  }

  if (QL[s::h] < dryTol) {
    FL[s::hu] = FL[s::hv] = 0.0;
  }
  if (QR[s::h] < dryTol) {
    FR[s::hu] = FR[s::hv] = 0.0;
  }

  return maxWaveSpeed;
"""
