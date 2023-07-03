#include <algorithm>

#include "kernels/GaussLegendreBasis.h"
#include "kernels/KernelUtils.h"

namespace Numerics {
  template <class Shortcuts>
  void rotate_dofs_inverse(double* Q, const int direction_1) {
    Shortcuts s;

    int direction_2 = direction_1 == 0 ? 1 : 0;
    int direction_3 = direction_1 == 2 ? 1 : 2;

    // velocities
    double u = Q[s.v + 0];
    double v = Q[s.v + 1];
    double w = Q[s.v + 2];

    // stresses
    double sigma_xx = Q[s.sigma + 0];
    double sigma_yy = Q[s.sigma + 1];
    double sigma_zz = Q[s.sigma + 2];

    double sigma_xy = Q[s.sigma + 3 + 0];
    double sigma_xz = Q[s.sigma + 3 + 1];
    double sigma_yz = Q[s.sigma + 3 + 2];

    Q[s.v + direction_1]                       = u;
    Q[s.v + direction_2]                       = v;
    Q[s.v + direction_3]                       = w;
    Q[s.sigma + direction_1]                   = sigma_xx;
    Q[s.sigma + direction_2]                   = sigma_yy;
    Q[s.sigma + direction_3]                   = sigma_zz;
    Q[s.sigma + 2 + direction_1 + direction_2] = sigma_xy;
    Q[s.sigma + 2 + direction_1 + direction_3] = sigma_xz;
    Q[s.sigma + 2 + direction_2 + direction_3] = sigma_yz;
  };

  template <class Shortcuts>
  void rotate_dofs(double* Q, const int direction_1) {
    Shortcuts s;

    int direction_2 = direction_1 == 0 ? 1 : 0;
    int direction_3 = direction_1 == 2 ? 1 : 2;

    // velocities
    double u = Q[s.v + direction_1];
    double v = Q[s.v + direction_2];
    double w = Q[s.v + direction_3];

    // stresses
    double sigma_xx = Q[s.sigma + direction_1];
    double sigma_yy = Q[s.sigma + direction_2];
    double sigma_zz = Q[s.sigma + direction_3];

    double sigma_xy = Q[s.sigma + 2 + direction_1 + direction_2];
    double sigma_xz = Q[s.sigma + 2 + direction_1 + direction_3];
    double sigma_yz = Q[s.sigma + 2 + direction_2 + direction_3];

    Q[s.v + 0]         = u;
    Q[s.v + 1]         = v;
    Q[s.v + 2]         = w;
    Q[s.sigma + 0]     = sigma_xx;
    Q[s.sigma + 1]     = sigma_yy;
    Q[s.sigma + 2]     = sigma_zz;
    Q[s.sigma + 3 + 0] = sigma_xy;
    Q[s.sigma + 3 + 1] = sigma_xz;
    Q[s.sigma + 3 + 2] = sigma_yz;
  };

  template <class Shortcuts>
  void right_eigenvectors(const double Q[], const int direction, double eigenvectors[]) {
    Shortcuts s;
    std::fill_n(eigenvectors, (s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters), 0.0);

    kernels::idx2 idx2(s.SizeVariables + s.SizeParameters, s.SizeVariables + s.SizeParameters);
    double        rho     = Q[s.rho];
    double        cp      = Q[s.cp];
    double        cs      = Q[s.cs];
    double        i_alpha = Q[s.alpha] <= 0 ? 0 : 1.0 / Q[s.alpha];

    eigenvectors[idx2(0, s.sigma + 0)] = rho * (cp * cp);
    eigenvectors[idx2(0, s.sigma + 1)] = rho * ((cp * cp) - 2 * (cs * cs));
    eigenvectors[idx2(0, s.sigma + 2)] = rho * ((cp * cp) - 2 * (cs * cs));
    eigenvectors[idx2(0, s.v + 0)]     = cp;

    eigenvectors[idx2(1, s.sigma + 3)] = rho * (cs * cs);
    eigenvectors[idx2(1, s.v + 1)]     = cs;

    eigenvectors[idx2(2, s.sigma + 4)] = rho * (cs * cs);
    eigenvectors[idx2(2, s.v + 2)]     = cs;

    eigenvectors[idx2(3, s.sigma + 1)] = 1;

    eigenvectors[idx2(4, s.sigma + 2)] = 1;

    eigenvectors[idx2(5, s.sigma + 5)] = 1;

    eigenvectors[idx2(6, s.cs)] = 1;

    eigenvectors[idx2(7, s.cp)] = 1;

    eigenvectors[idx2(8, s.rho)] = 1;

    eigenvectors[idx2(9, s.sigma + 0)] = -2.0 * Q[s.sigma + 0];
    eigenvectors[idx2(9, s.sigma + 3)] = -2.0 * Q[s.sigma + 3];
    eigenvectors[idx2(9, s.sigma + 4)] = -2.0 * Q[s.sigma + 4];
    eigenvectors[idx2(9, s.v + 0)]     = Q[s.v + 0] * i_alpha;
    eigenvectors[idx2(9, s.v + 1)]     = Q[s.v + 1] * i_alpha;
    eigenvectors[idx2(9, s.v + 2)]     = Q[s.v + 2] * i_alpha;
    eigenvectors[idx2(9, s.alpha)]     = 1;

    eigenvectors[idx2(10, s.sigma + 4)] = rho * (cs * cs);
    eigenvectors[idx2(10, s.v + 2)]     = -cs;

    eigenvectors[idx2(11, s.sigma + 3)] = rho * (cs * cs);
    eigenvectors[idx2(11, s.v + 1)]     = -cs;

    eigenvectors[idx2(12, s.sigma + 0)] = rho * (cp * cp);
    eigenvectors[idx2(12, s.sigma + 1)] = rho * ((cp * cp) - 2 * (cs * cs));
    eigenvectors[idx2(12, s.sigma + 2)] = rho * ((cp * cp) - 2 * (cs * cs));
    eigenvectors[idx2(12, s.v + 0)]     = -cp;

#ifdef Asserts
    for (int i = 0; i < s.SizeVariables + s.SizeParameters; i++) {
      for (int j = 0; j < s.SizeVariables + s.SizeParameters; j++) {
        if (!std::isfinite(eigenvectors[idx2(i, j)])) {
          for (int k = 0; k < s.SizeVariables + s.SizeParameters; k++) {
            std::cout << Q[k] << std::endl;
          }
        }
        assertion2(std::isfinite(eigenvectors[idx2(i, j)]), i, j);
      }
    }
#endif
  };

  template <class Shortcuts>
  void right_eigenvectors_inverse(const double Q[], const int direction, double eigenvectors[]) {
    Shortcuts s;
    std::fill_n(eigenvectors, (s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters), 0.0);
    kernels::idx2 idx2(s.SizeVariables + s.SizeParameters, s.SizeVariables + s.SizeParameters);

    double rho     = Q[s.rho];
    double cp      = Q[s.cp];
    double cs      = Q[s.cs];
    double i_alpha = Q[s.alpha] <= 0 ? 0 : 1.0 / Q[s.alpha];

    eigenvectors[idx2(0, s.sigma + 0)] = 1.0 / (rho * (cp * cp) * 2.0);
    eigenvectors[idx2(0, s.v + 0)]     = 1.0 / (cp * 2.0);
    eigenvectors[idx2(0, s.alpha)] = -(cp * rho * Q[s.v + 0] * i_alpha - 2 * Q[s.sigma + 0]) / (rho * (cp * cp) * 2.0);

    eigenvectors[idx2(1, s.sigma + 3)] = 1.0 / (rho * (cs * cs) * 2.0);
    eigenvectors[idx2(1, s.v + 1)]     = 1.0 / (cs * 2.0);
    eigenvectors[idx2(1, s.alpha)] = -(cs * rho * Q[s.v + 1] * i_alpha - 2 * Q[s.sigma + 3]) / (rho * (cs * cs) * 2.0);

    eigenvectors[idx2(2, s.sigma + 4)] = 1.0 / (2.0 * rho * (cs * cs));
    eigenvectors[idx2(2, s.v + 2)]     = 1.0 / (cs * 2.0);
    eigenvectors[idx2(2, s.alpha)] = -(cs * rho * Q[s.v + 2] * i_alpha - 2 * Q[s.sigma + 4]) / (rho * (cs * cs) * 2.0);

    eigenvectors[idx2(3, s.sigma + 0)] = (-(cp * cp) + 2 * (cs * cs)) / (cp * cp);
    eigenvectors[idx2(3, s.sigma + 1)] = 1.0;
    eigenvectors[idx2(3, s.alpha)]     = 2 * Q[s.sigma + 0] * ((-cp * cp) + 2 * (cs * cs)) / (cp * cp);

    eigenvectors[idx2(4, s.sigma + 0)] = (-(cp * cp) + 2 * (cs * cs)) / (cp * cp);
    eigenvectors[idx2(4, s.sigma + 2)] = 1.0;
    eigenvectors[idx2(4, s.alpha)]     = 2 * Q[s.sigma + 0] * ((-cp * cp) + 2 * (cs * cs)) / (cp * cp);

    eigenvectors[idx2(5, s.sigma + 5)] = 1.0;

    eigenvectors[idx2(6, s.cs)] = 1.0;

    eigenvectors[idx2(7, s.cp)] = 1.0;

    eigenvectors[idx2(8, s.rho)] = 1.0;

    eigenvectors[idx2(9, s.alpha)] = 1.0;

    eigenvectors[idx2(10, s.sigma + 4)] = 1.0 / ((cs * cs) * rho * 2.0);
    eigenvectors[idx2(10, s.v + 2)]     = -1.0 / (cs * 2.0);
    eigenvectors[idx2(10, s.alpha)] = (cs * rho * Q[s.v + 2] * i_alpha + 2 * Q[s.sigma + 4]) / (rho * (cs * cs) * 2.0);

    eigenvectors[idx2(11, s.sigma + 3)] = 1.0 / ((cs * cs) * rho * 2.0);
    eigenvectors[idx2(11, s.v + 1)]     = -1.0 / (cs * 2.0);
    eigenvectors[idx2(11, s.alpha)] = (cs * rho * Q[s.v + 1] * i_alpha + 2 * Q[s.sigma + 3]) / (rho * (cs * cs) * 2.0);

    eigenvectors[idx2(12, s.sigma + 0)] = 1.0 / ((cp * cp) * rho * 2.0);
    eigenvectors[idx2(12, s.v + 0)]     = -1.0 / (cp * 2.0);
    eigenvectors[idx2(12, s.alpha)] = (cp * rho * Q[s.v + 0] * i_alpha + 2 * Q[s.sigma + 0]) / (rho * (cp * cp) * 2.0);

#ifdef Asserts
    for (int i = 0; i < s.SizeVariables + s.SizeParameters; i++) {
      for (int j = 0; j < s.SizeVariables + s.SizeParameters; j++) {
        if (!std::isfinite(eigenvectors[idx2(i, j)])) {
          for (int k = 0; k < s.SizeVariables + s.SizeParameters; k++) {
            std::cout << Q[k] << std::endl;
          }
        }
        assertion2(std::isfinite(eigenvectors[idx2(i, j)]), i, j);
      }
    }
#endif
  };

  template <class Shortcuts>
  void eigenvalues(const double Q[], double lambda[]) {
    Shortcuts s;
    double    cp = Q[s.cp];
    double    cs = Q[s.cs];
    lambda[0]    = -cp;
    lambda[1]    = -cs;
    lambda[2]    = -cs;
    lambda[3]    = 0;
    lambda[4]    = 0;
    lambda[5]    = 0;
    lambda[6]    = 0;
    lambda[7]    = 0;
    lambda[8]    = 0;
    lambda[9]    = 0;
    lambda[10]   = cs;
    lambda[11]   = cs;
    lambda[12]   = cp;
  }

  template <class Shortcuts>
  double computeSmax(const double* const Q_L, const double* const Q_R) {
    Shortcuts s;

    // eigenvalues left and right
    double Ev_R[(s.SizeVariables + s.SizeParameters)];
    double Ev_L[(s.SizeVariables + s.SizeParameters)];
    std::fill_n(Ev_R, (s.SizeVariables + s.SizeParameters), 0.0);
    std::fill_n(Ev_L, (s.SizeVariables + s.SizeParameters), 0.0);

    eigenvalues<Shortcuts>(Q_R, Ev_R);
    eigenvalues<Shortcuts>(Q_L, Ev_L);

    double ev_max = std::numeric_limits<double>::min();

    for (int i = 0; i < s.SizeVariables; i++) {
      ev_max = std::max(std::abs(Ev_R[i]), ev_max);
      ev_max = std::max(std::abs(Ev_L[i]), ev_max);
    }
    return ev_max;
  }

  /**
   * Computes AT = 0.5 * smax * T * (I + R*deltaLambda + R^-1)
   */
  template <class Shortcuts>
  void computeAbsA(const double* const Q_L, const double* const Q_R, double absA[], int direction) {
    Shortcuts     s;
    kernels::idx2 idx2(Dimensions, s.SizeVariables);
    kernels::idx2 idxA(s.SizeVariables + s.SizeParameters, s.SizeVariables + s.SizeParameters);
    std::fill_n(absA, (s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters), 0.0);

    // transposed eigenvectors and inverse eigenvectors
    double R[(s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters)];
    std::fill_n(R, (s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters), 0.0);
    double R_inv[(s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters)];
    std::fill_n(R_inv, (s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters), 0.0);

    // Average Q
    double Qavg[s.SizeVariables + s.SizeParameters];
    std::fill_n(Qavg, (s.SizeVariables + s.SizeParameters), 0.0);

    // Eigenvalues for the average
    double Ev[(s.SizeVariables + s.SizeParameters)];
    std::fill_n(Ev, (s.SizeVariables + s.SizeParameters), 0.0);

    double ev_max = computeSmax<Shortcuts>(Q_L, Q_R);

    // Compute average of both sides
    for (int k = 0; k < s.SizeVariables + s.SizeParameters; k++) {
      Qavg[k] = (Q_L[k] + Q_R[k]) * 0.5;
    }

    rotate_dofs<Shortcuts>(Qavg, direction);

    assertion(!tarch::la::equals(Qavg[s.alpha], 0.0));
    assertion(!tarch::la::equals(Qavg[s.rho], 0.0));

    // compute eigenvectors for transformed third state
    right_eigenvectors<Shortcuts>(Qavg, direction, R);
    right_eigenvectors_inverse<Shortcuts>(Qavg, direction, R_inv);

    // We store T R = R^T T^-1 in R (nice it's already transposed there ;))
    for (int i = 0; i < s.SizeVariables + s.SizeParameters; i++) {
      rotate_dofs_inverse<Shortcuts>(R + idxA(i, 0), direction);
    }

    // we store R^-1*T^-1 in R^-1
    for (int i = 0; i < s.SizeVariables + s.SizeParameters; i++) {
      rotate_dofs_inverse<Shortcuts>(R_inv + idxA(i, 0), direction);
    }

    eigenvalues<Shortcuts>(Qavg, Ev);

    // Compute HLLEM anti diffusion
    for (int i = 0; i < s.SizeVariables + s.SizeParameters; i++) {
      Ev[i] = 1.0 - std::abs(Ev[i]) / ev_max; // we store deltaLambda directly in EV
    }

    // Add Rusanov diffusion
    for (int i = 0; i < s.SizeVariables + s.SizeParameters; i++) {
      absA[idxA(i, i)] = -0.5 * ev_max;
    }
    // std::cout << ev_max << std::endl;
    // std::cout <<"absA" << std::endl;
    /* for(int i=0; i< s.SizeParameters+s.SizeVariables; i++){ */
    /*   for(int j=0; j< s.SizeParameters+s.SizeVariables; j++){ */
    /*     //std::cout << absA[idxA(i,j)] << ";"; */
    /*   } */
    /*   //std::cout << std::endl; */
    /* } */

    for (int i = 0; i < s.SizeParameters + s.SizeVariables; i++) {
      for (int j = 0; j < s.SizeParameters + s.SizeVariables; j++) {
        R[idxA(j, i)] = R[idxA(j, i)] * Ev[j];
      }
    }

    for (int i = 0; i < s.SizeParameters + s.SizeVariables; i++) {
      for (int j = 0; j < s.SizeParameters + s.SizeVariables; j++) {
        // for(int k=0; k< s.SizeParameters+s.SizeVariables; k++){
        for (int k = 6; k < 10; k++) { // This needs clarification: Maurizio only uses EV 7-10 and not all of them
          absA[idxA(i, j)] += 0.5 * ev_max * R[idxA(k, i)] * R_inv[idxA(k, j)];
        }
      }
    }

    /* //std::cout <<"absA" << std::endl; */
    /* for(int i=0; i< s.SizeParameters+s.SizeVariables; i++){ */
    /*   for(int j=0; j< s.SizeParameters+s.SizeVariables; j++){ */
    /*     //std::cout << absA[idxA(i,j)] << ";"; */
    /*   } */
    /*   //std::cout << std::endl; */
    /* } */
  }

  template <class Solver, class Shortcuts>
  void hllem(
    Solver* solver, double* FL_, double* FR_, const double* const QL_, const double* const QR_, const int direction
  ) {
    Shortcuts s;

    // approximate graidient
    double gradQ[(s.SizeVariables + s.SizeParameters) * (Dimensions)];
    std::fill_n(gradQ, ((s.SizeVariables + s.SizeParameters) * Dimensions), 0.0);

    // average state
    double ncp[s.SizeVariables + s.SizeParameters];
    std::fill_n(ncp, (s.SizeVariables + s.SizeParameters), 0.0);

    // average state
    double Qavg[s.SizeVariables + s.SizeParameters];
    std::fill_n(Qavg, (s.SizeVariables + s.SizeParameters), 0.0);

    kernels::idx2 idx2(Dimensions, s.SizeVariables + s.SizeParameters);
    kernels::idx2 idxA(s.SizeVariables + s.SizeParameters, s.SizeVariables + s.SizeParameters);

    // HLLEM diffusion matrices;
    double absA[(s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters)];
    std::fill_n(absA, (s.SizeVariables + s.SizeParameters) * (s.SizeVariables + s.SizeParameters), 0.0);

    computeAbsA<Shortcuts>(QL_, QR_, absA, direction);

    // approximate gradient on node
    for (int k = 0; k < s.SizeVariables + s.SizeParameters; k++) {
      gradQ[idx2(direction, k)] = (QR_[k] - QL_[k]);
    }

    // compute average on node
    for (int k = 0; k < s.SizeVariables + s.SizeParameters; k++) {
      Qavg[k] = (QL_[k] + QR_[k]) * 0.5;
    }
    assertion2(!tarch::la::equals(Qavg[s.rho], 0.0), QL_[s.rho], QR_[s.rho]);

    // compute for middle state ncp
    solver->nonConservativeProduct(Qavg, gradQ, ncp);

    /* std::cout <<"ncp" << std::endl; */
    /* for(int j=0; j< s.SizeParameters+s.SizeVariables; j++){ */
    /*   std::cout << ncp[j] << ";"; */
    /* } */
    /* std::cout << std::endl; */

    /* std::cout <<"Qavg" << std::endl; */
    /* for(int j=0; j< s.SizeParameters+s.SizeVariables; j++){ */
    /*   std::cout << Qavg[j] << ";"; */
    /* } */
    /* std::cout << std::endl; */

    /* std::cout <<"gradQ" << std::endl; */
    /* for(int j=0; j< s.SizeParameters+s.SizeVariables; j++){ */
    /*   std::cout << gradQ[idx2(direction,j)] << ";"; */
    /* } */
    /* std::cout << std::endl; */

    for (int k = 0; k < s.SizeVariables; k++) {
      for (int l = 0; l < s.SizeVariables + s.SizeParameters; l++) {
        FR_[k] += absA[idxA(k, l)] * gradQ[idx2(direction, l)];
      }
    }

    //    std::cout <<"FR" << std::endl;
    /*  for(int j=0; j< s.SizeVariables; j++){  */
    /*    std::cout << FR_[j] << ";";  */
    /*  }  */
    /* std::cout << std::endl; */

    std::copy_n(FR_, s.SizeVariables, FL_);

    for (int k = 0; k < s.SizeVariables; k++) {
      FR_[k] -= 0.5 * ncp[k];
      FL_[k] += 0.5 * ncp[k];
    }
  }
}; // namespace Numerics
