
#include "riemannSolverLOH.h"

examples::exahype2::elastic::VariableShortcuts sh;

void loh::riemannSolver::riemannSolver(
  double* const FL_,
  double* const FR_,
  const double* const QL_,
  const double* const QR_,
  const double t,
  const double dt,
  const tarch::la::Vector<Dimensions, double>& lengthScale,
  const int normalNonZeroIndex, 
  bool isBoundaryFace, 
  int faceIndex){

#ifdef OPT_KERNELS
  double FL[converter::getFFaceGenArraySize()];
  double FR[converter::getFFaceGenArraySize()];
  double QL[converter::getQFaceGenArraySize()];
  double QR[converter::getQFaceGenArraySize()];
  
  converter::FFace_optimised2generic(FL_,FL);
  converter::FFace_optimised2generic(FR_,FR);
  converter::QFace_optimised2generic(QL_,QL);
  converter::QFace_optimised2generic(QR_,QR);
#else
  double* FL=FL_;
  double* FR=FR_;
  const double* QL=QL_;
  const double* QR=QR_;
#endif

  constexpr int numberOfVariables  = examples::exahype2::elastic::Abstractloh::NumberOfVariables;
  constexpr int numberOfVariables2 = numberOfVariables*numberOfVariables;
  constexpr int numberOfParameters = examples::exahype2::elastic::Abstractloh::NumberOfParameters;
  constexpr int numberOfData       = numberOfVariables+numberOfParameters;
  constexpr int basisSize          = examples::exahype2::elastic::Abstractloh::Order+1;
  constexpr int order              = basisSize - 1;

  kernels::idx3 idx_QLR(basisSize,basisSize,numberOfData);
  kernels::idx3 idx_FLR(basisSize,basisSize,numberOfVariables);

  double n[3]={0,0,0};
  n[normalNonZeroIndex]=1;

  double n_p[3]={0,0,0};
  double n_m[3]={0,0,0};

  double m_p[3]={0,0,0};
  double m_m[3]={0,0,0};

  double l_p[3]={0,0,0};  
  double l_m[3]={0,0,0};

  double norm_p_qr = 1.0;
  double norm_m_qr = 1.0;
  
  double FLn, FLm, FLl, FRn,FRm,FRl;
  double FL_n,FL_m,FL_l,FR_n,FR_m,FR_l;
  double FLx,FLy,FLz,FRx,FRy,FRz;
  double FL_x,FL_y,FL_z,FR_x,FR_y,FR_z;

   for (int k = 0; k < 3; k++){

     n_m[k] = n[k];
     n_p[k] = n[k];
   }

  for (int i = 0; i < basisSize; i++) {
    for (int j = 0; j < basisSize; j++) {
      double rho_p=QR[idx_QLR(i,j,9)];
      double c_p_p=QR[idx_QLR(i,j,10)];
      double c_s_p=QR[idx_QLR(i,j,11)];
      double mu_p=c_s_p*c_s_p*rho_p;
      double lam_p = rho_p*c_p_p*c_p_p-2*mu_p;      

      double rho_m=QL[idx_QLR(i,j,9)];
      double c_p_m=QL[idx_QLR(i,j,10)];
      double c_s_m=QL[idx_QLR(i,j,11)];
      double mu_m=c_s_m*c_s_m*rho_m;
      double lam_m = rho_m*c_p_m*c_p_m-2*mu_m;      
  

      double Tx_m,Ty_m,Tz_m,Tx_p,Ty_p,Tz_p;
      double vx_m,vy_m,vz_m,vx_p,vy_p,vz_p;
      
      extract_tractions_and_particle_velocity(n_p,QR+idx_QLR(i,j,0),Tx_p,Ty_p,Tz_p,vx_p,vy_p,vz_p );
      extract_tractions_and_particle_velocity(n_m,QL+idx_QLR(i,j,0),Tx_m,Ty_m,Tz_m,vx_m,vy_m,vz_m ); 
      
      localBasis(n_p, m_p, l_p, 3);
      localBasis(n_m, m_m, l_m, 3);

      double Tn_m,Tm_m,Tl_m,vn_m,vm_m,vl_m;
      double Tn_p,Tm_p,Tl_p,vn_p,vm_p,vl_p;

      // rotate fields into l, m, n basis
      rotate_into_orthogonal_basis(n_m,m_m,l_m,Tx_m,Ty_m,Tz_m,Tn_m,Tm_m,Tl_m);
      rotate_into_orthogonal_basis(n_m,m_m,l_m,vx_m,vy_m,vz_m,vn_m,vm_m,vl_m);
      rotate_into_orthogonal_basis(n_p,m_p,l_p,Tx_p,Ty_p,Tz_p,Tn_p,Tm_p,Tl_p);
      rotate_into_orthogonal_basis(n_p,m_p,l_p,vx_p,vy_p,vz_p,vn_p,vm_p,vl_p);      
      
  
      // extract local s-wave and p-wave impedances
      double zs_p=rho_p*c_s_p;
      double zp_p=rho_p*c_p_p;
      double zs_m=rho_m*c_s_m;
      double zp_m=rho_m*c_p_m;
      
      // impedance must be greater than zero !
      if (zp_p <= 0.0 || zp_m <= 0.0){
        std::cout << "At position: " << isBoundaryFace << "\n";
	std::cout<<zs_p<<" "<<zp_p<<" "<<zs_m<<" "<<zp_m<<"\n";
	std::cout<<" Impedance must be greater than zero ! "<< std::endl;
	std::exit(-1);
      }

      // generate interface data preserving the amplitude of the outgoing charactertritics
      // and satisfying interface conditions exactly.
      double vn_hat_p,vm_hat_p,vl_hat_p,Tn_hat_p,Tm_hat_p,Tl_hat_p;        
      double vn_hat_m,vm_hat_m,vl_hat_m,Tn_hat_m,Tm_hat_m,Tl_hat_m;    

      if (isBoundaryFace) {
	// 1 absorbing 0 free surface
	double r= faceIndex==2 ? 1 : 0;
	//	double r=1.;
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

//Gram Schmidt orthonormalization
void loh::riemannSolver::Gram_Schmidt(double* const y, double* const z){
  double  a_yz = y[0]*z[0] + y[1]*z[1] + y[2]*z[2];

  for (int i = 0; i< 3; i++){
    z[i] = z[i] - a_yz*y[i];
  }
  
  double norm_z = std::sqrt(z[0]*z[0] + z[1]*z[1] + z[2]*z[2]);
  
  for (int i = 0; i< 3; i++){
    z[i] =  z[i]/norm_z;
  }
}

void loh::riemannSolver::localBasis(double* const n, double* const m, double* const l, int d){
  if (d == 2){
      l[0] = 0.;
      l[1] = 0.;
      l[2] = 1.0;
      
      m[0] = n[1]*l[2]-n[2]*l[1];
      m[1] = -(n[0]*l[2]-n[2]*l[0]);
      m[2] = n[0]*l[1]-n[1]*l[0];
  }else if (d == 3){
      double tol, diff_norm1, diff_norm2;
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



void loh::riemannSolver::riemannSolver_Nodal(double v_p,double v_m, double sigma_p, double sigma_m, double z_p , double z_m, double& v_hat_p , double& v_hat_m, double& sigma_hat_p, double& sigma_hat_m){
  double p=0;
  double q=0;
  double phi=0;
  double v_hat=0;
  double eta=0;

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

void loh::riemannSolver::riemannSolver_BC0(double v, double sigma, double z,  double r, double& v_hat, double& sigma_hat){
   double p = 0.5*(z*v + sigma);
   if(z > 0){
     v_hat = (1+r)/z*p;
     sigma_hat = (1-r)*p;
   }else{
     v_hat = v;
     sigma_hat = sigma;
   }
}

void loh::riemannSolver::riemannSolver_BCn(double v,double sigma, double z, double r, double& v_hat, double& sigma_hat){
   double q = 0.5*(z*v - sigma);
   if(z > 0){
     v_hat = (1+r)/z*q;
     sigma_hat = -(1-r)*q;
   }else{
     v_hat = v;
     sigma_hat = sigma;
   }
}



void loh::riemannSolver::extract_tractions_and_particle_velocity(double* const n,const double* const Q, double& Tx,double& Ty,double& Tz,double& vx,double& vy,double& vz ){
  double sigma_xx = Q[3];
  double sigma_yy = Q[4];
  double sigma_zz = Q[5];
  double sigma_xy = Q[6];
  double sigma_xz = Q[7];
  double sigma_yz = Q[8];
  
  Tx = n[0]*sigma_xx + n[1]*sigma_xy + n[2]*sigma_xz;
  Ty = n[0]*sigma_xy + n[1]*sigma_yy + n[2]*sigma_yz;
  Tz = n[0]*sigma_xz + n[1]*sigma_yz + n[2]*sigma_zz;    
  
  vx = Q[0];
  vy = Q[1];
  vz = Q[2];    
}

void loh::riemannSolver::rotate_into_orthogonal_basis(double* const n,double* const m,double* const l, double Tx,double Ty,double Tz, double& Tn, double& Tm, double& Tl){
    Tn= Tx*n[0] + Ty*n[1] + Tz*n[2];
    Tm= Tx*m[0] + Ty*m[1] + Tz*m[2];
    Tl= Tx*l[0] + Ty*l[1] + Tz*l[2];
}

void loh::riemannSolver::rotate_into_physical_basis(double* const n,double* const m,double* const l, double Fn,double Fm,double Fl, double& Fx, double& Fy, double& Fz){
  Fx = n[0]*Fn + m[0]*Fm + l[0]*Fl;
  Fy = n[1]*Fn + m[1]*Fm + l[1]*Fl;
  Fz = n[2]*Fn + m[2]*Fm + l[2]*Fl;
}

void loh::riemannSolver::generate_fluctuations_left(double z,  double T,double T_hat,double v, double v_hat, double& F){
  F = 0.5*(z*(v-v_hat) + (T-T_hat));
}

void loh::riemannSolver::generate_fluctuations_right(double z,  double T,double T_hat,double v, double v_hat, double& F){
  F = 0.5*(z*(v-v_hat) - (T-T_hat));
}

void loh::riemannSolver::riemannSolver_boundary(int faceIndex,double r, double vn , double vm , double vl, double Tn , double Tm ,double Tl , double zp, double zs , double& vn_hat , double& vm_hat ,double& vl_hat , double& Tn_hat , double& Tm_hat ,double& Tl_hat)
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