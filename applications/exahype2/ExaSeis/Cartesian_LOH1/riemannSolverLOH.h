#pragma once

#include "tarch/la/Vector.h"
#include "Abstractloh.h"
#include "VariableShortcuts_loh.h"

namespace loh{
  namespace riemannSolver{

    template<typename T>
    void riemannSolver(
      T* const                                      FL,
      T* const                                      FR,
      const T* const                                QL,
      const T* const                                QR,
      const double                                  t,
      const double                                  dt,
      const tarch::la::Vector<Dimensions, double>&  h,
      const int                                     direction, 
      bool                                          isBoundaryFace, 
      int                                           faceIndex
    );

    template<typename T>
    void extractTransformation(
      const T* const Q,
      T& q_x, T& q_y, T& q_z,
      T& r_x, T& r_y, T& r_z,
      T& s_x, T& s_y, T& s_z
    );

    template<typename T>
    void riemannSolver_Nodal(T v_p, T v_m, T sigma_p, T sigma_m, T z_p, T z_m, T& v_hat_p, T& v_hat_m, T& sigma_hat_p, T& sigma_hat_m);
    template<typename T>
    void riemannSolver_boundary(int faceIndex, T r, T vn, T vm, T vl, T Tn, T Tm, T Tl, T zp, T zs, T& vn_hat, T& vm_hat, T& vl_hat, T& Tn_hat, T& Tm_hat, T& Tl_hat);
    
    template<typename T>
    void riemannSolver_BC0(T v, T sigma, T z, T r, T& v_hat, T& sigma_hat);
    template<typename T>
    void riemannSolver_BCn(T v, T sigma, T z, T r, T& v_hat, T& sigma_hat);

    template<typename T>
    void localBasis(T* const n, T* const m, T* const l, int d);
    template<typename T>
    void Gram_Schmidt(T* const y, T* const z);
    
    template<typename T>
    void generate_fluctuations_right(T z, T myT, T T_hat, T v, T v_hat, T& F);
    template<typename T>
    void generate_fluctuations_left(T z, T myT, T T_hat, T v, T v_hat, T& F);
    template<typename T>
    void rotate_into_physical_basis(T* const n, T* const m, T* const l, T Fn, T Fm, T Fl, T& Fx, T& Fy, T& Fz);
    template<typename T>
    void rotate_into_orthogonal_basis(T* const n, T* const m, T* const l, T Tx, T Ty, T Tz, T& Tn, T& Tm, T& Tl);
    template<typename T>
    void extract_tractions_and_particle_velocity(T* const n, const T* const Q, T& Tx, T& Ty, T& Tz, T& vx, T& vy, T& vz);

  }
}