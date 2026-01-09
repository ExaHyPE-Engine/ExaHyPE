def get_max_eigenvalue():
    return """
      #if !defined(WITH_GPU)
      assertion1(Q[Shortcuts::rho] > 0, Q[Shortcuts::rho]);
      #endif
      
      const auto M                  = Q[Shortcuts::O2] + Q[Shortcuts::O] + Q[Shortcuts::N2];
      const auto iM                 = 1.0 / M;
      const auto meanMolecularMass  = (MOLAR_MASS_NITROGEN * Q[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * Q[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * Q[Shortcuts::O]) * iM; // in kg/mol
      const auto pressure           = Q[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * Q[Shortcuts::T] / meanMolecularMass;
      #if !defined(WITH_GPU)
      assertion5(pressure > 0.0, M, meanMolecularMass, Q[Shortcuts::rho], Q[Shortcuts::T], pressure);
      #endif
      
      const auto gamma  = (1.4 * (M - Q[Shortcuts::O]) + 1.67 * Q[Shortcuts::O]) * iM;

      const auto irho         = 1.0 / Q[Shortcuts::rho];
      const auto speedOfSound = sqrt(gamma * pressure * irho);
      const auto u_n          = Q[Shortcuts::rhou + normal] * irho;
      const auto result       = std::fmax(std::fabs(u_n - speedOfSound), std::fabs(u_n + speedOfSound));

      #if !defined(WITH_GPU)
      assertion3(result > 0.0, u_n, speedOfSound, result);
      #endif

      return result;"""


def get_flux():
    return """#if !defined(WITH_GPU)
      assertion1(Q[Shortcuts::rho] > 0, Q[Shortcuts::rho]);
      assertion1(Q[Shortcuts::E] > 0, Q[Shortcuts::E]);
      #endif

      const auto M                  = Q[Shortcuts::O2] + Q[Shortcuts::O] + Q[Shortcuts::N2];
      const auto iM                 = 1.0 / M;
      const auto meanMolecularMass  = (MOLAR_MASS_NITROGEN * Q[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * Q[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * Q[Shortcuts::O]) * iM; // in kg/mol
      const auto pressure           = Q[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * Q[Shortcuts::T] / meanMolecularMass;
      #if !defined(WITH_GPU)
      assertion5(pressure > 0.0, M, meanMolecularMass, Q[Shortcuts::rho], Q[Shortcuts::T], pressure);
      #endif

      const auto u_n = Q[Shortcuts::rhou + normal] / Q[Shortcuts::rho];

      // Euler flux
      F[Shortcuts::rho]   = Q[Shortcuts::rhou + normal];
      F[Shortcuts::rhou]  = Q[Shortcuts::rhou] * u_n;
      F[Shortcuts::rhov]  = Q[Shortcuts::rhov] * u_n;
      #if DIMENSIONS == 3
      F[Shortcuts::rhow]  = Q[Shortcuts::rhow] * u_n;
      #endif
      F[Shortcuts::E]     = u_n * (Q[Shortcuts::E] + pressure);

      auto pressurePerturbation = pressure - Q[Shortcuts::backgroundPressure];
      if (std::abs(pressurePerturbation) < PRESSURE_THRESHOLD) {
        pressurePerturbation = 0.0;
      }

      F[Shortcuts::rhou + normal] += pressurePerturbation;"""
