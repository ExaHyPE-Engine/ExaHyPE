#pragma once

namespace Numerics {

  template <class Shortcuts, typename T>
  inline void computeTractions(const T* Q, const T* n, T& Tx, T& Ty, T& Tz) {
    T    sigma_xx = Q[Shortcuts::sigma + 0];
    T    sigma_yy = Q[Shortcuts::sigma + 1];
    T    sigma_zz = Q[Shortcuts::sigma + 2];
    T    sigma_xy = Q[Shortcuts::sigma + 3];
    T    sigma_xz = Q[Shortcuts::sigma + 4];
    T    sigma_yz = Q[Shortcuts::sigma + 5];

    Tx = n[0] * sigma_xx + n[1] * sigma_xy + n[2] * sigma_xz;
    Ty = n[0] * sigma_xy + n[1] * sigma_yy + n[2] * sigma_yz;
    Tz = n[0] * sigma_xz + n[1] * sigma_yz + n[2] * sigma_zz;
  }

  template <class Shortcuts, typename T>
  inline void getVelocities(const T* Q, T& vx, T& vy, T& vz) {
    vx = Q[Shortcuts::v + 0];
    vy = Q[Shortcuts::v + 1];
    vz = Q[Shortcuts::v + 2];
  }

  // Transformation routines
  // Gram Schmidt orthonormalization
  template<typename T>
  inline void GramSchmidt(T* y, T* z) {
    T a_yz = y[0] * z[0] + y[1] * z[1] + y[2] * z[2];

    for (int i = 0; i < 3; i++) {
      z[i] = z[i] - a_yz * y[i];
    }

    T norm_z = std::sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);

    for (int i = 0; i < 3; i++) {
      z[i] = z[i] / norm_z;
    }
  }

  template<typename T>
  inline void createLocalBasis(T* n, T* m, T* l) {

#if DIMENSIONS == 2
    l[0] = 0.;
    l[1] = 0.;
    l[2] = 1.0;

    m[0] = n[1] * l[2] - n[2] * l[1];
    m[1] = -(n[0] * l[2] - n[2] * l[0]);
    m[2] = n[0] * l[1] - n[1] * l[0];
#elif DIMENSIONS == 3

    T tol, diff_norm1, diff_norm2;
    tol  = 1e-12;
    m[0] = 0.;
    m[1] = 1.;
    m[2] = 0.;

    diff_norm1 = std::sqrt(pow(n[0] - m[0], 2) + pow(n[1] - m[1], 2) + pow(n[2] - m[2], 2));
    diff_norm2 = std::sqrt(pow(n[0] + m[0], 2) + pow(n[1] + m[1], 2) + pow(n[2] + m[2], 2));

    if (diff_norm1 >= tol && diff_norm2 >= tol) {
      GramSchmidt(n, m);
    } else {
      m[0] = 0.;
      m[1] = 0.;
      m[2] = 1.;
      GramSchmidt(n, m);
    }
    l[0] = n[1] * m[2] - n[2] * m[1];
    l[1] = -(n[0] * m[2] - n[2] * m[0]);
    l[2] = n[0] * m[1] - n[1] * m[0];

#endif
  }

  template<typename T>
  inline void riemannSolverNodal(
    T  v_p,
    T  v_m,
    T  sigma_p,
    T  sigma_m,
    T  z_p,
    T  z_m,
    T& v_hat_p,
    T& v_hat_m,
    T& sigma_hat_p,
    T& sigma_hat_m
  ) {

    T p = z_p * v_p + sigma_p;
    T q = z_m * v_m - sigma_m;

    if (z_p > 0 && z_m > 0) {

      T eta = (z_p * z_m) / (z_p + z_m);
      T phi = eta * (p / z_p - q / z_m);

      sigma_hat_p = phi;
      sigma_hat_m = phi;

      v_hat_p = (q + phi) / z_m;
      v_hat_m = (p - phi) / z_p;
    } else if (z_p > 0) {
      sigma_hat_p = 0;
      sigma_hat_m = sigma_m;

      v_hat_p = v_p;
      v_hat_m = v_m;
    } else if (z_m > 0) {
      sigma_hat_p = sigma_p;
      sigma_hat_m = 0;

      v_hat_p = v_p;
      v_hat_m = v_m;
    } else {
      sigma_hat_p = sigma_p;
      sigma_hat_m = sigma_m;

      v_hat_p = v_p;
      v_hat_m = v_m;
    }
  }

  template<typename T>
  inline void riemannSolverBC0(T v, T sigma, T z, T r, T& v_hat, T& sigma_hat) {
    T p = 0.5 * (z * v + sigma);
    if (z > 0) {
      v_hat     = (1 + r) / z * p;
      sigma_hat = (1 - r) * p;
    } else {
      v_hat     = v;
      sigma_hat = sigma;
    }
  }

  template<typename T>
  inline void riemannSolverBCn(T v, T sigma, T z, T r, T& v_hat, T& sigma_hat) {
    T q = 0.5 * (z * v - sigma);
    if (z > 0) {
      v_hat     = (1 + r) / z * q;
      sigma_hat = -(1 - r) * q;
    } else {
      v_hat     = v;
      sigma_hat = sigma;
    }
  }

  template<typename T>
  inline void rotateIntoOrthogonalBasis(
    T* n, T* m, T* l, T Tx, T Ty, T Tz, T& Tn, T& Tm, T& Tl
  ) {
    Tn = Tx * n[0] + Ty * n[1] + Tz * n[2];
    Tm = Tx * m[0] + Ty * m[1] + Tz * m[2];
    Tl = Tx * l[0] + Ty * l[1] + Tz * l[2];
  }

  template<typename T>
  inline void rotateIntoPhysicalBasis(
    T* n, T* m, T* l, T Fn, T Fm, T Fl, T& Fx, T& Fy, T& Fz
  ) {
    Fx = n[0] * Fn + m[0] * Fm + l[0] * Fl;
    Fy = n[1] * Fn + m[1] * Fm + l[1] * Fl;
    Fz = n[2] * Fn + m[2] * Fm + l[2] * Fl;
  }

  // Solver
  template<typename T>
  inline void computeFluctuationsLeft(T z, T myT, T T_hat, T v, T v_hat, T& F) {
    F = 0.5 * (z * (v - v_hat) + (myT - T_hat));
  }

  template<typename T>
  inline void computeFluctuationsRight(T z, T myT, T T_hat, T v, T v_hat, T& F) {
    F = 0.5 * (z * (v - v_hat) - (myT - T_hat));
  }

} // namespace Numerics
