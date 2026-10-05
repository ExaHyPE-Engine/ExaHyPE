// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "PDE.h"

namespace GRMHD {
  namespace InitialData {
    // Writes the lapse, shift and metric of a rotating black hole in Cartesian Kerr-Schild coordinates at x,
    // following section 11.4 of De Felice and Clarke.
    inline void metric(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double*                                      lapse,
      double*                                      gp,
      double*                                      gm,    // size 1
      double*                                      shift, // size 3
      double**                                     Kex,
      double                                       g_cov[][3],
      double                                       g_contr[][3], // all size 3x3
      double*                                      phi
    ) {
      constexpr double Spin = 0.0;
      constexpr double Mbh  = 1.0;

      phi[0] = 1.0;

      double z2          = x[2] * x[2];
      double spinSquared = Spin * Spin;

      double halfDifference = 0.5 * (x[0] * x[0] + x[1] * x[1] + x[2] * x[2] - spinSquared);
      double r              = std::sqrt(halfDifference + std::sqrt(halfDifference * halfDifference + z2 * spinSquared));
      double r2             = r * r;

      double HH = Mbh * r2 * r / (r2 * r2 + spinSquared * z2);
      double SS = 1.0 + 2.0 * HH;

      double lx = (r * x[0] + Spin * x[1]) / (r2 + spinSquared);
      double ly = (r * x[1] - Spin * x[0]) / (r2 + spinSquared);
      double lz = x[2] / r;

      lapse[0] = 1.0 / std::sqrt(SS);
      shift[0] = 2.0 * HH / SS * lx;
      shift[1] = 2.0 * HH / SS * ly;
      shift[2] = 2.0 * HH / SS * lz;

      g_cov[0][0] = 1.0 + 2.0 * HH * lx * lx;
      g_cov[0][1] = 2.0 * HH * lx * ly;
      g_cov[0][2] = 2.0 * HH * lx * lz;

      g_cov[1][0] = 2.0 * HH * lx * ly;
      g_cov[1][1] = 1.0 + 2.0 * HH * ly * ly;
      g_cov[1][2] = 2.0 * HH * ly * lz;

      g_cov[2][0] = 2.0 * HH * lx * lz;
      g_cov[2][1] = 2.0 * HH * ly * lz;
      g_cov[2][2] = 1.0 + 2.0 * HH * lz * lz;

      g_contr[0][0] = (1.0 + 2.0 * HH * ly * ly + 2.0 * HH * lz * lz) / SS;
      g_contr[0][1] = (-2.0 * HH * lx * ly) / SS;
      g_contr[0][2] = (-2.0 * HH * lx * lz) / SS;

      g_contr[1][0] = (-2.0 * HH * lx * ly) / SS;
      g_contr[1][1] = (1.0 + 2.0 * HH * lx * lx + 2.0 * HH * lz * lz) / SS;
      g_contr[1][2] = (-2.0 * HH * ly * lz) / SS;

      g_contr[2][0] = (-2.0 * HH * lx * lz) / SS;
      g_contr[2][1] = (-2.0 * HH * ly * lz) / SS;
      g_contr[2][2] = (1.0 + 2.0 * HH * lx * lx + 2.0 * HH * ly * ly) / SS;

      gp[0] = std::sqrt(SS);
      gm[0] = 1.0 / gp[0];
    }

    // Writes the stationary Michel accretion onto a black hole at x, as described in
    // https://doi.org/10.1016/j.cpc.2020.107251.
    inline void initialAccretionDisc3D(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const double                                 t,
      double* const                                Q
    ) {
      constexpr double Gamma = 4.0 / 3.0;

      double rhoc = 0.0625;
      double rc   = 8.0;

      int MaxNewton = 50;

      double ng = 1.0 / (Gamma - 1.0);

      double lapse, gp, gm, phi;
      double shift[3];
      double g_cov[3][3], g_contr[3][3];

      metric(x, &lapse, &gp, &gm, shift, nullptr, g_cov, g_contr, &phi);

      double r = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);

      double V[19];
      if (r < 0.8) {
        // This state inside the black hole avoids a division by zero, and no domain reaches it.
        using Shortcuts         = tests::exahype2::aderdg::VariableShortcutsADERDGSolver;
        V[Shortcuts::rho]       = 1;
        V[Shortcuts::vel + 0]   = 0.;
        V[Shortcuts::vel + 1]   = 0.;
        V[Shortcuts::vel + 2]   = 0.;
        V[Shortcuts::E]         = 1.;
        V[Shortcuts::B + 0]     = 0.;
        V[Shortcuts::B + 1]     = 0.;
        V[Shortcuts::B + 2]     = 0.;
        V[Shortcuts::psi]       = 0.;
        V[Shortcuts::lapse]     = 1.;
        V[Shortcuts::shift + 0] = 0.;
        V[Shortcuts::shift + 1] = 0.;
        V[Shortcuts::shift + 2] = 0.;
        V[Shortcuts::gij + 0]   = 1.;
        V[Shortcuts::gij + 1]   = 0.;
        V[Shortcuts::gij + 2]   = 0.;
        V[Shortcuts::gij + 3]   = 1.;
        V[Shortcuts::gij + 4]   = 0.;
        V[Shortcuts::gij + 5]   = 1.;

        PDE::PDEPrim2Cons(Q, V);

        return;
      }

      double theta  = acos(x[2] / r);
      phi           = atan2(x[1], x[0]);
      double zz     = 2.0 / r;
      double betaru = zz / (1.0 + zz);

      double urc = sqrt(1.0 / (2.0 * rc));
      double vc2 = urc * urc / (1.0 - 3.0 * urc * urc);
      double tc  = ng * vc2 / ((1.0 + ng) * (1.0 - ng * vc2));

      double c1 = urc * std::pow(tc, ng) * (rc * rc);
      double c2 = (1.0 + (1.0 + ng) * tc) * (1.0 + (1.0 + ng) * tc) * (1.0 - 2.0 / rc + urc * urc);

      double tt = tc;
      double urr;
      for (int i = 0; i < MaxNewton; i++) {
        urr       = c1 / (r * r * std::pow(tt, ng));
        double f  = (1.0 + (1.0 + ng) * tt) * (1.0 + (1.0 + ng) * tt) * (1.0 - 2.0 / r + urr * urr) - c2;
        double df = 2.0 * (1.0 + ng) * (1.0 + (1.0 + ng) * tt) * (1.0 - 2.0 / r + urr * urr)
          - 2.0 * ng * urr * urr / tt * (1.0 + (1.0 + ng) * tt) * (1.0 + (1.0 + ng) * tt);
        double dt = -f / df;
        if (std::abs(dt) < 1.e-10) {
          break;
        }
        tt = tt + dt;
      }

      double ut = (-zz * urr + sqrt(urr * urr - zz + 1.0)) / (zz - 1.0);
      double LF = lapse * ut;
      double vr = (urr / LF + betaru / lapse);

      double vx = sin(theta) * cos(phi) * vr;
      double vy = sin(theta) * sin(phi) * vr;
      double vz = cos(theta) * vr;

      double VV[3] = {vx, vy, vz};

      double VV_cov[3];
      PDE::matrixVectMult(g_cov, VV, VV_cov);

      // The Newton iteration solves for the temperature tt = p / rho.
      double rho = rhoc * std::pow(tt / tc, ng);
      double p   = rho * tt;

      double gammaij[6] = {g_cov[0][0], g_cov[0][1], g_cov[0][2], g_cov[1][1], g_cov[1][2], g_cov[2][2]};

      double B0          = 0;
      double BV_contr[3] = {
        B0 * x[0] / (sqrt(gp) * r * r * r),
        B0 * x[1] / (sqrt(gp) * r * r * r),
        B0 * x[2] / (sqrt(gp) * r * r * r)
      };

      double BV[3];
      PDE::matrixVectMult(g_cov, BV_contr, BV);

      using Shortcuts         = tests::exahype2::aderdg::VariableShortcutsADERDGSolver;
      V[Shortcuts::rho]       = rho;
      V[Shortcuts::vel + 0]   = VV_cov[0];
      V[Shortcuts::vel + 1]   = VV_cov[1];
      V[Shortcuts::vel + 2]   = VV_cov[2];
      V[Shortcuts::E]         = p;
      V[Shortcuts::B + 0]     = BV[0];
      V[Shortcuts::B + 1]     = BV[1];
      V[Shortcuts::B + 2]     = BV[2];
      V[Shortcuts::psi]       = 0.;
      V[Shortcuts::lapse]     = lapse;
      V[Shortcuts::shift + 0] = shift[0];
      V[Shortcuts::shift + 1] = shift[1];
      V[Shortcuts::shift + 2] = shift[2];
      V[Shortcuts::gij + 0]   = gammaij[0];
      V[Shortcuts::gij + 1]   = gammaij[1];
      V[Shortcuts::gij + 2]   = gammaij[2];
      V[Shortcuts::gij + 3]   = gammaij[3];
      V[Shortcuts::gij + 4]   = gammaij[4];
      V[Shortcuts::gij + 5]   = gammaij[5];

      PDE::PDEPrim2Cons(Q, V);
    }

    inline void AlfvenWaveTest(const tarch::la::Vector<DIMENSIONS, double>& x, const double t, double* const Q) {
      constexpr double Gamma = 4.0 / 3.0;
      constexpr double Rho0  = 1.;
      constexpr double P0    = 1.;
      constexpr double Eta   = 1.;
      constexpr double B0    = 1.;

      double hh     = 1.0 + Gamma / (Gamma - 1.) * P0 / Rho0;
      double tempaa = Rho0 * hh + B0 * B0 * (1.0 + Eta * Eta);
      double tempab = 2.0 * Eta * B0 * B0 / tempaa;
      double tempac = 0.5 * (1.0 + sqrt(1.0 - tempab * tempab));

      double va2 = B0 * B0 / (tempaa * tempac);
      double vax = sqrt(va2);

      using Shortcuts = tests::exahype2::aderdg::VariableShortcutsADERDGSolver;
      double V[19];

      V[Shortcuts::rho] = Rho0;

      V[Shortcuts::B + 0] = B0;
      V[Shortcuts::B + 1] = Eta * B0 * cos(x[0] - vax * t);
      V[Shortcuts::B + 2] = Eta * B0 * sin(x[0] - vax * t);

      V[Shortcuts::vel + 0] = 0.0;
      V[Shortcuts::vel + 1] = -vax * V[Shortcuts::B + 1] / B0;
      V[Shortcuts::vel + 2] = -vax * V[Shortcuts::B + 2] / B0;
      V[Shortcuts::E]       = P0;

      V[Shortcuts::psi]       = 0.;
      V[Shortcuts::lapse]     = 1.0;
      V[Shortcuts::shift + 0] = 0.;
      V[Shortcuts::shift + 1] = 0.;
      V[Shortcuts::shift + 2] = 0.;

      V[Shortcuts::gij + 0] = 1.;
      V[Shortcuts::gij + 1] = 0.;
      V[Shortcuts::gij + 2] = 0.;
      V[Shortcuts::gij + 3] = 1.;
      V[Shortcuts::gij + 4] = 0.;
      V[Shortcuts::gij + 5] = 1.;

      PDE::PDEPrim2Cons(Q, V);
    }
  } // namespace InitialData
} // namespace GRMHD
