#pragma once

#include "../Curvilinear/ContextDynamicRupture.h"
#include "riemannsolverPML.h"
#include "dynamicRupture.h"

template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
void ContextDynamicRupture<Shortcuts, basisSize, numberOfVariables, numberOfParameters, T>::riemannSolver(
  T* FL, T* FR,
  const T* const QL, const T* const QR,
  const double t, const double dt,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const int direction,
  bool isBoundaryFace,
  int faceIndex,
  int surface
){
  constexpr int numberOfData       = numberOfVariables+numberOfParameters;
  // constexpr int basisSize          = order+1;

  ::kernels::idx3 idx_QLR(basisSize,basisSize,numberOfData);
  ::kernels::idx3 idx_FLR(basisSize,basisSize,numberOfVariables);

  //Checking whether the face is on a fault
  int level = std::round(log(domainSize[0]/cellSize[0])/log(3.)) + 1;  

  int elt_z = int(std::round( (QL[idx_QLR(0,0,Shortcuts::curve_grid+2)] -
                              this->domainOffset[2])/ this->max_dx)) * basisSize;
  int elt_y = int(std::round((QL[idx_QLR(0,0,Shortcuts::curve_grid+1)] -
                              this->domainOffset[1])/ this->max_dx)) * basisSize;

  bool is_fault = (direction == 0);

  toolbox::curvi::Root* root = this->interface->getRoot();
  toolbox::curvi::InnerNode* fault_node = static_cast<toolbox::curvi::InnerNode*>(root->getChild());

  toolbox::curvi::Coordinate fault_coords[2];
  fault_node->getCoordinates(fault_coords);
  toolbox::curvi::Coordinate fault_normal = fault_node->getFaceNormal();
  T position = fault_node->getPosition();

  for (int i = 0; i < basisSize; i++) {
    for (int j = 0; j < basisSize; j++) {
      T eta = QL[idx_QLR(i,j,Shortcuts::curve_grid + (2-fault_normal))];
      T xi  = QL[idx_QLR(i,j,Shortcuts::curve_grid + (2-fault_coords[0]))];
      T mu  = QL[idx_QLR(i,j,Shortcuts::curve_grid + (2-fault_coords[1]))];

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

      T* F_m = FL + idx_FLR(i,j,0);
      T* F_p = FR + idx_FLR(i,j,0);
      T rho_m,cp_m,cs_m,mu_m,lam_m;
      T rho_p,cp_p,cs_p,mu_p,lam_p;

      ::Numerics::computeParameters<Shortcuts>(Q_m,rho_m,cp_m,cs_m,mu_m,lam_m);
      ::Numerics::computeParameters<Shortcuts>(Q_p,rho_p,cp_p,cs_p,mu_p,lam_p);

      T n_m[3],m_m[3],l_m[3];
      T n_p[3],m_p[3],l_p[3];
      T norm_p,norm_m;

      ::Numerics::getNormals<Shortcuts>(Q_m,direction,norm_m,n_m);
      ::Numerics::getNormals<Shortcuts>(Q_p,direction,norm_p,n_p);

      T Tx_m,Ty_m,Tz_m;
      T Tx_p,Ty_p,Tz_p;
      ::Numerics::computeTractions<Shortcuts>(Q_p,n_p,Tx_p,Ty_p,Tz_p);
      ::Numerics::computeTractions<Shortcuts>(Q_m,n_m,Tx_m,Ty_m,Tz_m);

      T vx_m,vy_m,vz_m;
      T vx_p,vy_p,vz_p;
      ::Numerics::getVelocities<Shortcuts>(Q_p,vx_p,vy_p,vz_p);	
      ::Numerics::getVelocities<Shortcuts>(Q_m,vx_m,vy_m,vz_m); 
          
      ::Numerics::createLocalBasis(n_p, m_p, l_p);
      ::Numerics::createLocalBasis(n_m, m_m, l_m);

      T Tn_m,Tm_m,Tl_m;
      T Tn_p,Tm_p,Tl_p;

      // rotate fields into l, m, n basis
      ::Numerics::rotateIntoOrthogonalBasis(n_m,m_m,l_m,Tx_m,Ty_m,Tz_m,Tn_m,Tm_m,Tl_m);
      ::Numerics::rotateIntoOrthogonalBasis(n_p,m_p,l_p,Tx_p,Ty_p,Tz_p,Tn_p,Tm_p,Tl_p);

      T vn_m,vm_m,vl_m;
      T vn_p,vm_p,vl_p;
      ::Numerics::rotateIntoOrthogonalBasis(n_m,m_m,l_m,vx_m,vy_m,vz_m,vn_m,vm_m,vl_m);      
      ::Numerics::rotateIntoOrthogonalBasis(n_p,m_p,l_p,vx_p,vy_p,vz_p,vn_p,vm_p,vl_p);      

      
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

        double x[3] = {QR[idx_QLR(i,j,Shortcuts::curve_grid + 0)],
                QR[idx_QLR(i,j,Shortcuts::curve_grid + 1)], 
                QR[idx_QLR(i,j,Shortcuts::curve_grid + 2)]};

        Sx_p = QR[idx_QLR(i,j,Shortcuts::u + 0)];
        Sy_p = QR[idx_QLR(i,j,Shortcuts::u + 1)];
        Sz_p = QR[idx_QLR(i,j,Shortcuts::u + 2)];

        Sx_m = QL[idx_QLR(i,j,Shortcuts::u + 0)];
        Sy_m = QL[idx_QLR(i,j,Shortcuts::u + 1)];
        Sz_m = QL[idx_QLR(i,j,Shortcuts::u + 2)];
        
        // tarch::la::Vector<3,double> coords;
        double coords[3] = {
          QL[idx_QLR(i,j,Shortcuts::curve_grid + 0 )],
          QL[idx_QLR(i,j,Shortcuts::curve_grid + 1 )],
          QL[idx_QLR(i,j,Shortcuts::curve_grid + 2 )] 
        };

        ::Numerics::rotateIntoOrthogonalBasis(n_m, m_m, l_m, Sx_m, Sy_m, Sz_m, Sn_m, Sm_m, Sl_m);
        ::Numerics::rotateIntoOrthogonalBasis(n_p, m_p, l_p, Sx_p, Sy_p, Sz_p, Sn_p, Sm_p, Sl_p);
  
        T S =  std::sqrt((Sl_p- Sl_m)*(Sl_p- Sl_m)+(Sm_p- Sm_m)*(Sm_p- Sm_m));

        slipWeakeningFriction(
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

        ::Numerics::riemannSolverBoundary(faceIndex,r,
            vn_m,vm_m,vl_m,
            Tn_m,Tm_m,Tl_m,
            zp_m,zs_m,
            vn_hat_m,vm_hat_m,vl_hat_m,
            Tn_hat_m,Tm_hat_m,Tl_hat_m);
        ::Numerics::riemannSolverBoundary(faceIndex,r,
            vn_p,vm_p,vl_p,
            Tn_p,Tm_p,Tl_p,
            zp_p,zs_p,
            vn_hat_p,vm_hat_p,vl_hat_p,
            Tn_hat_p,Tm_hat_p,Tl_hat_p);      
      }
      else {
        ::Numerics::riemannSolverNodal(vn_p, vn_m,
                Tn_p, Tn_m,
                zp_p , zp_m,
                vn_hat_p , vn_hat_m,
                Tn_hat_p, Tn_hat_m);
        ::Numerics::riemannSolverNodal(vm_p, vm_m,
                Tm_p, Tm_m,
                zs_p , zs_m,
                vm_hat_p , vm_hat_m,
                Tm_hat_p, Tm_hat_m);
        ::Numerics::riemannSolverNodal(vl_p, vl_m,
                Tl_p, Tl_m,
                zs_p , zs_m, 
                vl_hat_p , vl_hat_m,
                Tl_hat_p, Tl_hat_m);
      }

      //generate fluctuations in the local basis coordinates: n, m, l
      ::Numerics::computeFluctuationsLeft(zp_m,
              Tn_m,Tn_hat_m,
              vn_m,vn_hat_m,
              FLn);
      ::Numerics::computeFluctuationsLeft(zs_m,
              Tm_m,Tm_hat_m,
              vm_m,vm_hat_m,
              FLm);
      ::Numerics::computeFluctuationsLeft(zs_m,
              Tl_m,Tl_hat_m,
              vl_m,vl_hat_m,
              FLl);

      ::Numerics::computeFluctuationsRight(zp_p,
              Tn_p,Tn_hat_p,
              vn_p,vn_hat_p,
              FRn);
      ::Numerics::computeFluctuationsRight(zs_p,
              Tm_p,Tm_hat_p,
              vm_p,vm_hat_p,
              FRm);
      ::Numerics::computeFluctuationsRight(zs_p,
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
      ::Numerics::rotateIntoPhysicalBasis(n_m,m_m,l_m,
              FLn,FLm,FLl,
              FLx,FLy,FLz);
      ::Numerics::rotateIntoPhysicalBasis(n_p,m_p,l_p,
              FRn,FRm,FRl,
              FRx,FRy,FRz);
      ::Numerics::rotateIntoPhysicalBasis(n_m,m_m,l_m,
              FL_n,FL_m,FL_l,
              FL_x,FL_y,FL_z);
      ::Numerics::rotateIntoPhysicalBasis(n_p,m_p,l_p,
              FR_n,FR_m,FR_l,
              FR_x,FR_y,FR_z);
        
      // construct flux fluctuation vectors obeying the eigen structure of the PDE
      // and choose physically motivated penalties such that we can prove
      // numerical stability.

      F_p[Shortcuts::v + 0] = norm_p/rho_p*FRx;
      F_m[Shortcuts::v + 0] = norm_m/rho_m*FLx;
        
      F_p[Shortcuts::v + 1] = norm_p/rho_p*FRy;
      F_m[Shortcuts::v + 1] = norm_m/rho_m*FLy;

      F_p[Shortcuts::v + 2] = norm_p/rho_p*FRz;
      F_m[Shortcuts::v + 2] = norm_m/rho_m*FLz;

      F_m[Shortcuts::sigma + 0] =  norm_m*((2*mu_m+lam_m)*n_m[0]*FL_x+lam_m*n_m[1]*FL_y+lam_m*n_m[2]*FL_z);
      F_m[Shortcuts::sigma + 1] =  norm_m*((2*mu_m+lam_m)*n_m[1]*FL_y+lam_m*n_m[0]*FL_x+lam_m*n_m[2]*FL_z);
      F_m[Shortcuts::sigma + 2] =  norm_m*((2*mu_m+lam_m)*n_m[2]*FL_z+lam_m*n_m[0]*FL_x+lam_m*n_m[1]*FL_y);

      F_p[Shortcuts::sigma + 0] = -norm_p*((2*mu_p+lam_p)*n_p[0]*FR_x+lam_p*n_p[1]*FR_y+lam_p*n_p[2]*FR_z);
      F_p[Shortcuts::sigma + 1] = -norm_p*((2*mu_p+lam_p)*n_p[1]*FR_y+lam_p*n_p[0]*FR_x+lam_p*n_p[2]*FR_z);
      F_p[Shortcuts::sigma + 2] = -norm_p*((2*mu_p+lam_p)*n_p[2]*FR_z+lam_p*n_p[0]*FR_x+lam_p*n_p[1]*FR_y);
        
      F_m[Shortcuts::sigma + 3] =  norm_m*mu_m*(n_m[1]*FL_x + n_m[0]*FL_y);
      F_m[Shortcuts::sigma + 4] =  norm_m*mu_m*(n_m[2]*FL_x + n_m[0]*FL_z);
      F_m[Shortcuts::sigma + 5] =  norm_m*mu_m*(n_m[2]*FL_y + n_m[1]*FL_z);

      F_p[Shortcuts::sigma + 3] = -norm_p*mu_p*(n_p[1]*FR_x + n_p[0]*FR_y);
      F_p[Shortcuts::sigma + 4] = -norm_p*mu_p*(n_p[2]*FR_x + n_p[0]*FR_z);
      F_p[Shortcuts::sigma + 5] = -norm_p*mu_p*(n_p[2]*FR_y + n_p[1]*FR_z);

      F_m[Shortcuts::u + 0] = 0;
      F_m[Shortcuts::u + 1] = 0;
      F_m[Shortcuts::u + 2] = 0;

      F_p[Shortcuts::u + 0] = 0;
      F_p[Shortcuts::u + 1] = 0;
      F_p[Shortcuts::u + 2] = 0;

      T norm_p_qr=norm_p;
      T norm_m_qr=norm_m;

    }    
  }
}
