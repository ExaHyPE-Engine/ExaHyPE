#pragma once

#include "tarch/la/Vector.h"
#include "Abstractloh.h"
#include "VariableShortcuts_loh.h"

namespace loh{
  namespace riemannSolver{

    void riemannSolver(
      double* const FL,
      double* const FR,
      const double* const QL,
      const double* const QR,
      const double t,
      const double dt,
      const tarch::la::Vector<Dimensions, double>& h,
      const int direction, 
      bool isBoundaryFace, 
      int faceIndex);

    void extractTransformation(const double* const Q,
			       double& q_x,double& q_y,double& q_z,
			       double& r_x,double& r_y,double& r_z,
			       double& s_x,double& s_y,double& s_z);

    void riemannSolver_Nodal(double v_p,double v_m, double sigma_p, double sigma_m, double z_p , double z_m, double& v_hat_p , double& v_hat_m, double& sigma_hat_p, double& sigma_hat_m);
    void riemannSolver_boundary(int faceIndex,double r, double vn , double vm , double vl, double Tn , double Tm ,double Tl , double zp, double zs,  double& vn_hat , double& vm_hat ,double& vl_hat , double& Tn_hat , double& Tm_hat ,double& Tl_hat);
    
    void riemannSolver_BC0(double v, double sigma, double z,  double r, double& v_hat, double& sigma_hat);
    void riemannSolver_BCn(double v, double sigma, double z,  double r, double& v_hat, double& sigma_hat);

    void localBasis(double* const n, double* const m, double* const l, int d);
    void Gram_Schmidt(double* const y, double* const z);
    
    void generate_fluctuations_right(double z,  double T,double T_hat,double v, double v_hat, double& F);
    void generate_fluctuations_left(double z,  double T,double T_hat,double v, double v_hat, double& F);
    void rotate_into_physical_basis(double* const n,double* const m,double* const l, double Fn,double Fm,double Fl, double& Fx, double& Fy, double& Fz);
    void rotate_into_orthogonal_basis(double* const n,double* const m,double* const l, double Tx,double Ty,double Tz, double& Tn, double& Tm, double& Tl);
    void extract_tractions_and_particle_velocity(double* const n, const double* const Q, double& Tx,double& Ty,double& Tz,double& vx,double& vy,double& vz );

  }
}