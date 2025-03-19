#pragma once

#include "../Curvilinear/ContextCurvilinearPMLSlipWeakening.h"
#include "riemannsolver_pml.h"

template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
void ContextCurvilinearPMLSlipWeakening<Shortcuts, basisSize, numberOfVariables, numberOfParameters, T>::initialstresstensor(
  T& sxx, T& syy,
  T& szz, T& sxy,
  T& sxz, T& syz,
  double* const x
){

  easi::ArraysAdapter<T> adapter;
  adapter.addBindingPoint("s_xx",&sxx);
  adapter.addBindingPoint("s_yy",&syy);
  adapter.addBindingPoint("s_zz",&szz);
  adapter.addBindingPoint("s_xy",&sxy);
  adapter.addBindingPoint("s_yz",&syz);
  adapter.addBindingPoint("s_xz",&sxz);
  
  easi::Query query(1,3);
  query.group(0) = 0;
  query.x(0,0) = x[0];
  query.x(0,1) = x[1];
  query.x(0,2) = x[2];
  
  model->evaluate(query,adapter);

}


template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
void ContextCurvilinearPMLSlipWeakening<Shortcuts, basisSize, numberOfVariables, numberOfParameters, T>::prestress(
  T& T0_n, T& T0_m, T& T0_l,
  double* const x, double t,
  T* const l, T* const m, T* const n
){

  T sxx, syy, szz, sxy, sxz, syz;
  T Tx,Ty,Tz;

  // initial stress tensor
  initialstresstensor(sxx, syy, szz, sxy, sxz, syz, x);

  //Extract tractions
  Tx = n[0] * sxx + n[1] * sxy + n[2] * sxz;
  Ty = n[0] * sxy + n[1] * syy + n[2] * syz;
  Tz = n[0] * sxz + n[1] * syz + n[2] * szz;

  // rotate tractions into local orthogonal coordinates
  ::Numerics::rotate_into_orthogonal_basis(n,m,l,Tx,Ty,Tz,T0_n,T0_m,T0_l);
  
  easi::ArraysAdapter<T> adapter;
  T n_yz;
  adapter.addBindingPoint("n_yz",&n_yz);
  //

  easi::Query query(1,3);
  query.group(0) = 0;
  query.x(0,0) = x[0];
  query.x(0,1) = x[1];
  query.x(0,2) = x[2];

  model->evaluate(query,adapter);

  T0_l = T0_l + n_yz;
    
}

template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
void ContextCurvilinearPMLSlipWeakening<Shortcuts, basisSize, numberOfVariables, numberOfParameters, T>::TauStrength(
  T& tau_str, T sigma_n, T S,
  double* const x, double t
){

  T cohesion;
  T r_T;
  T f_T = 0.0;
  T d_c = 0.0;

  easi::ArraysAdapter<T> adapter;
  adapter.addBindingPoint("cohesion",&cohesion);
  adapter.addBindingPoint("forced_rupture_time",&r_T);
  adapter.addBindingPoint("d_c" ,&d_c);

  easi::Query query(1,3);
  query.group(0) = 0;
  query.x(0,0) = x[0];
  query.x(0,1) = x[1];
  query.x(0,2) = x[2];
  
  model->evaluate(query,adapter);

  T fy;
  T fz;
  
  boxcar(fy, x[1]-f_cy,f_wy);
  boxcar(fz, x[2]-f_cz,f_wz);

  cohesion += 1e10*(1.0-fy*fz);
  
  if(t<r_T){
    f_T=0.0;
  } else if ((t0_rupture > 0.00001) && ( t < r_T+t0_rupture)){
    f_T = (t-r_T)/t0_rupture;
  } else {
    f_T = 1.0;
  }
  
  // friction coefficient
  T fric_coeff = mu_s - (mu_s-mu_d) * std::max(f_T,std::min(S,d_c)/d_c);     

  tau_str = cohesion + fric_coeff*std::max(0.0,sigma_n);
}


template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
T ContextCurvilinearPMLSlipWeakening<Shortcuts, basisSize, numberOfVariables, numberOfParameters, T>::boxcar(T& f, double x, T w) {
  // f(x) is boxcar of unit amplitude in (x-w,x+w)
  T tol = 1e-8;

  if ((-w+tol)<x && x< (w-tol)){    // inside
    f = 1.0;
  }
  else if (std::abs(-w-x)<=tol || std::abs(x-w)<=tol){     // boundary
    f = 0.5;
  }
  else{    // outside
    f = 0.0;
  }
  return f;
}


// solve for slip-rate (vv):
template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
void ContextCurvilinearPMLSlipWeakening<Shortcuts, basisSize, numberOfVariables, numberOfParameters, T>::slip_weakening(
  T& v1, T& v2, T& Vel,
  T& tau1, T& tau2,
  T phi_1, T phi_2,
  T eta, T tau_str, T sigma_n
){
  
  T Phi = std::sqrt(std::pow(phi_1, 2) + std::pow(phi_2, 2));   // stress-transfer functional
  Vel = (Phi - tau_str)/eta;                // slip-rate

  //compute slip velocities
  v1 = phi_1/(eta+tau_str/Vel);
  v2 = phi_2/(eta+tau_str/Vel);
  
  //compute shear stress on the fault
  tau1 = phi_1 - eta*v1;
  tau2 = phi_2 - eta*v2;
}


// Rupture Dynamics
template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
void ContextCurvilinearPMLSlipWeakening<Shortcuts, basisSize, numberOfVariables, numberOfParameters, T>::SlipWeakeningFriction(
  T vn_p, T vn_m,
  T Tn_p, T Tn_m,
  T zn_p, T zn_m,
  T& vn_hat_p, T& vn_hat_m,
  T& Tn_hat_p, T& Tn_hat_m,
  T vm_p, T vm_m,
  T Tm_p, T Tm_m,
  T zl_p, T zl_m,
  T& vm_hat_p, T& vm_hat_m,
  T& Tm_hat_p, T& Tm_hat_m,
  T vl_p, T vl_m,
  T Tl_p, T Tl_m,
  T zm_p, T zm_m,
  T& vl_hat_p, T& vl_hat_m,
  T& Tl_hat_p, T& Tl_hat_m,
  T* const l, T* const m, T* const n, double* const x,  T S, double t
){
  // compute characteristics
  T p_l = zl_p*vl_p + Tl_p;
  T p_m = zm_p*vm_p + Tm_p;
  T p_n = zn_p*vn_p + Tn_p;
  
  T q_l = zl_m*vl_m - Tl_m;
  T q_m = zm_m*vm_m - Tm_m;
  T q_n = zn_m*vn_m - Tn_m;

  // half of the harmonic mean of Z1_s, Z2_s
  T eta_s=(zl_p*zl_m)/(zl_p+zl_m);                                    
  T eta_n=(zn_p*zn_m)/(zn_p+zn_m); 
  
  T  phi_l= eta_s*(p_l/zl_p - q_l/zl_m);
  T  phi_m= eta_s*(p_m/zm_p - q_m/zm_m);
  T  phi_n= eta_n*(p_n/zn_p - q_n/zn_m);
    
  T T0_l=0;
  T T0_m=0;
  T T0_n=0;

  // get prestress (where normal traction is effective normal traction)
  prestress(T0_n, T0_m, T0_l, x, 0.0, l, m, n);

  vn_hat_m = (p_n - phi_n)/zn_p;   //! continuity of normal velocity
  Tn_hat_m = phi_n;                //! continuity of normal stress
  vn_hat_p = (q_n + phi_n)/zn_m;   //! continuity of normal velocity
  Tn_hat_p = phi_n;

  T tau_lock = std::sqrt(std::pow(T0_l + phi_l, 2) + std::pow(T0_m + phi_m, 2));
  T sigma_n = std::max(0.0, -(T0_n + phi_n));   // including prestress
  T tau_str;
  T Vel = 0.0;
  T Tl, Tm, vv_l, vv_m; 
  TauStrength(tau_str, sigma_n, S, x, t);
  
  if (tau_lock >= tau_str){

    slip_weakening(vv_m,  vv_l, Vel, Tm, Tl, phi_m+T0_m, phi_l+T0_l, eta_s, tau_str, sigma_n);

    Tm_hat_m = Tm - T0_m;
    Tl_hat_m = Tl - T0_l;

    Tm_hat_p = Tm - T0_m;
    Tl_hat_p = Tl - T0_l;
  }else{
    Tm_hat_m = phi_m;
    Tl_hat_m = phi_l;

    Tm_hat_p = phi_m;
    Tl_hat_p = phi_l;
      
    vv_m = 0.0;
    vv_l = 0.0;
    
    Vel = 0.0;
  }
  
  vm_hat_p = (Tm_hat_m + q_m)/zm_m + vv_m;
  vl_hat_p = (Tl_hat_m + q_l)/zl_m + vv_l;
    
  vm_hat_m = (p_m - Tm_hat_p)/zm_p - vv_m;
  vl_hat_m = (p_l - Tl_hat_p)/zl_p - vv_l;

}


template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
void ContextCurvilinearPMLSlipWeakening<Shortcuts, basisSize, numberOfVariables, numberOfParameters, T>::riemannSolver(
  T* FL, T* FR,
  const T* const QL, const T* const QR,
  const double t, const double dt,
  const tarch::la::Vector<Dimensions, double>& cellSize,
  const int direction,
  bool isBoundaryFace,
  int faceIndex,
  int surface
){

  using s = Shortcuts;

  constexpr int numberOfData       = numberOfVariables+numberOfParameters;
  // constexpr int basisSize          = order+1;

  ::kernels::idx3 idx_QLR(basisSize,basisSize,numberOfData);
  ::kernels::idx3 idx_FLR(basisSize,basisSize,numberOfVariables);

  //Checking whether the face is on a fault
  int level = std::round(log(domainSize[0]/cellSize[0])/log(3.)) + 1;  

  int elt_z = int(std::round( (QL[idx_QLR(0,0,s::curve_grid+2)] -
                              this->domainOffset[2])/ this->max_dx)) * basisSize;
  int elt_y = int(std::round((QL[idx_QLR(0,0,s::curve_grid+1)] -
                              this->domainOffset[1])/ this->max_dx)) * basisSize;

  bool is_fault = (direction == 0);

  Root* root = this->interface->getRoot();
  InnerNode* fault_node = static_cast<InnerNode*>(root->getChild());

  Coordinate fault_coords[2];
  fault_node->getCoordinates(fault_coords);
  Coordinate fault_normal = fault_node->getFaceNormal();
  T position = fault_node->getPosition();

  for (int i = 0; i < basisSize; i++) {
    for (int j = 0; j < basisSize; j++) {
      T eta = QL[idx_QLR(i,j,s::curve_grid + (2-fault_normal))];
      T xi  = QL[idx_QLR(i,j,s::curve_grid + (2-fault_coords[0]))];
      T mu  = QL[idx_QLR(i,j,s::curve_grid + (2-fault_coords[1]))];

      T per_position = position + fault_node->evalPerturbation(xi,mu);

      is_fault = is_fault  && (std::abs(eta - per_position) < cellSize[2-fault_normal] * 0.5);
    }
  }

  T FLn ,FLm ,FLl ,FRn ,FRm ,FRl;
  T FLx ,FLy ,FLz ,FRx ,FRy ,FRz;
  T FL_n,FL_m,FL_l,FR_n,FR_m,FR_l;
  T FL_x,FL_y,FL_z,FR_x,FR_y,FR_z;

  for (int i = 0; i < basisSize; i++) {
    for (int j = 0; j < basisSize; j++) {

      const T* Q_m = QL+idx_QLR(i,j,0);
      const T* Q_p = QR+idx_QLR(i,j,0);
      
      T dmp_pml_x_m = Q_m[s::dmp_pml + 0];
      T dmp_pml_y_m = Q_m[s::dmp_pml + 1];
      T dmp_pml_z_m = Q_m[s::dmp_pml + 2];

      T dmp_pml_x_p = Q_p[s::dmp_pml + 0];
      T dmp_pml_y_p = Q_p[s::dmp_pml + 1];
      T dmp_pml_z_p = Q_p[s::dmp_pml + 2];

      T* F_m = FL + idx_FLR(i,j,0);
      T* F_p = FR + idx_FLR(i,j,0);
      T rho_m,cp_m,cs_m,mu_m,lam_m;
      T rho_p,cp_p,cs_p,mu_p,lam_p;

      ::Numerics::compute_parameters<exahype2::elastic::VariableShortcutselastic>(Q_m,rho_m,cp_m,cs_m,mu_m,lam_m);
      ::Numerics::compute_parameters<exahype2::elastic::VariableShortcutselastic>(Q_p,rho_p,cp_p,cs_p,mu_p,lam_p);

      T n_m[3],m_m[3],l_m[3];
      T n_p[3],m_p[3],l_p[3];
      T norm_p,norm_m;

      ::Numerics::get_normals<exahype2::elastic::VariableShortcutselastic>(Q_m,direction,norm_m,n_m);
      ::Numerics::get_normals<exahype2::elastic::VariableShortcutselastic>(Q_p,direction,norm_p,n_p);

      T Tx_m,Ty_m,Tz_m;
      T Tx_p,Ty_p,Tz_p;
      ::Numerics::compute_tractions<exahype2::elastic::VariableShortcutselastic>(Q_p,n_p,Tx_p,Ty_p,Tz_p);
      ::Numerics::compute_tractions<exahype2::elastic::VariableShortcutselastic>(Q_m,n_m,Tx_m,Ty_m,Tz_m);

      T vx_m,vy_m,vz_m;
      T vx_p,vy_p,vz_p;
      ::Numerics::get_velocities<exahype2::elastic::VariableShortcutselastic>(Q_p,vx_p,vy_p,vz_p);	
      ::Numerics::get_velocities<exahype2::elastic::VariableShortcutselastic>(Q_m,vx_m,vy_m,vz_m); 
          
      ::Numerics::create_local_basis(n_p, m_p, l_p);
      ::Numerics::create_local_basis(n_m, m_m, l_m);

      T Tn_m,Tm_m,Tl_m;
      T Tn_p,Tm_p,Tl_p;

      // rotate fields into l, m, n basis
      ::Numerics::rotate_into_orthogonal_basis(n_m,m_m,l_m,Tx_m,Ty_m,Tz_m,Tn_m,Tm_m,Tl_m);
      ::Numerics::rotate_into_orthogonal_basis(n_p,m_p,l_p,Tx_p,Ty_p,Tz_p,Tn_p,Tm_p,Tl_p);

      T vn_m,vm_m,vl_m;
      T vn_p,vm_p,vl_p;
      ::Numerics::rotate_into_orthogonal_basis(n_m,m_m,l_m,vx_m,vy_m,vz_m,vn_m,vm_m,vl_m);      
      ::Numerics::rotate_into_orthogonal_basis(n_p,m_p,l_p,vx_p,vy_p,vz_p,vn_p,vm_p,vl_p);      

      
      // extract local s-wave and p-wave impedances
      T zs_m=rho_m*cs_m;
      T zs_p=rho_p*cs_p;

      T zp_m=rho_m*cp_m;
      T zp_p=rho_p*cp_p;      
          
      // impedance must be greater than zero !
      assertion3(!(zp_p <= 0.0 || zp_m <= 0.0),"Impedance must be greater than zero !",zp_p,zs_p);

      // generate interface data preserving the amplitude of the outgoing charactertritics
      // and satisfying interface conditions exactly.
      T vn_hat_p,vm_hat_p,vl_hat_p;
      T Tn_hat_p,Tm_hat_p,Tl_hat_p;        
      T vn_hat_m,vm_hat_m,vl_hat_m;
      T Tn_hat_m,Tm_hat_m,Tl_hat_m;

      if(is_fault){

        T Sn_m,Sm_m,Sl_m,Sn_p,Sm_p,Sl_p;
        T Sx_m,Sy_m,Sz_m,Sx_p,Sy_p,Sz_p;

        double x[3] = {QR[idx_QLR(i,j,s::curve_grid + 0)],
                QR[idx_QLR(i,j,s::curve_grid + 1)], 
                QR[idx_QLR(i,j,s::curve_grid + 2)]};

        Sx_p = QR[idx_QLR(i,j,s::u + 0)];
        Sy_p = QR[idx_QLR(i,j,s::u + 1)];
        Sz_p = QR[idx_QLR(i,j,s::u + 2)];

        Sx_m = QL[idx_QLR(i,j,s::u + 0)];
        Sy_m = QL[idx_QLR(i,j,s::u + 1)];
        Sz_m = QL[idx_QLR(i,j,s::u + 2)];
        
        // tarch::la::Vector<3,double> coords;
        double coords[3] = {
          QL[idx_QLR(i,j,s::curve_grid + 0 )],
          QL[idx_QLR(i,j,s::curve_grid + 1 )],
          QL[idx_QLR(i,j,s::curve_grid + 2 )] 
        };

        ::Numerics::rotate_into_orthogonal_basis(n_m, m_m, l_m, Sx_m, Sy_m, Sz_m, Sn_m, Sm_m, Sl_m);
        ::Numerics::rotate_into_orthogonal_basis(n_p, m_p, l_p, Sx_p, Sy_p, Sz_p, Sn_p, Sm_p, Sl_p);
  
        T S =  std::sqrt((Sl_p- Sl_m)*(Sl_p- Sl_m)+(Sm_p- Sm_m)*(Sm_p- Sm_m));

        SlipWeakeningFriction(
          vn_p,vn_m, Tn_p,Tn_m, zp_p , zp_m, vn_hat_p , vn_hat_m, Tn_hat_p,Tn_hat_m, vm_p,vm_m,
          Tm_p,Tm_m, zs_p,zs_m, vm_hat_p, vm_hat_m, Tm_hat_p,Tm_hat_m, vl_p,vl_m,Tl_p,Tl_m, zs_p,
          zs_m, vl_hat_p , vl_hat_m, Tl_hat_p,Tl_hat_m, l_p, m_p, n_p, 
          // coords.data(), 
          coords,
          S, t
        );	

      }
      else if (isBoundaryFace) {
        // 0 absorbing 1 free surface
        T r= faceIndex == surface ? 1 : 0;

        ::Numerics::riemannSolver_boundary(faceIndex,r,
            vn_m,vm_m,vl_m,
            Tn_m,Tm_m,Tl_m,
            zp_m,zs_m,
            vn_hat_m,vm_hat_m,vl_hat_m,
            Tn_hat_m,Tm_hat_m,Tl_hat_m);
        ::Numerics::riemannSolver_boundary(faceIndex,r,
            vn_p,vm_p,vl_p,
            Tn_p,Tm_p,Tl_p,
            zp_p,zs_p,
            vn_hat_p,vm_hat_p,vl_hat_p,
            Tn_hat_p,Tm_hat_p,Tl_hat_p);      
      }
      else {
        ::Numerics::riemannSolver_Nodal(vn_p, vn_m,
                Tn_p, Tn_m,
                zp_p , zp_m,
                vn_hat_p , vn_hat_m,
                Tn_hat_p, Tn_hat_m);
        ::Numerics::riemannSolver_Nodal(vm_p, vm_m,
                Tm_p, Tm_m,
                zs_p , zs_m,
                vm_hat_p , vm_hat_m,
                Tm_hat_p, Tm_hat_m);
        ::Numerics::riemannSolver_Nodal(vl_p, vl_m,
                Tl_p, Tl_m,
                zs_p , zs_m, 
                vl_hat_p , vl_hat_m,
                Tl_hat_p, Tl_hat_m);
      }

      //generate fluctuations in the local basis coordinates: n, m, l
      ::Numerics::compute_fluctuations_left(zp_m,
              Tn_m,Tn_hat_m,
              vn_m,vn_hat_m,
              FLn);
      ::Numerics::compute_fluctuations_left(zs_m,
              Tm_m,Tm_hat_m,
              vm_m,vm_hat_m,
              FLm);
      ::Numerics::compute_fluctuations_left(zs_m,
              Tl_m,Tl_hat_m,
              vl_m,vl_hat_m,
              FLl);

      ::Numerics::compute_fluctuations_right(zp_p,
              Tn_p,Tn_hat_p,
              vn_p,vn_hat_p,
              FRn);
      ::Numerics::compute_fluctuations_right(zs_p,
              Tm_p,Tm_hat_p,
              vm_p,vm_hat_p,
              FRm);
      ::Numerics::compute_fluctuations_right(zs_p,
              Tl_p,Tl_hat_p,
              vl_p,vl_hat_p,
              FRl);

      //Consider acoustic boundary
      FL_n = FLn/zp_m;
      if(zs_m > 0){
        FL_m = FLm/zs_m;
        FL_l = FLl/zs_m;
      }else{
        FL_m=0;
        FL_l=0;
      }
        
      FR_n = FRn/zp_p;
      if(zs_p > 0){    
        FR_m = FRm/zs_p;
        FR_l = FRl/zs_p;
      }else{
        FR_m=0;
        FR_l=0;
      }

      // rotate back to the physical coordinates x, y, z
      ::Numerics::rotate_into_physical_basis(n_m,m_m,l_m,
              FLn,FLm,FLl,
              FLx,FLy,FLz);
      ::Numerics::rotate_into_physical_basis(n_p,m_p,l_p,
              FRn,FRm,FRl,
              FRx,FRy,FRz);
      ::Numerics::rotate_into_physical_basis(n_m,m_m,l_m,
              FL_n,FL_m,FL_l,
              FL_x,FL_y,FL_z);
      ::Numerics::rotate_into_physical_basis(n_p,m_p,l_p,
              FR_n,FR_m,FR_l,
              FR_x,FR_y,FR_z);
        
      // construct flux fluctuation vectors obeying the eigen structure of the PDE
      // and choose physically motivated penalties such that we can prove
      // numerical stability.

      F_p[s::v + 0] = norm_p/rho_p*FRx;
      F_m[s::v + 0] = norm_m/rho_m*FLx;
        
      F_p[s::v + 1] = norm_p/rho_p*FRy;
      F_m[s::v + 1] = norm_m/rho_m*FLy;

      F_p[s::v + 2] = norm_p/rho_p*FRz;
      F_m[s::v + 2] = norm_m/rho_m*FLz;

      F_m[s::sigma + 0] =  norm_m*((2*mu_m+lam_m)*n_m[0]*FL_x+lam_m*n_m[1]*FL_y+lam_m*n_m[2]*FL_z);
      F_m[s::sigma + 1] =  norm_m*((2*mu_m+lam_m)*n_m[1]*FL_y+lam_m*n_m[0]*FL_x+lam_m*n_m[2]*FL_z);
      F_m[s::sigma + 2] =  norm_m*((2*mu_m+lam_m)*n_m[2]*FL_z+lam_m*n_m[0]*FL_x+lam_m*n_m[1]*FL_y);

      F_p[s::sigma + 0] = -norm_p*((2*mu_p+lam_p)*n_p[0]*FR_x+lam_p*n_p[1]*FR_y+lam_p*n_p[2]*FR_z);
      F_p[s::sigma + 1] = -norm_p*((2*mu_p+lam_p)*n_p[1]*FR_y+lam_p*n_p[0]*FR_x+lam_p*n_p[2]*FR_z);
      F_p[s::sigma + 2] = -norm_p*((2*mu_p+lam_p)*n_p[2]*FR_z+lam_p*n_p[0]*FR_x+lam_p*n_p[1]*FR_y);
        
      F_m[s::sigma + 3] =  norm_m*mu_m*(n_m[1]*FL_x + n_m[0]*FL_y);
      F_m[s::sigma + 4] =  norm_m*mu_m*(n_m[2]*FL_x + n_m[0]*FL_z);
      F_m[s::sigma + 5] =  norm_m*mu_m*(n_m[2]*FL_y + n_m[1]*FL_z);

      F_p[s::sigma + 3] = -norm_p*mu_p*(n_p[1]*FR_x + n_p[0]*FR_y);
      F_p[s::sigma + 4] = -norm_p*mu_p*(n_p[2]*FR_x + n_p[0]*FR_z);
      F_p[s::sigma + 5] = -norm_p*mu_p*(n_p[2]*FR_y + n_p[1]*FR_z);

      F_m[s::u + 0] = 0;
      F_m[s::u + 1] = 0;
      F_m[s::u + 2] = 0;

      F_p[s::u + 0] = 0;
      F_p[s::u + 1] = 0;
      F_p[s::u + 2] = 0;

      T norm_p_qr=norm_p;
      T norm_m_qr=norm_m;
      ::kernels::idx2 idx_pml(Dimensions,9);

      F_p[s::pml + idx_pml(0,0)] = n_p[0]*dmp_pml_x_p*norm_p*FRx;
      F_m[s::pml + idx_pml(0,0)] = n_m[0]*dmp_pml_x_m*norm_m*FLx;

      F_p[s::pml + idx_pml(0,1)] = n_p[0]*dmp_pml_x_p*norm_p*FRy;
      F_m[s::pml + idx_pml(0,1)] = n_m[0]*dmp_pml_x_m*norm_m*FLy;

      F_p[s::pml + idx_pml(0,2)] = n_p[0]*dmp_pml_x_p*norm_p*FRz;
      F_m[s::pml + idx_pml(0,2)] = n_m[0]*dmp_pml_x_m*norm_m*FLz;

      F_m[s::pml + idx_pml(0,3)] = n_m[0]*dmp_pml_x_m*norm_m*n_m[0]*FL_x;
      F_m[s::pml + idx_pml(0,4)] = n_m[0]*dmp_pml_x_m*norm_m*n_m[1]*FL_y;
      F_m[s::pml + idx_pml(0,5)] = n_m[0]*dmp_pml_x_m*norm_m*n_m[2]*FL_z;

      F_p[s::pml + idx_pml(0,3)] = -n_p[0]*dmp_pml_x_p*norm_p*n_p[0]*FR_x;
      F_p[s::pml + idx_pml(0,4)] = -n_p[0]*dmp_pml_x_p*norm_p*n_p[1]*FR_y;
      F_p[s::pml + idx_pml(0,5)] = -n_p[0]*dmp_pml_x_p*norm_p*n_p[2]*FR_z;

      F_m[s::pml + idx_pml(0,6)] =  n_m[0]*dmp_pml_x_m*norm_m*(n_m[1]*FL_x + n_m[0]*FL_y);
      F_m[s::pml + idx_pml(0,7)] =  n_m[0]*dmp_pml_x_m*norm_m*(n_m[2]*FL_x + n_m[0]*FL_z);
      F_m[s::pml + idx_pml(0,8)] =  n_m[0]*dmp_pml_x_m*norm_m*(n_m[2]*FL_y + n_m[1]*FL_z);

      F_p[s::pml + idx_pml(0,6)] = -n_p[0]*dmp_pml_x_p*norm_p*(n_p[1]*FR_x + n_p[0]*FR_y);
      F_p[s::pml + idx_pml(0,7)] = -n_p[0]*dmp_pml_x_p*norm_p*(n_p[2]*FR_x + n_p[0]*FR_z);
      F_p[s::pml + idx_pml(0,8)] = -n_p[0]*dmp_pml_x_p*norm_p*(n_p[2]*FR_y + n_p[1]*FR_z);

      // fs.pml + idx_pml(0,0)or y-direction auxiliary variables
      F_p[s::pml + idx_pml(1,0)] = n_p[1]*dmp_pml_y_p*norm_p*FRx;
      F_m[s::pml + idx_pml(1,0)] = n_m[1]*dmp_pml_y_m*norm_m*FLx;

      F_p[s::pml + idx_pml(1,1)] = n_p[1]*dmp_pml_y_p*norm_p*FRy;
      F_m[s::pml + idx_pml(1,1)] = n_m[1]*dmp_pml_y_m*norm_m*FLy;

      F_p[s::pml + idx_pml(1,2)] = n_p[1]*dmp_pml_y_p*norm_p*FRz;
      F_m[s::pml + idx_pml(1,2)] = n_m[1]*dmp_pml_y_m*norm_m*FLz;

      F_m[s::pml + idx_pml(1,3)] = n_m[1]*dmp_pml_y_m*norm_m*n_m[0]*FL_x;
      F_m[s::pml + idx_pml(1,4)] = n_m[1]*dmp_pml_y_m*norm_m*n_m[1]*FL_y;
      F_m[s::pml + idx_pml(1,5)] = n_m[1]*dmp_pml_y_m*norm_m*n_m[2]*FL_z;

      F_p[s::pml + idx_pml(1,3)] = -n_p[1]*dmp_pml_y_p*norm_p*n_p[0]*FR_x;
      F_p[s::pml + idx_pml(1,4)] = -n_p[1]*dmp_pml_y_p*norm_p*n_p[1]*FR_y;
      F_p[s::pml + idx_pml(1,5)] = -n_p[1]*dmp_pml_y_p*norm_p*n_p[2]*FR_z;

      F_m[s::pml + idx_pml(1,6)] =  n_m[1]*dmp_pml_y_m*norm_m*(n_m[1]*FL_x + n_m[0]*FL_y);
      F_m[s::pml + idx_pml(1,7)] =  n_m[1]*dmp_pml_y_m*norm_m*(n_m[2]*FL_x + n_m[0]*FL_z);
      F_m[s::pml + idx_pml(1,8)] =  n_m[1]*dmp_pml_y_m*norm_m*(n_m[2]*FL_y + n_m[1]*FL_z);

      F_p[s::pml + idx_pml(1,6)] = -n_p[1]*dmp_pml_y_p*norm_p*(n_p[1]*FR_x + n_p[0]*FR_y);
      F_p[s::pml + idx_pml(1,7)] = -n_p[1]*dmp_pml_y_p*norm_p*(n_p[2]*FR_x + n_p[0]*FR_z);
      F_p[s::pml + idx_pml(1,8)] = -n_p[1]*dmp_pml_y_p*norm_p*(n_p[2]*FR_y + n_p[1]*FR_z);

      // fs.pml + idx_pml(0,0)or z-direction auxiliary variables
      F_p[s::pml + idx_pml(2,0)] = n_p[2]*dmp_pml_z_p*norm_p*FRx;
      F_m[s::pml + idx_pml(2,0)] = n_m[2]*dmp_pml_z_m*norm_m*FLx;

      F_p[s::pml + idx_pml(2,1)] = n_p[2]*dmp_pml_z_p*norm_p*FRy;
      F_m[s::pml + idx_pml(2,1)] = n_m[2]*dmp_pml_z_m*norm_m*FLy;

      F_p[s::pml + idx_pml(2,2)] = n_p[2]*dmp_pml_z_p*norm_p*FRz;
      F_m[s::pml + idx_pml(2,2)] = n_m[2]*dmp_pml_z_m*norm_m*FLz;

      F_m[s::pml + idx_pml(2,3)] = n_m[2]*dmp_pml_z_m*norm_m*n_m[0]*FL_x;
      F_m[s::pml + idx_pml(2,4)] = n_m[2]*dmp_pml_z_m*norm_m*n_m[1]*FL_y;
      F_m[s::pml + idx_pml(2,5)] = n_m[2]*dmp_pml_z_m*norm_m*n_m[2]*FL_z;

      F_p[s::pml + idx_pml(2,3)] = -n_p[2]*dmp_pml_z_p*norm_p*n_p[0]*FR_x;
      F_p[s::pml + idx_pml(2,4)] = -n_p[2]*dmp_pml_z_p*norm_p*n_p[1]*FR_y;
      F_p[s::pml + idx_pml(2,5)] = -n_p[2]*dmp_pml_z_p*norm_p*n_p[2]*FR_z;

      F_m[s::pml + idx_pml(2,6)] =  n_m[2]*dmp_pml_z_m*norm_m*(n_m[1]*FL_x + n_m[0]*FL_y);
      F_m[s::pml + idx_pml(2,7)] =  n_m[2]*dmp_pml_z_m*norm_m*(n_m[2]*FL_x + n_m[0]*FL_z);
      F_m[s::pml + idx_pml(2,8)] =  n_m[2]*dmp_pml_z_m*norm_m*(n_m[2]*FL_y + n_m[1]*FL_z);

      F_p[s::pml + idx_pml(2,6)] = -n_p[2]*dmp_pml_z_p*norm_p*(n_p[1]*FR_x + n_p[0]*FR_y);
      F_p[s::pml + idx_pml(2,7)] = -n_p[2]*dmp_pml_z_p*norm_p*(n_p[2]*FR_x + n_p[0]*FR_z);
      F_p[s::pml + idx_pml(2,8)] = -n_p[2]*dmp_pml_z_p*norm_p*(n_p[2]*FR_y + n_p[1]*FR_z);
    }    
  }
}