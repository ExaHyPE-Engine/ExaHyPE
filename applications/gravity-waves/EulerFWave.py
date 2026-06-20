"""
This solver is based on 
"A Wave Propagation Method for Conservation Laws and Balance Laws with Spatially Varying Flux Functions", Bale et al., 2003, doi:10.1137/S106482750139738X
and
"Finite Volume Methods for Hyperbolic Equations", Randall J. LeVeque, 2002, doi:10.1017/CBO9780511791253
"""

fWave = """
      /*******************************
      * Flux and source preparation *
      ******************************/

      double fluxRight[NumberOfUnknowns] {0.0};
      double fluxLeft[NumberOfUnknowns] {0.0};
      double deltaXPsi[NumberOfUnknowns] {0.0};

      tarch::la::Vector<DIMENSIONS, double> xL(x);
      tarch::la::Vector<DIMENSIONS, double> xR(x);
      xR[normal] += 0.5 * h(normal);
      xL[normal] -= 0.5 * h(normal);

      flux(QL, xL, h, t, dt, normal, fluxLeft);
      flux(QR, xR, h, t, dt, normal, fluxRight);

      if (normal == 1) {
        auto densityPerturbationLeft = QL[Shortcuts::rho] - QL[Shortcuts::backgroundDensity];
        auto densityPerturbationRight = QR[Shortcuts::rho] - QR[Shortcuts::backgroundDensity];

        if (std::abs(densityPerturbationLeft) < DENSITY_THRESHOLD) {
          densityPerturbationLeft = 0.0;
        }

        if (std::abs(densityPerturbationRight) < DENSITY_THRESHOLD) {
          densityPerturbationRight = 0.0;
        }

        deltaXPsi[Shortcuts::rhov]  = 0.5 * h(normal) * -GRAVITY * (densityPerturbationLeft + densityPerturbationRight);
        deltaXPsi[Shortcuts::E]     = 0.5 * h(normal) * -GRAVITY * (QL[Shortcuts::rhov] + QR[Shortcuts::rhov]);
      }

      std::fill_n(FL, NumberOfUnknowns, 0.0);
      std::fill_n(FR, NumberOfUnknowns, 0.0);

      /********************
      * State extraction *
      *******************/ 

      // left state
      const auto rhoL     = QL[Shortcuts::rho];
      const auto sqrtRhoL = sqrt(rhoL);
      const auto uL       = QL[Shortcuts::rhou] / rhoL;
      const auto vL       = QL[Shortcuts::rhov] / rhoL;
      #if DIMENSIONS == 3
      const auto wL       = QL[Shortcuts::rhow] / rhoL;
      #endif
      const auto EL       = QL[Shortcuts::E];

      assertion(rhoL > 0, rhoL);
      assertion(EL > 0, EL);

      const auto ML                 = QL[Shortcuts::O2] + QL[Shortcuts::O] + QL[Shortcuts::N2];
      const auto iML                = 1.0 / ML;
      const auto meanMolecularMassL = (MOLAR_MASS_NITROGEN * QL[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * QL[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * QL[Shortcuts::O]) * iML; // in kg/mol
      const auto pressureL          = QL[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * QL[Shortcuts::T] / meanMolecularMassL;
      assertion(pressureL > 0.0, ML, meanMolecularMassL, QL[Shortcuts::rho], QL[Shortcuts::T], pressureL);
      
      const auto gammaL = (1.4 * (ML - QL[Shortcuts::O]) + 1.67 * QL[Shortcuts::O]) * iML;
      const auto HL     = (EL + pressureL) / QL[Shortcuts::rho];

      // right state
      const auto rhoR     = QR[Shortcuts::rho];
      const auto sqrtRhoR = sqrt(rhoR);
      const auto uR       = QR[Shortcuts::rhou] / rhoR;
      const auto vR       = QR[Shortcuts::rhov] / rhoR;
      #if DIMENSIONS == 3
      const auto wR       = QR[Shortcuts::rhow] / rhoR;
      #endif
      const auto ER       = QR[Shortcuts::E];

      assertion(rhoR > 0, rhoR);
      assertion(ER > 0, ER);

      const auto MR                 = QR[Shortcuts::O2] + QR[Shortcuts::O] + QR[Shortcuts::N2];
      const auto iMR                = 1.0 / MR;
      const auto meanMolecularMassR = (MOLAR_MASS_NITROGEN * QR[Shortcuts::N2] + MOLAR_MASS_MOLECULAR_OXYGEN * QR[Shortcuts::O2] + MOLAR_MASS_ATOMIC_OXYGEN * QR[Shortcuts::O]) * iMR; // in kg/mol
      const auto pressureR          = QR[Shortcuts::rho] * UNIVERSAL_GAS_CONSTANT * QR[Shortcuts::T] / meanMolecularMassR;
      assertion(pressureR > 0.0, MR, meanMolecularMassR, QR[Shortcuts::rho], QR[Shortcuts::T], pressureR);
      
      const auto gammaR = (1.4 * (MR - QR[Shortcuts::O]) + 1.67 * QR[Shortcuts::O]) * iMR;
      const auto HR     = (ER + pressureR) / QR[Shortcuts::rho];
      
      /********************************************
      * Roe averages from LeVeque Chapter 15.3.4 *
      *******************************************/

      const auto iSumSqrtRho  = 1 / (sqrtRhoL + sqrtRhoR);

      const auto gammaAvg = 0.5 * (gammaL + gammaR);
      const auto HRoe     = (HL * sqrtRhoL + HR * sqrtRhoR) * iSumSqrtRho;

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
        delta(unknown) = fluxRight[unknown] - fluxLeft[unknown] - deltaXPsi[unknown];
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

      // return largest absolute Eigenvalue
      return tarch::la::maxAbs(roeEigenvalues);"""
