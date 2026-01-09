namespace riemannSolver{

void extract_tractions_and_particle_velocity(double* n,const double* Q, double& Tx,double& Ty,double& vx,double& vy){

  double sigma_xx = Q[2];
  double sigma_yy = Q[3];
  double sigma_xy = Q[4];
  
  
  Tx = n[0]*sigma_xx + n[1]*sigma_xy;
  Ty = n[0]*sigma_xy + n[1]*sigma_yy;   
  
  vx = Q[0];
  vy = Q[1];   
}

void rotate_into_orthogonal_basis(double* n,double* m, double Tx,double Ty, double& Tn, double& Tm){
  Tn= Tx*n[0] + Ty*n[1];
  Tm= Tx*m[0] + Ty*m[1];
}

void rotate_into_physical_basis(double* n,double* m, double Fn,double Fm, double& Fx, double& Fy){

  Fx = n[0]*Fn + m[0]*Fm;
  Fy = n[1]*Fn + m[1]*Fm;
  
}

void generate_fluctuations_left(double z,  double T,double T_hat,double v, double v_hat, double& F){
  F = 0.5*(z*(v-v_hat) + (T-T_hat));
}

void generate_fluctuations_right(double z,  double T,double T_hat,double v, double v_hat, double& F){
  F = 0.5*(z*(v-v_hat) - (T-T_hat));
}

void riemannSolver_BC0(double v, double sigma, double z,  double r, double& v_hat, double& sigma_hat){
  double p = 0.5*(z*v + sigma);

  v_hat = (1+r)/z*p;
  sigma_hat = (1-r)*p;
}

void riemannSolver_BCn(double v,double sigma, double z, double r, double& v_hat, double& sigma_hat){
  double q = 0.5*(z*v - sigma);

  v_hat = (1+r)/z*q;
  sigma_hat = -(1-r)*q;
}

void riemannSolver_boundary(int faceIndex,double r, double vn , double vm, double Tn , double Tm, double zp, double zs , double& vn_hat , double& vm_hat, double& Tn_hat , double& Tm_hat)
{

  if (faceIndex < DIMENSIONS) { //left face

    riemannSolver_BC0(vn, Tn, zp, r, vn_hat, Tn_hat);
    riemannSolver_BC0(vm, Tm, zs, r, vm_hat, Tm_hat);
  }
      
      
  else { //right face

    riemannSolver_BCn(vn, Tn, zp, r, vn_hat, Tn_hat);
    riemannSolver_BCn(vm, Tm, zs, r, vm_hat, Tm_hat);	
  }

}


void riemannSolver_Nodal(double v_p,double v_m, double sigma_p, double sigma_m, double z_p , double z_m, double& v_hat_p , double& v_hat_m, double& sigma_hat_p, double& sigma_hat_m){
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


void localBasis(double* n, double * m){
  m[0] = n[1];
  m[1] = n[0];
}

void extractTransformation(const double* const Q, double& q_x,double& q_y,double& r_x,double& r_y) {

  constexpr int numberOfVariables  = ::exahype2::elastic::ElasticSolver::NumberOfUnknowns;

  q_x     =Q[numberOfVariables+4];
  q_y     =Q[numberOfVariables+5];
  r_x     =Q[numberOfVariables+6];
  r_y     =Q[numberOfVariables+7];
}

void get_normals(int normalNonZeroIndex,double& norm, double* n,const double* Q){
  double q_x;
  double q_y;
  double r_x;
  double r_y;
  
  extractTransformation(Q,q_x,q_y,r_x,r_y);
  if (normalNonZeroIndex == 0){
    norm = std::sqrt(q_x*q_x + q_y*q_y);
    n[0] = q_x/norm;
    n[1] = q_y/norm;
  }
  if (normalNonZeroIndex == 1){
    norm = std::sqrt(r_x*r_x + r_y*r_y);
    n[0] = r_x/norm;
    n[1] = r_y/norm;
  }
}

 // solve for slip-rate (vv):  
void slip_weakening(double& v1, double& Vel, double& tau1,
								double phi_1, double eta, double tau_str, double sigma_n){
  
  double Phi = std::abs(phi_1);   // stress-transfer functional
  Vel = (Phi - tau_str)/eta;                    // slip-rate

  //compute slip velocities
  v1 = phi_1/(eta+tau_str/Vel);
  
  //compute shear stress on the fault
  tau1 = phi_1 - eta*v1;

  //std::cout << tau_str << " " << Phi << " " << tau1 << " " << v1 << std::endl;
  
}

double boxcar(double& f, double x, double w) 
{
  // f(x) is boxcar of unit amplitude in (x-w,x+w)
  double tol = 1e-8;
 
  if ((-w+tol)<x && x< (w-tol)){    // inside
    f = 1.0;
    //std::cout<< x << "  " << w <<std::endl;
  }
  else if (std::abs(-w-x)<=tol || std::abs(x-w)<=tol){     // boundary
    f = 0.5;
  }
  else{    // outside
    f = 0.0;
  }
  return f;
}

void TauStrength(double& tau_str, double sigma_n, double S, double* x, double t)
{
  // TPV5
  double mu_s = 0.677;                          // stastic friction
  double mu_d = 0.525;                          // dynamic friction
  double sigma0 = 120.0;                        // normal stress
  double S_c = 0.40;                            // critical slip

  double fy;
  double fz;
  
  boxcar(fy, x[1],15.0);

  mu_s = mu_s + 1e10*(1.0-fy);
  double fric_coeff = mu_s - (mu_s-mu_d) * std::min(S,S_c)/S_c;     // friction coefficient
  tau_str = fric_coeff*sigma_n;     
  

  // // TPV28:
  // double mu_s = 0.677;                          // stastic friction
  // double mu_d = 0.373;                          // dynamic friction
  // double S_c = 0.40;      
  // critical slip

  // double fy;
  // double fz;
  
  // boxcar(fy, x[1]-7.5,15.0);
  // boxcar(fz, x[2]-15.0,15.0);

  // mu_s = mu_s + 1e10*(1.0-fy*fz);
  // double fric_coeff = mu_s - (mu_s-mu_d) * std::min(S,S_c)/S_c;     // friction coefficient
  // tau_str = fric_coeff*sigma_n; 
  
}


void extract_tractions(double sxx, double syy, double sxy, double* n, double& Tx,double& Ty){
  
  Tx = n[0]*sxx + n[1]*sxy;
  Ty = n[0]*sxy + n[1]*syy;
      
}
void initialstresstensor(double& sxx, double& syy, double& sxy, double* x)
{
  // TPV5:
  sxx = -120.0;
  syy = 0.0;
  
  sxy =  70.0;

  // // TPV28:
  // sxx = -60;
  // syy = 0.0;
  // szz = 60.0;
  
  // sxy = 0.0;
  // sxz = 29.380;
  // syz = 0.00;
 
}


void prestress(double& T0_n, double& T0_m, double* x, double t, double* m, double* n)
{

  double sxx,syy,szz,sxy,sxz, syz;
  double Tx,Ty,Tz;

  // initial stress tensor
  initialstresstensor(sxx,syy,sxy, x);

  // extract tractions in xyz coordinates
  extract_tractions(sxx, syy, sxy, n, Tx,Ty);

  // rotate tractions into local orthogonal coordinates
  rotate_into_orthogonal_basis(n,m,Tx,Ty,T0_n,T0_m);
   
  //TPV5:
  double fy;

  boxcar(fy, x[1]-7.5,  1.5);
  
  T0_m = T0_m+11.6   *fy;



  // //====================================
  // //TPV28:
  // //====================================

  // // initial stress tensor
  // initialstresstensor(sxx,syy,szz,sxy,sxz, syz, x);


  // // extract tractions in xyz coordinates
  // extract_tractions(sxx, syy, szz, sxy, sxz, syz, n, Tx,Ty,Tz);

  // // rotate tractions into local orthogonal coordinates
  // rotate_into_orthogonal_basis(n,m,l,Tx,Ty,Tz,T0_n,T0_m,T0_l);

  // double r;
  // double tau_nuke;
  // double pi = 3.14159265359;

  // r = std::sqrt((x[1]-7.5)*(x[1]-7.5) + (x[2]-15)*(x[2]-15));

  // if (r <= 1.4){
  //   tau_nuke = 11.60;
  // }
  // else if(r >= 1.4 && r<=2.0) {
  //   tau_nuke = 5.8*(1.0 + std::cos(pi*(r-1.4)/0.6));
  // }
  // else{
  //   tau_nuke = 0.0;
  // }

  // T0_l = T0_l + tau_nuke;
    
}



// Rupture Dynamics
void SlipWeakeningFriction(double vn_p,double vn_m, double Tn_p, double Tn_m, double zn_p , double zn_m, 
                                                         double& vn_hat_p, double& vn_hat_m, double& Tn_hat_p, double& Tn_hat_m, 
                                                         double vm_p,double vm_m, double Tm_p, double Tm_m, double zm_p , double zm_m,
                                                         double& vm_hat_p, double& vm_hat_m, double& Tm_hat_p, double& Tm_hat_m,  
                                                         double* m, double* n, double* x,  double S)
{
  // compute characteristics
  double p_m = zm_p*vm_p + Tm_p;
  double p_n = zn_p*vn_p + Tn_p;
  
 
  double q_m = zm_m*vm_m - Tm_m;
  double q_n = zn_m*vn_m - Tn_m;

  // half of the harmonic mean of Z1_s, Z2_s
  double eta_s=(zm_p*zm_m)/(zm_p+zm_m);                                    
  double eta_n=(zn_p*zn_m)/(zn_p+zn_m); 
  
  double  phi_m= eta_s*(p_m/zm_p - q_m/zm_m);
  double  phi_n= eta_n*(p_n/zn_p - q_n/zn_m);
    
  double T0_m=0;
  double T0_n=0;

  // get prestress (where normal traction is effective normal traction)
  prestress(T0_n, T0_m, x, 0.0, m, n);

  vn_hat_m = (p_n - phi_n)/zn_p;   //! continuity of normal velocity
  Tn_hat_m = phi_n;                //! continuity of normal stress
  vn_hat_p = (q_n + phi_n)/zn_m;   //! continuity of normal velocity
  Tn_hat_p = phi_n;

  double tau_lock = std::sqrt(std::pow(T0_m + phi_m, 2));
  double sigma_n = std::max(0.0, -(T0_n + phi_n));   // including prestress
  double tau_str;
  double Vel = 0.0;
  double Tl, Tm, vv_l, vv_m; 

  TauStrength(tau_str, sigma_n, S, x, 0.0);

  double fault_position= 40.0/27*(27-1)*0.5;

  double r = std::sqrt((x[0]-fault_position)*(x[0]-fault_position) + (x[1]-7.5)*(x[1]-7.5));
  
  if (tau_lock >= tau_str){
    slip_weakening(vv_m, Vel, Tm, phi_m+T0_m, eta_s, tau_str, sigma_n);

    Tm_hat_m = Tm - T0_m;

    Tm_hat_p = Tm - T0_m;
    
  }else{
    Tm_hat_m = phi_m;

    Tm_hat_p = phi_m;
       
    vv_m = 0.0;
    
    Vel = 0.0;
  }
   
  vm_hat_p = (Tm_hat_m + q_m)/zm_m + vv_m;
    
  vm_hat_m = (p_m - Tm_hat_p)/zm_p - vv_m;

  
}





} //end of namespace