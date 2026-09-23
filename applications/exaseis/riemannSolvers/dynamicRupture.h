#pragma once

#include "curvilinearRoutines.h"
#include "idx.h"
#include "riemannsolverRoutines.h"

namespace applications::exahype2::exaseis {

  template <typename T>
  void preStress(T& T0_n, T& T0_m, T& T0_l, double* const x, double t, T* const l, T* const m, T* const n) {
    T Tx, Ty, Tz;

    // initial stress tensor
    T sxx, syy, szz, sxy, sxz, syz;
    initialStressTensor(x, sxx, syy, szz, sxy, sxz, syz);

    // Extract tractions
    Tx = n[0] * sxx + n[1] * sxy + n[2] * sxz;
    Ty = n[0] * sxy + n[1] * syy + n[2] * syz;
    Tz = n[0] * sxz + n[1] * syz + n[2] * szz;

    // rotate tractions into local orthogonal coordinates
    rotateIntoOrthogonalBasis(n, m, l, Tx, Ty, Tz, T0_n, T0_m, T0_l);

    T nyz = n_yz<T>(x);

    T0_l = T0_l + nyz;
  }

  template <typename T>
  T boxcar(T& f, double x, T w) {
    // f(x) is boxcar of unit amplitude in (x-w,x+w)
    constexpr T tol = 1e-8;

    if ((-w + tol) < (T)x && (T)x < (w - tol)) { // inside
      f = 1.0;
    } else if (std::abs(-w - (T)x) <= tol || std::abs((T)x - w) <= tol) { // boundary
      f = 0.5;
    } else { // outside
      f = 0.0;
    }
    return f;
  }

  template <typename T>
  void tauStrength(T& tau_str, T sigma_n, T S, double* const x, double t) {
    T cohesion = fault_cohesion<T>(x);
    T r_T      = forced_rupture_time<T>(x);
    T f_T      = 0;
    T d_c      = slipWeakeningCriticalDistance<T>(x);

    T fy;
    T fz;

    boxcar<T>(fy, x[1] - f_cy, f_wy);
    boxcar<T>(fz, x[2] - f_cz, f_wz);

    cohesion += (T)1e10 * (1 - fy * fz);

    if ((T)t < r_T) {
      f_T = 0;
    } else if ((t0_rupture > 0.00001) && ((T)t < r_T + (T)t0_rupture)) {
      f_T = ((T)t - r_T) / (T)t0_rupture;
    } else {
      f_T = 1;
    }

    // friction coefficient
    T fric_coeff = (T)mu_s - (T)(mu_s - mu_d) * std::max<T>(f_T, std::min<T>(S, d_c) / d_c);

    tau_str = cohesion + fric_coeff * std::max<T>(0.0, sigma_n);
  }

  // solve for slip-rate (vv):
  template <typename T>
  void slipWeakening(T& v1, T& v2, T& Vel, T& tau1, T& tau2, T phi_1, T phi_2, T eta, T tau_str, T sigma_n) {
    T Phi = std::sqrt(std::pow(phi_1, 2) + std::pow(phi_2, 2)); // stress-transfer functional
    Vel   = (Phi - tau_str) / eta;                              // slip-rate

    // compute slip velocities
    v1 = phi_1 / (eta + tau_str / Vel);
    v2 = phi_2 / (eta + tau_str / Vel);

    // compute shear stress on the fault
    tau1 = phi_1 - eta * v1;
    tau2 = phi_2 - eta * v2;
  }

  // Rupture Dynamics
  template <typename T>
  void slipWeakeningFriction(
    T             vn_p,
    T             vn_m,
    T             Tn_p,
    T             Tn_m,
    T             zn_p,
    T             zn_m,
    T&            vn_hat_p,
    T&            vn_hat_m,
    T&            Tn_hat_p,
    T&            Tn_hat_m,
    T             vm_p,
    T             vm_m,
    T             Tm_p,
    T             Tm_m,
    T             zl_p,
    T             zl_m,
    T&            vm_hat_p,
    T&            vm_hat_m,
    T&            Tm_hat_p,
    T&            Tm_hat_m,
    T             vl_p,
    T             vl_m,
    T             Tl_p,
    T             Tl_m,
    T             zm_p,
    T             zm_m,
    T&            vl_hat_p,
    T&            vl_hat_m,
    T&            Tl_hat_p,
    T&            Tl_hat_m,
    T* const      l,
    T* const      m,
    T* const      n,
    double* const x,
    T             S,
    double        t
  ) {
    // compute characteristics
    T p_l = zl_p * vl_p + Tl_p;
    T p_m = zm_p * vm_p + Tm_p;
    T p_n = zn_p * vn_p + Tn_p;

    T q_l = zl_m * vl_m - Tl_m;
    T q_m = zm_m * vm_m - Tm_m;
    T q_n = zn_m * vn_m - Tn_m;

    // half of the harmonic mean of Z1_s, Z2_s
    T eta_s = (zl_p * zl_m) / (zl_p + zl_m);
    T eta_n = (zn_p * zn_m) / (zn_p + zn_m);

    T phi_l = eta_s * (p_l / zl_p - q_l / zl_m);
    T phi_m = eta_s * (p_m / zm_p - q_m / zm_m);
    T phi_n = eta_n * (p_n / zn_p - q_n / zn_m);

    T T0_l = 0;
    T T0_m = 0;
    T T0_n = 0;

    // get prestress (where normal traction is effective normal traction)
    preStress(T0_n, T0_m, T0_l, x, 0.0, l, m, n);

    vn_hat_m = (p_n - phi_n) / zn_p; //! continuity of normal velocity
    Tn_hat_m = phi_n;                //! continuity of normal stress
    vn_hat_p = (q_n + phi_n) / zn_m; //! continuity of normal velocity
    Tn_hat_p = phi_n;

    T tau_lock = std::sqrt(std::pow(T0_l + phi_l, 2) + std::pow(T0_m + phi_m, 2));
    T sigma_n  = std::max<T>(0.0, -(T0_n + phi_n)); // including prestress
    T tau_str;
    T Vel = 0.0;
    T Tl, Tm, vv_l, vv_m;
    tauStrength(tau_str, sigma_n, S, x, t);

    if (tau_lock >= tau_str) {
      slipWeakening(vv_m, vv_l, Vel, Tm, Tl, phi_m + T0_m, phi_l + T0_l, eta_s, tau_str, sigma_n);

      Tm_hat_m = Tm - T0_m;
      Tl_hat_m = Tl - T0_l;

      Tm_hat_p = Tm - T0_m;
      Tl_hat_p = Tl - T0_l;
    } else {
      Tm_hat_m = phi_m;
      Tl_hat_m = phi_l;

      Tm_hat_p = phi_m;
      Tl_hat_p = phi_l;

      vv_m = 0.0;
      vv_l = 0.0;

      Vel = 0.0;
    }

    vm_hat_p = (Tm_hat_m + q_m) / zm_m + vv_m;
    vl_hat_p = (Tl_hat_m + q_l) / zl_m + vv_l;

    vm_hat_m = (p_m - Tm_hat_p) / zm_p - vv_m;
    vl_hat_m = (p_l - Tl_hat_p) / zl_p - vv_l;
  }

} // namespace applications::exahype2::exaseis
