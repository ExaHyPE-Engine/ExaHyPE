// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"

TARGET_DECLARE
namespace GRMHD {
  namespace PDE {
    using Shortcuts = tests::exahype2::aderdg::ADERDGSolverVariableShortcuts;

    // Multiplies a 3x3 matrix with a vector of size 3.
    inline GPU_CALLABLE_METHOD void matrixVectMult(const double M[][3], const double VIn[3], double VOut[3]) {
      VOut[0] = M[0][0] * VIn[0] + M[0][1] * VIn[1] + M[0][2] * VIn[2];
      VOut[1] = M[1][0] * VIn[0] + M[1][1] * VIn[1] + M[1][2] * VIn[2];
      VOut[2] = M[2][0] * VIn[0] + M[2][1] * VIn[1] + M[2][2] * VIn[2];
    }

    // Computes the inverse of a 3x3 matrix and returns its determinant
    inline GPU_CALLABLE_METHOD double matrixInverse3x3(double M[][3], double iM[][3]) {
      double det = M[0][0] * M[1][1] * M[2][2] - M[0][0] * M[1][2] * M[2][1] - M[1][0] * M[0][1] * M[2][2]
        + M[1][0] * M[0][2] * M[2][1] + M[2][0] * M[0][1] * M[1][2] - M[2][0] * M[0][2] * M[1][1]; // ComputeDet(M,3)

      iM[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) / det;
      iM[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / det;
      iM[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / det;
      iM[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) / det;
      iM[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / det;
      iM[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) / det;
      iM[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) / det;
      iM[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) / det;
      iM[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) / det;

      return det;
    }

    inline GPU_CALLABLE_METHOD void FUNC_C2P_RMHD1(
      double  x,
      double* f,
      double* df,
      double  gam,
      double  d,
      double  e,
      double  s2,
      double  b2,
      double  sb2,
      double* w
    ) {
      // The root function follows choice 3 of section 3.2 in Del Zanna et al. (2007), A&A 473, 11-30.
      constexpr double Third       = 1. / 3.;
      constexpr double InverseOf27 = 1. / 27.;

      double v2  = x;
      double rho = d * sqrt(1. - v2);
      double c3  = 1. - gam * (1. - v2);
      double c2  = gam * rho + .5 * b2 * (1 + v2) - e;
      double c0  = -0.5 * sb2;

      double c2ic3 = c2 / c3;
      if (std::abs(c0) < 1.0e-20) {
        w[0] = -c2ic3;
      } else {
        double c2ic33  = c2ic3 * c2ic3 * c2ic3;
        double q       = c0 / c3 + 2.0 * InverseOf27 * c2ic33;
        double p       = -Third * c2ic3 * c2ic3;
        double sqrtdet = std::sqrt(0.25 * q * q + InverseOf27 * p * p * p);
        w[0]           = -Third * c2ic3 + std::pow(-0.5 * q + sqrtdet, Third) + std::pow(-0.5 * q - sqrtdet, Third);
      }

      double dc3 = gam;
      double dc2 = 0.5 * (b2 - gam * rho / (1.0 - v2));

      double dlogw = (dc3 * w[0] + dc2) / (3.0 * c3 * w[0] + 2.0 * c2);
      double wb    = w[0] + b2;
      double vb2   = sb2 / (w[0] * w[0]);
      f[0]         = wb * wb * v2 - (2.0 * w[0] + b2) * vb2 - s2;
      df[0]        = wb * (wb + 2.0 * dlogw * (w[0] * v2 + vb2));
    }

    inline GPU_CALLABLE_METHOD void RTSAFE_C2P_RMHD1(
      double* v2,
      double* X1,
      double* X2,
      double* XACC,
      double* w,
      double  gam,
      double  d,
      double  e,
      double  s2,
      double  b2,
      double  sb2,
      bool*   failed
    ) {
      double xacc2 = 1e-60;
      failed[0]    = false;

      double FL, FH, DF;

      FUNC_C2P_RMHD1(*X1, &FL, &DF, gam, d, e, s2, b2, sb2, w);

      if (std::abs(FL) < xacc2) {
        v2[0] = X1[0];
        return;
      }

      FUNC_C2P_RMHD1(*X2, &FH, &DF, gam, d, e, s2, b2, sb2, w);

      if (std::abs(FH) < xacc2) {
        v2[0] = X2[0];
        return;
      }

      if (FL * FH > 0.) {
        failed[0] = true;
        v2[0]     = 0.;
        return;
      }

      double XL, XH, SWAP;

      if (FL < 0.) {
        XL = X1[0];
        XH = X2[0];
      } else {
        XH   = X1[0];
        XL   = X2[0];
        SWAP = FL;
        FL   = FH;
        FH   = SWAP;
      }

      v2[0]        = .5 * (X1[0] + X2[0]);
      double dxOld = std::abs(X2[0] - X1[0]);
      double dx    = dxOld;

      double F;
      FUNC_C2P_RMHD1(v2[0], &F, &DF, gam, d, e, s2, b2, sb2, w);

      constexpr int MaxIt = 400;
      for (int j = 0; j < MaxIt; j++) {
        if ((((v2[0] - XH) * DF - F) * ((v2[0] - XL) * DF - F) > 0.) or (std::abs(2 * F) > std::abs(dxOld * DF))) {
          dxOld = dx;
          dx    = 0.5 * (XH - XL);
          v2[0] = XL + dx;
          // The midpoint of a bracket that spans two adjacent doubles rounds onto one of its ends.
          if (XL == v2[0] or XH == v2[0]) {
            return;
          }
        } else {
          dxOld       = dx;
          dx          = F / DF;
          double TEMP = v2[0];
          v2[0]       = v2[0] - dx;
          if (TEMP == v2[0]) {
            return;
          }
        }

        if (std::abs(dx) < XACC[0]) {
          return;
        }

        FUNC_C2P_RMHD1(v2[0], &F, &DF, gam, d, e, s2, b2, sb2, w);

        if (F < 0) {
          XL = v2[0];
          FL = F;
        } else {
          XH = v2[0];
          FH = F;
        }

        if (std::abs(F) < xacc2) {
          return;
        }
      } // for j

      failed[0] = true;
      v2[0]     = 0.;
      return;
    }

    // Converts the primitive variables V into the conserved variables Q.
    inline GPU_CALLABLE_METHOD void PDEPrim2Cons(double* Q, const double* V) {
      constexpr double AdiabaticIndex = 4.0 / 3.0;

      double rho        = V[0];
      double v_cov[3]   = {V[1], V[2], V[3]};
      double p          = V[4];
      double B_contr[3] = {V[5], V[6], V[7]};

      // The six metric components fill the symmetric 3x3 matrix.
      double g_cov[3][3] = {{V[13], V[14], V[15]}, {V[14], V[16], V[17]}, {V[15], V[17], V[18]}};

      double g_contr[3][3];
      double gp = matrixInverse3x3(g_cov, g_contr);
      gp        = std::sqrt(gp);

      double gm = 1. / gp;

      double v_contr[3];
      matrixVectMult(g_cov, v_cov, v_contr);

      double B_cov[3];
      matrixVectMult(g_cov, B_contr, B_cov);

      double vxB_cov[3] = {
        gp * (v_contr[1] * B_contr[2] - v_contr[2] * B_contr[1]),
        gp * (v_contr[2] * B_contr[0] - v_contr[0] * B_contr[2]),
        gp * (v_contr[0] * B_contr[1] - v_contr[1] * B_contr[0])
      };

      double vxB_contr[3] = {
        gm * (v_cov[1] * B_cov[2] - v_cov[2] * B_cov[1]),
        gm * (v_cov[2] * B_cov[0] - v_cov[0] * B_cov[2]),
        gm * (v_cov[0] * B_cov[1] - v_cov[1] * B_cov[0])
      };

      double vb = v_cov[0] * B_contr[0] + v_cov[1] * B_contr[1] + v_cov[2] * B_contr[2];
      double v2 = v_contr[0] * v_cov[0] + v_contr[1] * v_cov[1] + v_contr[2] * v_cov[2];
      double e2 = vxB_contr[0] * vxB_cov[0] + vxB_contr[1] * vxB_cov[1] + vxB_contr[2] * vxB_cov[2];
      double b2 = B_contr[0] * B_cov[0] + B_contr[1] * B_cov[1] + B_contr[2] * B_cov[2];

      double uem = 0.5 * (b2 + e2);

      double lf     = 1.0 / sqrt(1.0 - v2);
      double gamma1 = AdiabaticIndex / (AdiabaticIndex - 1.0);
      double w      = rho + gamma1 * p;
      double ww     = w * lf * lf;

      Q[Shortcuts::rho]     = gp * rho * lf;
      Q[Shortcuts::vel + 0] = gp * ww * v_cov[0] + b2 * v_cov[0] - vb * B_cov[0];
      Q[Shortcuts::vel + 1] = gp * ww * v_cov[1] + b2 * v_cov[1] - vb * B_cov[1];
      Q[Shortcuts::vel + 2] = gp * ww * v_cov[2] + b2 * v_cov[2] - vb * B_cov[2];
      Q[Shortcuts::E]       = gp * ww - p + uem - Q[Shortcuts::rho];
      Q[Shortcuts::B + 0]   = gp * V[Shortcuts::B + 0];
      Q[Shortcuts::B + 1]   = gp * V[Shortcuts::B + 1];
      Q[Shortcuts::B + 2]   = gp * V[Shortcuts::B + 2];

      Q[Shortcuts::psi]       = V[Shortcuts::psi];
      Q[Shortcuts::lapse]     = V[Shortcuts::lapse];
      Q[Shortcuts::shift + 0] = V[Shortcuts::shift + 0];
      Q[Shortcuts::shift + 1] = V[Shortcuts::shift + 1];
      Q[Shortcuts::shift + 2] = V[Shortcuts::shift + 2];
      Q[Shortcuts::gij + 0]   = V[Shortcuts::gij + 0];
      Q[Shortcuts::gij + 1]   = V[Shortcuts::gij + 1];
      Q[Shortcuts::gij + 2]   = V[Shortcuts::gij + 2];
      Q[Shortcuts::gij + 3]   = V[Shortcuts::gij + 3];
      Q[Shortcuts::gij + 4]   = V[Shortcuts::gij + 4];
      Q[Shortcuts::gij + 5]   = V[Shortcuts::gij + 5];
    }

    // Converts the conserved variables Q into the primitive variables V.
    inline GPU_CALLABLE_METHOD void PDECons2Prim(const double* Q, double* V) {
      constexpr double AdiabaticIndex     = 4.0 / 3.0;
      constexpr double AtmospherePressure = 1e-12;
      constexpr double AtmosphereDensity  = 1e-11;

      double psi        = Q[Shortcuts::psi];
      double lapse      = Q[Shortcuts::lapse];
      double shift[3]   = {Q[Shortcuts::shift + 0], Q[Shortcuts::shift + 1], Q[Shortcuts::shift + 2]};
      double gammaij[6] = {
        Q[Shortcuts::gij + 0],
        Q[Shortcuts::gij + 1],
        Q[Shortcuts::gij + 2],
        Q[Shortcuts::gij + 3],
        Q[Shortcuts::gij + 4],
        Q[Shortcuts::gij + 5]
      };
      double g_cov[3][3] = {{Q[13], Q[14], Q[15]}, {Q[14], Q[16], Q[17]}, {Q[15], Q[17], Q[18]}};

      double g_contr[3][3];
      double gp = matrixInverse3x3(g_cov, g_contr);

      gp        = std::sqrt(gp);
      double gm = 1. / gp;

      double Qloc[19];
      for (int i = 0; i < 8; i++) {
        Qloc[i] = gm * Q[i];
      }

      double dd = Qloc[0];

      double B_contr[3] = {Qloc[5 + 0], Qloc[5 + 1], Qloc[5 + 2]};
      double sm_cov[3]  = {Qloc[1], Qloc[2], Qloc[3]};

      double gamma1 = AdiabaticIndex / (AdiabaticIndex - 1.0);
      double gam    = 1.0 / gamma1;

      // Solve for p
      bool failed = false;

      double sm[3];
      matrixVectMult(g_contr, sm_cov, sm);
      double B_cov[3];
      matrixVectMult(g_cov, B_contr, B_cov);

      double s2  = sm_cov[0] * sm[0] + sm_cov[1] * sm[1] + sm_cov[2] * sm[2];
      double b2  = B_contr[0] * B_cov[0] + B_contr[1] * B_cov[1] + B_contr[2] * B_cov[2];
      double sb  = sm_cov[0] * B_contr[0] + sm_cov[1] * B_contr[1] + sm_cov[2] * B_contr[2];
      double sb2 = sb * sb;

      // First option [Del Zanna et al. (2007) A&A, 473, 11-30 (method 3)]

      double e = Qloc[4] + dd;

      double x1  = 0.;
      double x2  = 1.0 - 1e-10;
      double tol = 1.0e-18;
      double w   = 0;
      double v2;
      RTSAFE_C2P_RMHD1(&v2, &x1, &x2, &tol, &w, gam, dd, e, s2, b2, sb2, &failed);

      double p, rho, den, vb;
      double v_cov[3];
      if (failed) {
        p   = AtmospherePressure;
        rho = AtmosphereDensity;

        v_cov[0] = 0.0;
        v_cov[1] = 0.0;
        v_cov[2] = 0.0;
      } else {
        den      = 1.0 / (w + b2);
        vb       = sb / w;
        rho      = dd * sqrt(1. - v2);
        v_cov[0] = (sm_cov[0] + vb * B_cov[0]) * den;
        v_cov[1] = (sm_cov[1] + vb * B_cov[1]) * den;
        v_cov[2] = (sm_cov[2] + vb * B_cov[2]) * den;
        p        = gam * (w * (1. - v2) - rho);
      }

      if (rho < AtmosphereDensity * 1.01) {
        v_cov[0] = 0.0;
        v_cov[1] = 0.0;
        v_cov[2] = 0.0;
        v2       = 0.;
        rho      = AtmosphereDensity;
        p        = gam * (w * (1. - v2) - rho);
      }
      if (p < AtmospherePressure * 1.01) {
        v_cov[0] = 0.0;
        v_cov[1] = 0.0;
        v_cov[2] = 0.0;
        v2       = 0.;
        p        = gam * (w * (1. - v2) - rho);
        p        = AtmospherePressure;
      }
      if (p < AtmospherePressure) {
        p = AtmospherePressure;
      }

      V[0]  = rho;
      V[1]  = v_cov[0];
      V[2]  = v_cov[1];
      V[3]  = v_cov[2];
      V[4]  = p;
      V[5]  = B_contr[0];
      V[6]  = B_contr[1];
      V[7]  = B_contr[2];
      V[8]  = psi;
      V[9]  = lapse;
      V[10] = shift[0];
      V[11] = shift[1];
      V[12] = shift[2];

      for (int i = 0; i < 6; i++) {
        V[13 + i] = gammaij[i];
      }
    } // PDECons2Prim
  } // namespace PDE
} // namespace GRMHD
TARGET_DECLARE_END
