# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

eigenvalue = r"""
  if (Q[Shortcuts::h] <= hThreshold) {
    return 0.0;
  }

  // Wave speed estimation based on the flux Jacobian
  const auto Vn{Q[Shortcuts::hu + normal] / Q[Shortcuts::h]};
  const auto c{std::sqrt(g * Q[Shortcuts::h])};
  const auto sFlux{std::fmax(std::fabs(Vn + c), std::fabs(Vn - c))};
"""

stiff_eigenvalue = r"""
  // Wave speed estimation based on the stiff source
  const auto momentum{std::sqrt(Q[Shortcuts::hu] * Q[Shortcuts::hu] + Q[Shortcuts::hv] * Q[Shortcuts::hv])};
  const auto sSource{2.0 * g * momentum * invXi / (Q[Shortcuts::h] * Q[Shortcuts::h])};
"""

flux = r"""
  for (int n = 0; n < NumberOfUnknowns; n++) {
    F[n] = 0.0;
  }

  if (Q[Shortcuts::h] <= hThreshold) {
    return;
  }

  const auto Vn{Q[Shortcuts::hu + normal] / Q[Shortcuts::h]};
  F[Shortcuts::h]  = Q[Shortcuts::hu + normal]; // Mass flux
  F[Shortcuts::hu] = Vn * Q[Shortcuts::hu]; // Momentum-x flux
  F[Shortcuts::hv] = Vn * Q[Shortcuts::hv]; // Momentum-y flux
  F[Shortcuts::hu + normal] += 0.5 * g * Q[Shortcuts::h] * Q[Shortcuts::h]; // Hydrostatic pressure
"""

nonconservative_product = r"""
  for (int n = 0; n < NumberOfUnknowns; n++) {
    BTimesDeltaQ[n] = 0.0;
  }

  if (Q[Shortcuts::h] <= hThreshold) {
    return;
  }

  // Bathymetry/topography-related effects
  BTimesDeltaQ[Shortcuts::hu + normal] = g * Q[Shortcuts::h] * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::z]);
"""

stiff_nonconservative_product = r"""
  // Compute stiff turbulent drag on the interface
  const auto momentumSQR{Q[Shortcuts::hu] * Q[Shortcuts::hu] + Q[Shortcuts::hv] * Q[Shortcuts::hv]};
  const auto momentum{std::sqrt(momentumSQR)};
  if (tarch::la::greater(momentum / Q[Shortcuts::h], 0.0)) {
    const auto turbulentDrag{g * momentumSQR * invXi / (Q[Shortcuts::h] * Q[Shortcuts::h])};
    const auto directionVect{Q[Shortcuts::hu + normal] / momentum};
    BTimesDeltaQ[Shortcuts::hu + normal] += h[normal] * turbulentDrag * directionVect;
  }
"""

stiff_source_term_aderdg = r"""
  for (int n = 0; n < NumberOfUnknowns; n++) {
    S[n] = 0.0;
  }

  if (Q[Shortcuts::h] <= hThreshold) {
    return;
  }

  // Compute local slope cF
  const auto dzx{deltaQ[Shortcuts::z]};
  const auto dzy{deltaQ[Shortcuts::z + NumberOfUnknowns + NumberOfAuxiliaryVariables]};
  const auto cF{1.0 / std::sqrt(1.0 + dzx * dzx + dzy * dzy)};

  // Apply friction only if there is some movement
  const auto momentumSQR{Q[Shortcuts::hu] * Q[Shortcuts::hu] + Q[Shortcuts::hv] * Q[Shortcuts::hv]};
  const auto momentum{std::sqrt(momentumSQR)};
  if (tarch::la::greater(momentum / Q[Shortcuts::h], 0.0)) {
    const auto coulombFriction{mu * cF * Q[Shortcuts::h]};
    const auto turbulentDrag{momentumSQR * invXi / (Q[Shortcuts::h] * Q[Shortcuts::h])};
    const auto frictionTerm{-g * (coulombFriction + turbulentDrag) / momentum};
    S[Shortcuts::hu] = Q[Shortcuts::hu] * frictionTerm;
    S[Shortcuts::hv] = Q[Shortcuts::hv] * frictionTerm;
  }
"""

stiff_source_term_fv = r"""
  for (int n = 0; n < NumberOfUnknowns; n++) {
    S[n] = 0.0;
  }

  if (Q[Shortcuts::h] <= hThreshold) {
    return;
  }

  // Compute local slope cF
  const auto dzx{deltaQ[Shortcuts::z]};
  const auto dzy{deltaQ[Shortcuts::z + NumberOfUnknowns + NumberOfAuxiliaryVariables]};
  const auto cF{1.0 / std::sqrt(1.0 + dzx * dzx + dzy * dzy)};

  // Apply friction only if there is some movement
  const auto momentumSQR{Q[Shortcuts::hu] * Q[Shortcuts::hu] + Q[Shortcuts::hv] * Q[Shortcuts::hv]};
  const auto momentum{std::sqrt(momentumSQR)};
  if (tarch::la::greater(momentum / Q[Shortcuts::h], 0.0)) {
    const auto coulombFriction{mu * cF * Q[Shortcuts::h]};
    const auto frictionTerm{-g * coulombFriction / momentum};
    S[Shortcuts::hu] = Q[Shortcuts::hu] * frictionTerm;
    S[Shortcuts::hv] = Q[Shortcuts::hv] * frictionTerm;
  }
"""
