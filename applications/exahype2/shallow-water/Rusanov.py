# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

rusanov_aderdg = r"""
  // Compute average left and right states
  const auto N{NumberOfUnknowns + NumberOfAuxiliaryVariables};
  double QavgL[N] = {};
  double QavgR[N] = {};
  for (auto xy = 0; xy < Order + 1; ++xy) {
    for (auto i = 0; i < N; ++i) {
      QavgL[i] += kernels::{{SOLVER_NAME}}::Quadrature<double>::weights2[xy] * QL[xy * N + i];
      QavgR[i] += kernels::{{SOLVER_NAME}}::Quadrature<double>::weights2[xy] * QR[xy * N + i];
    }
  }

  // Wave speed estimation based on the flux Jacobian
  auto sL{0.0};
  if (QavgL[Shortcuts::h] > hThreshold) {
    const auto VnL{QavgL[Shortcuts::hu + direction] / QavgL[Shortcuts::h]};
    const auto cL{std::sqrt(g * QavgL[Shortcuts::h])};
    sL = std::fmax(std::fabs(VnL - cL), std::fabs(VnL + cL));
  }
  auto sR{0.0};
  if (QavgR[Shortcuts::h] > hThreshold) {
    const auto VnR{QavgR[Shortcuts::hu + direction] / QavgR[Shortcuts::h]};
    const auto cR{std::sqrt(g * QavgR[Shortcuts::h])};
    sR = std::fmax(std::fabs(VnR - cR), std::fabs(VnR + cR));
  }
  const auto smax{std::fmax(sR, sL)};

  auto j{0};
  auto rusanovFlux{0.0};
  for (auto xy = 0; xy < Order + 1; ++xy) {
    for (auto i = 0; i < N; ++i) {
      j           = xy * N + i;
      rusanovFlux = 0.5 * (FL[j] + FR[j] + smax * (QL[j] - QR[j]));
      FL[j]       = rusanovFlux;
      FR[j]       = rusanovFlux;
    }
    // Ignore flux for flow topography
    j     = xy * N + Shortcuts::z;
    FL[j] = 0.0;
    FR[j] = 0.0;
  }
"""

rusanov_fv = r"""
  flux(QL, x, h, t, dt, normal, FL);
  flux(QR, x, h, t, dt, normal, FR);

  const auto smax{std::fmax(maxEigenvalue(QL, x, h, t, dt, normal), maxEigenvalue(QR, x, h, t, dt, normal))};
  const auto N{NumberOfAuxiliaryVariables + NumberOfUnknowns};

  double Qavg[N] = {};
  double DeltaQ[N] = {};
  for (int i = 0; i < NumberOfUnknowns; ++i) {
    Qavg[i]   = 0.5 * (QL[i] + QR[i]);
    DeltaQ[i] = QR[i] - QL[i];
    FL[i]     = 0.5 * (FL[i] + FR[i] - smax * DeltaQ[i]);
    FR[i]     = FL[i];
  }

  for (int i = NumberOfUnknowns; i < N; ++i) {
    Qavg[i]   = 0.5 * (QL[i] + QR[i]);
    DeltaQ[i] = QR[i] - QL[i];
  }

  double ncp[NumberOfUnknowns] = {};
  nonconservativeProduct(Qavg, DeltaQ, x, h, t, dt, normal, ncp);
  for (int i = 0; i < NumberOfUnknowns; ++i) {
    FL[i] += 0.5 * ncp[i];
    FR[i] -= 0.5 * ncp[i];
  }

  auto sL{0.0};
  if (QL[Shortcuts::h] > hThreshold) {
    const auto hL{QL[Shortcuts::h] > hThreshold ? QL[Shortcuts::h] : hThreshold};
    const auto momentumL{std::sqrt(QL[Shortcuts::hu] * QL[Shortcuts::hu] + QL[Shortcuts::hv] * QL[Shortcuts::hv])};
    sL = 2.0 * g * momentumL * invXi / (hL * hL);
  }

  auto sR{0.0};
  if (QL[Shortcuts::h] > hThreshold) {
    const auto hR{QR[Shortcuts::h] > hThreshold ? QR[Shortcuts::h] : hThreshold};
    const auto momentumR{std::sqrt(QR[Shortcuts::hu] * QR[Shortcuts::hu] + QR[Shortcuts::hv] * QR[Shortcuts::hv])};
    sR = 2.0 * g * momentumR * invXi / (hR * hR);
  }

  return std::fmax(smax, h[normal] * std::fmax(sL, sR));
"""
