
#include "exahype2/aderdg/kernels/Utils/KernelUtils.h"

namespace Numerics{

    //Getter routines
  template <class Shortcuts>
  inline void compute_parameters(const double*Q, double& rho, double& cp, double& cs, double& mu, double& lam){
    Shortcuts s;
    rho = Q[s.rho];
    cp  = Q[s.cp ];
    cs  = Q[s.cs ];
    mu  = cs * cs * rho;
    lam = rho* cp * cp - 2.0 * mu;
  }


  inline void riemannSolver_boundary(int faceIndex,double r, double vn , double vm , double vl, double Tn , double Tm ,double Tl , double zp, double zs , double& vn_hat , double& vm_hat ,double& vl_hat , double& Tn_hat , double& Tm_hat ,double& Tl_hat)
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


  template<class Shortcuts,int order, int numberOfVariables, int numberOfParameters , int surface = 2>
  void riemannSolver(double* FL,double* FR,const double* const QL,const double* const QR,const double dt,const int direction, bool isBoundaryFace, int faceIndex){
 
    constexpr int numberOfData       = numberOfVariables+numberOfParameters;
    constexpr int basisSize          = order+1;

    kernels::idx3 idx_QLR(basisSize,basisSize,numberOfData);
    kernels::idx3 idx_FLR(basisSize,basisSize,numberOfVariables);

    double FLn ,FLm ,FLl ,FRn ,FRm ,FRl;
    double FLx ,FLy ,FLz ,FRx ,FRy ,FRz;
    double FL_n,FL_m,FL_l,FR_n,FR_m,FR_l;
    double FL_x,FL_y,FL_z,FR_x,FR_y,FR_z;


    Shortcuts s;
    for (int i = 0; i < basisSize; i++) {
      for (int j = 0; j < basisSize; j++) {

	const double* Q_m = QL+idx_QLR(i,j,0);
	const double* Q_p = QR+idx_QLR(i,j,0);
        
        double dmp_pml_x_m = Q_m[s.dmp_pml + 0];
        double dmp_pml_y_m = Q_m[s.dmp_pml + 1];
        double dmp_pml_z_m = Q_m[s.dmp_pml + 2];

        double dmp_pml_x_p = Q_p[s.dmp_pml + 0];
        double dmp_pml_y_p = Q_p[s.dmp_pml + 1];
        double dmp_pml_z_p = Q_p[s.dmp_pml + 2];
	
	double* F_m = FL+idx_FLR(i,j,0);
	double* F_p = FR+idx_FLR(i,j,0);
	double rho_m,cp_m,cs_m,mu_m,lam_m;
	double rho_p,cp_p,cs_p,mu_p,lam_p;

	compute_parameters<Shortcuts>(Q_m,rho_m,cp_m,cs_m,mu_m,lam_m);
	compute_parameters<Shortcuts>(Q_p,rho_p,cp_p,cs_p,mu_p,lam_p);

	double n_m[3],m_m[3],l_m[3];
	double n_p[3],m_p[3],l_p[3];
	double norm_p,norm_m;

	get_normals<Shortcuts>(Q_m,direction,norm_m,n_m);
	get_normals<Shortcuts>(Q_p,direction,norm_p,n_p);


	double Tx_m,Ty_m,Tz_m;
	double Tx_p,Ty_p,Tz_p;
	compute_tractions<Shortcuts>(Q_p,n_p,Tx_p,Ty_p,Tz_p);
	compute_tractions<Shortcuts>(Q_m,n_m,Tx_m,Ty_m,Tz_m);

	double vx_m,vy_m,vz_m;
	double vx_p,vy_p,vz_p;
	get_velocities<Shortcuts>(Q_p,vx_p,vy_p,vz_p);	
	get_velocities<Shortcuts>(Q_m,vx_m,vy_m,vz_m); 
      
	create_local_basis(n_p, m_p, l_p);
	create_local_basis(n_m, m_m, l_m);

	double Tn_m,Tm_m,Tl_m;
	double Tn_p,Tm_p,Tl_p;

	// rotate fields into l, m, n basis
	rotate_into_orthogonal_basis(n_m,m_m,l_m,Tx_m,Ty_m,Tz_m,Tn_m,Tm_m,Tl_m);
	rotate_into_orthogonal_basis(n_p,m_p,l_p,Tx_p,Ty_p,Tz_p,Tn_p,Tm_p,Tl_p);

	double vn_m,vm_m,vl_m;
	double vn_p,vm_p,vl_p;
	rotate_into_orthogonal_basis(n_m,m_m,l_m,vx_m,vy_m,vz_m,vn_m,vm_m,vl_m);      
	rotate_into_orthogonal_basis(n_p,m_p,l_p,vx_p,vy_p,vz_p,vn_p,vm_p,vl_p);      

  
	// extract local s-wave and p-wave impedances
	double zs_m=rho_m*cs_m;
	double zs_p=rho_p*cs_p;

	double zp_m=rho_m*cp_m;
	double zp_p=rho_p*cp_p;      
      
	// impedance must be greater than zero !
	assertion3(!(zp_p <= 0.0 || zp_m <= 0.0),"Impedance must be greater than zero !",zp_p,zs_p);

	// generate interface data preserving the amplitude of the outgoing charactertritics
	// and satisfying interface conditions exactly.
	double vn_hat_p,vm_hat_p,vl_hat_p;
	double Tn_hat_p,Tm_hat_p,Tl_hat_p;        
	double vn_hat_m,vm_hat_m,vl_hat_m;
	double Tn_hat_m,Tm_hat_m,Tl_hat_m;    

	if (isBoundaryFace) {
	  // 1 absorbing 0 free surface
	  double r= faceIndex == surface ? 1 : 0;
	  //	double r=1.;
	  riemannSolver_boundary(faceIndex,r,
				 vn_m,vm_m,vl_m,
				 Tn_m,Tm_m,Tl_m,
				 zp_m,zs_m,
				 vn_hat_m,vm_hat_m,vl_hat_m,
				 Tn_hat_m,Tm_hat_m,Tl_hat_m);
	  riemannSolver_boundary(faceIndex,r,
				 vn_p,vm_p,vl_p,
				 Tn_p,Tm_p,Tl_p,
				 zp_p,zs_p,
				 vn_hat_p,vm_hat_p,vl_hat_p,
				 Tn_hat_p,Tm_hat_p,Tl_hat_p);      
	}else {
	  riemannSolver_Nodal(vn_p, vn_m,
			      Tn_p, Tn_m,
			      zp_p , zp_m,
			      vn_hat_p , vn_hat_m,
			      Tn_hat_p, Tn_hat_m);
	  riemannSolver_Nodal(vm_p, vm_m,
			      Tm_p, Tm_m,
			      zs_p , zs_m,
			      vm_hat_p , vm_hat_m,
			      Tm_hat_p, Tm_hat_m);
	  riemannSolver_Nodal(vl_p, vl_m,
			      Tl_p, Tl_m,
			      zs_p , zs_m, 
			      vl_hat_p , vl_hat_m,
			      Tl_hat_p, Tl_hat_m);
	}

	//generate fluctuations in the local basis coordinates: n, m, l
	compute_fluctuations_left(zp_m,
				  Tn_m,Tn_hat_m,
				  vn_m,vn_hat_m,
				  FLn);
	compute_fluctuations_left(zs_m,
				  Tm_m,Tm_hat_m,
				  vm_m,vm_hat_m,
				  FLm);
	compute_fluctuations_left(zs_m,
				  Tl_m,Tl_hat_m,
				  vl_m,vl_hat_m,
				  FLl);

	compute_fluctuations_right(zp_p,
				   Tn_p,Tn_hat_p,
				   vn_p,vn_hat_p,
				   FRn);
	compute_fluctuations_right(zs_p,
				   Tm_p,Tm_hat_p,
				   vm_p,vm_hat_p,
				   FRm);
	compute_fluctuations_right(zs_p,
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
	rotate_into_physical_basis(n_m,m_m,l_m,
				   FLn,FLm,FLl,
				   FLx,FLy,FLz);
	rotate_into_physical_basis(n_p,m_p,l_p,
				   FRn,FRm,FRl,
				   FRx,FRy,FRz);
	rotate_into_physical_basis(n_m,m_m,l_m,
				   FL_n,FL_m,FL_l,
				   FL_x,FL_y,FL_z);
	rotate_into_physical_basis(n_p,m_p,l_p,
				   FR_n,FR_m,FR_l,
				   FR_x,FR_y,FR_z);
     
	// construct flux fluctuation vectors obeying the eigen structure of the PDE
	// and choose physically motivated penalties such that we can prove
	// numerical stability.
	F_p[s.v + 0] = norm_p/rho_p*FRx;
	F_m[s.v + 0] = norm_m/rho_m*FLx;
    
	F_p[s.v + 1] = norm_p/rho_p*FRy;
	F_m[s.v + 1] = norm_m/rho_m*FLy;

	F_p[s.v + 2] = norm_p/rho_p*FRz;
	F_m[s.v + 2] = norm_m/rho_m*FLz;

	F_m[s.sigma + 0] =  norm_m*((2*mu_m+lam_m)*n_m[0]*FL_x+lam_m*n_m[1]*FL_y+lam_m*n_m[2]*FL_z);
	F_m[s.sigma + 1] =  norm_m*((2*mu_m+lam_m)*n_m[1]*FL_y+lam_m*n_m[0]*FL_x+lam_m*n_m[2]*FL_z);
	F_m[s.sigma + 2] =  norm_m*((2*mu_m+lam_m)*n_m[2]*FL_z+lam_m*n_m[0]*FL_x+lam_m*n_m[1]*FL_y);

	F_p[s.sigma + 0] = -norm_p*((2*mu_p+lam_p)*n_p[0]*FR_x+lam_p*n_p[1]*FR_y+lam_p*n_p[2]*FR_z);
	F_p[s.sigma + 1] = -norm_p*((2*mu_p+lam_p)*n_p[1]*FR_y+lam_p*n_p[0]*FR_x+lam_p*n_p[2]*FR_z);
	F_p[s.sigma + 2] = -norm_p*((2*mu_p+lam_p)*n_p[2]*FR_z+lam_p*n_p[0]*FR_x+lam_p*n_p[1]*FR_y);
    
	F_m[s.sigma + 3] =  norm_m*mu_m*(n_m[1]*FL_x + n_m[0]*FL_y);
	F_m[s.sigma + 4] =  norm_m*mu_m*(n_m[2]*FL_x + n_m[0]*FL_z);
	F_m[s.sigma + 5] =  norm_m*mu_m*(n_m[2]*FL_y + n_m[1]*FL_z);

	F_p[s.sigma + 3] = -norm_p*mu_p*(n_p[1]*FR_x + n_p[0]*FR_y);
	F_p[s.sigma + 4] = -norm_p*mu_p*(n_p[2]*FR_x + n_p[0]*FR_z);
	F_p[s.sigma + 5] = -norm_p*mu_p*(n_p[2]*FR_y + n_p[1]*FR_z);

        double norm_p_qr=norm_p;
        double norm_m_qr=norm_m;
        kernels::idx2 idx_pml(Dimensions,9);

        F_p[s.pml + idx_pml(0,0)] = n_p[0]*dmp_pml_x_p*norm_p*FRx;
        F_m[s.pml + idx_pml(0,0)] = n_m[0]*dmp_pml_x_m*norm_m*FLx;

        F_p[s.pml + idx_pml(0,1)] = n_p[0]*dmp_pml_x_p*norm_p*FRy;
        F_m[s.pml + idx_pml(0,1)] = n_m[0]*dmp_pml_x_m*norm_m*FLy;

        F_p[s.pml + idx_pml(0,2)] = n_p[0]*dmp_pml_x_p*norm_p*FRz;
        F_m[s.pml + idx_pml(0,2)] = n_m[0]*dmp_pml_x_m*norm_m*FLz;

        F_m[s.pml + idx_pml(0,3)] = n_m[0]*dmp_pml_x_m*norm_m*n_m[0]*FL_x;
        F_m[s.pml + idx_pml(0,4)] = n_m[0]*dmp_pml_x_m*norm_m*n_m[1]*FL_y;
        F_m[s.pml + idx_pml(0,5)] = n_m[0]*dmp_pml_x_m*norm_m*n_m[2]*FL_z;

        F_p[s.pml + idx_pml(0,3)] = -n_p[0]*dmp_pml_x_p*norm_p*n_p[0]*FR_x;
        F_p[s.pml + idx_pml(0,4)] = -n_p[0]*dmp_pml_x_p*norm_p*n_p[1]*FR_y;
        F_p[s.pml + idx_pml(0,5)] = -n_p[0]*dmp_pml_x_p*norm_p*n_p[2]*FR_z;

        F_m[s.pml + idx_pml(0,6)] =  n_m[0]*dmp_pml_x_m*norm_m*(n_m[1]*FL_x + n_m[0]*FL_y);
        F_m[s.pml + idx_pml(0,7)] =  n_m[0]*dmp_pml_x_m*norm_m*(n_m[2]*FL_x + n_m[0]*FL_z);
        F_m[s.pml + idx_pml(0,8)] =  n_m[0]*dmp_pml_x_m*norm_m*(n_m[2]*FL_y + n_m[1]*FL_z);

        F_p[s.pml + idx_pml(0,6)] = -n_p[0]*dmp_pml_x_p*norm_p*(n_p[1]*FR_x + n_p[0]*FR_y);
        F_p[s.pml + idx_pml(0,7)] = -n_p[0]*dmp_pml_x_p*norm_p*(n_p[2]*FR_x + n_p[0]*FR_z);
        F_p[s.pml + idx_pml(0,8)] = -n_p[0]*dmp_pml_x_p*norm_p*(n_p[2]*FR_y + n_p[1]*FR_z);

        // fs.pml + idx_pml(0,0)or y-direction auxiliary variables
        F_p[s.pml + idx_pml(1,0)] = n_p[1]*dmp_pml_y_p*norm_p*FRx;
        F_m[s.pml + idx_pml(1,0)] = n_m[1]*dmp_pml_y_m*norm_m*FLx;

        F_p[s.pml + idx_pml(1,1)] = n_p[1]*dmp_pml_y_p*norm_p*FRy;
        F_m[s.pml + idx_pml(1,1)] = n_m[1]*dmp_pml_y_m*norm_m*FLy;

        F_p[s.pml + idx_pml(1,2)] = n_p[1]*dmp_pml_y_p*norm_p*FRz;
        F_m[s.pml + idx_pml(1,2)] = n_m[1]*dmp_pml_y_m*norm_m*FLz;

        F_m[s.pml + idx_pml(1,3)] = n_m[1]*dmp_pml_y_m*norm_m*n_m[0]*FL_x;
        F_m[s.pml + idx_pml(1,4)] = n_m[1]*dmp_pml_y_m*norm_m*n_m[1]*FL_y;
        F_m[s.pml + idx_pml(1,5)] = n_m[1]*dmp_pml_y_m*norm_m*n_m[2]*FL_z;

        F_p[s.pml + idx_pml(1,3)] = -n_p[1]*dmp_pml_y_p*norm_p*n_p[0]*FR_x;
        F_p[s.pml + idx_pml(1,4)] = -n_p[1]*dmp_pml_y_p*norm_p*n_p[1]*FR_y;
        F_p[s.pml + idx_pml(1,5)] = -n_p[1]*dmp_pml_y_p*norm_p*n_p[2]*FR_z;

        F_m[s.pml + idx_pml(1,6)] =  n_m[1]*dmp_pml_y_m*norm_m*(n_m[1]*FL_x + n_m[0]*FL_y);
        F_m[s.pml + idx_pml(1,7)] =  n_m[1]*dmp_pml_y_m*norm_m*(n_m[2]*FL_x + n_m[0]*FL_z);
        F_m[s.pml + idx_pml(1,8)] =  n_m[1]*dmp_pml_y_m*norm_m*(n_m[2]*FL_y + n_m[1]*FL_z);

        F_p[s.pml + idx_pml(1,6)] = -n_p[1]*dmp_pml_y_p*norm_p*(n_p[1]*FR_x + n_p[0]*FR_y);
        F_p[s.pml + idx_pml(1,7)] = -n_p[1]*dmp_pml_y_p*norm_p*(n_p[2]*FR_x + n_p[0]*FR_z);
        F_p[s.pml + idx_pml(1,8)] = -n_p[1]*dmp_pml_y_p*norm_p*(n_p[2]*FR_y + n_p[1]*FR_z);

        // fs.pml + idx_pml(0,0)or z-direction auxiliary variables
        F_p[s.pml + idx_pml(2,0)] = n_p[2]*dmp_pml_z_p*norm_p*FRx;
        F_m[s.pml + idx_pml(2,0)] = n_m[2]*dmp_pml_z_m*norm_m*FLx;

        F_p[s.pml + idx_pml(2,1)] = n_p[2]*dmp_pml_z_p*norm_p*FRy;
        F_m[s.pml + idx_pml(2,1)] = n_m[2]*dmp_pml_z_m*norm_m*FLy;

        F_p[s.pml + idx_pml(2,2)] = n_p[2]*dmp_pml_z_p*norm_p*FRz;
        F_m[s.pml + idx_pml(2,2)] = n_m[2]*dmp_pml_z_m*norm_m*FLz;

        F_m[s.pml + idx_pml(2,3)] = n_m[2]*dmp_pml_z_m*norm_m*n_m[0]*FL_x;
        F_m[s.pml + idx_pml(2,4)] = n_m[2]*dmp_pml_z_m*norm_m*n_m[1]*FL_y;
        F_m[s.pml + idx_pml(2,5)] = n_m[2]*dmp_pml_z_m*norm_m*n_m[2]*FL_z;

        F_p[s.pml + idx_pml(2,3)] = -n_p[2]*dmp_pml_z_p*norm_p*n_p[0]*FR_x;
        F_p[s.pml + idx_pml(2,4)] = -n_p[2]*dmp_pml_z_p*norm_p*n_p[1]*FR_y;
        F_p[s.pml + idx_pml(2,5)] = -n_p[2]*dmp_pml_z_p*norm_p*n_p[2]*FR_z;

        F_m[s.pml + idx_pml(2,6)] =  n_m[2]*dmp_pml_z_m*norm_m*(n_m[1]*FL_x + n_m[0]*FL_y);
        F_m[s.pml + idx_pml(2,7)] =  n_m[2]*dmp_pml_z_m*norm_m*(n_m[2]*FL_x + n_m[0]*FL_z);
        F_m[s.pml + idx_pml(2,8)] =  n_m[2]*dmp_pml_z_m*norm_m*(n_m[2]*FL_y + n_m[1]*FL_z);

        F_p[s.pml + idx_pml(2,6)] = -n_p[2]*dmp_pml_z_p*norm_p*(n_p[1]*FR_x + n_p[0]*FR_y);
        F_p[s.pml + idx_pml(2,7)] = -n_p[2]*dmp_pml_z_p*norm_p*(n_p[2]*FR_x + n_p[0]*FR_z);
        F_p[s.pml + idx_pml(2,8)] = -n_p[2]*dmp_pml_z_p*norm_p*(n_p[2]*FR_y + n_p[1]*FR_z);
      }    
    }
  }
}
