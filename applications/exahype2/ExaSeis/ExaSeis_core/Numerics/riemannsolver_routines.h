namespace Numerics {

  template <class Shortcuts>
  inline void compute_tractions(const double* Q, const double* n, double& Tx, double& Ty, double& Tz) {
    Shortcuts s;
    double    sigma_xx = Q[s.sigma + 0];
    double    sigma_yy = Q[s.sigma + 1];
    double    sigma_zz = Q[s.sigma + 2];
    double    sigma_xy = Q[s.sigma + 3];
    double    sigma_xz = Q[s.sigma + 4];
    double    sigma_yz = Q[s.sigma + 5];

    Tx = n[0] * sigma_xx + n[1] * sigma_xy + n[2] * sigma_xz;
    Ty = n[0] * sigma_xy + n[1] * sigma_yy + n[2] * sigma_yz;
    Tz = n[0] * sigma_xz + n[1] * sigma_yz + n[2] * sigma_zz;
  }

  template <class Shortcuts>
  inline void get_velocities(const double* Q, double& vx, double& vy, double& vz) {
    Shortcuts s;
    vx = Q[s.v + 0];
    vy = Q[s.v + 1];
    vz = Q[s.v + 2];
  }

  // Transformation routines
  // Gram Schmidt orthonormalization
  inline void Gram_Schmidt(double* y, double* z) {
    double a_yz = y[0] * z[0] + y[1] * z[1] + y[2] * z[2];

    for (int i = 0; i < 3; i++) {
      z[i] = z[i] - a_yz * y[i];
    }

    double norm_z = std::sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);

    for (int i = 0; i < 3; i++) {
      z[i] = z[i] / norm_z;
    }
  }

  /* inline void create_local_basis(double* n, double * m, double* l){ */
  /*   if(std::abs(n[0]-n[1]) < 1.0e-10 && std::abs(n[1]-n[2]) < 1.0e-10){ */
  /*     const double norm = std::sqrt(2.0/3.0); */
  /*     m[0] = -2.0/3.0 / norm; */
  /*     m[1] =  1.0/3.0 / norm; */
  /*     m[2] =  1.0/3.0 / norm; */
  /*   }else{ */
  /*     const double norm = std::sqrt(2.0*( n[0]*n[0]+n[1]*n[1]+n[2]*n[2] */
  /*                                        -n[0]*n[1]-n[0]*n[2]-n[1]*n[2])); */
  /*     m[0] =  (n[2] - n[1])/norm; */
  /*     m[1] =  (n[0] - n[2])/norm; */
  /*     m[2] =  (n[1] - n[0])/norm; */
  /*   } */

  /*   l[0] =   n[1]*m[2]-n[2]*m[1]; */
  /*   l[1] =   n[2]*m[0]-n[0]*m[2]; */
  /*   l[2] =   n[0]*m[1]-n[1]*m[0]; */
  /* } */

  inline void create_local_basis(double* n, double* m, double* l) {

#if Dimensions == 2
    l[0] = 0.;
    l[1] = 0.;
    l[2] = 1.0;

    m[0] = n[1] * l[2] - n[2] * l[1];
    m[1] = -(n[0] * l[2] - n[2] * l[0]);
    m[2] = n[0] * l[1] - n[1] * l[0];
#elif Dimensions == 3

    double tol, diff_norm1, diff_norm2;
    tol  = 1e-12;
    m[0] = 0.;
    m[1] = 1.;
    m[2] = 0.;

    diff_norm1 = std::sqrt(pow(n[0] - m[0], 2) + pow(n[1] - m[1], 2) + pow(n[2] - m[2], 2));
    diff_norm2 = std::sqrt(pow(n[0] + m[0], 2) + pow(n[1] + m[1], 2) + pow(n[2] + m[2], 2));

    if (diff_norm1 >= tol && diff_norm2 >= tol) {
      Gram_Schmidt(n, m);
    } else {
      m[0] = 0.;
      m[1] = 0.;
      m[2] = 1.;
      Gram_Schmidt(n, m);
    }
    l[0] = n[1] * m[2] - n[2] * m[1];
    l[1] = -(n[0] * m[2] - n[2] * m[0]);
    l[2] = n[0] * m[1] - n[1] * m[0];

#endif
  }

  inline void riemannSolver_Nodal(
    double  v_p,
    double  v_m,
    double  sigma_p,
    double  sigma_m,
    double  z_p,
    double  z_m,
    double& v_hat_p,
    double& v_hat_m,
    double& sigma_hat_p,
    double& sigma_hat_m
  ) {

    double phi   = 0;
    double v_hat = 0;
    double eta   = 0;

    /* v_hat_p=v_m; */
    /* v_hat_m=v_p; */

    /* sigma_hat_p=sigma_m; */
    /* sigma_hat_m=sigma_p; */

    double p = z_p * v_p + sigma_p;
    double q = z_m * v_m - sigma_m;

    if (z_p > 0 && z_m > 0) {
      eta = (z_p * z_m) / (z_p + z_m);

      phi = eta * (p / z_p - q / z_m);

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

  inline void riemannSolver_BC0(double v, double sigma, double z, double r, double& v_hat, double& sigma_hat) {
    double p = 0.5 * (z * v + sigma);
    if (z > 0) {
      v_hat     = (1 + r) / z * p;
      sigma_hat = (1 - r) * p;
    } else {
      v_hat     = v;
      sigma_hat = sigma;
    }
  }

  inline void riemannSolver_BCn(double v, double sigma, double z, double r, double& v_hat, double& sigma_hat) {
    double q = 0.5 * (z * v - sigma);
    if (z > 0) {
      v_hat     = (1 + r) / z * q;
      sigma_hat = -(1 - r) * q;
    } else {
      v_hat     = v;
      sigma_hat = sigma;
    }
  }

  inline void rotate_into_orthogonal_basis(
    double* n, double* m, double* l, double Tx, double Ty, double Tz, double& Tn, double& Tm, double& Tl
  ) {
    Tn = Tx * n[0] + Ty * n[1] + Tz * n[2];
    Tm = Tx * m[0] + Ty * m[1] + Tz * m[2];
    Tl = Tx * l[0] + Ty * l[1] + Tz * l[2];
  }

  inline void rotate_into_physical_basis(
    double* n, double* m, double* l, double Fn, double Fm, double Fl, double& Fx, double& Fy, double& Fz
  ) {
    Fx = n[0] * Fn + m[0] * Fm + l[0] * Fl;
    Fy = n[1] * Fn + m[1] * Fm + l[1] * Fl;
    Fz = n[2] * Fn + m[2] * Fm + l[2] * Fl;
  }

  // Solver
  inline void compute_fluctuations_left(double z, double T, double T_hat, double v, double v_hat, double& F) {
    F = 0.5 * (z * (v - v_hat) + (T - T_hat));
  }

  inline void compute_fluctuations_right(double z, double T, double T_hat, double v, double v_hat, double& F) {
    F = 0.5 * (z * (v - v_hat) - (T - T_hat));
  }

} // namespace Numerics
