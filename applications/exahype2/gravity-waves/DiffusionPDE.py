stencil = """// calculate laplacian of velocities
      tarch::la::Vector<DIMENSIONS, double> velocityLaplacian(0.0);
      for (int velocity = 0; velocity < DIMENSIONS; ++velocity) {
        velocityLaplacian(velocity) += (
          (QRight0[Shortcuts::u + velocity] - QRight0[Shortcuts::backgroundU + velocity])
          - 2 * (Q[Shortcuts::u + velocity] - Q[Shortcuts::backgroundU + velocity])
          + (QLeft0[Shortcuts::u + velocity] - QLeft0[Shortcuts::backgroundU + velocity])
        ) / (h(0) * h(0));
      }

      for (int velocity = 0; velocity < DIMENSIONS; ++velocity) {
        velocityLaplacian(velocity) += (
          (QTop0[Shortcuts::u + velocity] - QTop0[Shortcuts::backgroundU + velocity])
          - 2 * (Q[Shortcuts::u + velocity] - Q[Shortcuts::backgroundU + velocity])
          + (QDown0[Shortcuts::u + velocity] - QDown0[Shortcuts::backgroundU + velocity])
        ) / (h(1) * h(1));
      }

      #if DIMENSIONS == 3
      for (int velocity = 0; velocity < DIMENSIONS; ++velocity) {
        velocityLaplacian(velocity) += (
          (QFront0[Shortcuts::u + velocity] - QFront0[Shortcuts::backgroundU + velocity])
          - 2 * (Q[Shortcuts::u + velocity] - Q[Shortcuts::backgroundU + velocity])
          + (QBack0[Shortcuts::u + velocity] - QBack0[Shortcuts::backgroundU + velocity])
        ) / (h(2) * h(2));
      }
      #endif

      for (int velocity = 0; velocity < DIMENSIONS; ++velocity) {
        if (std::fabs(velocityLaplacian(velocity)) < VELOCITY_THRESHOLD) {
          velocityLaplacian(velocity) = 0.0;
        }
      }

      QNew[Shortcuts::u] = Q[Shortcuts::u] + dt * Q[Shortcuts::nu] * velocityLaplacian(Shortcuts::u);
      QNew[Shortcuts::v] = Q[Shortcuts::v] + dt * Q[Shortcuts::nu] * velocityLaplacian(Shortcuts::v);
      #if DIMENSIONS == 3
      QNew[Shortcuts::w] = Q[Shortcuts::w] + dt * Q[Shortcuts::nu] * velocityLaplacian(Shortcuts::w);
      #endif

      // calculate temperature derivative
      auto temperaturePerturbation = [](const double* Q) {
        //calculate temperature based on energy density
        const auto uSquared = Q[Shortcuts::u] * Q[Shortcuts::u];
        const auto vSquared = Q[Shortcuts::v] * Q[Shortcuts::v];
        #if DIMENSIONS == 3
        const auto wSquared = Q[Shortcuts::w] * Q[Shortcuts::w];
        const auto squaredVelocities = uSquared + vSquared + wSquared;
        #else
        const auto squaredVelocities = uSquared + vSquared;
        #endif
        const auto internalEnergy    = Q[Shortcuts::E] - 0.5 * (Q[Shortcuts::rho] * squaredVelocities);
        const auto M = Q[Shortcuts::O2] + Q[Shortcuts::O] + Q[Shortcuts::N2];
        const auto iM = 1.0 / M;
        const auto gamma  = (1.4 * (M - Q[Shortcuts::O]) + 1.67 * Q[Shortcuts::O]) * iM;
        const auto meanMolecularMass  = (MOLAR_MASS_NITROGEN * Q[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * Q[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * Q[Shortcuts::O]) * iM; // in kg/mol
        const auto temperature = (internalEnergy * (gamma - 1.0) * meanMolecularMass) / ( Q[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT);

        // take derivative only of temperature perturbation
        return temperature - Q[Shortcuts::backgroundT];
      }; 

      double temperatureLaplacian = 0.0;
      const auto doubleTemperaturePerturbationQ = 2 * temperaturePerturbation(Q);

      // x direction
      temperatureLaplacian += (
        temperaturePerturbation(QRight0) 
        - doubleTemperaturePerturbationQ
        + temperaturePerturbation(QLeft0)
      ) / (h(0) * h(0)); 
      
      // y direction
      temperatureLaplacian += (
        temperaturePerturbation(QTop0) 
        - doubleTemperaturePerturbationQ
        + temperaturePerturbation(QDown0)
      ) / (h(1) * h(1)); 

      #if DIMENSIONS == 3
      // z direction
      temperatureLaplacian += (
        temperaturePerturbation(QFront0) 
        - doubleTemperaturePerturbationQ
        + temperaturePerturbation(QBack0)
      ) / (h(2) * h(2)); 
      #endif

      if (std::fabs(temperatureLaplacian) <  TEMPERATURE_THRESHOLD) {
        temperatureLaplacian = 0.0;
      }

      QNew[Shortcuts::T] = Q[Shortcuts::T] + dt * Q[Shortcuts::kappa] * temperatureLaplacian;

      // compute new pressure based on updated temperature
      const auto M = Q[Shortcuts::O2] + Q[Shortcuts::O] + Q[Shortcuts::N2];
      const auto iM = 1.0 / M;
      const auto gamma  = (1.4 * (M - Q[Shortcuts::O]) + 1.67 * Q[Shortcuts::O]) * iM;
      const auto meanMolecularMass  = (MOLAR_MASS_NITROGEN * Q[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * Q[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * Q[Shortcuts::O]) * iM; // in kg/mol

      const auto pressureNew = Q[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * QNew[Shortcuts::T] / meanMolecularMass;

      // compute new energy based on new pressure
      #if DIMENSIONS == 3
      const auto vSqNew = QNew[Shortcuts::u] * QNew[Shortcuts::u] + QNew[Shortcuts::v] * QNew[Shortcuts::v] + QNew[Shortcuts::w] * QNew[Shortcuts::w];
      #else
      const auto vSqNew = QNew[Shortcuts::u] * QNew[Shortcuts::u] + QNew[Shortcuts::v] * QNew[Shortcuts::v];
      #endif

      QNew[Shortcuts::E] = pressureNew / (gamma - 1.0) + 0.5 * Q[Shortcuts::rho] * vSqNew;

      return;"""
