#pragma once

#include "idx.h"
#include "riemannsolverRoutines.h"

namespace Numerics {

  // Getter routines
  template <class Shortcuts, typename T>
  inline void computeParameters(const T* Q, T& rho, T& cp, T& cs, T& mu, T& lam) {
    rho = Q[Shortcuts::rho];
    cp  = Q[Shortcuts::cp];
    cs  = Q[Shortcuts::cs];
    mu  = cs * cs * rho;
    lam = rho * cp * cp - 2.0 * mu;
  }

  template<typename T>
  inline void riemannSolverBoundary(
    int     faceIndex,
    T  r,
    T  vn,
    T  vm,
    T  vl,
    T  Tn,
    T  Tm,
    T  Tl,
    T  zp,
    T  zs,
    T& vn_hat,
    T& vm_hat,
    T& vl_hat,
    T& Tn_hat,
    T& Tm_hat,
    T& Tl_hat
  ) {
    // if (faceIndex % 2  == 0) { //left face
    if (faceIndex < DIMENSIONS) { //left face
      riemannSolverBC0(vn, Tn, zp, r, vn_hat, Tn_hat);
      riemannSolverBC0(vm, Tm, zs, r, vm_hat, Tm_hat);
      riemannSolverBC0(vl, Tl, zs, r, vl_hat, Tl_hat);
    }

    // if (faceIndex % 2 == 1) { //right face
    else { //right face
      riemannSolverBCn(vn, Tn, zp, r, vn_hat, Tn_hat);
      riemannSolverBCn(vm, Tm, zs, r, vm_hat, Tm_hat);
      riemannSolverBCn(vl, Tl, zs, r, vl_hat, Tl_hat);
    }
  }

  template <class Shortcuts, typename T, int basisSize, int numberOfVariables, int numberOfData, int surface = 2>
  void riemannSolver(
    T*             FL,
    T*             FR,
    const T* const QL,
    const T* const QR,
    const double   dt,
    const int      direction,
    bool           isBoundaryFace,
    int            faceIndex
  ) {

    kernels::idx3 idx_QLR(basisSize, basisSize, numberOfData);
    kernels::idx3 idx_FLR(basisSize, basisSize, numberOfVariables);

    T FLn, FLm, FLl, FRn, FRm, FRl;
    T FLx, FLy, FLz, FRx, FRy, FRz;
    T FL_n, FL_m, FL_l, FR_n, FR_m, FR_l;
    T FL_x, FL_y, FL_z, FR_x, FR_y, FR_z;

    for (int i = 0; i < basisSize; i++) {
#pragma simd
      for (int j = 0; j < basisSize; j++) {

        const T* Q_p = QR + idx_QLR(i, j, 0);
        const T* Q_m = QL + idx_QLR(i, j, 0);

        T* F_m = FL + idx_FLR(i, j, 0);
        T* F_p = FR + idx_FLR(i, j, 0);
        T  rho_m, cp_m, cs_m, mu_m, lam_m;
        T  rho_p, cp_p, cs_p, mu_p, lam_p;

        computeParameters<Shortcuts>(Q_m, rho_m, cp_m, cs_m, mu_m, lam_m);
        computeParameters<Shortcuts>(Q_p, rho_p, cp_p, cs_p, mu_p, lam_p);

        T n_m[3], m_m[3], l_m[3];
        T n_p[3], m_p[3], l_p[3];
        T norm_p, norm_m;

        getNormals<Shortcuts>(Q_m, direction, norm_m, n_m);
        getNormals<Shortcuts>(Q_p, direction, norm_p, n_p);

        T Tx_m, Ty_m, Tz_m;
        T Tx_p, Ty_p, Tz_p;
        computeTractions<Shortcuts>(Q_p, n_p, Tx_p, Ty_p, Tz_p);
        computeTractions<Shortcuts>(Q_m, n_m, Tx_m, Ty_m, Tz_m);

        T vx_m, vy_m, vz_m;
        T vx_p, vy_p, vz_p;
        getVelocities<Shortcuts>(Q_p, vx_p, vy_p, vz_p);
        getVelocities<Shortcuts>(Q_m, vx_m, vy_m, vz_m);

        createLocalBasis(n_p, m_p, l_p);
        createLocalBasis(n_m, m_m, l_m);

        T Tn_m, Tm_m, Tl_m;
        T Tn_p, Tm_p, Tl_p;

        // rotate fields into l, m, n basis
        rotateIntoOrthogonalBasis(n_m, m_m, l_m, Tx_m, Ty_m, Tz_m, Tn_m, Tm_m, Tl_m);
        rotateIntoOrthogonalBasis(n_p, m_p, l_p, Tx_p, Ty_p, Tz_p, Tn_p, Tm_p, Tl_p);

        T vn_m, vm_m, vl_m;
        T vn_p, vm_p, vl_p;
        rotateIntoOrthogonalBasis(n_m, m_m, l_m, vx_m, vy_m, vz_m, vn_m, vm_m, vl_m);
        rotateIntoOrthogonalBasis(n_p, m_p, l_p, vx_p, vy_p, vz_p, vn_p, vm_p, vl_p);

        // extract local s-wave and p-wave impedances
        T zs_m = rho_m * cs_m;
        T zs_p = rho_p * cs_p;

        T zp_m = rho_m * cp_m;
        T zp_p = rho_p * cp_p;

        // impedance must be greater than zero !
        assertion3(!(zp_p <= 0.0 || zp_m <= 0.0), "Impedance must be greater than zero !", zp_p, zp_m);

        // generate interface data preserving the amplitude of the outgoing charactertritics
        // and satisfying interface conditions exactly.
        T vn_hat_p, vm_hat_p, vl_hat_p;
        T Tn_hat_p, Tm_hat_p, Tl_hat_p;
        T vn_hat_m, vm_hat_m, vl_hat_m;
        T Tn_hat_m, Tm_hat_m, Tl_hat_m;

        if (isBoundaryFace) {
          // 0 absorbing 1 free surface
          T r = faceIndex == surface ? 1 : 0;
          //	double r=1.;
          riemannSolverBoundary(
            faceIndex,
            r,
            vn_m,
            vm_m,
            vl_m,
            Tn_m,
            Tm_m,
            Tl_m,
            zp_m,
            zs_m,
            vn_hat_m,
            vm_hat_m,
            vl_hat_m,
            Tn_hat_m,
            Tm_hat_m,
            Tl_hat_m
          );
          riemannSolverBoundary(
            faceIndex,
            r,
            vn_p,
            vm_p,
            vl_p,
            Tn_p,
            Tm_p,
            Tl_p,
            zp_p,
            zs_p,
            vn_hat_p,
            vm_hat_p,
            vl_hat_p,
            Tn_hat_p,
            Tm_hat_p,
            Tl_hat_p
          );
        } else {
          riemannSolverNodal(vn_p, vn_m, Tn_p, Tn_m, zp_p, zp_m, vn_hat_p, vn_hat_m, Tn_hat_p, Tn_hat_m);
          riemannSolverNodal(vm_p, vm_m, Tm_p, Tm_m, zs_p, zs_m, vm_hat_p, vm_hat_m, Tm_hat_p, Tm_hat_m);
          riemannSolverNodal(vl_p, vl_m, Tl_p, Tl_m, zs_p, zs_m, vl_hat_p, vl_hat_m, Tl_hat_p, Tl_hat_m);
        }

        // generate fluctuations in the local basis coordinates: n, m, l
        computeFluctuationsLeft(zp_m, Tn_m, Tn_hat_m, vn_m, vn_hat_m, FLn);
        computeFluctuationsLeft(zs_m, Tm_m, Tm_hat_m, vm_m, vm_hat_m, FLm);
        computeFluctuationsLeft(zs_m, Tl_m, Tl_hat_m, vl_m, vl_hat_m, FLl);

        computeFluctuationsRight(zp_p, Tn_p, Tn_hat_p, vn_p, vn_hat_p, FRn);
        computeFluctuationsRight(zs_p, Tm_p, Tm_hat_p, vm_p, vm_hat_p, FRm);
        computeFluctuationsRight(zs_p, Tl_p, Tl_hat_p, vl_p, vl_hat_p, FRl);

        // Consider acoustic boundary
        FL_n = FLn / zp_m;
        if (zs_m > 0) {
          FL_m = FLm / zs_m;
          FL_l = FLl / zs_m;
        } else {
          FL_m = 0;
          FL_l = 0;
        }

        FR_n = FRn / zp_p;
        if (zs_p > 0) {
          FR_m = FRm / zs_p;
          FR_l = FRl / zs_p;
        } else {
          FR_m = 0;
          FR_l = 0;
        }

        // rotate back to the physical coordinates x, y, z
        rotateIntoPhysicalBasis(n_m, m_m, l_m, FLn, FLm, FLl, FLx, FLy, FLz);
        rotateIntoPhysicalBasis(n_p, m_p, l_p, FRn, FRm, FRl, FRx, FRy, FRz);
        rotateIntoPhysicalBasis(n_m, m_m, l_m, FL_n, FL_m, FL_l, FL_x, FL_y, FL_z);
        rotateIntoPhysicalBasis(n_p, m_p, l_p, FR_n, FR_m, FR_l, FR_x, FR_y, FR_z);

        // construct flux fluctuation vectors obeying the eigen structure of the PDE
        // and choose physically motivated penalties such that we can prove
        // numerical stability.
        F_p[Shortcuts::v + 0] = norm_p / rho_p * FRx;
        F_m[Shortcuts::v + 0] = norm_m / rho_m * FLx;

        F_p[Shortcuts::v + 1] = norm_p / rho_p * FRy;
        F_m[Shortcuts::v + 1] = norm_m / rho_m * FLy;

        F_p[Shortcuts::v + 2] = norm_p / rho_p * FRz;
        F_m[Shortcuts::v + 2] = norm_m / rho_m * FLz;

        F_m[Shortcuts::sigma + 0] = norm_m * ((2 * mu_m + lam_m) * n_m[0] * FL_x + lam_m * n_m[1] * FL_y + lam_m * n_m[2] * FL_z);
        F_m[Shortcuts::sigma + 1] = norm_m * ((2 * mu_m + lam_m) * n_m[1] * FL_y + lam_m * n_m[0] * FL_x + lam_m * n_m[2] * FL_z);
        F_m[Shortcuts::sigma + 2] = norm_m * ((2 * mu_m + lam_m) * n_m[2] * FL_z + lam_m * n_m[0] * FL_x + lam_m * n_m[1] * FL_y);

        F_p[Shortcuts::sigma + 0] = -norm_p
                           * ((2 * mu_p + lam_p) * n_p[0] * FR_x + lam_p * n_p[1] * FR_y + lam_p * n_p[2] * FR_z);
        F_p[Shortcuts::sigma + 1] = -norm_p
                           * ((2 * mu_p + lam_p) * n_p[1] * FR_y + lam_p * n_p[0] * FR_x + lam_p * n_p[2] * FR_z);
        F_p[Shortcuts::sigma + 2] = -norm_p
                           * ((2 * mu_p + lam_p) * n_p[2] * FR_z + lam_p * n_p[0] * FR_x + lam_p * n_p[1] * FR_y);

        F_m[Shortcuts::sigma + 3] = norm_m * mu_m * (n_m[1] * FL_x + n_m[0] * FL_y);
        F_m[Shortcuts::sigma + 4] = norm_m * mu_m * (n_m[2] * FL_x + n_m[0] * FL_z);
        F_m[Shortcuts::sigma + 5] = norm_m * mu_m * (n_m[2] * FL_y + n_m[1] * FL_z);

        F_p[Shortcuts::sigma + 3] = -norm_p * mu_p * (n_p[1] * FR_x + n_p[0] * FR_y);
        F_p[Shortcuts::sigma + 4] = -norm_p * mu_p * (n_p[2] * FR_x + n_p[0] * FR_z);
        F_p[Shortcuts::sigma + 5] = -norm_p * mu_p * (n_p[2] * FR_y + n_p[1] * FR_z);
      }
    }
  }

} // namespace Numerics
