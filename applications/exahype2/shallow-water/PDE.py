# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

eigenvalue = r"""
  if (Q[s::h] <= hThreshold) {
    return 0.0;
  }

  // Wave speed estimation based on the flux Jacobian
  const auto Vn{Q[s::hu + normal] / Q[s::h]};
  const auto c{std::sqrt(g * Q[s::h])};
  const auto sFlux{std::fmax(std::fabs(Vn + c), std::fabs(Vn - c))};
"""

stiff_eigenvalue = r"""
  // Wave speed estimation based on the stiff source
  const auto momentum{std::sqrt(Q[s::hu] * Q[s::hu] + Q[s::hv] * Q[s::hv])};
  const auto sSource{2.0 * g * momentum * invXi / (Q[s::h] * Q[s::h])};
"""

flux = r"""
  for (int n = 0; n < NumberOfUnknowns; n++) {
    F[n] = 0.0;
  }

  if (Q[s::h] <= hThreshold) {
    return;
  }

  const auto Vn{Q[s::hu + normal] / Q[s::h]};
  F[s::h]  = Q[s::hu + normal]; // Mass flux
  F[s::hu] = Vn * Q[s::hu]; // Momentum-x flux
  F[s::hv] = Vn * Q[s::hv]; // Momentum-y flux
  F[s::hu + normal] += 0.5 * g * Q[s::h] * Q[s::h]; // Hydrostatic pressure
"""

nonconservative_product = r"""
  for (int n = 0; n < NumberOfUnknowns; n++) {
    BTimesDeltaQ[n] = 0.0;
  }

  if (Q[s::h] <= hThreshold) {
    return;
  }

  // Bathymetry/topography-related effects
  BTimesDeltaQ[s::hu + normal] = g * Q[s::h] * (deltaQ[s::h] + deltaQ[s::z]);
"""

stiff_nonconservative_product = r"""
  // Compute stiff turbulent drag on the interface
  const auto momentumSQR{Q[s::hu] * Q[s::hu] + Q[s::hv] * Q[s::hv]};
  const auto momentum{std::sqrt(momentumSQR)};
  if (tarch::la::greater(momentum / Q[s::h], 0.0)) {
    const auto turbulentDrag{g * momentumSQR * invXi / (Q[s::h] * Q[s::h])};
    const auto directionVect{Q[s::hu + normal] / momentum};
    BTimesDeltaQ[s::hu + normal] += h[normal] * turbulentDrag * directionVect;
  }
"""

stiff_source_term_aderdg = r"""
  for (int n = 0; n < NumberOfUnknowns; n++) {
    S[n] = 0.0;
  }

  if (Q[s::h] <= hThreshold) {
    return;
  }

  // Compute local slope cF
  const auto dzx{deltaQ[s::z]};
  const auto dzy{deltaQ[s::z + NumberOfUnknowns + NumberOfAuxiliaryVariables]};
  const auto cF{1.0 / std::sqrt(1.0 + dzx * dzx + dzy * dzy)};

  // Apply friction only if there is some movement
  const auto momentumSQR{Q[s::hu] * Q[s::hu] + Q[s::hv] * Q[s::hv]};
  const auto momentum{std::sqrt(momentumSQR)};
  if (tarch::la::greater(momentum / Q[s::h], 0.0)) {
    const auto coulombFriction{mu * cF * Q[s::h]};
    const auto turbulentDrag{momentumSQR * invXi / (Q[s::h] * Q[s::h])};
    const auto frictionTerm{-g * (coulombFriction + turbulentDrag) / momentum};
    S[s::hu] = Q[s::hu] * frictionTerm;
    S[s::hv] = Q[s::hv] * frictionTerm;
  }
"""

stiff_source_term_fv = r"""
  for (int n = 0; n < NumberOfUnknowns; n++) {
    S[n] = 0.0;
  }

  if (Q[s::h] <= hThreshold) {
    return;
  }

  // Compute local slope cF
  const auto dzx{deltaQ[s::z]};
  const auto dzy{deltaQ[s::z + NumberOfUnknowns + NumberOfAuxiliaryVariables]};
  const auto cF{1.0 / std::sqrt(1.0 + dzx * dzx + dzy * dzy)};

  // Apply friction only if there is some movement
  const auto momentumSQR{Q[s::hu] * Q[s::hu] + Q[s::hv] * Q[s::hv]};
  const auto momentum{std::sqrt(momentumSQR)};
  if (tarch::la::greater(momentum / Q[s::h], 0.0)) {
    const auto coulombFriction{mu * cF * Q[s::h]};
    const auto frictionTerm{-g * coulombFriction / momentum};
    S[s::hu] = Q[s::hu] * frictionTerm;
    S[s::hv] = Q[s::hv] * frictionTerm;
  }
"""
