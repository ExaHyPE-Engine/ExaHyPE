
#include "riemannSolverLOH.h"

examples::exahype2::elastic::VariableShortcuts sh;

template<typename T>
void loh::riemannSolver::riemannSolver(
  T* const FL_,
  T* const FR_,
  const T* const QL_,
  const T* const QR_,
  const double t,
  const double dt,
  const tarch::la::Vector<Dimensions, double>& lengthScale,
  const int normalNonZeroIndex, 
  bool isBoundaryFace, 
  int faceIndex){

#ifdef OPT_KERNELS
  T FL[converter::getFFaceGenArraySize()];
  T FR[converter::getFFaceGenArraySize()];
  T QL[converter::getQFaceGenArraySize()];
  T QR[converter::getQFaceGenArraySize()];
  
  converter::FFace_optimised2generic(FL_,FL);
  converter::FFace_optimised2generic(FR_,FR);
  converter::QFace_optimised2generic(QL_,QL);
  converter::QFace_optimised2generic(QR_,QR);
#else
  T* FL=FL_;
  T* FR=FR_;
  const T* QL=QL_;
  const T* QR=QR_;
#endif

  constexpr int numberOfVariables  = examples::exahype2::elastic::Abstractloh::NumberOfVariables;
  constexpr int numberOfVariables2 = numberOfVariables*numberOfVariables;
  constexpr int numberOfParameters = examples::exahype2::elastic::Abstractloh::NumberOfParameters;
  constexpr int numberOfData       = numberOfVariables+numberOfParameters;
  constexpr int basisSize          = examples::exahype2::elastic::Abstractloh::Order+1;
  constexpr int order              = basisSize - 1;

  kernels::idx3 idx_QLR(basisSize,basisSize,numberOfData);
  kernels::idx3 idx_FLR(basisSize,basisSize,numberOfVariables);

  T n[3]={0,0,0};
  n[normalNonZeroIndex]=1;

  T n_p[3]={0,0,0};
  T n_m[3]={0,0,0};

  T m_p[3]={0,0,0};
  T m_m[3]={0,0,0};

  T l_p[3]={0,0,0};  
  T l_m[3]={0,0,0};

  T norm_p_qr = 1.0;
  T norm_m_qr = 1.0;
  
  T FLn, FLm, FLl, FRn,FRm,FRl;
  T FL_n,FL_m,FL_l,FR_n,FR_m,FR_l;
  T FLx,FLy,FLz,FRx,FRy,FRz;
  T FL_x,FL_y,FL_z,FR_x,FR_y,FR_z;

  for (int k = 0; k < 3; k++){

    n_m[k] = n[k];
    n_p[k] = n[k];
  }

  for (int i = 0; i < basisSize; i++) {
    for (int j = 0; j < basisSize; j++) {
      T rho_p=QR[idx_QLR(i,j,9)];
      T c_p_p=QR[idx_QLR(i,j,10)];
      T c_s_p=QR[idx_QLR(i,j,11)];
      T mu_p=c_s_p*c_s_p*rho_p;
      T lam_p = rho_p*c_p_p*c_p_p-2*mu_p;      

      T rho_m=QL[idx_QLR(i,j,9)];
      T c_p_m=QL[idx_QLR(i,j,10)];
      T c_s_m=QL[idx_QLR(i,j,11)];
      T mu_m=c_s_m*c_s_m*rho_m;
      T lam_m = rho_m*c_p_m*c_p_m-2*mu_m;      
  

      T Tx_m,Ty_m,Tz_m,Tx_p,Ty_p,Tz_p;
      T vx_m,vy_m,vz_m,vx_p,vy_p,vz_p;
      
      extract_tractions_and_particle_velocity(n_p,QR+idx_QLR(i,j,0),Tx_p,Ty_p,Tz_p,vx_p,vy_p,vz_p );
      extract_tractions_and_particle_velocity(n_m,QL+idx_QLR(i,j,0),Tx_m,Ty_m,Tz_m,vx_m,vy_m,vz_m ); 
      
      localBasis(n_p, m_p, l_p, 3);
      localBasis(n_m, m_m, l_m, 3);

      T Tn_m,Tm_m,Tl_m,vn_m,vm_m,vl_m;
      T Tn_p,Tm_p,Tl_p,vn_p,vm_p,vl_p;

      // rotate fields into l, m, n basis
      rotate_into_orthogonal_basis(n_m,m_m,l_m,Tx_m,Ty_m,Tz_m,Tn_m,Tm_m,Tl_m);
      rotate_into_orthogonal_basis(n_m,m_m,l_m,vx_m,vy_m,vz_m,vn_m,vm_m,vl_m);
      rotate_into_orthogonal_basis(n_p,m_p,l_p,Tx_p,Ty_p,Tz_p,Tn_p,Tm_p,Tl_p);
      rotate_into_orthogonal_basis(n_p,m_p,l_p,vx_p,vy_p,vz_p,vn_p,vm_p,vl_p);      
      
  
      // extract local s-wave and p-wave impedances
      T zs_p=rho_p*c_s_p;
      T zp_p=rho_p*c_p_p;
      T zs_m=rho_m*c_s_m;
      T zp_m=rho_m*c_p_m;
      
      // impedance must be greater than zero !
      if (zp_p <= 0.0 || zp_m <= 0.0){
        std::cout << "At position: " << isBoundaryFace << "\n";
        std::cout<<zs_p<<" "<<zp_p<<" "<<zs_m<<" "<<zp_m<<"\n";
        std::cout<<" Impedance must be greater than zero ! "<< std::endl;
        std::exit(-1);
      }

      // generate interface data preserving the amplitude of the outgoing charactertritics
      // and satisfying interface conditions exactly.
      T vn_hat_p,vm_hat_p,vl_hat_p,Tn_hat_p,Tm_hat_p,Tl_hat_p;        
      T vn_hat_m,vm_hat_m,vl_hat_m,Tn_hat_m,Tm_hat_m,Tl_hat_m;    

      if (isBoundaryFace) {
        // 1 absorbing 0 free surface
        T r= faceIndex==2 ? 1 : 0;
        //	T r=1.;
        riemannSolver_boundary(faceIndex,r,vn_m,vm_m,vl_m,Tn_m,Tm_m,Tl_m,zp_m,zs_m,vn_hat_m,vm_hat_m,vl_hat_m,Tn_hat_m,Tm_hat_m,Tl_hat_m);
        riemannSolver_boundary(faceIndex,r,vn_p,vm_p,vl_p,Tn_p,Tm_p,Tl_p,zp_p,zs_p,vn_hat_p,vm_hat_p,vl_hat_p,Tn_hat_p,Tm_hat_p,Tl_hat_p);      
      }else {
        riemannSolver_Nodal(vn_p,vn_m, Tn_p, Tn_m, zp_p , zp_m, vn_hat_p , vn_hat_m, Tn_hat_p, Tn_hat_m);
        riemannSolver_Nodal(vm_p,vm_m, Tm_p, Tm_m, zs_p , zs_m, vm_hat_p , vm_hat_m, Tm_hat_p, Tm_hat_m);
        riemannSolver_Nodal(vl_p,vl_m, Tl_p, Tl_m, zs_p , zs_m, vl_hat_p , vl_hat_m, Tl_hat_p, Tl_hat_m);
      }

      //generate fluctuations in the local basis coordinates: n, m, l
      generate_fluctuations_left(zp_m,Tn_m,Tn_hat_m,vn_m,vn_hat_m,FLn);
      generate_fluctuations_left(zs_m,Tm_m,Tm_hat_m,vm_m,vm_hat_m,FLm);
      generate_fluctuations_left(zs_m,Tl_m,Tl_hat_m,vl_m,vl_hat_m,FLl);

      generate_fluctuations_right(zp_p,Tn_p,Tn_hat_p,vn_p,vn_hat_p,FRn);
      generate_fluctuations_right(zs_p,Tm_p,Tm_hat_p,vm_p,vm_hat_p,FRm);
      generate_fluctuations_right(zs_p,Tl_p,Tl_hat_p,vl_p,vl_hat_p,FRl);

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
      rotate_into_physical_basis(n_m,m_m,l_m,FLn,FLm,FLl,FLx,FLy,FLz);
      rotate_into_physical_basis(n_p,m_p,l_p,FRn,FRm,FRl,FRx,FRy,FRz);
      rotate_into_physical_basis(n_m,m_m,l_m,FL_n,FL_m,FL_l,FL_x,FL_y,FL_z);
      rotate_into_physical_basis(n_p,m_p,l_p,FR_n,FR_m,FR_l,FR_x,FR_y,FR_z);
     
      // construct flux fluctuation vectors obeying the eigen structure of the PDE
      // and choose physically motivated penalties such that we can prove
      // numerical stability.

      FR[idx_FLR(i,j, 0)] = norm_p_qr/rho_p*FRx;
      FL[idx_FLR(i,j, 0)] = norm_m_qr/rho_m*FLx;
    
      FR[idx_FLR(i,j, 1)] = norm_p_qr/rho_p*FRy;
      FL[idx_FLR(i,j, 1)] = norm_m_qr/rho_m*FLy;

      FR[idx_FLR(i,j, 2)] = norm_p_qr/rho_p*FRz;
      FL[idx_FLR(i,j, 2)] = norm_m_qr/rho_m*FLz;

      FL[idx_FLR(i,j, 3)] = norm_m_qr*((2*mu_m+lam_m)*n_m[0]*FL_x+lam_m*n_m[1]*FL_y+lam_m*n_m[2]*FL_z);
      FL[idx_FLR(i,j, 4)] = norm_m_qr*((2*mu_m+lam_m)*n_m[1]*FL_y+lam_m*n_m[0]*FL_x+lam_m*n_m[2]*FL_z);
      FL[idx_FLR(i,j, 5)] = norm_m_qr*((2*mu_m+lam_m)*n_m[2]*FL_z+lam_m*n_m[0]*FL_x+lam_m*n_m[1]*FL_y);

      FR[idx_FLR(i,j, 3)] = -norm_p_qr*((2*mu_p+lam_p)*n_p[0]*FR_x+lam_p*n_p[1]*FR_y+lam_p*n_p[2]*FR_z);
      FR[idx_FLR(i,j, 4)] = -norm_p_qr*((2*mu_p+lam_p)*n_p[1]*FR_y+lam_p*n_p[0]*FR_x+lam_p*n_p[2]*FR_z);
      FR[idx_FLR(i,j, 5)] = -norm_p_qr*((2*mu_p+lam_p)*n_p[2]*FR_z+lam_p*n_p[0]*FR_x+lam_p*n_p[1]*FR_y);
    
      FL[idx_FLR(i,j, 6)] =  norm_m_qr*mu_m*(n_m[1]*FL_x + n_m[0]*FL_y);
      FL[idx_FLR(i,j, 7)] =  norm_m_qr*mu_m*(n_m[2]*FL_x + n_m[0]*FL_z);
      FL[idx_FLR(i,j, 8)] =  norm_m_qr*mu_m*(n_m[2]*FL_y + n_m[1]*FL_z);

      FR[idx_FLR(i,j, 6)] = -norm_p_qr*mu_p*(n_p[1]*FR_x + n_p[0]*FR_y);
      FR[idx_FLR(i,j, 7)] = -norm_p_qr*mu_p*(n_p[2]*FR_x + n_p[0]*FR_z);
      FR[idx_FLR(i,j, 8)] = -norm_p_qr*mu_p*(n_p[2]*FR_y + n_p[1]*FR_z);
    }    
  }

#ifdef OPT_KERNELS
  converter::FFace_generic2optimised(FL,FL_);
  converter::FFace_generic2optimised(FR,FR_);
#endif 

}

template void loh::riemannSolver::riemannSolver<float>(
  float* const FL_,
  float* const FR_,
  const float* const QL_,
  const float* const QR_,
  const double t,
  const double dt,
  const tarch::la::Vector<Dimensions, double>& lengthScale,
  const int normalNonZeroIndex, 
  bool isBoundaryFace, 
  int faceIndex);
template void loh::riemannSolver::riemannSolver<double>(
  double* const FL_,
  double* const FR_,
  const double* const QL_,
  const double* const QR_,
  const double t,
  const double dt,
  const tarch::la::Vector<Dimensions, double>& lengthScale,
  const int normalNonZeroIndex, 
  bool isBoundaryFace, 
  int faceIndex);

//Gram Schmidt orthonormalization
template<typename T>
void loh::riemannSolver::Gram_Schmidt(T* const y, T* const z){
  T  a_yz = y[0]*z[0] + y[1]*z[1] + y[2]*z[2];

  for (int i = 0; i< 3; i++){
    z[i] = z[i] - a_yz*y[i];
  }
  
  T norm_z = std::sqrt(z[0]*z[0] + z[1]*z[1] + z[2]*z[2]);
  
  for (int i = 0; i< 3; i++){
    z[i] =  z[i]/norm_z;
  }
}

template<typename T>
void loh::riemannSolver::localBasis(T* const n, T* const m, T* const l, int d){
  if (d == 2){
    l[0] = 0.;
    l[1] = 0.;
    l[2] = 1.0;
    
    m[0] = n[1]*l[2]-n[2]*l[1];
    m[1] = -(n[0]*l[2]-n[2]*l[0]);
    m[2] = n[0]*l[1]-n[1]*l[0];
  }else if (d == 3){
    T tol, diff_norm1, diff_norm2;
    tol = 1e-12;
    m[0] = 0.;
    m[1] = 1.;
    m[2] = 0.;
    
    diff_norm1 = kernels::pow2(n[0]-m[0]) + kernels::pow2(n[1]-m[1]) + kernels::pow2(n[2]-m[2]);
    diff_norm2 = kernels::pow2(n[0]+m[0]) + kernels::pow2(n[1]+m[1]) + kernels::pow2(n[2]+m[2]);
    
    if (diff_norm1 >= kernels::pow2(tol) && diff_norm2 >= kernels::pow2(tol)){
      Gram_Schmidt(n, m);
    }else{
      m[0] = 0.;
      m[1] = 0.;
      m[2] = 1.;
      Gram_Schmidt(n, m);
    }
    l[0] = n[1]*m[2]-n[2]*m[1];
    l[1] = -(n[0]*m[2]-n[2]*m[0]);
    l[2] = n[0]*m[1]-n[1]*m[0];
  }
} 



template<typename T>
void loh::riemannSolver::riemannSolver_Nodal(T v_p,T v_m, T sigma_p, T sigma_m, T z_p , T z_m, T& v_hat_p , T& v_hat_m, T& sigma_hat_p, T& sigma_hat_m){
  T p=0;
  T q=0;
  T phi=0;
  T v_hat=0;
  T eta=0;

  p=z_m*v_p + sigma_p;
  q=z_p*v_m - sigma_m;

  if(z_p > 0 && z_m > 0){
    eta=(z_p*z_m)/(z_p+z_m);

    phi= eta*(p/z_p - q/z_m);
     
    sigma_hat_p=phi;
    sigma_hat_m=phi;

    v_hat_p=(q+phi)/z_m;     
    v_hat_m=(p-phi)/z_p;
  }else if(z_p > 0){
    sigma_hat_p=0;
    sigma_hat_m=sigma_m;

    v_hat_p=v_p;     
    v_hat_m=v_m;
  }else if(z_m > 0){
    sigma_hat_p=sigma_p;
    sigma_hat_m=0;

    v_hat_p=v_p;     
    v_hat_m=v_m;
  }else{
    sigma_hat_p=sigma_p;
    sigma_hat_m=sigma_m;
     
    v_hat_p=v_p;
    v_hat_m=v_m;     
  }
 }

template<typename T>
void loh::riemannSolver::riemannSolver_BC0(T v, T sigma, T z,  T r, T& v_hat, T& sigma_hat){
  T p = 0.5*(z*v + sigma);
  if(z > 0){
    v_hat = (1+r)/z*p;
    sigma_hat = (1-r)*p;
  }else{
    v_hat = v;
    sigma_hat = sigma;
  }
}

template<typename T>
void loh::riemannSolver::riemannSolver_BCn(T v,T sigma, T z, T r, T& v_hat, T& sigma_hat){
  T q = 0.5*(z*v - sigma);
  if(z > 0){
    v_hat = (1+r)/z*q;
    sigma_hat = -(1-r)*q;
  }else{
    v_hat = v;
    sigma_hat = sigma;
  }
}



template<typename T>
void loh::riemannSolver::extract_tractions_and_particle_velocity(T* const n,const T* const Q, T& Tx,T& Ty,T& Tz,T& vx,T& vy,T& vz ){
  T sigma_xx = Q[3];
  T sigma_yy = Q[4];
  T sigma_zz = Q[5];
  T sigma_xy = Q[6];
  T sigma_xz = Q[7];
  T sigma_yz = Q[8];
  
  Tx = n[0]*sigma_xx + n[1]*sigma_xy + n[2]*sigma_xz;
  Ty = n[0]*sigma_xy + n[1]*sigma_yy + n[2]*sigma_yz;
  Tz = n[0]*sigma_xz + n[1]*sigma_yz + n[2]*sigma_zz;    
  
  vx = Q[0];
  vy = Q[1];
  vz = Q[2];    
}

template<typename T>
void loh::riemannSolver::rotate_into_orthogonal_basis(T* const n,T* const m,T* const l, T Tx,T Ty,T Tz, T& Tn, T& Tm, T& Tl){
    Tn= Tx*n[0] + Ty*n[1] + Tz*n[2];
    Tm= Tx*m[0] + Ty*m[1] + Tz*m[2];
    Tl= Tx*l[0] + Ty*l[1] + Tz*l[2];
}

template<typename T>
void loh::riemannSolver::rotate_into_physical_basis(T* const n,T* const m,T* const l, T Fn,T Fm,T Fl, T& Fx, T& Fy, T& Fz){
  Fx = n[0]*Fn + m[0]*Fm + l[0]*Fl;
  Fy = n[1]*Fn + m[1]*Fm + l[1]*Fl;
  Fz = n[2]*Fn + m[2]*Fm + l[2]*Fl;
}

template<typename T>
void loh::riemannSolver::generate_fluctuations_left(T z,  T myT,T T_hat,T v, T v_hat, T& F){
  F = 0.5*(z*(v-v_hat) + (myT-T_hat));
}

template<typename T>
void loh::riemannSolver::generate_fluctuations_right(T z,  T myT,T T_hat,T v, T v_hat, T& F){
  F = 0.5*(z*(v-v_hat) - (myT-T_hat));
}

template<typename T>
void loh::riemannSolver::riemannSolver_boundary(int faceIndex,T r, T vn , T vm , T vl, T Tn , T Tm ,T Tl , T zp, T zs , T& vn_hat , T& vm_hat ,T& vl_hat , T& Tn_hat , T& Tm_hat ,T& Tl_hat)
{
  if (faceIndex % 2  == 0) {
    riemannSolver_BC0(vn, Tn, zp, r, vn_hat, Tn_hat);
    riemannSolver_BC0(vm, Tm, zs, r, vm_hat, Tm_hat);
    riemannSolver_BC0(vl, Tl, zs, r, vl_hat, Tl_hat);	
  }
      
  if (faceIndex % 2 == 1) {
    riemannSolver_BCn(vn, Tn, zp, r, vn_hat, Tn_hat);
    riemannSolver_BCn(vm, Tm, zs, r, vm_hat, Tm_hat);
    riemannSolver_BCn(vl, Tl, zs, r, vl_hat, Tl_hat);	
  }
}