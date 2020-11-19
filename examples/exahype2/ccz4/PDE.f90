#ifndef HEADER_PDE
#define HEADER_PDE

!#include "MainVariables.f90"
!#include "SpecificVarEqn99.f90"
!#include "expintegrator_ode.f90"
!#include "expintegrator_type.f90"
!#include "SpecificVarEqn99.f90"

! GRMHD PDE.f90
! Trento (EQNTYPE4)

RECURSIVE SUBROUTINE PDEFlux(f,g,hz,Q)
  USE MainVariables, ONLY : nVar, nDim, EQN, d 
  USE iso_c_binding
  IMPLICIT NONE
  REAL :: f(nVar), g(nVar), h(nVar), hz(nVar), Q(nVar), V(nVar)
  REAL, PARAMETER :: epsilon = 1e-14 
  INTENT(IN)  :: Q
  INTENT(OUT) :: f, g, hz
  ! Local varialbes
  REAL :: p, A(3,3), AU(3), detA, GT(3,3), devG(3,3), TT(3,3), Id(3,3), T,falpha
  INTEGER :: iErr
  REAL :: ialpha,LEalpha,alpha,phi,u(3)
  real :: LL_gpr,MM_gpr,dMM_gpr,dKK_gpr,YY,etaloc
  REAL :: x(3),time,rho,Jx,Jy,Jz, Jv
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVarGRMHD = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVarGRMHD = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVarGRMHD = 19                           ! The number of variables of the PDE system 
#endif
  REAL :: QGRMHD(nVarGRMHD), FGRMHD(nVarGRMHD,d)  

    f=0
    g=0
    h=0

#ifdef CCZ4GRHD

    alpha         = EXP(Q(17)) 
    phi           = EXP(Q(55)) 
    QGRMHD(1:5)   = Q(60:64)        ! hydro variables 
    QGRMHD(6:9)   = 0.0             ! EM variables 
    QGRMHD(10)    = alpha           ! lapse 
    QGRMHD(11:13) = Q(18:20)        ! shift 
    QGRMHD(14:19) = Q(1:6)/phi**2   ! metric 
    CALL PDEFluxGRMHD(FGRMHD,QGRMHD) 
    f(60:64) = FGRMHD(1:5,1) 
    g(60:64) = FGRMHD(1:5,2) 
    h(60:64) = FGRMHD(1:5,3) 

#endif 		
	
  IF(nDim==3) THEN
    hz=h
  END IF  
  END SUBROUTINE PDEFlux


RECURSIVE SUBROUTINE PDENCP(BgradQ,Q,gradQin) 
   USE MainVariables, ONLY :  nVar, nDim, EQN,nParam,d
   IMPLICIT NONE
   ! 11. Oct 21:40: This was a matrix BGradQ(nVar, nDim) but is a vector in spaceTimePredictorNonlinear
   REAL, INTENT(OUT) :: BgradQ(nVar)
   REAL, INTENT(IN)  :: gradQin(nVar, 3)
   REAL, INTENT(IN)  :: Q(nVar)

   
   REAL 			:: gradQ(nVar, 3)
    ! Argument list 
    REAL :: par(nParam)  
    ! Local variables 
    INTEGER :: i,j,k,l,m,n,ip,iq,ii,jj,kk,ll,iErr,count    
    REAL :: p, irho, lam, mu 
    REAL :: Qx(nVar), Qy(nVar), Qz(nVar) 
    REAL :: k1,k2,k3,fff,ggg,e,c,ds,xi,sk,sknl,bs,g_cov(3,3),g_contr(3,3),dgup(3,3,3)
    REAL :: det, alpha, fa, k0, dk0(3), beta0(3), b0(3), u(3), ialpha 
    REAL :: s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, eta, itau    
    REAL :: AQx(nVar), BQy(nVar), CQz(nVar)  
    REAL :: lapse, shift(3), gammaij(6), delta(3,3), bv(3), vxb(3), vxb_contr(3), psi, qv_contr(3), qb_contr(3), bv_contr(3) 
    REAL :: v2,vf(3),uem,b2,e2,gp,gm,vc(nVar),lf,w,ww,gamma1,rho,vf_cov(3), s_contr(3), w_ij, wim
    REAL :: v_cov(3) 
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVarGRMHD = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVarGRMHD = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVarGRMHD = 19                           ! The number of variables of the PDE system 
#endif
    REAL :: QGRMHD(nVarGRMHD), gradQGRMHD(nVarGRMHD,d), BgradQGRMHD(nVarGRMHD), ov(3) 
#ifdef VECTOR    
      INTEGER, PARAMETER :: nVarGRGPR = 32                           ! The number of variables of the PDE system 
#else
      INTEGER, PARAMETER :: nVarGRGPR = 30                           ! The number of variables of the PDE system 
#endif
    REAL :: QGRGPR(nVarGRGPR), gradQGRGPR(nVarGRGPR,d), BgradQGRGPR(nVarGRGPR)
    !
    REAL :: Christoffel(3,3,3), RiemannNCP(3,3,3,3), RiemannSrc(3,3,3,3), dChristoffelNCP(3,3,3,3), dChristoffel_tildeNCP(3,3,3,3), dChristoffelSrc(3,3,3,3), DD(3,3,3), dDD(3,3,3,3)  
    REAL :: AA(3), dAA(3,3), BB(3,3), dBB(3,3,3), beta(3), Kex(3,3), Kmix(3,3), Kup(3,3), Z(3), dZ(3,3), nablaZNCP(3,3), nablaZSrc(3,3), RplusNablaZNCP, RplusNablaZSrc  
    REAL :: Theta, dTheta(3), nablaijalphaNCP(3,3), nablaijalphaSrc(3,3), Ricci(3,3), RicciNCP(3,3), RicciSrc(3,3), dtraceK(3), dtraceKNCP(3), dKtempNCP(3), dZNCP(3,3), dZSrc(3,3)  
    REAL :: dtgamma(3,3), dtK(3,3), dK(3,3,3), dtTheta, dtZ(3), dtalpha, dtGhat(3), dtbeta(3), dtbb(3), dxbb(3,3), dtA(3), dtB(3,3), dtD(3,3,3)  
    REAL :: Aupdown, Aex(3,3), dAex(3,3,3), Amix(3,3), Aup(3,3), Ghat(3), Gtilde(3), dGhat(3,3), traceK, Kupdown, phi, PP(3), dPP(3,3), TwoNablaZNCP, TwoNablaZSrc, dKex(3,3,3)      
    REAL :: dGtildeSrc(3,3), dGtildeNCP(3,3), RiccitildeNCP(3,3), RicciphiNCP(3,3), RiccitildeSrc(3,3), RicciphiSrc(3,3), Mom(3), Ham, Pup(3), DDcontr(3)   
    REAL :: Christoffel_tilde(3,3,3), Christoffel_kind1(3,3,3), Zup(3), RicciPlusNablaZNCP(3,3), RicciPlusNablaZSrc(3,3), traceA, traceB, QG(3), b(3), faa, temp   
    REAL :: SecondOrderTermsNCP(3,3), SecondOrderTermsSrc(3,3), traceNCP, traceSrc, dtphi, dtTraceK, dtP(3), dtX(3), XX(3), dXX(3,3), nablaXNCP(3,3)     
    REAL :: RPlusTwoNablaZNCP, RNCP, RSrc, RPlusTwoNablaZSrc, nablanablaalpha, nablanablaalphaNCP, nablanablaalphaSrc, Riemann(3,3,3,3), dChristoffel(3,3,3,3), divAupNCP(3), divAupSrc(3) 
    !
#ifdef GLMROT     
    REAL(8) :: dpsiA(3,3), dpsiP(3,3), dpsiB(3,3,3), dpsiD(3,3,3,3), dphiA(3), dphiP(3), dphiB(3,3), dphiD(3,3,3)   
    REAL(8) :: dtpsiA(3), dtpsiP(3), dtpsiB(3,3), dtpsiD(3,3,3), dtphiA, dtphiP, dtphiB(3), dtphiD(3,3)    
#endif         
    !
    BgradQ = 0.0
	!return
    !
#ifdef EULER     
    !
    ! 3D compressible Euler equations 
    !
    BgradQ = 0. 
    !
#endif
    !
#ifdef MAXWELL
    !
    Qx = gradQ(:,1) 
    Qy = gradQ(:,2) 
    Qz = gradQ(:,3) 
    !
    ! 3D Maxwell 
    BgradQ(1) = +EQN%CCZ4GLMc*( Qy(6) - Qz(5) ) + EQN%CCZ4GLMd*Qx(7)     ! Bx 
    BgradQ(2) = -EQN%CCZ4GLMc*( Qx(6) - Qz(4) ) + EQN%CCZ4GLMd*Qy(7)     ! By 
    BgradQ(3) = +EQN%CCZ4GLMc*( Qx(5) - Qy(4) ) + EQN%CCZ4GLMd*Qz(7)     ! Bz  
    BgradQ(4) = -EQN%CCZ4GLMc*( Qy(3) - Qz(2) ) + EQN%CCZ4GLMd*Qx(8)     ! Ex
    BgradQ(5) = +EQN%CCZ4GLMc*( Qx(3) - Qz(1) ) + EQN%CCZ4GLMd*Qy(8)     ! Ey
    BgradQ(6) = -EQN%CCZ4GLMc*( Qx(2) - Qy(1) ) + EQN%CCZ4GLMd*Qz(8)     ! Ez
    BgradQ(7) = +EQN%CCZ4GLMd*( Qx(1) + Qy(2) + Qz(3) ) 
    BgradQ(8) = +EQN%CCZ4GLMd*( Qx(4) + Qy(5) + Qz(6) )    
    ! 
#endif 
    !
#ifdef HSM
    !
    BgradQ(2) = (EQN%g*Q(1) + EQN%gamma*Q(5)/Q(1))*gradQ(6,1)
    BgradQ(3) = (EQN%g*Q(1) + EQN%gamma*Q(5)/Q(1))*gradQ(6,2) 
    !BgradQ(5) = EQN%c0**2 * ( gradQ(2,1) - Q(2)/Q(1)*gradQ(1,1) + gradQ(3,2) - Q(3)/Q(1)*gradQ(1,2) ) & 
    !            + 2.0*Q(1)*EQN%c0**2*(Q(2)*gradQ(6,1)+Q(3)*gradQ(6,2)) 
    BgradQ(5) = EQN%c0**2 * (  - Q(2)/Q(1)*gradQ(1,1) - Q(3)/Q(1)*gradQ(1,2) ) &
                + 2.0*EQN%c0**2*(-Q(2)/Q(1)*gradQ(6,1)-Q(3)/Q(1)*gradQ(6,2)) 
   !
   !Start with the 1D version of the model 
   BgradQ(3) = 0.0
   BgradQ(5) = EQN%c0**2 * (  - Q(2)/Q(1)*gradQ(1,1)  ) &
                + 2.0*EQN%c0**2*(-Q(2)/Q(1)*gradQ(6,1))
#endif
    !
#ifdef HGNB
    BgradQ(2) = (-Q(7)/Q(1)+EQN%g*Q(1))*gradQ(8,1) 
    BgradQ(3) = (-Q(7)/Q(1)+EQN%g*Q(1))*gradQ(8,2) 
    BgradQ(5) = - EQN%c0**2*Q(2)/Q(1)*gradQ(1,1) - EQN%c0**2*Q(3)/Q(1)*gradQ(1,2) 
    BgradQ(6) = Q(2)/Q(1)*(6.*Q(4)/Q(1)-Q(6)/Q(1))*gradQ(8,1)+Q(3)/Q(1)*(6.*Q(4)/Q(1)-Q(6)/Q(1))*gradQ(8,2) 
    BgradQ(7) = Q(2)/Q(1)*gradQ(7,1) + Q(3)/Q(1)*gradQ(7,2) + Q(2)/Q(1)*(6.*Q(5)/Q(1)-Q(7)/Q(1)+6.*EQN%c0**2)*gradQ(8,1) + Q(3)/Q(1)*(6.*Q(5)/Q(1)-Q(7)/Q(1)+6.*EQN%c0**2)*gradQ(8,2)
#endif      
    !
#ifdef ECHGNB
    BgradQ(2) = (Q(7)/Q(1)+EQN%g*Q(1))*gradQ(8,1)
    BgradQ(3) = (Q(7)/Q(1)+EQN%g*Q(1))*gradQ(8,2)
    BgradQ(6) = -EQN%c0**2*Q(2)/Q(1)*gradQ(1,1)-EQN%c0**2*Q(3)/Q(1)*gradQ(1,2)
    BgradQ(7) = -6.0*EQN%c0**2*Q(2)/Q(1)*gradQ(8,1)-6.0*EQN%c0**2*Q(3)/Q(1)*gradQ(8,2)
!Start with the 1D version of the model
BgradQ(3) = 0.0
BgradQ(6) = -EQN%c0**2*Q(2)/Q(1)*gradQ(1,1)
BgradQ(7) = -6.0*EQN%c0**2*Q(2)/Q(1)*gradQ(8,1)
#endif
#ifdef HSCHROEDINGER
    ! 
    Qx = gradQ(:,1) 
    Qy = gradQ(:,2) 
    Qz = gradQ(:,3) 
    !
    u(:) = Q(2:4)/Q(1) 
    ! 
    BgradQ(7) = u(2)*( Qy(7) - Qx(8) ) + u(3)*( Qz(7) - Qx(9) ) 
    BgradQ(8) = u(1)*( Qx(8) - Qy(7) ) + u(3)*( Qz(8) - Qy(9) )
    BgradQ(9) = u(1)*( Qx(9) - Qz(7) ) + u(2)*( Qy(9) - Qz(8) )
    ! 
#endif      
    !
#ifdef THETAMODEL  
    !
    ! theta model of Ernesto 
    ! Q = ( h, hu, hv, htheta, b ) 
    !
    BgradQ(:) = 0.0 
    Qx = gradQ(:,1) 
    Qy = gradQ(:,2) 
    !     
    BgradQ(2) = EQN%g*Q(4)*( Qx(1) + Qx(5) ) + 0.5*EQN%g*( Q(1)*Qx(4) - Q(4)*Qx(1) )   
    BgradQ(3) = EQN%g*Q(4)*( Qy(1) + Qy(5) ) + 0.5*EQN%g*( Q(1)*Qy(4) - Q(4)*Qy(1) )   
    !
#endif
    !
#ifdef ELASTICITY
    !
    lam  = Q(10)
    mu   = Q(11) 
    irho = 1./Q(12)
    IF(Q(13)<=1e-3) THEN
        BgradQ = 0.0 
        RETURN 
        !
    ELSE
        ialpha = 1./Q(13) 
        u      = Q(7:9)*ialpha 
    ENDIF 
    !
    Qx = gradQ(:,1) 
    Qy = gradQ(:,2)
    Qz = gradQ(:,3)
    !
    BgradQ(1) = - (lam+2*mu)*Qx(7) + (lam+2*mu)*u(1)*Qx(13) 
    BgradQ(2) = - lam*Qx(7)        + lam*u(1)*Qx(13)
    BgradQ(3) = - lam*Qx(7)        + lam*u(1)*Qx(13) 
    BgradQ(4) = - mu *Qx(8)        + mu *u(2)*Qx(13) 
    BgradQ(5) =   0.0 
    BgradQ(6) = - mu *Qx(9)        + mu *u(3)*Qx(13) 
    BgradQ(7) = - irho * Qx(1) - 2*Q(1)*irho*Qx(13)  
    BgradQ(8) = - irho * Qx(4) - 2*Q(4)*irho*Qx(13)   
    BgradQ(9) = - irho * Qx(6) - 2*Q(6)*irho*Qx(13)       
    !
    BgradQ(1) = BgradQ(1) - lam*Qy(8)        + lam*u(2)*Qy(13) 
    BgradQ(2) = BgradQ(2) - (lam+2*mu)*Qy(8) + (lam+2*mu)*u(2)*Qy(13) 
    BgradQ(3) = BgradQ(3) - lam*Qy(8)        + lam*u(2)*Qy(13)  
    BgradQ(4) = BgradQ(4) - mu *Qy(7)        + mu *u(1)*Qy(13) 
    BgradQ(5) = BgradQ(5) - mu *Qy(9)        + mu *u(3)*Qy(13) 
    BgradQ(6) = BgradQ(6) - 0.0  
    BgradQ(7) = BgradQ(7) - irho * Qy(4) - 2*Q(4)*irho*Qy(13)  
    BgradQ(8) = BgradQ(8) - irho * Qy(2) - 2*Q(2)*irho*Qy(13)   
    BgradQ(9) = BgradQ(9) - irho * Qy(5) - 2*Q(5)*irho*Qy(13)      
    !
    BgradQ(1) = BgradQ(1) - lam*Qz(9)        + lam*u(3)*Qz(13) 
    BgradQ(2) = BgradQ(2) - lam*Qz(9)        + lam*u(3)*Qz(13) 
    BgradQ(3) = BgradQ(3) - (lam+2*mu)*Qz(9) + (lam+2*mu)*u(3)*Qz(13) 
    BgradQ(4) = BgradQ(4) - 0.0  
    BgradQ(5) = BgradQ(5) - mu *Qz(8)        + mu *u(2)*Qz(13)  
    BgradQ(6) = BgradQ(6) - mu *Qz(7)        + mu *u(1)*Qz(13) 
    BgradQ(7) = BgradQ(7) - irho * Qz(6) - 2*Q(6)*irho*Qz(13)  
    BgradQ(8) = BgradQ(8) - irho * Qz(5) - 2*Q(5)*irho*Qz(13)  
    BgradQ(9) = BgradQ(9) - irho * Qz(3) - 2*Q(3)*irho*Qz(13)     
    ! 
    ! advection equation 
    !BgradQ = 0. 
    !BgradQ(1,1) = Qx(1) 
    !BgradQ(1,2) = Qy(1) 
    !BgradQ(1,3) = Qz(1) 
    ! 
    ! acoustic wave equation 
!    BgradQ(1,1) = -Qx(2) 
!    BgradQ(1,2) = -Qy(3) 
!    BgradQ(1,3) = -Qz(4) 
!    BgradQ(2,1) = -Qx(1) 
!    BgradQ(3,2) = -Qy(1) 
!    BgradQ(4,3) = -Qz(1) 

    
#endif 

#ifdef ACOUSTIC 
    !
    Qx = gradQ(:,1) 
    Qy = gradQ(:,2)
    Qz = gradQ(:,3)
    !
    ! acoustic wave equation 
    BgradQ(1) = -EQN%c0**2*( Qx(2) + Qy(3) + Qz(4) ) 
    BgradQ(2) = -Qx(1) 
    BgradQ(3) = -Qy(1) 
    BgradQ(4) = -Qz(1) 
    
    ! debug 
    !BgradQ = Qx 
    
#endif 
    !

    ! ---------------------------------
    !
#if defined(CCZ4EINSTEIN) || defined(CCZ4GRMHD) || defined(CCZ4GRHD) || defined(CCZ4GRGPR) 
    !
    Qx = gradQin(:,1) 
    Qy = gradQin(:,2)
	IF(nDim.eq.2) THEN
	    Qz = 0.0
		gradQ(:,1:2)=gradQin(:,1:2)
		gradQ(:,3)	=0.0
	ELSE
		Qz = gradQin(:,3)
		gradQ=gradQin
	ENDIF 

    k1   = EQN%CCZ4k1  
    k2   = EQN%CCZ4k2  
    k3   = EQN%CCZ4k3   
    fff  = EQN%CCZ4f 
    ggg  = EQN%CCZ4g 
    e    = EQN%CCZ4e 
    itau = EQN%CCZ4itau  
    eta  = EQN%CCZ4eta  
    c    = EQN%CCZ4c 
    mu   = EQN%CCZ4mu 
    ds   = EQN%CCZ4ds 
    bs   = EQN%CCZ4bs 
    xi   = EQN%CCZ4xi 
    sk   = EQN%CCZ4sk
    !
    ! These are the tilde quantities, so be careful !    
    g_cov(1,1) = Q(1)
    g_cov(1,2) = Q(2)
    g_cov(1,3) = Q(3)
    g_cov(2,1) = Q(2)
    g_cov(2,2) = Q(4)
    g_cov(2,3) = Q(5)
    g_cov(3,1) = Q(3)
    g_cov(3,2) = Q(5)
    g_cov(3,3) = Q(6)
    ! This determinant should be close to unity, since we use the conformal decomposition 
    det = (Q(1)*Q(4)*Q(6)-Q(1)*Q(5)**2-Q(2)**2*Q(6)+2*Q(2)*Q(3)*Q(5)-Q(3)**2*Q(4)) 
    !g_cov = det**(-1./3.) * g_cov 
    !det = (g_cov(1,1)*g_cov(2,2)*g_cov(3,3)-g_cov(1,1)*g_cov(2,3)*g_cov(3,2)-g_cov(2,1)*g_cov(1,2)*g_cov(3,3)+g_cov(2,1)*g_cov(1,3)*g_cov(3,2)+g_cov(3,1)*g_cov(1,2)*g_cov(2,3)-g_cov(3,1)*g_cov(1,3)*g_cov(2,2)) 
    
    g_contr(1,1) =  (g_cov(2,2)*g_cov(3,3)-g_cov(2,3)*g_cov(3,2)) / det 
    g_contr(1,2) = -(g_cov(1,2)*g_cov(3,3)-g_cov(1,3)*g_cov(3,2)) / det
    g_contr(1,3) = -(-g_cov(1,2)*g_cov(2,3)+g_cov(1,3)*g_cov(2,2))/ det 
    g_contr(2,1) = -(g_cov(2,1)*g_cov(3,3)-g_cov(2,3)*g_cov(3,1)) / det 
    g_contr(2,2) = (g_cov(1,1)*g_cov(3,3)-g_cov(1,3)*g_cov(3,1))  / det 
    g_contr(2,3) = -(g_cov(1,1)*g_cov(2,3)-g_cov(1,3)*g_cov(2,1)) / det 
    g_contr(3,1) = -(-g_cov(2,1)*g_cov(3,2)+g_cov(2,2)*g_cov(3,1))/ det 
    g_contr(3,2) = -(g_cov(1,1)*g_cov(3,2)-g_cov(1,2)*g_cov(3,1)) / det 
    g_contr(3,3) = (g_cov(1,1)*g_cov(2,2)-g_cov(1,2)*g_cov(2,1))  / det 
      
    alpha = EXP(MAX(-20.,MIN(20.,Q(17))))  
    SELECT CASE(EQN%CCZ4LapseType) 
    CASE(0)  ! harmonic 
        fa = 1.0 
        faa = 0.0 
    CASE DEFAULT  ! 1 + log 
        fa = 2.0/alpha
        faa = -2.0/alpha**2   
    END SELECT 
    ! 
    K0    = Q(59)
    dK0   = 0.0 ! sk*gradQ(59,:) 
    !  
    Aex(1,1) = Q(7) 
    Aex(1,2) = Q(8) 
    Aex(1,3) = Q(9) 
    Aex(2,1) = Q(8) 
    Aex(2,2) = Q(10) 
    Aex(2,3) = Q(11) 
    Aex(3,1) = Q(9) 
    Aex(3,2) = Q(11) 
    Aex(3,3) = Q(12) 
    !
    traceA = SUM(g_contr*Aex) 
    Aex = Aex - 1./3.*g_cov*traceA 
    !
    dAex(:,1,1) = gradQ(7,:) 
    dAex(:,1,2) = gradQ(8,:) 
    dAex(:,1,3) = gradQ(9,:) 
    dAex(:,2,1) = gradQ(8,:) 
    dAex(:,2,2) = gradQ(10,:) 
    dAex(:,2,3) = gradQ(11,:) 
    dAex(:,3,1) = gradQ(9,:) 
    dAex(:,3,2) = gradQ(11,:) 
    dAex(:,3,3) = gradQ(12,:) 
    !
    Amix = MATMUL(g_contr, Aex)
    Aup  = MATMUL(g_contr, TRANSPOSE(Amix)) 
    !
    Theta = Q(13)
    dTheta = gradQ(13,:) 
    ! 
    Ghat = (/ Q(14), Q(15), Q(16) /)
    dGhat(:,1) = gradQ(14,:)
    dGhat(:,2) = gradQ(15,:)
    dGhat(:,3) = gradQ(16,:)
    !
    b = Q(21:23) 
    !
    AA    = (/ Q(24), Q(25), Q(26) /) 
    dAA(:,1) = gradQ(24,:) 
    dAA(:,2) = gradQ(25,:) 
    dAA(:,3) = gradQ(26,:) 
    !
    traceK = Q(54) 
    dtraceK = gradQ(54,:) 
    !
    phi   = EXP(MAX(-20.,MIN(20.,Q(55))))   

    PP    = Q(56:58) 
    dPP(:,1) = gradQ(56,:) 
    dPP(:,2) = gradQ(57,:) 
    dPP(:,3) = gradQ(58,:) 
    !
    beta = (/ Q(18), Q(19), Q(20) /) 
    BB(1,1) = Q(27) 
    BB(2,1) = Q(28) 
    BB(3,1) = Q(29) 
    BB(1,2) = Q(30) 
    BB(2,2) = Q(31) 
    BB(3,2) = Q(32) 
    BB(1,3) = Q(33) 
    BB(2,3) = Q(34) 
    BB(3,3) = Q(35) 
    !
    dBB(:,1,1) = sk*gradQ(27,:) 
    dBB(:,2,1) = sk*gradQ(28,:) 
    dBB(:,3,1) = sk*gradQ(29,:) 
    dBB(:,1,2) = sk*gradQ(30,:) 
    dBB(:,2,2) = sk*gradQ(31,:) 
    dBB(:,3,2) = sk*gradQ(32,:) 
    dBB(:,1,3) = sk*gradQ(33,:) 
    dBB(:,2,3) = sk*gradQ(34,:) 
    dBB(:,3,3) = sk*gradQ(35,:) 
    !
    !dBB = dBB*sk    
    ! 
    DD(1,1,1)=Q(36) 
    DD(1,1,2)=Q(37) 
    DD(1,1,3)=Q(38) 
    DD(1,2,1)=Q(37) 
    DD(1,2,2)=Q(39) 
    DD(1,2,3)=Q(40)
    DD(1,3,1)=Q(38) 
    DD(1,3,2)=Q(40) 
    DD(1,3,3)=Q(41)
    ! 
    DD(2,1,1)=Q(42) 
    DD(2,1,2)=Q(43) 
    DD(2,1,3)=Q(44) 
    DD(2,2,1)=Q(43) 
    DD(2,2,2)=Q(45) 
    DD(2,2,3)=Q(46)
    DD(2,3,1)=Q(44) 
    DD(2,3,2)=Q(46) 
    DD(2,3,3)=Q(47)
    !
    DD(3,1,1)=Q(48) 
    DD(3,1,2)=Q(49) 
    DD(3,1,3)=Q(50) 
    DD(3,2,1)=Q(49) 
    DD(3,2,2)=Q(51) 
    DD(3,2,3)=Q(52)
    DD(3,3,1)=Q(50) 
    DD(3,3,2)=Q(52) 
    DD(3,3,3)=Q(53)
    !
    dDD(:,1,1,1)=gradQ(36,:) 
    dDD(:,1,1,2)=gradQ(37,:) 
    dDD(:,1,1,3)=gradQ(38,:) 
    dDD(:,1,2,1)=gradQ(37,:) 
    dDD(:,1,2,2)=gradQ(39,:) 
    dDD(:,1,2,3)=gradQ(40,:)
    dDD(:,1,3,1)=gradQ(38,:) 
    dDD(:,1,3,2)=gradQ(40,:) 
    dDD(:,1,3,3)=gradQ(41,:)
    dDD(:,2,1,1)=gradQ(42,:) 
    dDD(:,2,1,2)=gradQ(43,:) 
    dDD(:,2,1,3)=gradQ(44,:) 
    dDD(:,2,2,1)=gradQ(43,:) 
    dDD(:,2,2,2)=gradQ(45,:) 
    dDD(:,2,2,3)=gradQ(46,:)
    dDD(:,2,3,1)=gradQ(44,:) 
    dDD(:,2,3,2)=gradQ(46,:) 
    dDD(:,2,3,3)=gradQ(47,:) 
    dDD(:,3,1,1)=gradQ(48,:) 
    dDD(:,3,1,2)=gradQ(49,:) 
    dDD(:,3,1,3)=gradQ(50,:) 
    dDD(:,3,2,1)=gradQ(49,:) 
    dDD(:,3,2,2)=gradQ(51,:) 
    dDD(:,3,2,3)=gradQ(52,:)
    dDD(:,3,3,1)=gradQ(50,:) 
    dDD(:,3,3,2)=gradQ(52,:) 
    dDD(:,3,3,3)=gradQ(53,:)
    !
    dgup = 0.0 
    DO k = 1, 3 
     DO m = 1, 3 
      DO l = 1, 3 
       DO n = 1, 3
        DO j = 1, 3 
           dgup(k,m,l) = dgup(k,m,l)-g_contr(m,n)*g_contr(j,l)*2*DD(k,n,j) 
        ENDDO
       ENDDO 
      ENDDO 
     ENDDO 
    ENDDO         
    !
    Kex  = Aex/phi**2 + 1./3.*traceK*g_cov/phi**2 
    Kmix = MATMUL( phi**2*g_contr, Kex  ) 
    Kup  = MATMUL( phi**2*g_contr, Kmix ) 
    !
    Christoffel_tilde = 0.0  
    Christoffel       = 0.0 
    Gtilde = 0.0 
    !
    DO j = 1, 3
     DO i = 1, 3
      DO k = 1, 3
       !Christoffel_kind1(i,j,k) =  DD(i,j,k)+DD(j,i,k)-DD(k,i,j)     ! this definition does not work ! 
       Christoffel_kind1(i,j,k) = DD(k,i,j)+DD(j,i,k)-DD(i,j,k)      ! this definition seems to work ! 
       DO l = 1, 3
          Christoffel_tilde(i,j,k) = Christoffel_tilde(i,j,k) + g_contr(k,l)*( DD(i,j,l)+DD(j,i,l)-DD(l,i,j) ) 
          Christoffel(i,j,k)       = Christoffel(i,j,k)       + g_contr(k,l)*( DD(i,j,l)+DD(j,i,l)-DD(l,i,j) ) -g_contr(k,l)*( g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l) ) 
          Gtilde(i)                = Gtilde(i)+2*g_contr(i,j)*g_contr(k,l)*DD(l,j,k) 
        ENDDO 
      ENDDO
     ENDDO
    ENDDO
    Z   = 0.5*MATMUL( g_cov, Ghat - Gtilde ) 
    Zup = MATMUL(phi**2*g_contr, Z) 
    !
    ! 
    !
    DO i = 1, 3 
     DO ip = 1, 3 
      DO m = 1, 3 
       DO k = 1, 3
          !dChristoffel(k,i,ip,m) = 0 
          dChristoffelNCP(k,i,ip,m) = 0 
          dChristoffel_tildeNCP(k,i,ip,m) = 0 
          DO l = 1, 3 
            !dChristoffel(k,i,ip,m) = dChristoffel(k,i,ip,m) + g_contr(m,l)*(0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)))         & 
            !                                                - g_contr(m,l)*(g_cov(ip,l)*0.5*(dPP(k,i)+dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,ip)+dPP(ip,k))-g_cov(i,ip)*0.5*(dPP(k,l)+dPP(l,k)))  & 
            !                                                 +dgup(k,m,l)*(DD(i,ip,l)+DD(ip,i,l)-DD(l,i,ip)) - dgup(k,m,l)*(g_cov(ip,l)*PP(i)+g_cov(i,l)*PP(ip)-g_cov(i,ip)*PP(l)) - g_contr(m,l)*( 2*DD(k,ip,l)*PP(i)+2*DD(k,i,l)*PP(ip)-2*DD(k,i,ip)*PP(l) ) 
            ! 
            dChristoffelNCP(k,i,ip,m) = dChristoffelNCP(k,i,ip,m) + g_contr(m,l)*( 0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)) )         & 
                                                                  - g_contr(m,l)*( g_cov(ip,l)*0.5*(dPP(k,i)+dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,ip)+dPP(ip,k))-g_cov(i,ip)*0.5*(dPP(k,l)+dPP(l,k)) ) 
            ! 
            dChristoffel_tildeNCP(k,i,ip,m) = dChristoffel_tildeNCP(k,i,ip,m) + g_contr(m,l)*( 0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)) )           
            ! 
            !dChristoffelSrc(k,i,ip,m) = dChristoffelSrc(k,i,ip,m) + dgup(k,m,l)*(DD(i,ip,l)+DD(ip,i,l)-DD(l,i,ip)) - dgup(k,m,l)*(g_cov(ip,l)*PP(i)+g_cov(i,l)*PP(ip)-g_cov(i,ip)*PP(l)) - g_contr(m,l)*( 2*DD(k,ip,l)*PP(i)+2*DD(k,i,l)*PP(ip)-2*DD(k,i,ip)*PP(l) ) 
          ENDDO 
       ENDDO
      ENDDO
     ENDDO
    ENDDO
    ! 
    !Riemann = 0.0 
    !RiemannSrc = 0.0 
    RiemannNCP = 0.0 
    DO i = 1, 3 
     DO ip = 1, 3 
      DO m = 1, 3 
       DO k = 1, 3
          !Riemann(i,k,ip,m)    = dChristoffel(k,i,ip,m)-dChristoffel(ip,i,k,m)
          RiemannNCP(i,k,ip,m) = dChristoffelNCP(k,i,ip,m)-dChristoffelNCP(ip,i,k,m)
          !RiemannSrc(i,k,ip,m) = dChristoffelSrc(k,i,ip,m)-dChristoffelSrc(ip,i,k,m) 
          DO j = 1, 3
           !Riemann(i,k,ip,m)    = Riemann(i,k,ip,m)    + Christoffel(i,ip,j)*Christoffel(j,k,m) - Christoffel(i,k,j)*Christoffel(j,ip,m) 
           !RiemannSrc(i,k,ip,m) = RiemannSrc(i,k,ip,m) + Christoffel(i,ip,j)*Christoffel(j,k,m) - Christoffel(i,k,j)*Christoffel(j,ip,m) 
          ENDDO 
       ENDDO
      ENDDO
     ENDDO
    ENDDO    
    ! 
    !Ricci = 0.0 
    RicciNCP = 0.0 
    !RicciSrc = 0.0 
    DO m = 1, 3 
     DO n = 1, 3
      DO l = 1, 3    
         !Ricci(m,n) = Ricci(m,n) + Riemann(m,l,n,l)  
         RicciNCP(m,n) = RicciNCP(m,n) + RiemannNCP(m,l,n,l)  
         !RicciSrc(m,n) = RicciSrc(m,n) + RiemannSrc(m,l,n,l)  
      ENDDO
     ENDDO
    ENDDO    
    !
    RNCP = phi**2*SUM(g_contr*RicciNCP) 
    !
    dGtildeNCP = 0.0
    !DO k = 1, 3 
    ! DO j = 1, 3 
    !  DO l = 1, 3 
    !   DO m = 1, 3
    !    DO n = 1, 3 
    !     dGtildeNCP(j,k) = dGtildeNCP(j,k) + 2.0*g_contr(k,l)*g_contr(m,n)*0.5*(dDD(j,n,l,m)+dDD(n,j,l,m)) 
    !    ENDDO
    !   ENDDO 
    !  ENDDO 
    ! ENDDO 
    !ENDDO 
    !
    ! Here we directly compute the derivative of Gtilde from its original definition as contracted Christoffel symbol,
    ! without assuming unit determinant of the conformal metric. Back to the roots, and as few assumptions as possible...      
    !
    DO i = 1, 3 
     DO k = 1, 3
      DO j = 1, 3
       DO l = 1, 3
           dGtildeNCP(k,i) = dGtildeNCP(k,i) + g_contr(j,l)*dChristoffel_tildeNCP(k,j,l,i) 
       ENDDO
      ENDDO
     ENDDO
    ENDDO    
    !
    !!RiccitildeNCP = 0.0 
    !!RicciphiNCP   = 0.0 
    !!RiccitildeSrc = 0.0 
    !!RicciphiSrc   = 0.0 
    !!DO j = 1, 3 
    !! DO i = 1, 3 
    !!    RiccitildeNCP(i,j) = 0 
    !!    DO l = 1, 3
    !!     DO m = 1, 3
    !!        RiccitildeNCP(i,j) = RiccitildeNCP(i,j)-g_contr(l,m)*0.5*(dDD(l,m,i,j)+dDD(m,l,i,j))
    !!     ENDDO
    !!    ENDDO 
    !!    DO k = 1, 3 
    !!        RiccitildeNCP(i,j) = RiccitildeNCP(i,j) + 0.5*(g_cov(k,i)*dGtildeNCP(j,k)+g_cov(k,j)*dGtildeNCP(i,k)) 
    !!        !RiccitildeNCP(i,j) = RiccitildeNCP(i,j) + 0.5*(g_cov(k,i)*dGhat(j,k)+g_cov(k,j)*dGhat(i,k)) 
    !!    ENDDO
    !! ENDDO
    !!ENDDO            
    !!! 
    !!DO j = 1, 3 
    !! DO i = 1, 3 
    !!   RicciphiNCP(i,j) = 0.5*(dPP(i,j)+dPP(j,i))
    !!   DO k = 1, 3 
    !!    DO l = 1, 3 
    !!       RicciphiNCP(i,j) = RicciphiNCP(i,j)+g_cov(i,j)*g_contr(l,k)*0.5*(dPP(k,l)+dPP(l,k))
    !!    ENDDO
    !!   ENDDO 
    !! ENDDO
    !!ENDDO    
    !
    dZNCP = 0.0
    !dZSrc = 0.0 
    DO j = 1, 3 
     DO i = 1, 3
      DO k = 1, 3    
        dZNCP(k,i) = dZNCP(k,i) + ds*0.5*g_cov(i,j)*(dGhat(k,j)-dGtildeNCP(k,j))  
        !dZSrc(k,i) = dZSrc(k,i) + DD(k,i,j)*(Ghat(j)-Gtilde(j)) + 0.5*g_cov(i,j)*( -dGtildeSrc(k,j) )
       ENDDO 
      ENDDO 
    ENDDO     
    !
    DO j = 1, 3 
     DO i = 1, 3 
      nablaZNCP(i,j) = dZNCP(i,j)
      !nablaZSrc(i,j) = dZSrc(i,j)
      !DO k = 1, 3 
      !  nablaZSrc(i,j) = nablaZSrc(i,j) - Christoffel(i,j,k)*Z(k) 
      !ENDDO 
     ENDDO
    ENDDO    
    !
    !RicciPlusNablaZNCP = RiccitildeNCP + RicciphiNCP + ( nablaZNCP + TRANSPOSE(nablaZNCP) ) 
    !RicciPlusNablaZSrc = RiccitildeSrc + RicciphiSrc + ( nablaZSrc + TRANSPOSE(nablaZSrc) ) 
    !
    RicciPlusNablaZNCP = RicciNCP + ( nablaZNCP + TRANSPOSE(nablaZNCP) ) 
    !RicciPlusNablaZSrc = RicciSrc + ( nablaZSrc + TRANSPOSE(nablaZSrc) ) 
    !
    RPlusTwoNablaZNCP = phi**2*SUM(g_contr*RicciPlusNablaZNCP) 
    !RPlusTwoNablaZSrc = phi**2*SUM(g_contr*RicciPlusNablaZSrc) 
    !
    !Kupdown = SUM(Kex*Kup) 
    !temp = RPlusTwoNablaZNCP + RPlusTwoNablaZSrc - KupDown + traceK**2 
    !temp = SUM(phi**2*g_contr*Ricci) - KupDown + traceK**2 
    !
    nablaijalphaNCP = 0.0
    !nablaijalphaSrc = 0.0
    DO j = 1, 3 
     DO i = 1, 3 
       nablaijalphaNCP(i,j) = alpha*0.5*( dAA(i,j)+dAA(j,i) ) 
       !nablaijalphaSrc(i,j) = alpha*AA(j)*AA(i) 
       !DO k = 1, 3 
       !  nablaijalphaSrc(i,j) = nablaijalphaSrc(i,j) - Christoffel(i,j,k)*alpha*AA(k)  
       !ENDDO
     ENDDO
    ENDDO 
    nablanablaalphaNCP = phi**2*SUM( g_contr*nablaijalphaNCP ) 
    !nablanablaalphaSrc = phi**2*SUM( g_contr*nablaijalphaSrc ) 
!    nablanablaalpha = 0.0
!    DO j = 1, 3 
!     DO i = 1, 3
!         nablanablaalpha = nablanablaalpha + phi**2*( g_contr(i,j)*AA(j)*AA(i)*alpha + alpha*g_contr(i,j)*dAA(i,j) - 2*g_contr(i,j)*SUM(g_contr(:,:)*TRANSPOSE(DD(:,j,:))*AA(i)*alpha)- g_contr(i,j)*PP(i)*AA(j)*alpha )  
!     ENDDO
!    ENDDO 
    !
    SecondOrderTermsNCP = -nablaijalphaNCP + alpha*RicciPlusNablaZNCP 
    !SecondOrderTermsSrc = -nablaijalphaSrc + alpha*RicciPlusNablaZSrc 
    traceNCP = SUM( g_contr*SecondOrderTermsNCP ) 
    SecondOrderTermsNCP = SecondOrderTermsNCP - 1./3.*g_cov*traceNCP 
    !traceSrc = SUM( g_contr*SecondOrderTermsSrc ) 
    !SecondOrderTermsSrc = SecondOrderTermsSrc - 1./3.*g_cov*traceSrc 
    !
    ! Now assemble all this terrible stuff... 
    !
    dtgamma = 0.0 
    !
    ! Main variables of the CCZ4 system 
    dtK = phi**2*SecondOrderTermsNCP + beta(1)*dAex(1,:,:) + beta(2)*dAex(2,:,:) + beta(3)*dAex(3,:,:)      ! extrinsic curvature
    !dtK = dtK + phi**2*SecondOrderTermsSrc + alpha*Aex*(traceK-2*Theta) - 2*alpha*MATMUL(Aex,Amix) 
!    DO j = 1, 3 
!     DO i = 1, 3 
!      DO k = 1, 3 
!         dtK(i,j) = dtK(i,j) + Aex(k,i)*BB(j,k) + Aex(k,j)*BB(i,k) - 2./3.*Aex(i,j)*BB(k,k) 
!      ENDDO
!     ENDDO
!    ENDDO 
    !
    dtTraceK = -nablanablaalphaNCP + alpha*RPlusTwoNablaZNCP + SUM(beta(:)*dtraceK(:)) 
    !
    traceB = BB(1,1) + BB(2,2) + BB(3,3) 
    dtphi   = 0.0 
    dtalpha = 0.0 

    Aupdown = SUM(Aex*Aup) 
    dtTheta = 0.5*alpha*e**2*( RplusTwoNablaZNCP ) + beta(1)*dTheta(1) + beta(2)*dTheta(2) + beta(3)*dTheta(3)        ! *** original cleaning *** 
    !dtTheta = 0.5*e**2*( RplusTwoNablaZNCP ) + beta(1)*dTheta(1) + beta(2)*dTheta(2) + beta(3)*dTheta(3)             ! *** use turbo cleaning here *** original 0.5*alpha*e**2*RplusTwoNablaZNCP 
    !
    divAupNCP = 0.0
    DO i = 1, 3
        DO j = 1, 3
         DO l = 1, 3
          DO k = 1, 3    
            divAupNCP(i) = divAupNCP(i) + g_contr(i,l)*g_contr(j,k)*dAex(j,l,k) 
          ENDDO
         ENDDO
        ENDDO        
    ENDDO     
    DO i = 1, 3 
        Mom(i) = - 2./3.*SUM(g_contr(i,:)*dtraceK(:)) + divAupNCP(i)  
    ENDDO 
    !
    !!dKex = 0.0
    !!DO j = 1, 3
    !! DO i = 1, 3
    !!  DO k = 1, 3
    !!      dKex(k,i,j) = 1.0/phi**2*( dAex(k,i,j) + 1./3.*dtraceK(k)*g_cov(i,j) ) 
    !!  ENDDO
    !! ENDDO
    !!ENDDO 
    !!! 
    !!Mom(:) = 0.0
    !!DO ii = 1, 3
    !!    DO jj = 1, 3
    !!        DO ll = 1, 3
    !!            Mom(ii) = Mom(ii) + phi**2*g_contr(jj,ll)*(dKex(ll,ii,jj) - dKex(ii,jj,ll))
    !!            !DO mm = 1, 3
    !!            !    Mom(ii) = Mom(ii) + g_contr(jj,ll)*( - Christoffel(jj,ll,mm)*Kex(mm,ii) + Christoffel(jj,ii,mm)*Kex(mm,ll)) 
    !!            !ENDDO
    !!        ENDDO     
    !!    ENDDO
    !!ENDDO     
    !!Mom = MATMUL( g_contr, Mom )     
    !
    DO i = 1, 3
        dtGhat(i) = - 4./3.*alpha*SUM(g_contr(i,:)*dtraceK(:))     &  
        !dtGhat(i)  =  2.0*alpha*( -divAupNCP(i) + ds**2*Mom(i) )  &          
                    + 2.0*alpha*SUM( g_contr(:,i)*( dTheta(:)  ) ) &                    
                    + beta(1)*dGhat(1,i) + beta(2)*dGhat(2,i) + beta(3)*dGhat(3,i) 
        DO l = 1, 3
         DO k = 1, 3
             dtGhat(i) = dtGhat(i) + g_contr(k,l)*0.5*(dBB(k,l,i)+dBB(l,k,i)) + 1./3*g_contr(i,k)*0.5*(dBB(k,l,l)+dBB(l,k,l)) 
         ENDDO
        ENDDO         
    ENDDO 
    DO k = 1, 3 
        ov(k) = 2*alpha*( SUM(g_contr(:,:)*dAex(k,:,:)) )           ! here we can use the constraint that trace A tilde = 0. 
    ENDDO
    dtGhat = dtGhat + sk*MATMUL(g_contr,ov)                         ! Ghat is an "up" vector, so we need to multiply with g_contr 
    !
    dtbb = xi*dtGhat + bs*( beta(1)*gradQ(21:23,1) + beta(2)*gradQ(21:23,2) + beta(3)*gradQ(21:23,3) - beta(1)*gradQ(14:16,1) - beta(2)*gradQ(14:16,2) - beta(3)*gradQ(14:16,3) ) 
    dtbb = sk*dtbb  
    !
    dtbeta  = 0.0    
    !
    ! Auxiliary variables 
    dtA = -alpha*fa*( dtraceK(:) -dK0(:) - c*2*dTheta(:) ) + beta(1)*dAA(1,:) + beta(2)*dAA(2,:) + beta(3)*dAA(3,:)  
    DO k = 1, 3 
        dtA(k) = dtA(k) - sk*alpha*fa*( SUM(g_contr(:,:)*dAex(k,:,:)) )   ! here we can use the constraint that trace A tilde = 0. 
    ENDDO
    !
    ! We have removed the conservative fluxes for CCZ4, so put all the stuff into the NCP and FusedNCP 
    dtB(:,1) = fff*gradQ(21,:)  
    dtB(:,2) = fff*gradQ(22,:)  
    dtB(:,3) = fff*gradQ(23,:)  
    ! #ordB1#     
    ! for the ordering constraints, we have to check whether they should be multiplied by alpha**2, or not... 
    DO i = 1, 3 
     DO k = 1, 3 
      DO j = 1, 3 
         !dtB(k,i) = -mu*g_contr(i,j)*dZNCP(k,j)    
         dtB(k,i) = dtB(k,i) + mu*alpha**2*g_contr(i,j)*( (dPP(k,j)-dPP(j,k)) )  
         DO n = 1, 3 
          DO l = 1, 3 
            dtB(k,i) = dtB(k,i) - mu*alpha**2*g_contr(i,j)*g_contr(n,l)*( dDD(k,l,j,n)-dDD(l,k,j,n) )   
          ENDDO 
         ENDDO
      ENDDO 
      !
      ENDDO 
    ENDDO 
    dtB = dtB + bs*( beta(1)*dBB(1,:,:) + beta(2)*dBB(2,:,:) + beta(3)*dBB(3,:,:) ) 
    ! New stuff 1, which makes appear a Lie derivative and a second order ordering constraint 
    !DO i = 1, 3
    ! DO k = 1, 3 
    !   dtB(k,i) = dtB(k,i) + bs*( beta(1)*dBB(1,k,i) + beta(2)*dBB(2,k,i) + beta(3)*dBB(3,k,i) - beta(1)*dBB(k,1,i) - beta(2)*dBB(k,2,i) - beta(3)*dBB(k,3,i) ) 
    ! ENDDO
    !ENDDO 
    dtB = dtB*sk 
    !
    dtD = -alpha*dAex  
    DO i = 1, 3
      DO j = 1, 3
       DO k = 1, 3 
        DO m = 1, 3
            dtD(k,i,j) = dtD(k,i,j) + ( 0.5*(g_cov(m,i)*0.5*(dBB(k,j,m)+dBB(j,k,m))+g_cov(m,j)*0.5*(dBB(k,i,m)+dBB(i,k,m)) ) - 1./3.*g_cov(i,j)*0.5*(dBB(k,m,m)+dBB(m,k,m)) ) 
            DO n = 1, 3
                dtD(k,i,j) = dtD(k,i,j) + 1./3*alpha*g_cov(i,j)*g_contr(n,m)*dAex(k,n,m)     ! explicitly remove the trace of tilde A again 
            ENDDO 
        ENDDO        
       ENDDO
      ENDDO
    ENDDO 
    dtD = dtD + beta(1)*dDD(1,:,:,:) + beta(2)*dDD(2,:,:,:) + beta(3)*dDD(3,:,:,:)
    !
    dtP = + beta(1)*dPP(1,:) + beta(2)*dPP(2,:) + beta(3)*dPP(3,:)    
    DO k = 1, 3 
     dtP(k) = dtP(k) + 1./3.*alpha*dtraceK(k) + sk*1./3.*alpha*SUM(g_contr(:,:)*dAex(k,:,:))  ! use the fact that trace A tilde = 0 
     DO i = 1, 3 
          dtP(k) = dtP(k) - 1./3.*0.5*(dBB(k,i,i)+dBB(i,k,i))  
     ENDDO
    ENDDO 
    !
#ifdef GLMROT
    ! A  
    dpsiA(1,1) = Qx(65);   dpsiA(2,1) = Qy(65);     dpsiA(3,1) = Qz(65)   
    dpsiA(1,2) = Qx(66);   dpsiA(2,2) = Qy(66);     dpsiA(3,2) = Qz(66)   
    dpsiA(1,3) = Qx(67);   dpsiA(2,3) = Qy(67);     dpsiA(3,3) = Qz(67)   
    ! P  
    dpsiP(1,1) = Qx(68);   dpsiP(2,1) = Qy(68);     dpsiP(3,1) = Qz(68)   
    dpsiP(1,2) = Qx(69);   dpsiP(2,2) = Qy(69);     dpsiP(3,2) = Qz(69)   
    dpsiP(1,3) = Qx(70);   dpsiP(2,3) = Qy(70);     dpsiP(3,3) = Qz(70) 
    ! D 
    dpsiD(1,1,1,1)= Qx(71);  dpsiD(2,1,1,1)= Qy(71); dpsiD(3,1,1,1)= Qz(71)    
    dpsiD(1,1,1,2)= Qx(72);  dpsiD(2,1,1,2)= Qy(72); dpsiD(3,1,1,2)= Qz(72)    
    dpsiD(1,1,1,3)= Qx(73);  dpsiD(2,1,1,3)= Qy(73); dpsiD(3,1,1,3)= Qz(73)    
    dpsiD(1,1,2,1)= Qx(72);  dpsiD(2,1,2,1)= Qy(72); dpsiD(3,1,2,1)= Qz(72)    
    dpsiD(1,1,2,2)= Qx(74);  dpsiD(2,1,2,2)= Qy(74); dpsiD(3,1,2,2)= Qz(74)    
    dpsiD(1,1,2,3)= Qx(75);  dpsiD(2,1,2,3)= Qy(75); dpsiD(3,1,2,3)= Qz(75)    
    dpsiD(1,1,3,1)= Qx(73);  dpsiD(2,1,3,1)= Qy(73); dpsiD(3,1,3,1)= Qz(73)    
    dpsiD(1,1,3,2)= Qx(75);  dpsiD(2,1,3,2)= Qy(75); dpsiD(3,1,3,2)= Qz(75)    
    dpsiD(1,1,3,3)= Qx(76);  dpsiD(2,1,3,3)= Qy(76); dpsiD(3,1,3,3)= Qz(76)    
    dpsiD(1,2,1,1)= Qx(77);  dpsiD(2,2,1,1)= Qy(77); dpsiD(3,2,1,1)= Qz(77)    
    dpsiD(1,2,1,2)= Qx(78);  dpsiD(2,2,1,2)= Qy(78); dpsiD(3,2,1,2)= Qz(78)    
    dpsiD(1,2,1,3)= Qx(79);  dpsiD(2,2,1,3)= Qy(79); dpsiD(3,2,1,3)= Qz(79)    
    dpsiD(1,2,2,1)= Qx(78);  dpsiD(2,2,2,1)= Qy(78); dpsiD(3,2,2,1)= Qz(78)    
    dpsiD(1,2,2,2)= Qx(80);  dpsiD(2,2,2,2)= Qy(80); dpsiD(3,2,2,2)= Qz(80)    
    dpsiD(1,2,2,3)= Qx(81);  dpsiD(2,2,2,3)= Qy(81); dpsiD(3,2,2,3)= Qz(81)    
    dpsiD(1,2,3,1)= Qx(79);  dpsiD(2,2,3,1)= Qy(79); dpsiD(3,2,3,1)= Qz(79)    
    dpsiD(1,2,3,2)= Qx(81);  dpsiD(2,2,3,2)= Qy(81); dpsiD(3,2,3,2)= Qz(81)    
    dpsiD(1,2,3,3)= Qx(82);  dpsiD(2,2,3,3)= Qy(82); dpsiD(3,2,3,3)= Qz(82)    
    dpsiD(1,3,1,1)= Qx(83);  dpsiD(2,3,1,1)= Qy(83); dpsiD(3,3,1,1)= Qz(83)    
    dpsiD(1,3,1,2)= Qx(84);  dpsiD(2,3,1,2)= Qy(84); dpsiD(3,3,1,2)= Qz(84)    
    dpsiD(1,3,1,3)= Qx(85);  dpsiD(2,3,1,3)= Qy(85); dpsiD(3,3,1,3)= Qz(85)    
    dpsiD(1,3,2,1)= Qx(84);  dpsiD(2,3,2,1)= Qy(84); dpsiD(3,3,2,1)= Qz(84)    
    dpsiD(1,3,2,2)= Qx(86);  dpsiD(2,3,2,2)= Qy(86); dpsiD(3,3,2,2)= Qz(86)    
    dpsiD(1,3,2,3)= Qx(87);  dpsiD(2,3,2,3)= Qy(87); dpsiD(3,3,2,3)= Qz(87)    
    dpsiD(1,3,3,1)= Qx(85);  dpsiD(2,3,3,1)= Qy(85); dpsiD(3,3,3,1)= Qz(85)    
    dpsiD(1,3,3,2)= Qx(87);  dpsiD(2,3,3,2)= Qy(87); dpsiD(3,3,3,2)= Qz(87)    
    dpsiD(1,3,3,3)= Qx(88);  dpsiD(2,3,3,3)= Qy(88); dpsiD(3,3,3,3)= Qz(88)       
    !
    dphiA(1)     = Qx(89);   dphiA(2)     = Qy(89);  dphiA(3)      =  Qz(89) 
    dphiP(1)     = Qx(90);   dphiP(2)     = Qy(90);  dphiP(3)      =  Qz(90) 
    dphiD(1,1,1) = Qx(91);   dphiD(2,1,1) = Qy(91);  dphiD(3,1,1)  =  Qz(91) 
    dphiD(1,1,2) = Qx(92);   dphiD(2,1,2) = Qy(92);  dphiD(3,1,2)  =  Qz(92) 
    dphiD(1,1,3) = Qx(93);   dphiD(2,1,3) = Qy(93);  dphiD(3,1,3)  =  Qz(93) 
    dphiD(1,2,1) = Qx(92);   dphiD(2,2,1) = Qy(92);  dphiD(3,2,1)  =  Qz(92) 
    dphiD(1,2,2) = Qx(94);   dphiD(2,2,2) = Qy(94);  dphiD(3,2,2)  =  Qz(94) 
    dphiD(1,2,3) = Qx(95);   dphiD(2,2,3) = Qy(95);  dphiD(3,2,3)  =  Qz(95) 
    dphiD(1,3,1) = Qx(93);   dphiD(2,3,1) = Qy(93);  dphiD(3,3,1)  =  Qz(93) 
    dphiD(1,3,2) = Qx(95);   dphiD(2,3,2) = Qy(95);  dphiD(3,3,2)  =  Qz(95) 
    dphiD(1,3,3) = Qx(96);   dphiD(2,3,3) = Qy(96);  dphiD(3,3,3)  =  Qz(96) 
    !
#ifdef GLMB     
    ! B 
    dpsiB(1,1,1) = Qx( 97);   dpsiB(2,1,1) = Qy( 97);  dpsiB(3,1,1) = Qz( 97)   
    dpsiB(1,2,1) = Qx( 98);   dpsiB(2,2,1) = Qy( 98);  dpsiB(3,2,1) = Qz( 98)   
    dpsiB(1,3,1) = Qx( 99);   dpsiB(2,3,1) = Qy( 99);  dpsiB(3,3,1) = Qz( 99)   
    dpsiB(1,1,2) = Qx(100);   dpsiB(2,1,2) = Qy(100);  dpsiB(3,1,2) = Qz(100)   
    dpsiB(1,2,2) = Qx(101);   dpsiB(2,2,2) = Qy(101);  dpsiB(3,2,2) = Qz(101)   
    dpsiB(1,3,2) = Qx(102);   dpsiB(2,3,2) = Qy(102);  dpsiB(3,3,2) = Qz(102)   
    dpsiB(1,1,3) = Qx(103);   dpsiB(2,1,3) = Qy(103);  dpsiB(3,1,3) = Qz(103)   
    dpsiB(1,2,3) = Qx(104);   dpsiB(2,2,3) = Qy(104);  dpsiB(3,2,3) = Qz(104)   
    dpsiB(1,3,3) = Qx(105);   dpsiB(2,3,3) = Qy(105);  dpsiB(3,3,3) = Qz(105)   
    dphiB(1,1)   = Qx(106);   dphiB(2,1)   = Qy(106);  dphiB(3,1)   = Qz(106) 
    dphiB(1,2)   = Qx(107);   dphiB(2,2)   = Qy(107);  dphiB(3,2)   = Qz(107) 
    dphiB(1,3)   = Qx(108);   dphiB(2,3)   = Qy(108);  dphiB(3,3)   = Qz(108) 
#endif     
    !
    ! for glmdebugging
    ! dtgamma  = 0.0
    ! dtK      = 0.0
    ! dtTheta  = 0.0 
    ! dtGhat   = 0.0
    ! dtbeta   = 0.0
    ! dtphi    = 0.0 
    ! dtbb     = 0.0
    ! dtTraceK = 0.0 
    ! dtA = 0.0
    ! dtP = 0.0
    ! dtD = 0.0 
    ! dtB = 0.0 
    !
    ! A 
    dtA(1) = dtA(1) - ( dpsiA(2,3) - dpsiA(3,2) ) 
    dtA(2) = dtA(2) + ( dpsiA(1,3) - dpsiA(3,1) ) 
    dtA(3) = dtA(3) - ( dpsiA(1,2) - dpsiA(2,1) )  
    ! psiA    A = ( 24, 25, 26 ) 
    dtpsiA(1) = +EQN%CCZ4GLMc0**2 * ( dAA(2,3) - dAA(3,2) ) - dphiA(1) 
    dtpsiA(2) = -EQN%CCZ4GLMc0**2 * ( dAA(1,3) - dAA(3,1) ) - dphiA(2) 
    dtpsiA(3) = +EQN%CCZ4GLMc0**2 * ( dAA(1,2) - dAA(2,1) ) - dphiA(3) 
    ! phiA 
    dtphiA    = - EQN%CCZ4GLMd**2 * ( dpsiA(1,1) + dpsiA(2,2) + dpsiA(3,3) )  
    ! P 
    dtP(1) = dtP(1) - ( dpsiP(2,3) - dpsiP(3,2) )  
    dtP(2) = dtP(2) + ( dpsiP(1,3) - dpsiP(3,1) )   
    dtP(3) = dtP(3) - ( dpsiP(1,2) - dpsiP(2,1) )  
    ! psiP    P = ( 56, 57, 58 ) 
    dtpsiP(1) = +EQN%CCZ4GLMc0**2 * ( dPP(2,3) - dPP(3,2) ) - dphiP(1) 
    dtpsiP(2) = -EQN%CCZ4GLMc0**2 * ( dPP(1,3) - dPP(3,1) ) - dphiP(2) 
    dtpsiP(3) = +EQN%CCZ4GLMc0**2 * ( dPP(1,2) - dPP(2,1) ) - dphiP(3) 
    ! phiP 
    dtphiP    = - EQN%CCZ4GLMd**2 * ( dpsiP(1,1) + dpsiP(2,2) + dpsiP(3,3) )   
    !    
    ! D
    DO j = 1, 3
     DO i = 1, 3
        dtD(1,i,j) = dtD(1,i,j) - ( dpsiD(2,3,i,j) - dpsiD(3,2,i,j) )  
        dtD(2,i,j) = dtD(2,i,j) + ( dpsiD(1,3,i,j) - dpsiD(3,1,i,j) )   
        dtD(3,i,j) = dtD(3,i,j) - ( dpsiD(1,2,i,j) - dpsiD(2,1,i,j) ) 
     ENDDO
    ENDDO 
    ! 
    ! psiD  
    !
    dtpsiD(1,1,1) = +EQN%CCZ4GLMc**2 * ( dDD(2,3,1,1) - dDD(3,2,1,1) ) - dphiD(1,1,1)
    dtpsiD(2,1,1) = -EQN%CCZ4GLMc**2 * ( dDD(1,3,1,1) - dDD(3,1,1,1) ) - dphiD(2,1,1)
    dtpsiD(3,1,1) = +EQN%CCZ4GLMc**2 * ( dDD(1,2,1,1) - dDD(2,1,1,1) ) - dphiD(3,1,1)
    !
    dtpsiD(1,1,2) = +EQN%CCZ4GLMc**2 * ( dDD(2,3,1,2) - dDD(3,2,1,2) ) - dphiD(1,1,2)
    dtpsiD(2,1,2) = -EQN%CCZ4GLMc**2 * ( dDD(1,3,1,2) - dDD(3,1,1,2) ) - dphiD(2,1,2)
    dtpsiD(3,1,2) = +EQN%CCZ4GLMc**2 * ( dDD(1,2,1,2) - dDD(2,1,1,2) ) - dphiD(3,1,2)
    !
    dtpsiD(1,1,3) = +EQN%CCZ4GLMc**2 * ( dDD(2,3,1,3) - dDD(3,2,1,3) ) - dphiD(1,1,3)
    dtpsiD(2,1,3) = -EQN%CCZ4GLMc**2 * ( dDD(1,3,1,3) - dDD(3,1,1,3) ) - dphiD(2,1,3)
    dtpsiD(3,1,3) = +EQN%CCZ4GLMc**2 * ( dDD(1,2,1,3) - dDD(2,1,1,3) ) - dphiD(3,1,3)
    !    
    dtpsiD(1,2,2) = +EQN%CCZ4GLMc**2 * ( dDD(2,3,2,2) - dDD(3,2,2,2) ) - dphiD(1,2,2)
    dtpsiD(2,2,2) = -EQN%CCZ4GLMc**2 * ( dDD(1,3,2,2) - dDD(3,1,2,2) ) - dphiD(2,2,2)
    dtpsiD(3,2,2) = +EQN%CCZ4GLMc**2 * ( dDD(1,2,2,2) - dDD(2,1,2,2) ) - dphiD(3,2,2)
    !
    dtpsiD(1,2,3) = +EQN%CCZ4GLMc**2 * ( dDD(2,3,2,3) - dDD(3,2,2,3) ) - dphiD(1,2,3)
    dtpsiD(2,2,3) = -EQN%CCZ4GLMc**2 * ( dDD(1,3,2,3) - dDD(3,1,2,3) ) - dphiD(2,2,3)
    dtpsiD(3,2,3) = +EQN%CCZ4GLMc**2 * ( dDD(1,2,2,3) - dDD(2,1,2,3) ) - dphiD(3,2,3)
    !
    dtpsiD(1,3,3) = +EQN%CCZ4GLMc**2 * ( dDD(2,3,3,3) - dDD(3,2,3,3) ) - dphiD(1,3,3)
    dtpsiD(2,3,3) = -EQN%CCZ4GLMc**2 * ( dDD(1,3,3,3) - dDD(3,1,3,3) ) - dphiD(2,3,3)
    dtpsiD(3,3,3) = +EQN%CCZ4GLMc**2 * ( dDD(1,2,3,3) - dDD(2,1,3,3) ) - dphiD(3,3,3)    
    !
    ! phiD 
    dtphiD(1,1)   = - EQN%CCZ4GLMd**2 * ( dpsiD(1,1,1,1) + dpsiD(2,2,1,1) + dpsiD(3,3,1,1) )  
    dtphiD(1,2)   = - EQN%CCZ4GLMd**2 * ( dpsiD(1,1,1,2) + dpsiD(2,2,1,2) + dpsiD(3,3,1,2) )  
    dtphiD(1,3)   = - EQN%CCZ4GLMd**2 * ( dpsiD(1,1,1,3) + dpsiD(2,2,1,3) + dpsiD(3,3,1,3) )  
    dtphiD(2,2)   = - EQN%CCZ4GLMd**2 * ( dpsiD(1,1,2,2) + dpsiD(2,2,2,2) + dpsiD(3,3,2,2) )  
    dtphiD(2,3)   = - EQN%CCZ4GLMd**2 * ( dpsiD(1,1,2,3) + dpsiD(2,2,2,3) + dpsiD(3,3,2,3) )  
    dtphiD(3,3)   = - EQN%CCZ4GLMd**2 * ( dpsiD(1,1,3,3) + dpsiD(2,2,3,3) + dpsiD(3,3,3,3) )      
    !
#ifdef GLMB     
    ! B     
    dtB(1,1) = dtB(1,1) - ( dpsiB(2,3,1) - dpsiB(3,2,1) )  
    dtB(2,1) = dtB(2,1) + ( dpsiB(1,3,1) - dpsiB(3,1,1) )   
    dtB(3,1) = dtB(3,1) - ( dpsiB(1,2,1) - dpsiB(2,1,1) )  
    !
    dtB(1,2) = dtB(1,2) - ( dpsiB(2,3,2) - dpsiB(3,2,2) )  
    dtB(2,2) = dtB(2,2) + ( dpsiB(1,3,2) - dpsiB(3,1,2) )   
    dtB(3,2) = dtB(3,2) - ( dpsiB(1,2,2) - dpsiB(2,1,2) )  
    !
    dtB(1,3) = dtB(1,3) - ( dpsiB(2,3,3) - dpsiB(3,2,3) )  
    dtB(2,3) = dtB(2,3) + ( dpsiB(1,3,3) - dpsiB(3,1,3) )   
    dtB(3,3) = dtB(3,3) - ( dpsiB(1,2,3) - dpsiB(2,1,3) )  
    ! 
    ! psiB   B = (27, 28, 29)  (30, 31, 32)  (33, 34, 35) 
    dtpsiB(1,1) = +EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(2,3,1) - dBB(3,2,1) ) - dphiB(1,1)
    dtpsiB(2,1) = -EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(1,3,1) - dBB(3,1,1) ) - dphiB(2,1)
    dtpsiB(3,1) = +EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(1,2,1) - dBB(2,1,1) ) - dphiB(3,1)
    !
    dtpsiB(1,2) = +EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(2,3,2) - dBB(3,2,2) ) - dphiB(1,2)
    dtpsiB(2,2) = -EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(1,3,2) - dBB(3,1,2) ) - dphiB(2,2)
    dtpsiB(3,2) = +EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(1,2,2) - dBB(2,1,2) ) - dphiB(3,2)
    !
    dtpsiB(1,3) = +EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(2,3,3) - dBB(3,2,3) ) - dphiB(1,3)
    dtpsiB(2,3) = -EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(1,3,3) - dBB(3,1,3) ) - dphiB(2,3)
    dtpsiB(3,3) = +EQN%CCZ4sk*EQN%CCZ4GLMc**2 * ( dBB(1,2,3) - dBB(2,1,3) ) - dphiB(3,3)
    !
    ! phiB 
    dtphiB(1)   = - EQN%CCZ4GLMd**2 * ( dpsiB(1,1,1) + dpsiB(2,2,1) + dpsiB(3,3,1) )  
    dtphiB(2)   = - EQN%CCZ4GLMd**2 * ( dpsiB(1,1,2) + dpsiB(2,2,2) + dpsiB(3,3,2) )  
    dtphiB(3)   = - EQN%CCZ4GLMd**2 * ( dpsiB(1,1,3) + dpsiB(2,2,3) + dpsiB(3,3,3) )  
    ! 
#endif     
    !
#endif       
    !
    BgradQ(1:6)    = (/ dtgamma(1,1), dtgamma(1,2), dtgamma(1,3), dtgamma(2,2), dtgamma(2,3), dtgamma(3,3) /)          ! \tilde \gamma_ij 
    BgradQ(7:12)   = (/ dtK(1,1), dtK(1,2), dtK(1,3), dtK(2,2), dtK(2,3), dtK(3,3) /)                                  ! \tilde A_ij 
    BgradQ(13)     = dtTheta                                                                                           ! Theta       
    BgradQ(14:16)  = dtGhat(1:3)                                                                                       ! \hat \Gamma^i           
    BgradQ(17)     = dtalpha                                                                                           ! log alpha 
    BgradQ(18:20)  = dtbeta                                                                                            ! beta^i 
    BgradQ(21:23)  = dtbb                                                                                              ! b^i 
    BgradQ(24:26)  = dtA(1:3)                                                                                          ! A_k       
    BgradQ(27:35)  = (/ dtB(1,1), dtB(2,1), dtB(3,1), dtB(1,2), dtB(2,2), dtB(3,2), dtB(1,3), dtB(2,3), dtB(3,3) /)    ! B_k^i 
    BgradQ(36:41)  = (/ dtD(1,1,1), dtD(1,1,2), dtD(1,1,3), dtD(1,2,2), dtD(1,2,3), dtD(1,3,3) /)                      ! D_kij 
    BgradQ(42:47)  = (/ dtD(2,1,1), dtD(2,1,2), dtD(2,1,3), dtD(2,2,2), dtD(2,2,3), dtD(2,3,3) /)                      ! D_kij 
    BgradQ(48:53)  = (/ dtD(3,1,1), dtD(3,1,2), dtD(3,1,3), dtD(3,2,2), dtD(3,2,3), dtD(3,3,3) /)                      ! D_kij 
    BgradQ(54)     = dtTraceK                                                                                          ! traceK 
    BgradQ(55)     = dtphi                                                                                             ! log phi 
    BgradQ(56:58)  = dtP                                                                                               ! P_k 
    BgradQ(59:nVar) = 0.0  
    !
    BgradQ = -BgradQ ! change sign, since we work on the left hand side in PDENCP 
    !
    ! for the matter source terms in FO-CCZ4
    !
    v_cov(1)  = Q(61)
    v_cov(2)  = Q(62)
    v_cov(3)  = Q(63)   
    !
    BgradQ(60) = v_cov(1)*Qx(60) + v_cov(2)*Qy(60) + v_cov(3)*Qz(60)  
    BgradQ(64) = v_cov(1)*Qx(64) + v_cov(2)*Qy(64) + v_cov(3)*Qz(64)             
    !
#ifndef GLMROT     
    !
    CONTINUE 
    !
#else   
    ! psiA 
    BgradQ(65) = -dtpsiA(1) 
    BgradQ(66) = -dtpsiA(2) 
    BgradQ(67) = -dtpsiA(3) 
    ! psiP 
    BgradQ(68) = -dtpsiP(1) 
    BgradQ(69) = -dtpsiP(2) 
    BgradQ(70) = -dtpsiP(3) 
    ! psiD 
    BgradQ(71) = -dtpsiD(1,1,1) 
    BgradQ(72) = -dtpsiD(1,1,2) 
    BgradQ(73) = -dtpsiD(1,1,3) 
    BgradQ(74) = -dtpsiD(1,2,2) 
    BgradQ(75) = -dtpsiD(1,2,3) 
    BgradQ(76) = -dtpsiD(1,3,3) 
    BgradQ(77) = -dtpsiD(2,1,1) 
    BgradQ(78) = -dtpsiD(2,1,2) 
    BgradQ(79) = -dtpsiD(2,1,3) 
    BgradQ(80) = -dtpsiD(2,2,2) 
    BgradQ(81) = -dtpsiD(2,2,3) 
    BgradQ(82) = -dtpsiD(2,3,3) 
    BgradQ(83) = -dtpsiD(3,1,1) 
    BgradQ(84) = -dtpsiD(3,1,2) 
    BgradQ(85) = -dtpsiD(3,1,3) 
    BgradQ(86) = -dtpsiD(3,2,2) 
    BgradQ(87) = -dtpsiD(3,2,3) 
    BgradQ(88) = -dtpsiD(3,3,3) 
    ! phiA 
    BgradQ(89) = -dtphiA
    ! phiP 
    BgradQ(90) = -dtphiP
    ! phiD 
    BgradQ(91) = -dtphiD(1,1)
    BgradQ(92) = -dtphiD(1,2)
    BgradQ(93) = -dtphiD(1,3)
    BgradQ(94) = -dtphiD(2,2)
    BgradQ(95) = -dtphiD(2,3)
    BgradQ(96) = -dtphiD(3,3)
#ifdef GLMB
    ! psiB 
    BgradQ( 97) = -dtpsiB(1,1) 
    BgradQ( 98) = -dtpsiB(2,1) 
    BgradQ( 99) = -dtpsiB(3,1) 
    BgradQ(100) = -dtpsiB(1,2) 
    BgradQ(101) = -dtpsiB(2,2) 
    BgradQ(102) = -dtpsiB(3,2) 
    BgradQ(103) = -dtpsiB(1,3) 
    BgradQ(104) = -dtpsiB(2,3) 
    BgradQ(105) = -dtpsiB(3,3) 
    ! phiB 
    BgradQ(106) = -dtphiB(1)
    BgradQ(107) = -dtphiB(2)
    BgradQ(108) = -dtphiB(3)
#endif
#endif      
    !
    CONTINUE
    !
#endif
    ! 
    ! ---------------------------------
    !
#if defined(BSSZ4EINSTEIN) || defined(BSSZ4GRMHD) 
    !
    Qx = gradQ(:,1) 
    Qy = gradQ(:,2)
    Qz = gradQ(:,3)

    k1   = EQN%CCZ4k1  
    k2   = EQN%CCZ4k2  
    k3   = EQN%CCZ4k3   
    fff  = EQN%CCZ4f 
    ggg  = EQN%CCZ4g 
    eta  = EQN%CCZ4eta 
    itau = EQN%CCZ4itau  
    e    = EQN%CCZ4e 
    c    = EQN%CCZ4c 
    mu   = EQN%CCZ4mu 
    ds   = EQN%CCZ4ds 
    bs   = EQN%CCZ4bs 
    xi   = EQN%CCZ4xi 
    sk   = EQN%CCZ4sk
    !
    ! These are the tilde quantities, so be careful !    
    g_cov(1,1) = Q(1)
    g_cov(1,2) = Q(2)
    g_cov(1,3) = Q(3)
    g_cov(2,1) = Q(2)
    g_cov(2,2) = Q(4)
    g_cov(2,3) = Q(5)
    g_cov(3,1) = Q(3)
    g_cov(3,2) = Q(5)
    g_cov(3,3) = Q(6)
    ! This determinant should be unity, since we use the conformal decomposition 
    det = (Q(1)*Q(4)*Q(6)-Q(1)*Q(5)**2-Q(2)**2*Q(6)+2*Q(2)*Q(3)*Q(5)-Q(3)**2*Q(4)) 
    g_cov = det**(-1./3.) * g_cov 
    det = (g_cov(1,1)*g_cov(2,2)*g_cov(3,3)-g_cov(1,1)*g_cov(2,3)*g_cov(3,2)-g_cov(2,1)*g_cov(1,2)*g_cov(3,3)+g_cov(2,1)*g_cov(1,3)*g_cov(3,2)+g_cov(3,1)*g_cov(1,2)*g_cov(2,3)-g_cov(3,1)*g_cov(1,3)*g_cov(2,2)) 
    
    g_contr(1,1) =  (g_cov(2,2)*g_cov(3,3)-g_cov(2,3)*g_cov(3,2)) / det 
    g_contr(1,2) = -(g_cov(1,2)*g_cov(3,3)-g_cov(1,3)*g_cov(3,2)) / det
    g_contr(1,3) = -(-g_cov(1,2)*g_cov(2,3)+g_cov(1,3)*g_cov(2,2))/ det 
    g_contr(2,1) = -(g_cov(2,1)*g_cov(3,3)-g_cov(2,3)*g_cov(3,1)) / det 
    g_contr(2,2) = (g_cov(1,1)*g_cov(3,3)-g_cov(1,3)*g_cov(3,1))  / det 
    g_contr(2,3) = -(g_cov(1,1)*g_cov(2,3)-g_cov(1,3)*g_cov(2,1)) / det 
    g_contr(3,1) = -(-g_cov(2,1)*g_cov(3,2)+g_cov(2,2)*g_cov(3,1))/ det 
    g_contr(3,2) = -(g_cov(1,1)*g_cov(3,2)-g_cov(1,2)*g_cov(3,1)) / det 
    g_contr(3,3) = (g_cov(1,1)*g_cov(2,2)-g_cov(1,2)*g_cov(2,1))  / det 
      
    alpha = EXP(MAX(-20.,MIN(20.,Q(17)))) 
    SELECT CASE(EQN%CCZ4LapseType) 
    CASE(0)  ! harmonic 
        fa = 1.0 
        faa = 0.0 
    CASE DEFAULT  ! 1 + log 
        fa = 2.0/alpha
        faa = -2.0/alpha**2   
    END SELECT 
    ! 
    K0    = Q(62)
    dK0   = sk*gradQ(62,:) 
    !  
    Aex(1,1) = Q(7) 
    Aex(1,2) = Q(8) 
    Aex(1,3) = Q(9) 
    Aex(2,1) = Q(8) 
    Aex(2,2) = Q(10) 
    Aex(2,3) = Q(11) 
    Aex(3,1) = Q(9) 
    Aex(3,2) = Q(11) 
    Aex(3,3) = Q(12) 
    !
    traceA = SUM(g_contr*Aex) 
    Aex = Aex - 1./3.*g_cov*traceA 
    !
    dAex(:,1,1) = gradQ(7,:) 
    dAex(:,1,2) = gradQ(8,:) 
    dAex(:,1,3) = gradQ(9,:) 
    dAex(:,2,1) = gradQ(8,:) 
    dAex(:,2,2) = gradQ(10,:) 
    dAex(:,2,3) = gradQ(11,:) 
    dAex(:,3,1) = gradQ(9,:) 
    dAex(:,3,2) = gradQ(11,:) 
    dAex(:,3,3) = gradQ(12,:) 
    !
    Amix = MATMUL(g_contr, Aex)
    Aup  = MATMUL(g_contr, TRANSPOSE(Amix)) 
    !
    Theta = Q(13)
    dTheta = gradQ(13,:) 
    ! 
    Ghat = (/ Q(14), Q(15), Q(16) /)
    dGhat(:,1) = gradQ(14,:)
    dGhat(:,2) = gradQ(15,:)
    dGhat(:,3) = gradQ(16,:)
    !
    XX = (/ Q(59), Q(60), Q(61) /) 
    dXX(:,1) = gradQ(59,:) 
    dXX(:,2) = gradQ(60,:) 
    dXX(:,3) = gradQ(61,:) 
    !
    b = Q(21:23) 
    !
    AA    = (/ Q(24), Q(25), Q(26) /) 
    dAA(:,1) = gradQ(24,:) 
    dAA(:,2) = gradQ(25,:) 
    dAA(:,3) = gradQ(26,:) 
    !
    traceK = Q(54) 
    dtraceK = gradQ(54,:) 
    !
    phi   =  EXP(MAX(-20.,MIN(20.,Q(55))))  

    PP    = Q(56:58) 
    dPP(:,1) = gradQ(56,:) 
    dPP(:,2) = gradQ(57,:) 
    dPP(:,3) = gradQ(58,:) 
    !
    beta = (/ Q(18), Q(19), Q(20) /) 
    BB(1,1) = Q(27) 
    BB(2,1) = Q(28) 
    BB(3,1) = Q(29) 
    BB(1,2) = Q(30) 
    BB(2,2) = Q(31) 
    BB(3,2) = Q(32) 
    BB(1,3) = Q(33) 
    BB(2,3) = Q(34) 
    BB(3,3) = Q(35) 
    !
    dBB(:,1,1) = gradQ(27,:) 
    dBB(:,2,1) = gradQ(28,:) 
    dBB(:,3,1) = gradQ(29,:) 
    dBB(:,1,2) = gradQ(30,:) 
    dBB(:,2,2) = gradQ(31,:) 
    dBB(:,3,2) = gradQ(32,:) 
    dBB(:,1,3) = gradQ(33,:) 
    dBB(:,2,3) = gradQ(34,:) 
    dBB(:,3,3) = gradQ(35,:) 
    !
    dBB = dBB*sk    
    ! 
    DD(1,1,1)=Q(36) 
    DD(1,1,2)=Q(37) 
    DD(1,1,3)=Q(38) 
    DD(1,2,1)=Q(37) 
    DD(1,2,2)=Q(39) 
    DD(1,2,3)=Q(40)
    DD(1,3,1)=Q(38) 
    DD(1,3,2)=Q(40) 
    DD(1,3,3)=Q(41)
    ! 
    DD(2,1,1)=Q(42) 
    DD(2,1,2)=Q(43) 
    DD(2,1,3)=Q(44) 
    DD(2,2,1)=Q(43) 
    DD(2,2,2)=Q(45) 
    DD(2,2,3)=Q(46)
    DD(2,3,1)=Q(44) 
    DD(2,3,2)=Q(46) 
    DD(2,3,3)=Q(47)
    !
    DD(3,1,1)=Q(48) 
    DD(3,1,2)=Q(49) 
    DD(3,1,3)=Q(50) 
    DD(3,2,1)=Q(49) 
    DD(3,2,2)=Q(51) 
    DD(3,2,3)=Q(52)
    DD(3,3,1)=Q(50) 
    DD(3,3,2)=Q(52) 
    DD(3,3,3)=Q(53)
    !
    dDD(:,1,1,1)=gradQ(36,:) 
    dDD(:,1,1,2)=gradQ(37,:) 
    dDD(:,1,1,3)=gradQ(38,:) 
    dDD(:,1,2,1)=gradQ(37,:) 
    dDD(:,1,2,2)=gradQ(39,:) 
    dDD(:,1,2,3)=gradQ(40,:)
    dDD(:,1,3,1)=gradQ(38,:) 
    dDD(:,1,3,2)=gradQ(40,:) 
    dDD(:,1,3,3)=gradQ(41,:)
    dDD(:,2,1,1)=gradQ(42,:) 
    dDD(:,2,1,2)=gradQ(43,:) 
    dDD(:,2,1,3)=gradQ(44,:) 
    dDD(:,2,2,1)=gradQ(43,:) 
    dDD(:,2,2,2)=gradQ(45,:) 
    dDD(:,2,2,3)=gradQ(46,:)
    dDD(:,2,3,1)=gradQ(44,:) 
    dDD(:,2,3,2)=gradQ(46,:) 
    dDD(:,2,3,3)=gradQ(47,:) 
    dDD(:,3,1,1)=gradQ(48,:) 
    dDD(:,3,1,2)=gradQ(49,:) 
    dDD(:,3,1,3)=gradQ(50,:) 
    dDD(:,3,2,1)=gradQ(49,:) 
    dDD(:,3,2,2)=gradQ(51,:) 
    dDD(:,3,2,3)=gradQ(52,:)
    dDD(:,3,3,1)=gradQ(50,:) 
    dDD(:,3,3,2)=gradQ(52,:) 
    dDD(:,3,3,3)=gradQ(53,:)
    !
    dgup = 0.0 
    DO k = 1, 3 
     DO m = 1, 3 
      DO l = 1, 3 
       DO n = 1, 3
        DO j = 1, 3 
           dgup(k,m,l) = dgup(k,m,l)-g_contr(m,n)*g_contr(j,l)*2*DD(k,n,j) 
        ENDDO
       ENDDO 
      ENDDO 
     ENDDO 
    ENDDO         
    !
    Kex  = Aex/phi**2 + 1./3.*traceK*g_cov/phi**2 
    Kmix = MATMUL( phi**2*g_contr, Kex  ) 
    Kup  = MATMUL( phi**2*g_contr, Kmix ) 
    !
    Christoffel_tilde = 0.0  
    Christoffel       = 0.0 
    Gtilde = 0.0 
    !
    DO j = 1, 3
     DO i = 1, 3
      DO k = 1, 3
       !Christoffel_kind1(i,j,k) =  DD(i,j,k)+DD(j,i,k)-DD(k,i,j)     ! this definition does not work ! 
       Christoffel_kind1(i,j,k) = DD(k,i,j)+DD(j,i,k)-DD(i,j,k)      ! this definition seems to work ! 
       DO l = 1, 3
          Christoffel_tilde(i,j,k) = Christoffel_tilde(i,j,k) + g_contr(k,l)*( DD(i,j,l)+DD(j,i,l)-DD(l,i,j) ) 
          Christoffel(i,j,k)       = Christoffel(i,j,k)       + g_contr(k,l)*( DD(i,j,l)+DD(j,i,l)-DD(l,i,j) ) -g_contr(k,l)*( g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l) ) 
          !Gtilde(i)                = Gtilde(i)+2*g_contr(i,j)*g_contr(k,l)*DD(l,j,k) 
        ENDDO 
      ENDDO
     ENDDO
    ENDDO
    !
    DO i = 1, 3
     DO j = 1, 3
      DO l = 1, 3
          Gtilde(i) = Gtilde(i) + g_contr(j,l)*Christoffel_tilde(j,l,i) 
      ENDDO
     ENDDO     
    ENDDO    
    !
    Z   = 0.5*MATMUL( g_cov, Ghat - Gtilde ) 
    Zup = MATMUL(phi**2*g_contr, Z) 
    !
    !dChristoffel    = 0.0 
    dChristoffelNCP = 0.0
    dChristoffelSrc = 0.0 
    dChristoffel_tildeNCP = 0.0
    DO i = 1, 3 
     DO ip = 1, 3 
      DO m = 1, 3 
       DO k = 1, 3
          DO l = 1, 3 
            !dChristoffel(k,i,ip,m) = dChristoffel(k,i,ip,m) + g_contr(m,l)*(0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)))         & 
            !                                                - g_contr(m,l)*(g_cov(ip,l)*0.5*(dPP(k,i)+dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,ip)+dPP(ip,k))-g_cov(i,ip)*0.5*(dPP(k,l)+dPP(l,k)))  & 
            !                                                 +dgup(k,m,l)*(DD(i,ip,l)+DD(ip,i,l)-DD(l,i,ip)) - dgup(k,m,l)*(g_cov(ip,l)*PP(i)+g_cov(i,l)*PP(ip)-g_cov(i,ip)*PP(l)) - g_contr(m,l)*( 2*DD(k,ip,l)*PP(i)+2*DD(k,i,l)*PP(ip)-2*DD(k,i,ip)*PP(l) ) 
            ! 
            dChristoffelNCP(k,i,ip,m) = dChristoffelNCP(k,i,ip,m) + g_contr(m,l)*( 0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)) )         & 
                                                                  - g_contr(m,l)*( g_cov(ip,l)*0.5*(dPP(k,i)+dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,ip)+dPP(ip,k))-g_cov(i,ip)*0.5*(dPP(k,l)+dPP(l,k)) ) 
            !
            dChristoffel_tildeNCP(k,i,ip,m) = dChristoffel_tildeNCP(k,i,ip,m) + g_contr(m,l)*( 0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)) ) 
            ! 
            !dChristoffelOrd(k,i,ip,m) = dChristoffelOrd(k,i,ip,m) + g_contr(m,l)*( 0.5*(dDD(k,i,ip,l)-dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)-dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)-dDD(l,k,i,ip)) )         & 
            !                                                      - g_contr(m,l)*( g_cov(ip,l)*0.5*(dPP(k,i)-dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,ip)-dPP(ip,k))-g_cov(i,ip)*0.5*(dPP(k,l)-dPP(l,k)) )             
            !
            dChristoffelSrc(k,i,ip,m) = dChristoffelSrc(k,i,ip,m) + dgup(k,m,l)*(DD(i,ip,l)+DD(ip,i,l)-DD(l,i,ip)) - dgup(k,m,l)*(g_cov(ip,l)*PP(i)+g_cov(i,l)*PP(ip)-g_cov(i,ip)*PP(l)) - g_contr(m,l)*( 2*DD(k,ip,l)*PP(i)+2*DD(k,i,l)*PP(ip)-2*DD(k,i,ip)*PP(l) ) 
            !
          ENDDO 
       ENDDO
      ENDDO
     ENDDO
    ENDDO
    ! 
    !Riemann = 0.0 
    RiemannSrc = 0.0 
    RiemannNCP = 0.0 
    DO i = 1, 3 
     DO ip = 1, 3 
      DO m = 1, 3 
       DO k = 1, 3
          !Riemann(i,k,ip,m)    = dChristoffel(k,i,ip,m)-dChristoffel(ip,i,k,m)
          RiemannNCP(i,k,ip,m) = dChristoffelNCP(k,i,ip,m)-dChristoffelNCP(ip,i,k,m)
          RiemannSrc(i,k,ip,m) = dChristoffelSrc(k,i,ip,m)-dChristoffelSrc(ip,i,k,m) 
          DO j = 1, 3
           !Riemann(i,k,ip,m)    = Riemann(i,k,ip,m)    + Christoffel(i,ip,j)*Christoffel(j,k,m) - Christoffel(i,k,j)*Christoffel(j,ip,m) 
           RiemannSrc(i,k,ip,m) = RiemannSrc(i,k,ip,m) + Christoffel(i,ip,j)*Christoffel(j,k,m) - Christoffel(i,k,j)*Christoffel(j,ip,m) 
          ENDDO 
       ENDDO
      ENDDO
     ENDDO
    ENDDO    
    ! 
    !Ricci = 0.0 
    RicciNCP = 0.0 
    RicciSrc = 0.0 
    DO m = 1, 3 
     DO n = 1, 3
      DO l = 1, 3    
         !Ricci(m,n) = Ricci(m,n) + Riemann(m,l,n,l)  
         RicciNCP(m,n) = RicciNCP(m,n) + RiemannNCP(m,l,n,l)  
         RicciSrc(m,n) = RicciSrc(m,n) + RiemannSrc(m,l,n,l)  
      ENDDO
     ENDDO
    ENDDO    
    !
    RNCP = phi**2*SUM(g_contr*RicciNCP) 
    RSrc = phi**2*SUM(g_contr*RicciSrc) 
    !
    dGtildeNCP = 0.0
    dGtildeSrc = 0.0
    !DO k = 1, 3 
    ! DO j = 1, 3 
    !  DO l = 1, 3 
    !   DO m = 1, 3
    !    DO n = 1, 3 
    !     dGtildeNCP(j,k) = dGtildeNCP(j,k) + 2.0*g_contr(k,l)*g_contr(m,n)*0.5*(dDD(j,n,l,m)+dDD(n,j,l,m)) 
    !     dGtildeSrc(j,k) = dGtildeSrc(j,k) + 2.0*( g_contr(m,n)*DD(n,l,m)*dgup(j,k,l) + g_contr(k,l)*DD(n,l,m)*dgup(j,m,n) )         
    !    ENDDO
    !   ENDDO 
    !  ENDDO 
    ! ENDDO 
    !ENDDO 
    !    
    ! Here we directly compute the derivative of Gtilde from its original definition as contracted Christoffel symbol,
    ! without assuming unit determinant of the conformal metric. Back to the roots, and as few assumptions as possible... 
    !
    DO i = 1, 3 
     DO k = 1, 3
      DO j = 1, 3
       DO l = 1, 3
           dGtildeNCP(k,i) = dGtildeNCP(k,i)                                        + g_contr(j,l)*dChristoffel_tildeNCP(k,j,l,i) 
       ENDDO
      ENDDO
     ENDDO
    ENDDO
    !
    ! If you want the original computation of the Ricci tensor according to the CCZ4 paper of Alic et al. 2012, use the following version. 
    ! By default, however, we compute the Ricci tensor ab definitionem from the Riemann tensor and the Christoffel symbols. 
    !
    DO j = 1, 3 
     DO i = 1, 3 
        RiccitildeNCP(i,j) = 0 
        DO l = 1, 3
         DO m = 1, 3
            RiccitildeNCP(i,j) = RiccitildeNCP(i,j)-g_contr(l,m)*0.5*(dDD(l,m,i,j)+dDD(m,l,i,j))
         ENDDO
        ENDDO 
        DO k = 1, 3 
            RiccitildeNCP(i,j) = RiccitildeNCP(i,j) + 0.5*(g_cov(k,i)*dGtildeNCP(j,k)+g_cov(k,j)*dGtildeNCP(i,k)) 
            !RiccitildeNCP(i,j) = RiccitildeNCP(i,j) + 0.5*(g_cov(k,i)*dGhat(j,k)+g_cov(k,j)*dGhat(i,k)) 
        ENDDO
     ENDDO
    ENDDO    
        
    DO j = 1, 3 
     DO i = 1, 3         
        RiccitildeSrc(i,j) = 0
        DO k = 1, 3 
          RiccitildeSrc(i,j) = RiccitildeSrc(i,j) + 0.5*(g_cov(k,i)*dGtildeSrc(j,k)+g_cov(k,j)*dGtildeSrc(i,k)) 
          RiccitildeSrc(i,j) = RiccitildeSrc(i,j) + Ghat(k)*0.5*(Christoffel_kind1(i,j,k)+Christoffel_kind1(j,i,k)) 
          DO l = 1, 3 
           DO m = 1, 3 
            RiccitildeSrc(i,j) = RiccitildeSrc(i,j)+g_contr(l,m)*(Christoffel_tilde(l,i,k)*Christoffel_kind1(j,k,m)+Christoffel_tilde(l,j,k)*Christoffel_kind1(i,k,m)+Christoffel_tilde(i,m,k)*Christoffel_kind1(k,j,l))
           ENDDO
          ENDDO
        ENDDO
     ENDDO
    ENDDO
    
    
    DO j = 1, 3 
     DO i = 1, 3 
       RicciphiNCP(i,j) = 0.5*(dPP(i,j)+dPP(j,i))
       DO k = 1, 3 
        DO l = 1, 3 
           RicciphiNCP(i,j) = RicciphiNCP(i,j)+g_cov(i,j)*g_contr(l,k)*0.5*(dPP(k,l)+dPP(l,k))
        ENDDO
       ENDDO 
     ENDDO
    ENDDO
    
    Pup = MATMUL(g_contr,PP) 
    DO m = 1, 3
        DDcontr(m) = SUM(g_contr*DD(:,m,:)) 
    ENDDO 
    
    DO i = 1, 3 
     DO j = 1, 3 
        RicciphiSrc(i,j) = PP(i)*PP(j) 
        DO k = 1, 3 
         RicciphiSrc(i,j) = RicciphiSrc(i,j) - Christoffel_tilde(i,j,k)*PP(k) - 2*g_cov(i,j)*DDcontr(k)*Pup(k)       
         DO l = 1, 3 
            RicciphiSrc(i,j) = RicciphiSrc(i,j) - g_cov(i,j)*g_contr(l,k)*PP(l)*PP(k) !-g_contr(k,l)*PP(k)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j))   
         ENDDO
        ENDDO
     ENDDO
    ENDDO    
    !
    dZNCP = 0.0 
    dZSrc = 0.0 
    DO i = 1, 3
     DO k = 1, 3    
      DO j = 1, 3 
        dZNCP(k,i) = dZNCP(k,i) + 0.5*g_cov(i,j)*( dGhat(k,j)-dGtildeNCP(k,j) )  
        dZSrc(k,i) = dZSrc(k,i) + DD(k,i,j)*(Ghat(j)-Gtilde(j)) + 0.5*g_cov(i,j)*( -dGtildeSrc(k,j) )
       ENDDO 
      ENDDO 
    ENDDO     
    !
    nablaZNCP = dZNCP 
    nablaXNCP = dXX  
    nablaZSrc = 0.0 
    DO j = 1, 3 
     DO i = 1, 3 
      nablaZSrc(i,j) = dZSrc(i,j)
      DO k = 1, 3 
        nablaZSrc(i,j) = nablaZSrc(i,j) - Christoffel(i,j,k)*Z(k) 
      ENDDO 
     ENDDO
    ENDDO    
    !
    RicciPlusNablaZNCP = RiccitildeNCP + RicciphiNCP + ( nablaZNCP + TRANSPOSE(nablaZNCP) ) ! + ( nablaXNCP + TRANSPOSE(nablaXNCP) ) 
    !
    !RicciPlusNablaZNCP = RicciNCP + ( nablaZNCP + TRANSPOSE(nablaZNCP) ) 
    !RicciPlusNablaZSrc = RicciSrc + ( nablaZSrc + TRANSPOSE(nablaZSrc) ) 
    !
    RPlusTwoNablaZNCP = phi**2*SUM(g_contr*RicciPlusNablaZNCP) 
    !
    !Kupdown = SUM(Kex*Kup) 
    !temp = RPlusTwoNablaZNCP + RPlusTwoNablaZSrc - KupDown + traceK**2 
    !temp = SUM(phi**2*g_contr*Ricci) - KupDown + traceK**2 
    !
    nablaijalphaNCP = 0.0
    nablaijalphaSrc = 0.0
    DO j = 1, 3 
     DO i = 1, 3 
       nablaijalphaNCP(i,j) = alpha*0.5*( dAA(i,j)+dAA(j,i) ) 
       nablaijalphaSrc(i,j) = alpha*AA(j)*AA(i) 
       DO k = 1, 3 
         nablaijalphaSrc(i,j) = nablaijalphaSrc(i,j) - Christoffel(i,j,k)*alpha*AA(k)  
       ENDDO
     ENDDO
    ENDDO 
    nablanablaalphaNCP = phi**2*SUM( g_contr*nablaijalphaNCP ) 
    nablanablaalphaSrc = phi**2*SUM( g_contr*nablaijalphaSrc ) 
!    nablanablaalpha = 0.0
!    DO j = 1, 3 
!     DO i = 1, 3
!         nablanablaalpha = nablanablaalpha + phi**2*( g_contr(i,j)*AA(j)*AA(i)*alpha + alpha*g_contr(i,j)*dAA(i,j) - 2*g_contr(i,j)*SUM(g_contr(:,:)*TRANSPOSE(DD(:,j,:))*AA(i)*alpha)- g_contr(i,j)*PP(i)*AA(j)*alpha )  
!     ENDDO
!    ENDDO 
    !
    SecondOrderTermsNCP = -nablaijalphaNCP + alpha*RicciPlusNablaZNCP 
    SecondOrderTermsSrc = -nablaijalphaSrc + alpha*RicciPlusNablaZSrc 
    traceNCP = SUM( g_contr*SecondOrderTermsNCP ) 
    SecondOrderTermsNCP = SecondOrderTermsNCP - 1./3.*g_cov*traceNCP 
    traceSrc = SUM( g_contr*SecondOrderTermsSrc ) 
    SecondOrderTermsSrc = SecondOrderTermsSrc - 1./3.*g_cov*traceSrc 
    !
    ! Now assemble all this terrible stuff... 
    !
    dtgamma = 0.0 
    !
    ! Main variables of the CCZ4 system 
    dtK = phi**2*SecondOrderTermsNCP + beta(1)*dAex(1,:,:) + beta(2)*dAex(2,:,:) + beta(3)*dAex(3,:,:)      ! extrinsic curvature
    !
    dtTraceK = - nablanablaalphaNCP + alpha*( RPlusTwoNablaZNCP ) + SUM(beta(:)*dtraceK(:)) 
    !
    dtphi   = 0.0 
    dtalpha = 0.0 

    Aupdown = SUM(Aex*Aup) 
    ! *** original 
    dtTheta = 0.5*alpha*e**2*(RplusTwoNablaZNCP ) + beta(1)*dTheta(1) + beta(2)*dTheta(2) + beta(3)*dTheta(3)               ! temporal Z 
    !
    dKex = 0.0
    DO j = 1, 3
     DO i = 1, 3
      DO k = 1, 3
          dKex(k,i,j) = 1.0/phi**2*( dAex(k,i,j) + 1./3.*dtraceK(k)*g_cov(i,j) ) 
      ENDDO
     ENDDO
    ENDDO 
    !
    Mom(:) = 0.0
    DO ii = 1, 3
        DO jj = 1, 3
            DO ll = 1, 3
                Mom(ii) = Mom(ii) + phi**2*g_contr(jj,ll)*(dKex(ll,ii,jj) - dKex(ii,jj,ll))
            ENDDO     
        ENDDO
    ENDDO   
    !
    dtX = alpha*ds**2*Mom + beta(1)*dXX(1,:) + beta(2)*dXX(2,:) + beta(3)*dXX(3,:) 
    !
    DO i = 1, 3
        dtGhat(i) = 2*alpha*( - 2./3.*SUM(g_contr(i,:)*dtraceK(:)) )    &
                    + 2*alpha*SUM( g_contr(:,i)*( dTheta(:)   ) )       & 
                    + beta(1)*dGhat(1,i) + beta(2)*dGhat(2,i) + beta(3)*dGhat(3,i)  
        DO l = 1, 3
         DO k = 1, 3
             dtGhat(i) = dtGhat(i) + g_contr(k,l)*0.5*(dBB(k,l,i)+dBB(l,k,i)) + 1./3*g_contr(i,k)*0.5*(dBB(k,l,l)+dBB(l,k,l)) 
         ENDDO
        ENDDO         
    ENDDO 
    DO k = 1, 3 
        ov(k) = + 2*alpha*( SUM(g_contr(:,:)*dAex(k,:,:)) )    ! here we can use the constraint that trace A tilde = 0.         
    ENDDO        
    dtGhat = dtGhat + MATMUL(g_contr,ov)                                                  ! add the ordering constraint "up" (raised by g_contr) 
    !
    dtbb = xi*dtGhat    !  <= be careful, this damping term -eta*b may be dangerous for the gamma driver, since it may kill the waves that you want !  
    !
    ! Add the following terms if you want shift convection in the PDE for b^i 
    dtbb = dtbb + bs*( - beta(1)*gradQ(14:16,1) - beta(2)*gradQ(14:16,2) - beta(3)*gradQ(14:16,3)  + beta(1)*gradQ(21:23,1) + beta(2)*gradQ(21:23,2) + beta(3)*gradQ(21:23,3)  ) !     
    dtbb = sk*dtbb 
    !
    dtbeta  = 0.0 
    ! Add the following term if you want to have shift convection in the PDE for beta^i 
    !
    ! Auxiliary variables 
    dtA = -alpha*fa*( dtraceK(:) -dK0(:) - c*2*dTheta(:) ) + beta(1)*dAA(1,:) + beta(2)*dAA(2,:) + beta(3)*dAA(3,:) 
    DO k = 1, 3 
        dtA(k) = dtA(k) - alpha*fa*( SUM(g_contr(:,:)*dAex(k,:,:)) )   ! here we can use the constraint that trace A tilde = 0. 
    ENDDO    
    ! 
    ! #xordB1# 
    ! for the ordering constraints, we have to check whether they should be multiplied by alpha**2, or not... 
    dtB = 0.0 
    DO i = 1, 3 
     DO k = 1, 3     
      DO j = 1, 3 
         !dtB(k,i) = +mu*alpha*g_contr(i,j)*( dZNCP(k,j) + dZSrc(k,j) ) + mu*dgup(k,i,j)*Z(j)      
         dtB(k,i) = dtB(k,i) + mu*alpha*g_contr(i,j)*( (dPP(k,j)-dPP(j,k)) )  
         DO n = 1, 3 
          DO l = 1, 3 
            !dtB(k,i) = dtB(k,i) + mu*alpha**2*g_contr(i,j)*( -2*g_contr(n,l)*0.5*(dDD(k,l,n,j)-dDD(l,k,n,j)) ) 
            dtB(k,i) = dtB(k,i) - mu*alpha*g_contr(i,j)*g_contr(n,l)*( dDD(k,l,j,n)-dDD(l,k,j,n) )   
          ENDDO 
         ENDDO 
      ENDDO 
      !
      ENDDO 
    ENDDO 
    dtB = dtB + bs*( beta(1)*dBB(1,:,:) + beta(2)*dBB(2,:,:) + beta(3)*dBB(3,:,:) ) 
    dtB = dtB*sk 
    !
    dtD = -alpha*dAex  
    DO i = 1, 3
      DO j = 1, 3
       DO k = 1, 3 
        DO m = 1, 3
            dtD(k,i,j) = dtD(k,i,j) + ( 0.5*(g_cov(m,i)*0.5*(dBB(k,j,m)+dBB(j,k,m))+g_cov(m,j)*0.5*(dBB(k,i,m)+dBB(i,k,m)) ) - 1./3.*g_cov(i,j)*0.5*(dBB(k,m,m)+dBB(m,k,m)) ) 
            DO n = 1, 3
                dtD(k,i,j) = dtD(k,i,j) + 1./3*alpha*g_cov(i,j)*g_contr(n,m)*dAex(k,n,m)   ! explicitly remove the trace of tilde A again 
            ENDDO 
        ENDDO        
       ENDDO
      ENDDO
    ENDDO     
    !
    dtD = dtD + beta(1)*dDD(1,:,:,:) + beta(2)*dDD(2,:,:,:) + beta(3)*dDD(3,:,:,:)
    !
    dtP = + beta(1)*dPP(1,:) + beta(2)*dPP(2,:) + beta(3)*dPP(3,:)    
    DO k = 1, 3 
     dtP(k) = dtP(k) + 1./3.*alpha*dtraceK(k) + 1./3.*alpha*( SUM(g_contr(:,:)*dAex(k,:,:))  )  
     DO i = 1, 3 
          dtP(k) = dtP(k) - 1./3.*0.5*(dBB(k,i,i)+dBB(i,k,i)) 
     ENDDO
    ENDDO 
    !
    BgradQ(1:6)    = (/ dtgamma(1,1), dtgamma(1,2), dtgamma(1,3), dtgamma(2,2), dtgamma(2,3), dtgamma(3,3) /)          ! \tilde \gamma_ij 
    BgradQ(7:12)   = (/ dtK(1,1), dtK(1,2), dtK(1,3), dtK(2,2), dtK(2,3), dtK(3,3) /)                                  ! \tilde A_ij 
    BgradQ(13)     = dtTheta                                                                                           ! Theta       
    BgradQ(14:16)  = dtGhat(1:3)                                                                                       ! \hat \Gamma^i           
    BgradQ(17)     = dtalpha                                                                                           ! log alpha 
    BgradQ(18:20)  = dtbeta                                                                                            ! beta^i 
    BgradQ(21:23)  = dtbb                                                                                              ! b^i 
    BgradQ(24:26)  = dtA(1:3)                                                                                          ! A_k       
    BgradQ(27:35)  = (/ dtB(1,1), dtB(2,1), dtB(3,1), dtB(1,2), dtB(2,2), dtB(3,2), dtB(1,3), dtB(2,3), dtB(3,3) /)    ! B_k^i 
    BgradQ(36:41)  = (/ dtD(1,1,1), dtD(1,1,2), dtD(1,1,3), dtD(1,2,2), dtD(1,2,3), dtD(1,3,3) /)                      ! D_kij 
    BgradQ(42:47)  = (/ dtD(2,1,1), dtD(2,1,2), dtD(2,1,3), dtD(2,2,2), dtD(2,2,3), dtD(2,3,3) /)                      ! D_kij 
    BgradQ(48:53)  = (/ dtD(3,1,1), dtD(3,1,2), dtD(3,1,3), dtD(3,2,2), dtD(3,2,3), dtD(3,3,3) /)                      ! D_kij 
    BgradQ(54)     = dtTraceK                                                                                          ! traceK 
    BgradQ(55)     = dtphi                                                                                             ! log phi 
    BgradQ(56:58)  = dtP                                                                                               ! P_k 
    BgradQ(59:61)  = dtX                                                                                               ! X_k 
    !
    BgradQ = -BgradQ    ! here, we have to change sign, since we work on the left hand side 
    !
    RETURN
    !
#endif 
    !
    ! *********************************
    !
#ifdef GRMHD
    !
    CALL PDENCPGRMHD(BgradQ,Q,gradQ,par) 
    RETURN
    !
#endif 
    ! 
#ifdef GRGPR
    !
    CALL PDENCPGRGPR(BgradQ,Q,gradQ,par) 
    RETURN
    !
#endif 
    !            
#ifdef GPR3D

    AQx = 0. 
    BQy = 0. 
    CQz = 0. 
    !
    Qx = gradQ(:,1) 
    Qy = gradQ(:,2)
    Qz = gradQ(:,3)    
    !
    u = Q(2:4)/Q(1)  
    ! 
    AQx(6) =  -u(2)*Qx(7) - u(3)*Qx(8) 
    BQy(6) =  +u(2)*Qy(6) 
    CQz(6) =  +u(3)*Qz(6)  
    ! 
    AQx(7) =  +u(1)*Qx(7) 
    BQy(7) =  -u(1)*Qy(6) - u(3)*Qy(8) 
    CQz(7) =  +u(3)*Qz(7) 
    !
    AQx(8) =  +u(1)*Qx(8) 
    BQy(8) =  +u(2)*Qy(8) 
    CQz(8) =  -u(1)*Qz(6) - u(2)*Qz(7) 
    ! 
    AQx(9) =  -u(2)*Qx(10) - u(3)*Qx(11) 
    BQy(9) =  +u(2)*Qy(9) 
    CQz(9) =  +u(3)*Qz(9) 
    !
    AQx(10) = +u(1)*Qx(10)     
    BQy(10) = -u(1)*Qy(9) - u(3)*Qy(11) 
    CQz(10) = +u(3)*Qz(10) 
    !
    AQx(11) = +u(1)*Qx(11) 
    BQy(11) = +u(2)*Qy(11) 
    CQz(11) = -u(1)*Qz(9) - u(2)*Qz(10) 
    !
    AQx(12) = -u(2)*Qx(13) - u(3)*Qx(14)  
    BQy(12) = +u(2)*Qy(12)
    CQz(12) = +u(3)*Qz(12) 
    !
    AQx(13) = +u(1)*Qx(13)  
    BQy(13) = -u(1)*Qy(12) - u(3)*Qy(14) 
    CQz(13) = +u(3)*Qz(13) 
    !
    AQx(14) = +u(1)*Qx(14) 
    BQy(14) = +u(2)*Qy(14) 
    CQz(14) = -u(1)*Qz(12) - u(2)*Qz(13) 
    !
    BgradQ = AQx + BQy + CQz 
    !
#endif 
 
    !if( nDim .eq. 2) then
    !    BgradQ = AQx + BQy         
    !else
    !    BgradQ = AQx + BQy + CQz     
    !end if
END SUBROUTINE PDENCP


RECURSIVE SUBROUTINE PDEEigenvalues(Lambda,Q,nv)
  USE MainVariables, ONLY :  nVar, nDim, EQN
  USE iso_c_binding
  IMPLICIT NONE
  REAL :: Lambda(nVar), nv(3), Q(nVar), Vp(nVar)
  INTENT(IN)  :: Q,nv
  INTENT(OUT) :: Lambda 
  ! Local Variables 
  REAL :: lam, mu, irho, VPR(3), cs,c0,uu,alpha
#if defined(CCZ4EINSTEIN)  
    alpha = MAX( 1.0, EXP(Q(17)) )*MAX( 1.0, EXP(Q(55)) )/MIN( SQRT(Q(1)), SQRT(Q(4)), SQRT(Q(6)) )     
#else
    alpha = 1.0 
#endif
    Lambda = 0.0 
    Lambda(1) = -alpha*MAX(SQRT(2.0), EQN%CCZ4e, EQN%CCZ4ds, EQN%CCZ4GLMc/alpha, EQN%CCZ4GLMd/alpha ) - DOT_PRODUCT(Q(18:20),nv(:))   ! MAX( SQRT(2.0), EQN%CCZ4e, EQN%CCZ4ds ) + SQRT(SUM(Q(18:20)**2)) 
    Lambda(2) = +alpha*MAX(SQRT(2.0), EQN%CCZ4e, EQN%CCZ4ds, EQN%CCZ4GLMc/alpha, EQN%CCZ4GLMd/alpha ) - DOT_PRODUCT(Q(18:20),nv(:))   ! MAX( SQRT(2.0), EQN%CCZ4e, EQN%CCZ4ds ) + SQRT(SUM(Q(18:20)**2)) 
    !
#if defined(CCZ4GRMHD) || defined(CCZ4GRHD) || defined(CCZ4GRGPR) 
    Lambda = 0.0
    Lambda(1) = -1.0*MAX( EQN%CCZ4e, EQN%CCZ4ds, EQN%CCZ4GLMc, EQN%CCZ4GLMd )   
    Lambda(2) = +1.0*MAX( EQN%CCZ4e, EQN%CCZ4ds, EQN%CCZ4GLMc, EQN%CCZ4GLMd )   
    RETURN 
#endif
    ! 
END SUBROUTINE PDEEigenvalues

RECURSIVE SUBROUTINE PDESource(S,Q) 
  USE MainVariables, ONLY:  nVar, nDim,EQN
  USE iso_c_binding
  IMPLICIT NONE
  ! --------------------------------------------
  ! Argument list declaration
  REAL :: S(nvar), Q(nvar)
  INTENT(IN)  :: Q 
  INTENT(OUT) :: S
  ! --------------------------------------------

  ! Local variable declaration 
  REAL :: gradQ(nVar,3)
  REAL :: AM(3,3), Id(3,3), G(3,3) ,devG(3,3), detA, detA2,psiM(3,3), temp2, T
  REAL :: V(nvar)
  REAL :: X(3),time, theta2
  INTEGER :: iErr

  gradQ = 0
  CALL PDEFusedSrcNCP(S, Q, gradQ)


END SUBROUTINE PDESource

RECURSIVE SUBROUTINE PDEVarName(MyNameOUT,ind) 
  USE MainVariables, ONLY: nVar  
  IMPLICIT NONE     
  CHARACTER(LEN=10):: MyName(nVar),MyNameOUT
  INTEGER			:: ind

#if defined(CCZ4EINSTEIN) || defined(CCZ4GRMHD) || defined(CCZ4GRHD) || defined(CCZ4GRGPR) 
  MyName(:)  = 'Unknown'
  MyName(1)  = 'g11' 
  MyName(2)  = 'g12' 
  MyName(3)  = 'g13'
  MyName(4)  = 'g22'
  MyName(5)  = 'g23' 
  MyName(6)  = 'g33' 
  MyName(7)  = 'A11' 
  MyName(8)  = 'A12' 
  MyName(9)  = 'A13'
  MyName(10) = 'A22'
  MyName(11) = 'A23' 
  MyName(12) = 'A33'   
  MyName(13) = 'Theta' 
  MyName(14) = 'G1'
  MyName(15) = 'G2'
  MyName(16) = 'G3'
  MyName(17) = 'lapse'
  MyName(18) = 'shift1'
  MyName(19) = 'shift2'
  MyName(20) = 'shift3'
  MyName(21) = 'b1'
  MyName(22) = 'b2'
  MyName(23) = 'b3'
  MyName(24) = 'A1'
  MyName(25) = 'A2'
  MyName(26) = 'A3'
  MyName(27) = 'B11'
  MyName(28) = 'B21'
  MyName(29) = 'B31'
  MyName(30) = 'B12'
  MyName(31) = 'B22'
  MyName(32) = 'B32'
  MyName(33) = 'B13'
  MyName(34) = 'B23'
  MyName(35) = 'B33' 
  MyName(36) = 'D111'
  MyName(37) = 'D112'
  MyName(38) = 'D113'
  MyName(39) = 'D122'
  MyName(40) = 'D123'
  MyName(41) = 'D133'
  MyName(42) = 'D211'
  MyName(43) = 'D212'
  MyName(44) = 'D213'
  MyName(45) = 'D222'
  MyName(46) = 'D223'
  MyName(47) = 'D233'
  MyName(48) = 'D311'
  MyName(49) = 'D312'
  MyName(50) = 'D313'
  MyName(51) = 'D322'
  MyName(52) = 'D323'
  MyName(53) = 'D333'
  MyName(54) = 'K'  
  MyName(55) = 'phi'  
  MyName(56) = 'P1'  
  MyName(57) = 'P2'  
  MyName(58) = 'P3'
  MyName(59) = 'K0'  
  
#if defined(CCZ4EINSTEIN) || defined(CCZ4GRHD)   
  MyName(60) = 'rho'  
  MyName(61) = 'u'  
  MyName(62) = 'v'  
  MyName(63) = 'w'  
  MyName(64) = 'p'   
#ifdef GLMROT 
  MyName(65) = 'psiA1' 
  MyName(66) = 'psiA2' 
  MyName(67) = 'psiA3' 
  MyName(68) = 'psiP1' 
  MyName(69) = 'psiP2' 
  MyName(70) = 'psiP3' 
  MyName(71) = 'psiD111' 
  MyName(72) = 'psiD112' 
  MyName(73) = 'psiD113' 
  MyName(74) = 'psiD122' 
  MyName(75) = 'psiD123' 
  MyName(76) = 'psiD133' 
  MyName(77) = 'psiD211' 
  MyName(78) = 'psiD212' 
  MyName(79) = 'psiD213' 
  MyName(80) = 'psiD222' 
  MyName(81) = 'psiD223' 
  MyName(82) = 'psiD233' 
  MyName(83) = 'psiD311' 
  MyName(84) = 'psiD312' 
  MyName(85) = 'psiD313' 
  MyName(86) = 'psiD322' 
  MyName(87) = 'psiD323' 
  MyName(88) = 'psiD333' 
  MyName(89) = 'phiA' 
  MyName(90) = 'phiP' 
  MyName(91) = 'phiD11' 
  MyName(92) = 'phiD12' 
  MyName(93) = 'phiD13' 
  MyName(94) = 'phiD22' 
  MyName(95) = 'phiD23' 
  MyName(96) = 'phiD33' 
#ifdef GLMB   
  MyName(97) = 'psiB11' 
  MyName(98) = 'psiB21' 
  MyName(99) = 'psiB31' 
  MyName(100) = 'psiB12' 
  MyName(101) = 'psiB22' 
  MyName(102) = 'psiB32' 
  MyName(103) = 'psiB13' 
  MyName(104) = 'psiB23' 
  MyName(105) = 'psiB33' 
  MyName(106) = 'phiB1' 
  MyName(107) = 'phiB2' 
  MyName(108) = 'phiB3' 
  MyName(109) = 'pad1' 
  MyName(110) = 'pad2' 
  MyName(111) = 'pad3' 
  MyName(112) = 'pad4' 
#endif   
  
#endif   
#endif
#endif
	MyNameOUT=MyName(ind+1)
    END SUBROUTINE PDEVarName

RECURSIVE SUBROUTINE PDEAuxName(MyNameOUT,ind) 
	USE MainVariables, ONLY: nAux  
	IMPLICIT NONE     
	CHARACTER(LEN=10):: AuxName(nAux),MyNameOUT
	INTEGER			:: ind

	!MyNameOUT=AuxName(ind+1)
END SUBROUTINE PDEAuxName
	
RECURSIVE SUBROUTINE PDEAuxVar(aux,Q,x,time)
    USE MainVariables, ONLY : nVar,nAux,EQN
	implicit none

	real :: aux(nAux),Q(nVar),x(3),time
	REAL :: LL_gpr, MM_gpr
	real :: detA,A(3,3), Id(3,3),GT(3,3),devG(3,3),TT(3,3)
	integer :: iErr
	REAL :: V(nVar)
	! print *, "ENTERED HERE ------------------------------------------------------------------------------"
	!aux=0.
	!return

END SUBROUTINE PDEAuxVar
	

RECURSIVE SUBROUTINE PDEEigenvectors(R,L,iR,Q,nv) 
	USE MainVariables, ONLY : nVar, nDim, EQN
	USE iso_c_binding  
	IMPLICIT NONE
	! Argument list 
	REAL :: R(nVar,nVar), L(nVar,nVar), iR(nVar,nVar)
	REAL :: Q(nVar), nv(3)
	REAL :: x(nDim), time
	INTENT(IN)  :: Q,nv
	INTENT(OUT) :: R,L,iR 
	! Local variables
	REAL    :: Lambda(nVar),sv(3),tv(3),TM(3,3),iTM(3,3),Qrot(3),ialpha,uv(3)
    INTEGER :: j,i    

	Lambda=0.
	R=0.
	L = 0.
	iR=0.
	do j=1,nVar
		R(j,j)=1.
		iR(j,j)=1.
	end do

    END SUBROUTINE PDEEigenvectors 
	
RECURSIVE SUBROUTINE PDEJacobian(An,Q,gradQ,nv)
  USE MainVariables, ONLY : nVar, nDim
  USE iso_c_binding
  IMPLICIT NONE
  ! Argument list 
  REAL :: An(nVar,nVar)
  REAL :: Q(nVar), gradQ(nVar,nDim), nv(3) 
  INTENT(IN)  :: Q, gradQ, nv
  INTENT(OUT) :: An 
  
  CALL PDEMatrixB(An,Q,nv) 
  
END SUBROUTINE PDEJacobian
	
RECURSIVE SUBROUTINE PDEMatrixB(An,Q,nv) 
  USE MainVariables, ONLY : nVar, nDim, EQN
  USE iso_c_binding
  IMPLICIT NONE
  ! Argument list 
  REAL :: An(nVar,nVar)
  REAL :: Q(nVar), nv(3) 
  INTENT(IN)  :: Q,nv
  INTENT(OUT) :: An  
  ! Local variables
  ! Linear elasticity variables
  REAL :: A(nVar,nVar), B(nVar,nVar), C(nVar,nVar), Vp(nVar)
  REAL :: alpha , ialpha, uv(3)
  INTEGER :: i
   
    A=0.
    B=0.
    C=0.
	print *, 'Impossible error!'
	
	stop
    if( nDim .eq. 2) then
        An = A*nv(1) + B*nv(2)         
    else
        An = A*nv(1) + B*nv(2) + C*nv(3)     
    end if

END SUBROUTINE PDEMatrixB


RECURSIVE SUBROUTINE HLLEMFluxFV(smaxout,FL,FR,QL,QR,NormalNonZero) 
  USE MainVariables, ONLY : nVar, nDim, nLin
  USE iso_c_binding 
  ! Local variables
  INTEGER, INTENT(IN)   :: NormalNonZero
  REAL, INTENT(IN)     :: QL(nVar)
  REAL, INTENT(IN)     :: QR(nVar)
  REAL, INTENT(INOUT)  :: FL(nVar)
  REAL, INTENT(INOUT)  :: FR(nVar)
  REAL, INTENT(OUT)    :: smaxout
  REAL    :: QavL(nVar), QavR(nVar)  
    ! Local variables 
  INTEGER           :: i,j,k,l, ml(1)  ,iErr
  REAL              :: Qav(nVar)!,LambdaOut
  REAL              ::  nv(3), flattener(nLin), smax
  REAL    :: absA(nVar,nVar), amax  ,gradQ(nVar,3), ncp(nVar)
  REAL    :: QM(nVar),LL(nVar),LR(nVar),LM(nVar)
  REAL    :: deltaL(nLin,nLin),Lam(nLin,nLin),Lap(nLin,nLin) 
  REAL    :: RL(nVar,nLin),iRL(nLin,nVar),LLin(nLin,nLin) , TMPM(nLin, nVar),TMPM2(nVar,nVar)
  REAL    :: Aroe(nVar,nVar),Aroep(nVar,nVar), Aroem(nVar,nVar), Dm(nVar), Dp(nVar), dQ(nVar)
  REAL :: f1R(nVar), g1R(nVar), h1R(nVar) ,flux(nVar),flux2(nVar)
  REAL :: f1L(nVar), g1L(nVar), h1L(nVar) , VM(nVar) 
  REAL :: minLL,minLM,maxLR,maxLM
  REAL  :: QRL(nVar),fR2(nVar),fL2(nVar), alpha, phi 
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVarGRMHD = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVarGRMHD = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVarGRMHD = 19                           ! The number of variables of the PDE system 
#endif
  REAL :: QR_GRMHD(nVarGRMHD), QL_GRMHD(nVarGRMHD)  
  REAL :: LR_GRMHD(nVarGRMHD), LL_GRMHD(nVarGRMHD)  
  REAL :: FR_GRMHD(nVarGRMHD), FL_GRMHD(nVarGRMHD)  
  
  nv(:)=0.
  nv(NormalNonZero+1)=1.
  !

  QM = 0.5*(QL+QR) 
  
	CALL PDEFlux(f1L,g1L,h1L,QL)
	CALL PDEFlux(f1R,g1R,h1R,QR)
	!
	fR = f1R*nv(1)+g1R*nv(2)+h1R*nv(3)
	fL = f1L*nv(1)+g1L*nv(2)+h1L*nv(3)

  
  CALL PDEEigenvalues(LL,QL,nv)  
  CALL PDEEigenvalues(LR,QR,nv)  

  smax = MAX( MAXVAL(ABS(LL)), MAXVAL(ABS(LR)) ) 
  smaxout=smax

  flux = 0.5*(fL + fR) -0.5*smax*( QR - QL ) 
  !
  !
  ! decoupled Rusanov 
  !
#ifdef CCZ4GRHD  

    alpha           = EXP(QL(17)) 
    phi             = EXP(QL(55)) 
    QL_GRMHD(1:5)   = QL(60:64)        ! hydro variables 
    QL_GRMHD(6:9)   = 0.0             ! EM variables 
    QL_GRMHD(10)    = alpha           ! lapse 
    QL_GRMHD(11:13) = QL(18:20)        ! shift 
    QL_GRMHD(14:19) = QL(1:6)/phi**2   ! metric 
    
	CALL PDEEigenvaluesGRMHD(LL_GRMHD,QL_GRMHD,nv)	
  
    alpha           = EXP(QR(17)) 
    phi             = EXP(QR(55)) 
    QR_GRMHD(1:5)   = QR(60:64)        ! hydro variables 
    QR_GRMHD(6:9)   = 0.0             ! EM variables 
    QR_GRMHD(10)    = alpha           ! lapse 
    QR_GRMHD(11:13) = QR(18:20)        ! shift 
    QR_GRMHD(14:19) = QR(1:6)/phi**2   ! metric 
    
	CALL PDEEigenvaluesGRMHD(LR_GRMHD,QR_GRMHD,nv)	
 
  !
  smax = MAX( MAXVAL(ABS(LL_GRMHD)), MAXVAL(ABS(LR_GRMHD)) ) 
  flux(60:64) = 0.5*(fL(60:64) + fR(60:64)) -0.5*smax*( QR(60:64) - QL(60:64) ) 
  ! 	
#endif   
  !
  gradQ=0.
  gradQ(:,NormalNonZero+1) = QR(:) - QL(:) 
  CALL PDENCP(ncp,QM,gradQ)
  Dp = +0.5*ncp
  Dm = +0.5*ncp        
  
  fR = flux - Dp
  fL = flux + Dm
  
  !
  ! HLLEM for GRHD 
  ! 
  !CALL HLLEMFluxFVGRMHD(FL_GRMHD,FR_GRMHD,QL_GRMHD,QR_GRMHD,nv)   
  !
  !fR(60:64) = FR_GRMHD(1:5)
  !fL(60:64) = FL_GRMHD(1:5)
  !
  
 !fR(18:nVar) = 0.- 0.5*ncp(18:nVar)
 !fL(18:nVar) = 0.+ 0.5*ncp(18:nVar) 
  if(any(isnan(fR))) then
	print *, 'Issue here'
    print *,'---------------------'
    print *,QL
    print *, '====================='
    print *,QR
    print *, '====================='
    print *, fR
    print *, '====================='
    print *, h1R
    print *, '====================='
    print *, sR
    print *, '====================='
    print *, sL
	 print *, '====================='
	print *, gradQ
    print *, '***********************'
    print *, ncp
    print *, '***********************'
    print *, flux
	print *, '***********************'
    !print *, Lam
	!    print *, '***********************'
    !print *, Lam
    print *,'---------------------'	
	pause
  end if
    END SUBROUTINE HLLEMFluxFV	

RECURSIVE SUBROUTINE PDEFusedSrcNCP(Src_BgradQ,Q,gradQin)
    USE MainVariables, ONLY : nVar, nParam, d, EQN, nDim 
    IMPLICIT NONE
    ! Argument list 
    REAL, INTENT(IN)  :: Q(nVar), gradQin(nVar,d)
    REAL, INTENT(OUT) :: Src_BgradQ(nVar) 
    ! Local variables 
	REAL				:: gradQ(nVar,d)
	REAL	:: par(nParam), time=0.0
    INTEGER :: i,j,k,l,m,n,ip,iq,ii,jj,kk,ll,mm,iErr,count    
    REAL :: p, irho, lam, mu 
    REAL :: Qx(nVar), Qy(nVar), Qz(nVar), BgradQ(nVar), src(nVar), V(nVar)   
    REAL :: k1,k2,k3,fff,ggg,e,c,ds,xi,sk,sknl,bs,g_cov(3,3),g_contr(3,3),dgup(3,3,3)
    REAL :: det, alpha, fa, k0, dk0(3), beta0(3), b0(3), u(3), ialpha 
    REAL :: s1, s2, s3, s4, s5, s6, s7, s8, s9, s10 
    REAL :: AQx(nVar), BQy(nVar), CQz(nVar), Smom(3), iphi2, ddm(3,3,3)   
    REAL :: lapse, shift(3), gammaij(6), delta(3,3), bv(3), vxb(3), vxb_contr(3), psi, qv_contr(3), qb_contr(3), bv_contr(3) 
    REAL :: v2,vf(3),uem,b2,e2,gp,gm,vc(nVar),lf,w,ww,gamma1,rho,vf_cov(3), s_contr(3), w_ij, wim 
    REAL :: v_cov(3), v_contr(3), SijTF(3,3), phi2, EE 
	

#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVarGRMHD = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVarGRMHD = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVarGRMHD = 19                           ! The number of variables of the PDE system 
#endif
    REAL :: VGRMHD(nVarGRMHD), QGRMHD(nVarGRMHD), gradQGRMHD(nVarGRMHD,d), BgradQGRMHD(nVarGRMHD), eta, itau, Ham, Mom(3), dKex(3,3,3), ov(3) 
#ifdef VECTOR    
      INTEGER, PARAMETER :: nVarGRGPR = 32                           ! The number of variables of the PDE system 
#else
      INTEGER, PARAMETER :: nVarGRGPR = 30                           ! The number of variables of the PDE system 
#endif
    REAL :: SGRGPR(nVarGRGPR),QGRGPR(nVarGRGPR), VGRGPR(nVarGRGPR), gradQGRGPR(nVarGRGPR,d), BgradQGRGPR(nVarGRGPR)
    !    
    REAL :: Christoffel(3,3,3), RiemannNCP(3,3,3,3), RiemannSrc(3,3,3,3), dChristoffelNCP(3,3,3,3), dChristoffelSrc(3,3,3,3), DD(3,3,3), dDD(3,3,3,3), dChristoffelOrd(3,3,3,3)   
    REAL :: dChristoffel_tildeNCP(3,3,3,3), dChristoffel_tildeSrc(3,3,3,3) 
    REAL :: R, AA(3), dAA(3,3), BB(3,3), dBB(3,3,3), beta(3), Kex(3,3), Kmix(3,3), Kup(3,3), Z(3), dZ(3,3), nablaZNCP(3,3), nablaZSrc(3,3), RplusNablaZNCP, RplusNablaZSrc  
    REAL :: Theta, dTheta(3), nablaijalphaNCP(3,3), nablaijalphaSrc(3,3), Ricci(3,3), RicciNCP(3,3), RicciSrc(3,3), dtraceK(3), dtraceKNCP(3), dKtempNCP(3), dZNCP(3,3), dZSrc(3,3)  
    REAL :: dtgamma(3,3), dtK(3,3), dK(3,3,3), dtTheta, dtZ(3), dtalpha, dtGhat(3), dtbeta(3), dtbb(3), dtA(3), dtB(3,3), dtD(3,3,3)  
    REAL :: Aupdown, Aex(3,3), dAex(3,3,3), Amix(3,3), Aup(3,3), Ghat(3), Gtilde(3), dGhat(3,3), traceK, Kupdown, phi, PP(3), dPP(3,3), Pup(3), DDcontr(3) 
    REAL :: dGtildeSrc(3,3), dGtildeNCP(3,3), RiccitildeNCP(3,3), RicciphiNCP(3,3), RiccitildeSrc(3,3), RicciphiSrc(3,3)  
    REAL :: Christoffel_tilde(3,3,3), Christoffel_kind1(3,3,3), Zup(3), RicciPlusNablaZNCP(3,3), RicciPlusNablaZSrc(3,3), traceA, traceB, QG(3), b(3), faa, temp   
    REAL :: SecondOrderTermsNCP(3,3), SecondOrderTermsSrc(3,3), traceNCP, traceSrc, dtphi, dtTraceK, dtP(3)    
    REAL :: RNCP, RSrc, RPlusTwoNablaZNCP, RPlusTwoNablaZSrc, nablanablaalpha, nablanablaalphaNCP, nablanablaalphaSrc, Riemann(3,3,3,3), dChristoffel(3,3,3,3) 
    REAL :: TwoNablaZNCP, TwoNablaZSrc,  divAupNCP(3), divAupSrc(3), XX(3), dXX(3,3), nablaXNCP(3,3), nablaXSrc(3,3), dtX(3)  
    ! Matter contributions
    REAL :: sm(3), Sij(3,3), Sij_contr(3,3), sm_contr(3), S, tau, dens, bv_cov(3), sqrdet 
    REAL :: srctraceK, srcTheta
    REAL :: SrcK(3,3), SrcGhat(3),mytmp1(3,3,3,3), mytmp2(3,3,3,3) 
    
    REAL, PARAMETER :: Pi = ACOS(-1.0) 
    !
    BgradQ = 0.0 
    !
#if defined(CCZ4EINSTEIN) || defined(CCZ4GRHD) || defined(CCZ4GRMHD) || defined(CCZ4GRGPR) 
    !
    Qx = gradQin(:,1) 
    Qy = gradQin(:,2)
	IF(nDim.eq.2) THEN
	    Qz = 0.0
		gradQ(:,1:2)=gradQin(:,1:2)
		gradQ(:,3)	=0.0
	ELSE
		Qz = gradQin(:,3)
		gradQ=gradQin
	ENDIF 

    k1   = EQN%CCZ4k1  
    k2   = EQN%CCZ4k2  
    k3   = EQN%CCZ4k3   
    fff  = EQN%CCZ4f 
    ggg  = EQN%CCZ4g 
    eta  = EQN%CCZ4eta 
    itau = EQN%CCZ4itau  
    e    = EQN%CCZ4e 
    c    = EQN%CCZ4c 
    mu   = EQN%CCZ4mu 
    ds   = EQN%CCZ4ds 
    bs   = EQN%CCZ4bs 
    xi   = EQN%CCZ4xi 
    sk   = EQN%CCZ4sk
    !
    ! These are the tilde quantities, so be careful !    
    g_cov(1,1) = Q(1)
    g_cov(1,2) = Q(2)
    g_cov(1,3) = Q(3)
    g_cov(2,1) = Q(2)
    g_cov(2,2) = Q(4)
    g_cov(2,3) = Q(5)
    g_cov(3,1) = Q(3)
    g_cov(3,2) = Q(5)
    g_cov(3,3) = Q(6)
    ! This determinant should be unity, since we use the conformal decomposition 
    det = (Q(1)*Q(4)*Q(6)-Q(1)*Q(5)**2-Q(2)**2*Q(6)+2*Q(2)*Q(3)*Q(5)-Q(3)**2*Q(4)) 
    !g_cov = det**(-1./3.) * g_cov 
    !det = (g_cov(1,1)*g_cov(2,2)*g_cov(3,3)-g_cov(1,1)*g_cov(2,3)*g_cov(3,2)-g_cov(2,1)*g_cov(1,2)*g_cov(3,3)+g_cov(2,1)*g_cov(1,3)*g_cov(3,2)+g_cov(3,1)*g_cov(1,2)*g_cov(2,3)-g_cov(3,1)*g_cov(1,3)*g_cov(2,2)) 
    
    g_contr(1,1) =  (g_cov(2,2)*g_cov(3,3)-g_cov(2,3)*g_cov(3,2)) / det 
    g_contr(1,2) = -(g_cov(1,2)*g_cov(3,3)-g_cov(1,3)*g_cov(3,2)) / det
    g_contr(1,3) = -(-g_cov(1,2)*g_cov(2,3)+g_cov(1,3)*g_cov(2,2))/ det 
    g_contr(2,1) = -(g_cov(2,1)*g_cov(3,3)-g_cov(2,3)*g_cov(3,1)) / det 
    g_contr(2,2) = (g_cov(1,1)*g_cov(3,3)-g_cov(1,3)*g_cov(3,1))  / det 
    g_contr(2,3) = -(g_cov(1,1)*g_cov(2,3)-g_cov(1,3)*g_cov(2,1)) / det 
    g_contr(3,1) = -(-g_cov(2,1)*g_cov(3,2)+g_cov(2,2)*g_cov(3,1))/ det 
    g_contr(3,2) = -(g_cov(1,1)*g_cov(3,2)-g_cov(1,2)*g_cov(3,1)) / det 
    g_contr(3,3) = (g_cov(1,1)*g_cov(2,2)-g_cov(1,2)*g_cov(2,1))  / det 
      
    alpha = EXP(MAX(-20.,MIN(20.,Q(17)))) 
    SELECT CASE(EQN%CCZ4LapseType) 
    CASE(0)  ! harmonic 
        fa = 1.0 
        faa = 0.0 
    CASE DEFAULT  ! 1 + log 
        fa = 2.0/alpha
        faa = -2.0/alpha**2   
    END SELECT 
    ! 
    K0    = Q(59)
    dK0   = sk*gradQ(59,:) 
    !  
    Aex(1,1) = Q(7) 
    Aex(1,2) = Q(8) 
    Aex(1,3) = Q(9) 
    Aex(2,1) = Q(8) 
    Aex(2,2) = Q(10) 
    Aex(2,3) = Q(11) 
    Aex(3,1) = Q(9) 
    Aex(3,2) = Q(11) 
    Aex(3,3) = Q(12) 
    !
    traceA = SUM(g_contr*Aex) 
    Aex = Aex - 1./3.*g_cov*traceA 
    !
    dAex(:,1,1) = gradQ(7,:) 
    dAex(:,1,2) = gradQ(8,:) 
    dAex(:,1,3) = gradQ(9,:) 
    dAex(:,2,1) = gradQ(8,:) 
    dAex(:,2,2) = gradQ(10,:) 
    dAex(:,2,3) = gradQ(11,:) 
    dAex(:,3,1) = gradQ(9,:) 
    dAex(:,3,2) = gradQ(11,:) 
    dAex(:,3,3) = gradQ(12,:) 
    !
    Amix = MATMUL(g_contr, Aex)
    Aup  = MATMUL(g_contr, TRANSPOSE(Amix)) 
    !
    Theta = Q(13)
    dTheta = gradQ(13,:) 
    ! 
    Ghat = (/ Q(14), Q(15), Q(16) /)
    dGhat(:,1) = gradQ(14,:)
    dGhat(:,2) = gradQ(15,:)
    dGhat(:,3) = gradQ(16,:)
    !
    b = Q(21:23) 
    !
    AA    = (/ Q(24), Q(25), Q(26) /) 
    dAA(:,1) = gradQ(24,:) 
    dAA(:,2) = gradQ(25,:) 
    dAA(:,3) = gradQ(26,:) 
    !
    traceK = Q(54) 
    dtraceK = gradQ(54,:) 
    !
    phi   =  EXP(MAX(-20.,MIN(20.,Q(55))))  

    PP    = Q(56:58) 
    dPP(:,1) = gradQ(56,:) 
    dPP(:,2) = gradQ(57,:) 
    dPP(:,3) = gradQ(58,:) 
    !
    beta = (/ Q(18), Q(19), Q(20) /) 
    BB(1,1) = Q(27) 
    BB(2,1) = Q(28) 
    BB(3,1) = Q(29) 
    BB(1,2) = Q(30) 
    BB(2,2) = Q(31) 
    BB(3,2) = Q(32) 
    BB(1,3) = Q(33) 
    BB(2,3) = Q(34) 
    BB(3,3) = Q(35) 
    !
    dBB(:,1,1) = gradQ(27,:) 
    dBB(:,2,1) = gradQ(28,:) 
    dBB(:,3,1) = gradQ(29,:) 
    dBB(:,1,2) = gradQ(30,:) 
    dBB(:,2,2) = gradQ(31,:) 
    dBB(:,3,2) = gradQ(32,:) 
    dBB(:,1,3) = gradQ(33,:) 
    dBB(:,2,3) = gradQ(34,:) 
    dBB(:,3,3) = gradQ(35,:) 
    !
    dBB = dBB*sk    
    ! 
    DD(1,1,1)=Q(36) 
    DD(1,1,2)=Q(37) 
    DD(1,1,3)=Q(38) 
    DD(1,2,1)=Q(37) 
    DD(1,2,2)=Q(39) 
    DD(1,2,3)=Q(40)
    DD(1,3,1)=Q(38) 
    DD(1,3,2)=Q(40) 
    DD(1,3,3)=Q(41)
    ! 
    DD(2,1,1)=Q(42) 
    DD(2,1,2)=Q(43) 
    DD(2,1,3)=Q(44) 
    DD(2,2,1)=Q(43) 
    DD(2,2,2)=Q(45) 
    DD(2,2,3)=Q(46)
    DD(2,3,1)=Q(44) 
    DD(2,3,2)=Q(46) 
    DD(2,3,3)=Q(47)
    !
    DD(3,1,1)=Q(48) 
    DD(3,1,2)=Q(49) 
    DD(3,1,3)=Q(50) 
    DD(3,2,1)=Q(49) 
    DD(3,2,2)=Q(51) 
    DD(3,2,3)=Q(52)
    DD(3,3,1)=Q(50) 
    DD(3,3,2)=Q(52) 
    DD(3,3,3)=Q(53)
    !
    dDD(:,1,1,1)=gradQ(36,:) 
    dDD(:,1,1,2)=gradQ(37,:) 
    dDD(:,1,1,3)=gradQ(38,:) 
    dDD(:,1,2,1)=gradQ(37,:) 
    dDD(:,1,2,2)=gradQ(39,:) 
    dDD(:,1,2,3)=gradQ(40,:)
    dDD(:,1,3,1)=gradQ(38,:) 
    dDD(:,1,3,2)=gradQ(40,:) 
    dDD(:,1,3,3)=gradQ(41,:)
    dDD(:,2,1,1)=gradQ(42,:) 
    dDD(:,2,1,2)=gradQ(43,:) 
    dDD(:,2,1,3)=gradQ(44,:) 
    dDD(:,2,2,1)=gradQ(43,:) 
    dDD(:,2,2,2)=gradQ(45,:) 
    dDD(:,2,2,3)=gradQ(46,:)
    dDD(:,2,3,1)=gradQ(44,:) 
    dDD(:,2,3,2)=gradQ(46,:) 
    dDD(:,2,3,3)=gradQ(47,:) 
    dDD(:,3,1,1)=gradQ(48,:) 
    dDD(:,3,1,2)=gradQ(49,:) 
    dDD(:,3,1,3)=gradQ(50,:) 
    dDD(:,3,2,1)=gradQ(49,:) 
    dDD(:,3,2,2)=gradQ(51,:) 
    dDD(:,3,2,3)=gradQ(52,:)
    dDD(:,3,3,1)=gradQ(50,:) 
    dDD(:,3,3,2)=gradQ(52,:) 
    dDD(:,3,3,3)=gradQ(53,:)
    !
    dgup = 0.0 
    DO n = 1, 3
     DO j = 1, 3 
      DO l = 1, 3 
       DO m = 1, 3 
        DO k = 1, 3 
           dgup(k,m,l) = dgup(k,m,l)-g_contr(m,n)*g_contr(j,l)*2*DD(k,n,j) 
        ENDDO
       ENDDO 
      ENDDO 
     ENDDO 
    ENDDO         
    !
    Kex  = Aex/phi**2 + 1./3.*traceK*g_cov/phi**2 
    Kmix = MATMUL( phi**2*g_contr, Kex  ) 
    Kup  = MATMUL( phi**2*g_contr, TRANSPOSE(Kmix) ) 
    !
    Christoffel_tilde = 0.0  
    Christoffel       = 0.0 
    Gtilde = 0.0 
    !
    DO k = 1, 3
     DO j = 1, 3
      DO i = 1, 3
       !Christoffel_kind1(i,j,k) =  DD(i,j,k)+DD(j,i,k)-DD(k,i,j)     ! this definition does not work ! 
       Christoffel_kind1(i,j,k) = DD(k,i,j)+DD(j,i,k)-DD(i,j,k)      ! this definition seems to work ! 
       DO l = 1, 3
          Christoffel_tilde(i,j,k) = Christoffel_tilde(i,j,k) + g_contr(k,l)*( DD(i,j,l)+DD(j,i,l)-DD(l,i,j) ) 
		  !mytmp1(i,j,k,l)                    = DD(i,j,l)+DD(j,i,l)-DD(l,i,j) 
          !Christoffel(i,j,k)       = Christoffel(i,j,k)       + g_contr(k,l)*( mytmp1(i,j,k,l)		  ) 
		  !mytmp2(i,j,k,l)=( g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l) ) 
		  !Christoffel(i,j,k)       = Christoffel(i,j,k)       -g_contr(k,l)*mytmp2(i,j,k,l) 
          !Gtilde(i)                = Gtilde(i)+2*g_contr(i,j)*g_contr(k,l)*DD(l,j,k) 
		  Christoffel(i,j,k)       = Christoffel(i,j,k)       + g_contr(k,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j))-g_contr(k,l)*( g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l) ) 
		  !PRINT *, Christoffel(i,j,k) ,I,J,K
        ENDDO 
      ENDDO
     ENDDO
    ENDDO
    !print *, 'g_contr',g_contr
	!print *, 'g_cov',g_cov
	!print *, 'DD',DD
	!print *, 'PP',PP
	!print *, 'mytmp1',mytmp1
	!print *, 'mytmp2',mytmp2
	!print *, 'Christoffel',Christoffel
	!pause
	
    DO l = 1, 3
     DO j = 1, 3
      DO i = 1, 3
          Gtilde(i) = Gtilde(i) + g_contr(j,l)*Christoffel_tilde(j,l,i) 
      ENDDO
     ENDDO     
    ENDDO    
    !
    Z   = ds*0.5*MATMUL( g_cov, Ghat - Gtilde ) 
    Zup = MATMUL(phi**2*g_contr, Z) 
    !
    dChristoffelNCP = 0.0
    dChristoffelSrc = 0.0 
    dChristoffel_tildeNCP = 0.0
    dChristoffel_tildeSrc = 0.0 
    DO l = 1, 3 
     DO m = 1, 3 
      DO j = 1, 3 
       DO i = 1, 3 
        DO k = 1, 3
            dChristoffelNCP(k,i,j,m) = dChristoffelNCP(k,i,j,m) + g_contr(m,l)*( 0.5*(dDD(k,i,j,l)+dDD(i,k,j,l))+0.5*(dDD(k,j,i,l)+dDD(j,k,i,l))-0.5*(dDD(k,l,i,j)+dDD(l,k,i,j)) )         & 
                                                                  - g_contr(m,l)*( g_cov(j,l)*0.5*(dPP(k,i)+dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,j)+dPP(j,k))-g_cov(i,j)*0.5*(dPP(k,l)+dPP(l,k)) ) 
            !
            dChristoffel_tildeNCP(k,i,j,m) = dChristoffel_tildeNCP(k,i,j,m) + g_contr(m,l)*( 0.5*(dDD(k,i,j,l)+dDD(i,k,j,l))+0.5*(dDD(k,j,i,l)+dDD(j,k,i,l))-0.5*(dDD(k,l,i,j)+dDD(l,k,i,j)) ) 
            ! 
            dChristoffelSrc(k,i,j,m) = dChristoffelSrc(k,i,j,m) + dgup(k,m,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j)) - dgup(k,m,l)*(g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l)) - g_contr(m,l)*( 2*DD(k,j,l)*PP(i)+2*DD(k,i,l)*PP(j)-2*DD(k,i,j)*PP(l) ) 
            !
            dChristoffel_tildeSrc(k,i,j,m) = dChristoffel_tildeSrc(k,i,j,m) + dgup(k,m,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j)) 
          ENDDO 
       ENDDO
      ENDDO
     ENDDO
    ENDDO
    ! 
    RiemannSrc = 0.0 
    RiemannNCP = 0.0 
    DO m = 1, 3 
     DO j = 1, 3 
      DO k = 1, 3 
       DO i = 1, 3
          RiemannNCP(i,k,j,m) = dChristoffelNCP(k,i,j,m)-dChristoffelNCP(j,i,k,m)
          RiemannSrc(i,k,j,m) = dChristoffelSrc(k,i,j,m)-dChristoffelSrc(j,i,k,m) 
          DO l = 1, 3
           RiemannSrc(i,k,j,m) = RiemannSrc(i,k,j,m) + Christoffel(i,j,l)*Christoffel(l,k,m) - Christoffel(i,k,l)*Christoffel(l,j,m) 
          ENDDO 
       ENDDO
      ENDDO
     ENDDO
    ENDDO    
    ! 
    RicciNCP = 0.0 
    RicciSrc = 0.0 
    DO l = 1, 3 
     DO n = 1, 3
      DO m = 1, 3    
         RicciNCP(m,n) = RicciNCP(m,n) + RiemannNCP(m,l,n,l)  
         RicciSrc(m,n) = RicciSrc(m,n) + RiemannSrc(m,l,n,l)  
      ENDDO
     ENDDO
    ENDDO    
    !
    RNCP = phi**2*SUM(g_contr*RicciNCP) 
    RSrc = phi**2*SUM(g_contr*RicciSrc) 
    !
    ! Here we directly compute the derivative of Gtilde from its original definition as contracted Christoffel symbol,
    ! without assuming unit determinant of the conformal metric. Back to the roots, and as few assumptions as possible... 
    ! 
    dGtildeNCP = 0.0
    dGtildeSrc = 0.0
    DO l = 1, 3
     DO j = 1, 3
      DO i = 1, 3 
       DO k = 1, 3
           dGtildeSrc(k,i) = dGtildeSrc(k,i) + dgup(k,j,l)*Christoffel_tilde(j,l,i) + g_contr(j,l)*dChristoffel_tildeSrc(k,j,l,i) 
           dGtildeNCP(k,i) = dGtildeNCP(k,i)                                        + g_contr(j,l)*dChristoffel_tildeNCP(k,j,l,i) 
       ENDDO
      ENDDO
     ENDDO
    ENDDO
    !
    dZNCP = 0.0 
    dZSrc = 0.0 
    DO j = 1, 3
     DO i = 1, 3    
      DO k = 1, 3 
        dZNCP(k,i) = dZNCP(k,i) + ds*0.5*g_cov(i,j)*( dGhat(k,j)-dGtildeNCP(k,j) )  
        dZSrc(k,i) = dZSrc(k,i) + ds*(DD(k,i,j)*(Ghat(j)-Gtilde(j)) + 0.5*g_cov(i,j)*( -dGtildeSrc(k,j) ) ) 
       ENDDO 
      ENDDO 
    ENDDO     
    !
    nablaZNCP = dZNCP 
    nablaZSrc = 0.0 
    DO j = 1, 3 
     DO i = 1, 3 
      nablaZSrc(i,j) = dZSrc(i,j)
      DO k = 1, 3 
        nablaZSrc(i,j) = nablaZSrc(i,j) - Christoffel(i,j,k)*Z(k) 
      ENDDO 
     ENDDO
    ENDDO    
    !
    RicciPlusNablaZNCP = RicciNCP + ( nablaZNCP + TRANSPOSE(nablaZNCP) ) 
    RicciPlusNablaZSrc = RicciSrc + ( nablaZSrc + TRANSPOSE(nablaZSrc) ) 
    !
    RPlusTwoNablaZNCP = phi**2*SUM(g_contr*RicciPlusNablaZNCP) 
    RPlusTwoNablaZSrc = phi**2*SUM(g_contr*RicciPlusNablaZSrc) 
    !
    nablaijalphaNCP = 0.0
    nablaijalphaSrc = 0.0
    DO j = 1, 3 
     DO i = 1, 3 
       nablaijalphaNCP(i,j) = alpha*0.5*( dAA(i,j)+dAA(j,i) ) 
       nablaijalphaSrc(i,j) = alpha*AA(j)*AA(i) 
       DO k = 1, 3 
         nablaijalphaSrc(i,j) = nablaijalphaSrc(i,j) - Christoffel(i,j,k)*alpha*AA(k)  
       ENDDO
     ENDDO
    ENDDO 
    nablanablaalphaNCP = phi**2*SUM( g_contr*nablaijalphaNCP ) 
    nablanablaalphaSrc = phi**2*SUM( g_contr*nablaijalphaSrc ) 
    !
    SecondOrderTermsNCP = -nablaijalphaNCP + alpha*RicciPlusNablaZNCP 
    SecondOrderTermsSrc = -nablaijalphaSrc + alpha*RicciPlusNablaZSrc 
    traceNCP = SUM( g_contr*SecondOrderTermsNCP ) 
    SecondOrderTermsNCP = SecondOrderTermsNCP - 1./3.*g_cov*traceNCP 
    traceSrc = SUM( g_contr*SecondOrderTermsSrc ) 
    SecondOrderTermsSrc = SecondOrderTermsSrc - 1./3.*g_cov*traceSrc 
    !
    ! Now assemble all this terrible stuff... 
    !
    dtgamma = - 2*alpha*Aex - itau*(det-1.0)*g_cov 
    DO k = 1, 3 
     DO j = 1, 3
      DO i = 1, 3
          dtgamma(i,j) = dtgamma(i,j) + g_cov(k,i)*BB(j,k) + g_cov(k,j)*BB(i,k) - 2./3.*g_cov(i,j)*BB(k,k) + beta(k)*2*DD(k,i,j) 
      ENDDO
     ENDDO
    ENDDO 
    !
    ! Main variables of the CCZ4 system 
    dtK = phi**2*SecondOrderTermsNCP + beta(1)*dAex(1,:,:) + beta(2)*dAex(2,:,:) + beta(3)*dAex(3,:,:)      ! extrinsic curvature
    dtK = dtK + phi**2*SecondOrderTermsSrc + alpha*Aex*(traceK-2*Theta) - 2*alpha*MATMUL(Aex,Amix) - itau*g_cov*traceA 
    DO j = 1, 3 
     DO i = 1, 3 
      DO k = 1, 3 
         dtK(i,j) = dtK(i,j) + Aex(k,i)*BB(j,k) + Aex(k,j)*BB(i,k) - 2./3.*Aex(i,j)*BB(k,k) 
      ENDDO
     ENDDO
    ENDDO 
    !
    dtTraceK = - nablanablaalphaNCP - nablanablaalphaSrc + alpha*( RPlusTwoNablaZNCP + RPlusTwoNablaZSrc + traceK**2 - 2*Theta*traceK ) - 3*alpha*k1*(1+k2)*Theta + SUM(beta(:)*dtraceK(:)) 
    !
	!print *, 'dtTraceK=', dtTraceK
	!print *, 'nablanablaalphaNCP=', nablanablaalphaNCP
	!print *, 'nablanablaalphaSrc=', nablanablaalphaSrc
	!print *, 'RPlusTwoNablaZNCP=', RPlusTwoNablaZNCP
	!print *, 'RPlusTwoNablaZSrc=', RPlusTwoNablaZSrc
	!print *, 'traceK=', traceK
	!print *, 'dtraceK=', dtraceK
	!print *, 'Theta=', Theta
	!print *, 'Christoffel',Christoffel
	!print *, 'Christoffel',Christoffel
	!print *, 'Gtilde',Gtilde
	!print *, 'Christoffel_tilde',Christoffel_tilde
	!print *, 'dChristoffelsrc',dChristoffelsrc
	!print *, 'dChristoffel_tildesrc',dChristoffel_tildesrc
	!print *, 'Z',Z
	!pause
	
    traceB = BB(1,1) + BB(2,2) + BB(3,3) 
    dtphi   = beta(1)*PP(1) + beta(2)*PP(2) + beta(3)*PP(3) + 1./3.*alpha*traceK - 1./3.*traceB 
    dtalpha = -alpha*fa*(traceK-K0-c*2*Theta) + beta(1)*AA(1) + beta(2)*AA(2) + beta(3)*AA(3) 

    Aupdown = SUM(Aex*Aup) 
    ! *** original 
    dtTheta =  0.5*alpha*e**2*(RplusTwoNablaZNCP + RplusTwoNablaZSrc) + beta(1)*dTheta(1) + beta(2)*dTheta(2) + beta(3)*dTheta(3)    &            ! temporal Z 
             + 0.5*alpha*e**2*( - Aupdown + 2./3.*traceK**2 ) - alpha*Theta*traceK - SUM(Zup*alpha*AA) - alpha*k1*(2+k2)*Theta  
    ! *** use turbo cleaning here, i.e. without multiplying with alpha *** 
    !dtTheta = 0.5*e**2*( RplusTwoNablaZNCP + RplusTwoNablaZSrc ) + beta(1)*dTheta(1) + beta(2)*dTheta(2) + beta(3)*dTheta(3)    &                ! temporal Z 
    !         + 0.5*e**2*( - Aupdown + 2./3.*traceK**2 ) - c*alpha*Theta*traceK - SUM(Zup*alpha*AA) - k2*Theta 
    !
    divAupNCP = 0.0
    divAupSrc = 0.0 
    DO k = 1, 3
        DO j = 1, 3
         DO l = 1, 3
          DO i = 1, 3    
            divAupNCP(i) = divAupNCP(i) + g_contr(i,l)*g_contr(j,k)*dAex(j,l,k) 
            divAupSrc(i) = divAupSrc(i) + ( dgup(j,i,l)*g_contr(j,k) + g_contr(i,l)*dgup(j,j,k) )*Aex(l,k) 
          ENDDO
         ENDDO
        ENDDO        
    ENDDO 
    ! 
    !DO i = 1, 3 
    !    Mom(i) = SUM(Christoffel_tilde(:,:,i)*Aup(:,:)) - 3.0*SUM(Aup(i,:)*PP(:)) - 2./3.*SUM(g_contr(i,:)*dtraceK(:)) + divAupNCP(i) + divAupSrc(i)  
    !ENDDO 
    !
    !Kupdown = SUM(Kex*Kup) 
    !Ham = RPlusTwoNablaZNCP + RPlusTwoNablaZSrc - KupDown + traceK**2     
    !
    dtGhat = 0.0 
    DO i = 1, 3
          dtGhat(i) = dtGhat(i) +        & 
                      + 2*alpha*( SUM(Christoffel_tilde(:,:,i)*Aup(:,:)) - 3.0*SUM(Aup(i,:)*PP(:)) - 2./3.*SUM(g_contr(i,:)*dtraceK(:)) )    &
                      + 2*alpha*SUM( g_contr(:,i)*( dTheta(:) - Theta*AA(:) - 2./3.*traceK*Z(:)  ) )  & 
                      - 2*SUM( Aup(i,:)*alpha*AA(:) ) - 2*alpha*k1*SUM(g_contr(i,:)*Z(:)) - SUM(Gtilde(:)*BB(:,i))   &
                    + beta(1)*dGhat(1,i) + beta(2)*dGhat(2,i) + beta(3)*dGhat(3,i) + 2./3.*Gtilde(i)*traceB 
        DO l = 1, 3
         DO k = 1, 3
             dtGhat(i) = dtGhat(i) + g_contr(k,l)*0.5*(dBB(k,l,i)+dBB(l,k,i)) + 1./3*g_contr(i,k)*0.5*(dBB(k,l,l)+dBB(l,k,l)) + & 
                                     2*k3*( 2./3.*g_contr(i,l)*Z(l)*BB(k,k) - g_contr(l,k)*Z(l)*BB(k,i) ) 
         ENDDO
        ENDDO         
    ENDDO 
    DO k = 1, 3 
        ov(k) = + 2*alpha*( SUM(g_contr(:,:)*dAex(k,:,:)) + SUM(dgup(k,:,:)*Aex(:,:)) )   ! here we can use the constraint that trace A tilde = 0.         
    ENDDO        
    dtGhat = dtGhat + sk*MATMUL(g_contr,ov)                                               ! the above ordering constraint is "down", so we must raise the index via g_contr. 
    !
    dtbb = xi*dtGhat - eta*b                                                              !  <= be careful, this damping term -eta*b may be dangerous for the gamma driver, since it may kill the waves that you want !  
    ! Add the following terms if you want shift convection in the PDE for b^i 
    dtbb = dtbb + bs*( - beta(1)*gradQ(14:16,1) - beta(2)*gradQ(14:16,2) - beta(3)*gradQ(14:16,3)  + beta(1)*gradQ(21:23,1) + beta(2)*gradQ(21:23,2) + beta(3)*gradQ(21:23,3)  )   !         
    dtbb = sk*dtbb 
    !
    dtbeta  = + fff*b  
    ! Add the following term if you want to have shift convection in the PDE for beta^i 
    ! Do not add it if you want a real Lie derivative for beta. In this case, the advection term cancels out. 
    dtbeta = dtbeta + bs*( beta(1)*BB(1,:) + beta(2)*BB(2,:) + beta(3)*BB(3,:) )      
    dtbeta = sk*dtbeta 
    !
    ! Auxiliary variables 
    dtA = -alpha*fa*( dtraceK(:) -dK0(:) - c*2*dTheta(:) ) + beta(1)*dAA(1,:) + beta(2)*dAA(2,:) + beta(3)*dAA(3,:) - alpha*AA*(fa+alpha*faa)*(traceK-K0-c*2*Theta) + MATMUL(BB, AA) 
    DO k = 1, 3 
        dtA(k) = dtA(k) - sk*alpha*fa*( SUM(g_contr(:,:)*dAex(k,:,:)) + SUM(dgup(k,:,:)*Aex(:,:)) )   ! here we can use the constraint that trace A tilde = 0. 
    ENDDO    
    ! 
    ! In CCZ4 we have completely removed all the conservative fluxes. 
    dtB(:,1) = fff*gradQ(21,:)  
    dtB(:,2) = fff*gradQ(22,:)  
    dtB(:,3) = fff*gradQ(23,:)  
    ! #ordB2# 
    ! for the ordering constraints, we have to check whether they should be multiplied by alpha**2, or not... 
    DO j = 1, 3 
     DO i = 1, 3     
      DO k = 1, 3 
         !dtB(k,i) = +mu*alpha*g_contr(i,j)*( dZNCP(k,j) + dZSrc(k,j) ) + mu*dgup(k,i,j)*Z(j)      
         dtB(k,i) = dtB(k,i) + mu*alpha**2*g_contr(i,j)*( (dPP(k,j)-dPP(j,k)) )  
         DO n = 1, 3 
          DO l = 1, 3 
            !dtB(k,i) = dtB(k,i) + mu*alpha**2*g_contr(i,j)*( -2*g_contr(n,l)*0.5*(dDD(k,l,n,j)-dDD(l,k,n,j)) ) 
            dtB(k,i) = dtB(k,i) - mu*alpha**2*g_contr(i,j)*g_contr(n,l)*( dDD(k,l,j,n)-dDD(l,k,j,n) )   
          ENDDO 
         ENDDO 
      ENDDO 
      !
      ENDDO 
    ENDDO 
    !
    dtB = dtB + bs*( beta(1)*dBB(1,:,:) + beta(2)*dBB(2,:,:) + beta(3)*dBB(3,:,:) + MATMUL(BB,BB) ) 
    dtB = dtB*sk 
    !
    dtD = -alpha*dAex  
    DO m = 1, 3
     DO j = 1, 3
      DO i = 1, 3
        DO k = 1, 3 
            dtD(k,i,j) = dtD(k,i,j) + ( 0.5*(g_cov(m,i)*0.5*(dBB(k,j,m)+dBB(j,k,m))+g_cov(m,j)*0.5*(dBB(k,i,m)+dBB(i,k,m)) ) - 1./3.*g_cov(i,j)*0.5*(dBB(k,m,m)+dBB(m,k,m)) ) 
            DO n = 1, 3
                dtD(k,i,j) = dtD(k,i,j) + 1./3*alpha*g_cov(i,j)*g_contr(n,m)*dAex(k,n,m) + 1./3.*alpha*g_cov(i,j)*dgup(k,n,m)*Aex(n,m)   ! explicitly remove the trace of tilde A again 
            ENDDO 
        ENDDO        
       ENDDO
      ENDDO
    ENDDO 
    ! 
    DO j = 1, 3 
     DO i = 1, 3
      DO k = 1, 3 
        dtD(k,i,j) = dtD(k,i,j) - alpha*AA(k)*Aex(i,j) 
        DO l = 1, 3 
          dtD(k,i,j) = dtD(k,i,j) + BB(k,l)*DD(l,i,j) + DD(k,l,i)*BB(j,l) + DD(k,l,j)*BB(i,l) - 2./3.*DD(k,i,j)*BB(l,l) 
        ENDDO 
      ENDDO
     ENDDO
    ENDDO         
    !
    dtD = dtD + beta(1)*dDD(1,:,:,:) + beta(2)*dDD(2,:,:,:) + beta(3)*dDD(3,:,:,:)
    !
    dtP = MATMUL(BB,PP) + beta(1)*dPP(1,:) + beta(2)*dPP(2,:) + beta(3)*dPP(3,:)    
    DO k = 1, 3 
     dtP(k) = dtP(k) + 1./3.*alpha*dtraceK(k) + 1./3.*alpha*AA(k)*traceK + sk*1./3.*alpha*( SUM(g_contr(:,:)*dAex(k,:,:)) + SUM(dgup(k,:,:)*Aex(:,:)) )  
     DO i = 1, 3 
          dtP(k) = dtP(k) - 1./3.*0.5*(dBB(k,i,i)+dBB(i,k,i)) 
     ENDDO
    ENDDO 
    !
#ifdef CCZ4GRMHD 
    ! 
    ! Add algebraic matter source terms to CCZ4 
    sqrdet   = phi**(-6.0/2.0) ! square root of determinant
    dens     = QGRMHD(1)   / sqrdet
    sm       = QGRMHD(2:4) / sqrdet
    sm_contr = MATMUL(phi**2*g_contr,sm) 
    tau      = QGRMHD(5)   / sqrdet
    Bv_cov   = QGRMHD(6:8) / sqrdet
    Bv_contr = MATMUL(phi**2*g_contr,Bv_cov)
    E        = tau + dens            ! U = E
    ! prepare the PDECons2PrimGRMHD.
    QGRMHD(1:9)   = Q(60:68)      ! hydro variables
    QGRMHD(10)    = alpha         ! lapse
    QGRMHD(11:13) = Q(18:20)      ! shift
    QGRMHD(14:19) = Q(1:6)/phi**2 ! metric (without tilde).
    CALL PDECons2PrimGRMHD(VGRMHD,QGRMHD,iErr)
    rho    = VGRMHD(1)
    vf_cov = VGRMHD(2:4)
    p      = VGRMHD(5)
    ! compute the lorentz factor
    vf       = MATMUL(phi**2*g_contr,vf_cov)
    v2       = SUM(vf*vf_cov)
    lf       = 1.0/sqrt(1.0 - v2)
    gamma1 = EQN%gamma/(EQN%gamma-1.0)
    w      = rho + gamma1*p   ! rho*hentalpy
    ww     = w*lf**2          ! rho*hentalpy*Lorentz^2 
    b2     = SUM(Bv_cov*Bv_contr)**2/lf**2 + SUM(Bv_contr*Bv_cov)**2 ! magnetic pressure
    !
    if(rho>0.9*1e-2) then
        continue
    endif 
    !
    DO j = 1, 3
      DO i = 1, 3
        Sij(i,j) = ww*vf_cov(i)*vf_cov(j) + (p + b2/2)*g_cov(i,j)/(phi**2) &
		 - Bv_cov(i)*Bv_cov(j)/lf**2 - SUM(Bv_contr*vf_cov)*vf_cov(i)*Bv_cov(j)
      ENDDO
    ENDDO
    !
    Sij_contr = MATMUL(phi**2*g_contr,MATMUL(phi**2*g_contr,Sij)) 
    S = SUM(phi**2*g_contr*Sij) ! Trace of Sij

    ! Compute Source contributions
    SrcK        = -(phi**2)*8*pi*alpha*( Sij - 1./3.*g_cov/phi**2 * S ) ! Tracefree
    dtK         = dtK + SrcK

    SrcTraceK   = 4*pi*alpha*(S - 3*E)
    dtTraceK    = dtTraceK + SrcTraceK
    
    SrcGhat     = -16*pi*alpha*MATMUL(g_contr, sm)
    dtGhat      = dtGhat + SrcGhat
    
    SrcTheta    = -8*pi*alpha*E
    dtTheta     = dtTheta + SrcTheta
    ! 
#elif defined(CCZ4GRHD) 
    !
    ! Add algebraic matter source terms to CCZ4
    CALL PDECons2Prim(V,Q,iErr) 
    phi2       = phi*phi 
    gm         = phi2*phi      ! inverse square root of determinant = phi**3 
    gp         = 1./gm 
    dens       = Q(60) * gm
    sm(1)       = Q(61) * gm
    sm(2)       = Q(62) * gm
    sm(3)       = Q(63) * gm
    sm_contr(1) = phi2*( g_contr(1,1)*sm(1) + g_contr(1,2)*sm(2) + g_contr(1,3)*sm(3) ) 
    sm_contr(2) = phi2*( g_contr(2,1)*sm(1) + g_contr(2,2)*sm(2) + g_contr(2,3)*sm(3) ) 
    sm_contr(3) = phi2*( g_contr(3,1)*sm(1) + g_contr(3,2)*sm(2) + g_contr(3,3)*sm(3) ) 
    tau        = Q(64) * gm
    EE          = tau + dens            ! U = E
    ! Use the results of the primitive variables contained in V.
    rho      = V(60)
    v_cov(1)  = V(61)
    v_cov(2)  = V(62)
    v_cov(3)  = V(63)
    p        = V(64)
    !
    IF( Q(60) < 1e-6 ) THEN  
        rho = 0.0
        p   = 0.0
        EE  = 0.0
    ENDIF 
    !
    ! compute the contravariant velocity and Lorentz factor
    v_contr(1) = phi2*( g_contr(1,1)*v_cov(1) + g_contr(1,2)*v_cov(2) + g_contr(1,3)*v_cov(3) )
    v_contr(2) = phi2*( g_contr(2,1)*v_cov(1) + g_contr(2,2)*v_cov(2) + g_contr(2,3)*v_cov(3) )
    v_contr(3) = phi2*( g_contr(3,1)*v_cov(1) + g_contr(3,2)*v_cov(2) + g_contr(3,3)*v_cov(3) )
    !
    !!
    v2 = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
    e2 = 0.0 
    b2 = 0.0 
    !
    uem = 0.0 
    !
    lf   = 1.0/SQRT(1.0 - v2)
    gamma1  = EQN%gamma/(EQN%gamma-1.0)
    w    = rho + gamma1*p     ! rho*hentalpy
    ww   = w*lf**2               ! rho*hentalpy*Lorentz^2 
    
    !
    DO j = 1, 3
      DO i = 1, 3
        Sij(i,j)       = ww*v_cov(i)*v_cov(j)     + ( p + uem )*g_cov(i,j)/phi2   !- B_cov(i)*B_cov(j) - vxB_cov(i)*vxB_cov(j)  
        Sij_contr(i,j) = ww*v_contr(i)*v_contr(j) + ( p + uem )*g_contr(i,j)*phi2 !- B_contr(i)*B_contr(j) - vxB_contr(i)*vxB_contr(j)  
      ENDDO
    ENDDO
    ! Trace of S_ij 
    S = phi2* (  g_contr(1,1)*Sij(1,1) + g_contr(1,2)*Sij(1,2) + g_contr(1,3)*Sij(1,3)  +  &    
                       g_contr(2,1)*Sij(2,1) + g_contr(2,2)*Sij(2,2) + g_contr(2,3)*Sij(2,3)  +  &    
                       g_contr(3,1)*Sij(3,1) + g_contr(3,2)*Sij(3,2) + g_contr(3,3)*Sij(3,3)  )       ! SUM(phi**2*g_contr*Sij) ! Trace of Sij
    !
    DO j = 1, 3 
     DO i = 1, 3 
        SijTF(i,j) = Sij(i,j) - 1./3.*g_cov(i,j) * S/phi2       
     ENDDO
    ENDDO
    ! 
    ! Compute Source contributions
    DO j = 1, 3 
     DO i = 1, 3 
        SrcK(i,j)     = -phi2*8.0*pi*alpha*SijTF(i,j)   
     ENDDO
    ENDDO
    ! 
    dtK(:,:)      = dtK(:,:) + SrcK(:,:) 
    ! 
    SrcTraceK  = 4.0*pi*alpha*( S - 3*EE )
    dtTraceK   = dtTraceK + SrcTraceK 
    !    
    SrcGhat(1)  = -16*pi*alpha/phi2*sm_contr(1) 
    SrcGhat(2)  = -16*pi*alpha/phi2*sm_contr(2) 
    SrcGhat(3)  = -16*pi*alpha/phi2*sm_contr(3) 
    ! 
    dtGhat(:)   = dtGhat(:) + SrcGhat(:)  
    !     
    SrcTheta    = -e**2*8*pi*alpha*EE 
    dtTheta     = dtTheta + SrcTheta 
    !
    continue    
    !
#elif defined(CCZ4GRGPR)  
    ! 
    ! Add algebraic matter source terms to CCZ4 
    sqrdet   = phi**(-6.0/2.0) ! square root of determinant
    dens     = QGRGPR(1)   / sqrdet
    sm       = QGRGPR(2:4) / sqrdet
    sm_contr = MATMUL(phi**2*g_contr,sm) 
    tau      = QGRGPR(5)   / sqrdet
    Bv_cov   = 0.  !QGRGPR(6:8) / sqrdet
    Bv_contr = 0.  !MATMUL(phi**2*g_contr,Bv_cov)
    E        = tau + dens            ! U = E
    ! prepare the PDECons2PrimGRGPR.
    QGRGPR(1:14)   = Q(60:73)       ! gpr variables
    QGRGPR(25:30)   = Q(74:79)      ! gpr variables
    QGRGPR(15)    = alpha         ! lapse
    QGRGPR(16:18) = Q(18:20)      ! shift
    QGRGPR(19:24) = Q(1:6)/phi**2 ! metric (without tilde).
    CALL PDECons2PrimGRGPR(VGRGPR,QGRGPR,iErr)
    rho    = VGRGPR(1)
    vf_cov = VGRGPR(2:4)
    p      = VGRGPR(5)
    ! compute the lorentz factor
    vf       = MATMUL(phi**2*g_contr,vf_cov)
    v2       = SUM(vf*vf_cov)
    lf       = 1.0/sqrt(1.0 - v2)
    gamma1 = EQN%gamma/(EQN%gamma-1.0)
    w      = rho + gamma1*p   ! rho*hentalpy
    ww     = w*lf**2          ! rho*hentalpy*Lorentz^2 
    b2     = 0. ! SUM(Bv_cov*Bv_contr)**2/lf**2 + SUM(Bv_contr*Bv_cov)**2 ! magnetic pressure
    !
    if(rho>0.9*1e-2) then
        continue
    endif 
    !
    DO j = 1, 3
      DO i = 1, 3
        !Sij(i,j) = ww*vf_cov(i)*vf_cov(j) + (p + b2/2)*g_cov(i,j)/(phi**2) &
		 !- Bv_cov(i)*Bv_cov(j)/lf**2 - SUM(Bv_contr*vf_cov)*vf_cov(i)*Bv_cov(j)
        Sij(i,j) = ww*vf_cov(i)*vf_cov(j) + p*g_cov(i,j)/(phi**2)
      ENDDO
    ENDDO
    !
    Sij_contr = MATMUL(phi**2*g_contr,MATMUL(phi**2*g_contr,Sij)) 
    S = SUM(phi**2*g_contr*Sij) ! Trace of Sij

    ! Compute Source contributions
    SrcK        = -(phi**2)*8*pi*alpha*( Sij - 1./3.*g_cov/phi**2 * S ) ! Tracefree
    dtK         = dtK + SrcK

    SrcTraceK   = 4*pi*alpha*(S - 3*E)
    dtTraceK    = dtTraceK + SrcTraceK
    
    SrcGhat     = -16*pi*alpha*MATMUL(g_contr, sm)
    dtGhat      = dtGhat + SrcGhat
    
    SrcTheta    = -8*pi*alpha*E
    dtTheta     = dtTheta + SrcTheta
    !     
#else     
    !
    BgradQ(59:nVar) = 0.0   
    ! use padding variables to produce source terms 
    ! Add algebraic matter source terms to CCZ4 
    phi2       = phi*phi 
    gm         = phi2*phi      ! inverse square root of determinant = phi**3 
    gp         = 1./gm 
    dens       = Q(60)   
    sm(1)      = 0.0   ! *Q(:,61) 
    sm(2)      = 0.0   ! *Q(:,62) 
    sm(3)      = 0.0   ! *Q(:,63) 
    sm_contr(1) = phi2*( g_contr(1,1)*sm(1) + g_contr(1,2)*sm(2) + g_contr(1,3)*sm(3) ) 
    sm_contr(2) = phi2*( g_contr(2,1)*sm(1) + g_contr(2,2)*sm(2) + g_contr(2,3)*sm(3) ) 
    sm_contr(3) = phi2*( g_contr(3,1)*sm(1) + g_contr(3,2)*sm(2) + g_contr(3,3)*sm(3) ) 
    !tau(:)        = Q(:,64) * gm(:)
    EE        = Q(60) + 1.0/(EQN%gamma-1)*Q(64)     ! only valid for v=0 and thus LF=1. 
    ! Use the results of the primitive variables contained in V.
    rho       = Q(60)
    v_cov(1)  = 0.0  ! *V(:,61)
    v_cov(2)  = 0.0  ! *V(:,62)
    v_cov(3)  = 0.0  ! *V(:,63)
    p         = Q(64)
    ! 
    ! In the atmosphere, we are actually in vacuum, hence we must switch off the matter source terms there 
    ! 
    IF( rho < 1e-6 ) THEN 
        rho = 0.0
        p   = 0.0
        EE  = 0.0
    ENDIF  
    !
    ! compute the contravariant velocity and Lorentz factor
    v_contr(1) = phi2*( g_contr(1,1)*v_cov(1) + g_contr(1,2)*v_cov(2) + g_contr(1,3)*v_cov(3) )
    v_contr(2) = phi2*( g_contr(2,1)*v_cov(1) + g_contr(2,2)*v_cov(2) + g_contr(2,3)*v_cov(3) )
    v_contr(3) = phi2*( g_contr(3,1)*v_cov(1) + g_contr(3,2)*v_cov(2) + g_contr(3,3)*v_cov(3) )
    !
    v2 = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
    !
    lf   = 1.0/SQRT(1.0 - v2)
    gamma1  = EQN%gamma/(EQN%gamma-1.0)    
    w    = rho + gamma1*p        ! rho*hentalpy
    ww   = w*lf**2               ! rho*hentalpy*Lorentz^2   
    !
    DO j = 1, 3
      DO i = 1, 3
        Sij(i,j)       = ww*v_cov(i)*v_cov(j)     + p *g_cov(i,j)/phi2   
        Sij_contr(i,j) = ww*v_contr(i)*v_contr(j) + p *g_contr(i,j)*phi2 
      ENDDO
    ENDDO
    ! Trace of S_ij 
    S = phi2* (  g_contr(1,1)*Sij(1,1) + g_contr(1,2)*Sij(1,2) + g_contr(1,3)*Sij(1,3)  +  &    
                       g_contr(2,1)*Sij(2,1) + g_contr(2,2)*Sij(2,2) + g_contr(2,3)*Sij(2,3)  +  &    
                       g_contr(3,1)*Sij(3,1) + g_contr(3,2)*Sij(3,2) + g_contr(3,3)*Sij(3,3)  )       ! SUM(phi**2*g_contr*Sij) ! Trace of Sij
    !
    DO j = 1, 3 
     DO i = 1, 3 
        SijTF(i,j) = Sij(i,j) - 1./3.*g_cov(i,j) * S/phi2       
     ENDDO
    ENDDO
    ! 
    ! Compute Source contributions
    DO j = 1, 3 
     DO i = 1, 3 
        SrcK(i,j)     = -phi2*8.0*pi*alpha*SijTF(i,j)   
     ENDDO
    ENDDO
    ! 
    dtK(:,:)      = dtK(:,:) + SrcK(:,:) 
    ! 
    SrcTraceK  = 4.0*pi*alpha*( S - 3*EE )
    dtTraceK   = dtTraceK + SrcTraceK 
    !    
    SrcGhat(1)  = -16*pi*alpha/phi2*sm_contr(1) 
    SrcGhat(2)  = -16*pi*alpha/phi2*sm_contr(2) 
    SrcGhat(3)  = -16*pi*alpha/phi2*sm_contr(3) 
    ! 
    dtGhat(:)   = dtGhat(:) + SrcGhat(:)  
    !     
    SrcTheta    = -e**2*8*pi*alpha*EE 
    dtTheta     = dtTheta + SrcTheta 
    !
    v_cov(1)  = Q(61)
    v_cov(2)  = Q(62)
    v_cov(3)  = Q(63)   
    !
    BgradQ(60) = -v_cov(1)*Qx(60) - v_cov(2)*Qy(60) - v_cov(3)*Qz(60)  
    BgradQ(64) = -v_cov(1)*Qx(64) - v_cov(2)*Qy(64) - v_cov(3)*Qz(64)  
    ! 
    continue    
    !

#endif 
    !
    BgradQ(1:6)    = (/ dtgamma(1,1), dtgamma(1,2), dtgamma(1,3), dtgamma(2,2), dtgamma(2,3), dtgamma(3,3) /)          ! \tilde \gamma_ij 
    BgradQ(7:12)   = (/ dtK(1,1), dtK(1,2), dtK(1,3), dtK(2,2), dtK(2,3), dtK(3,3) /)                                  ! \tilde A_ij 
    BgradQ(13)     = dtTheta                                                                                           ! Theta       
    BgradQ(14:16)  = dtGhat(1:3)                                                                                       ! \hat \Gamma^i           
    BgradQ(17)     = dtalpha                                                                                           ! log alpha 
    BgradQ(18:20)  = dtbeta                                                                                            ! beta^i 
    BgradQ(21:23)  = dtbb                                                                                              ! b^i 
    BgradQ(24:26)  = dtA(1:3)                                                                                          ! A_k       
    BgradQ(27:35)  = (/ dtB(1,1), dtB(2,1), dtB(3,1), dtB(1,2), dtB(2,2), dtB(3,2), dtB(1,3), dtB(2,3), dtB(3,3) /)    ! B_k^i 
    BgradQ(36:41)  = (/ dtD(1,1,1), dtD(1,1,2), dtD(1,1,3), dtD(1,2,2), dtD(1,2,3), dtD(1,3,3) /)                      ! D_kij 
    BgradQ(42:47)  = (/ dtD(2,1,1), dtD(2,1,2), dtD(2,1,3), dtD(2,2,2), dtD(2,2,3), dtD(2,3,3) /)                      ! D_kij 
    BgradQ(48:53)  = (/ dtD(3,1,1), dtD(3,1,2), dtD(3,1,3), dtD(3,2,2), dtD(3,2,3), dtD(3,3,3) /)                      ! D_kij 
    BgradQ(54)     = dtTraceK                                                                                          ! traceK 
    BgradQ(55)     = dtphi                                                                                             ! log phi 
    BgradQ(56:58)  = dtP                                                                                               ! P_k 
    !
    Src_BgradQ = BgradQ    ! here, we do not have to change sign, since we work on the right hand side in the fused subroutine 
    !
#ifdef CCZ4GRMHD 
    !
    ! Add the metric terms to the GRMHD equations 
    !
    alpha = EXP(Q(17)) 
    phi   = EXP(Q(55)) 
    QGRMHD(1:9)   = Q(60:68)        ! hydro variables 
    QGRMHD(10)    = alpha           ! lapse 
    QGRMHD(11:13) = Q(18:20)        ! shift 
    QGRMHD(14:19) = Q(1:6)/phi**2   ! metric 
    !
    gradQGRMHD(1:9,:)   = 0.0   ! hydro variables (not needed) 
    gradQGRMHD(10,:)    = alpha*(/ Q(24), Q(25), Q(26) /)           ! lapse 
    gradQGRMHD(11,:)    = (/ Q(27), Q(28), Q(29) /)                 ! shift1 
    gradQGRMHD(12,:)    = (/ Q(30), Q(31), Q(32) /)                 ! shift2 
    gradQGRMHD(13,:)    = (/ Q(33), Q(34), Q(35) /)                 ! shift3 
    gradQGRMHD(14:19,1)  = 2.0/phi**2*( Q(36:41) - PP(1)*Q(1:6) )   ! metric 
    gradQGRMHD(14:19,2)  = 2.0/phi**2*( Q(42:47) - PP(2)*Q(1:6) )   ! metric 
    gradQGRMHD(14:19,3)  = 2.0/phi**2*( Q(48:53) - PP(3)*Q(1:6) )   ! metric 
    !
    CALL PDENCPGRMHD(BgradQGRMHD,QGRMHD,gradQGRMHD,par) 
    src_bgradQ(60:68) = -BgradQGRMHD(1:9)   ! Change sign, since the NCPGRMHD is on the left, but the fused stuff is on the right. 
    !  Do not use the Cowling approximation! => Extr. Curvature:
    src_bgradQ(64) = sqrdet*( alpha*SUM(Kex*Sij_contr) - SUM(sm_contr*aa*alpha) ) ! Energy with dynamical extr. curvature     
    !
    ! CCZ4end 
    !
    CONTINUE
    !
#elif defined(CCZ4GRHD) 

    phi           = V(55)             ! phi 
    iphi2         = 1./phi**2 
    !
    CONTINUE
    !
    DO j = 1, 3 
     DO i = 1, 3 
         Kex(i,j)  = Aex(i,j)/phi**2 + 1./3.*traceK*g_cov(i,j)/phi**2 
     ENDDO
    ENDDO 
    S = Sij_contr(1,1)*Kex(1,1) + Sij_contr(1,2)*Kex(1,2) + Sij_contr(1,3)*Kex(1,3)  +  &    
           Sij_contr(2,1)*Kex(2,1) + Sij_contr(2,2)*Kex(2,2) + Sij_contr(2,3)*Kex(2,3)  +  &    
           Sij_contr(3,1)*Kex(3,1) + Sij_contr(3,2)*Kex(3,2) + Sij_contr(3,3)*Kex(3,3)  -  &    ! SUM(phi**2*g_contr*Sij) ! Trace of Sij
           ( sm_contr(1)*AA(1) + sm_contr(2)*AA(2) + sm_contr(3)*AA(3) )                        ! no alpha here, since it will be multiplied later 
    !
    ! compute derivative of the real metric (*0.5), not the one of the conformal metric 
    DO j = 1, 3
     DO i = 1, 3
      DO k = 1, 3
          DDm(k,i,j) = iphi2 * ( DD(k,i,j) - g_cov(i,j)*PP(k) ) 
      ENDDO
     ENDDO
    ENDDO    
    !
    DO j = 1, 3
     Smom(j) = -alpha*AA(j)*EE
     DO i = 1, 3
      Smom(j) = Smom(j) + sm(i)*BB(i,j) 
      DO k = 1, 3 
          Smom(j) = Smom(j) + alpha*Sij_contr(i,k)*DDm(j,i,k) 
      ENDDO
     ENDDO
    ENDDO    
    !
    ! Here we do NOT use the Cowling approximation 
    src_bgradQ(61) = gp*Smom(1)           ! algebraic source term, directly implemented 
    src_bgradQ(62) = gp*Smom(2)           ! 
    src_bgradQ(63) = gp*Smom(3)           ! 
    src_bgradQ(64) = gp*alpha*S       ! Energy with dynamic extr. curvature   
    !
    !
#elif defined(CCZ4GRGPR)
    !
    ! Add the metric terms to the GRGPR equations 
    !
    alpha = EXP(Q(17)) 
    phi   = EXP(Q(55)) 
    QGRGPR(1:14)   = Q(60:73)        ! gpr variables 
    QGRGPR(25:30)   = Q(74:79)        ! gpr variables 
    QGRGPR(15)    = alpha           ! lapse 
    QGRGPR(16:18) = Q(18:20)        ! shift 
    QGRGPR(19:24) = Q(1:6)/phi**2   ! metric 
    !
    gradQGRGPR(1:14,:)  = gradQ(60:73,:)   ! gpr variables (not needed?) 
    gradQGRGPR(25:30,:) = gradQ(74:79,:)   ! gpr variables (not needed?) 
    !gradQGRGPR(1:14,:) = 0.0   ! gpr variables (not needed?) 
    !gradQGRGPR(25:30,:) = 0.0   ! gpr variables (not needed?) 
    gradQGRGPR(15,:)    = alpha*(/ Q(24), Q(25), Q(26) /)           ! lapse 
    gradQGRGPR(16,:)    = (/ Q(27), Q(28), Q(29) /)                 ! shift1 
    gradQGRGPR(17,:)    = (/ Q(30), Q(31), Q(32) /)                 ! shift2 
    gradQGRGPR(18,:)    = (/ Q(33), Q(34), Q(35) /)                 ! shift3 
    gradQGRGPR(19:24,1) = 2.0/phi**2*( Q(36:41) - PP(1)*Q(1:6) )   ! metric 
    gradQGRGPR(19:24,2) = 2.0/phi**2*( Q(42:47) - PP(2)*Q(1:6) )   ! metric 
    gradQGRGPR(19:24,3) = 2.0/phi**2*( Q(48:53) - PP(3)*Q(1:6) )   ! metric 
    !
    CALL PDENCPGRGPR(BgradQGRGPR,QGRGPR,gradQGRGPR,par) 
    CALL PDESourceGRGPR(SGRGPR,QGRGPR,par,time)  
    !
    src_bgradQ(60:73) = SGRGPR(1:14) - BgradQGRGPR(1:14)   ! Change sign, since the NCPGRGPR is on the left, but the fused stuff is on the right.
    src_bgradQ(74:79) = SGRGPR(25:30) - BgradQGRGPR(25:30)   ! Change sign, since the NCPGRGPR is on the left, but the fused stuff is on the right. 
    ! Do not use the Cowling approximation! => Extr. Curvature:
    src_bgradQ(64) = sqrdet*( alpha*SUM(Kex*Sij_contr) - SUM(sm_contr*aa*alpha) ) ! Energy with dynamical extr. curvature    
    !
    ! CCZ4end 
    !
    CONTINUE
    ! 
    !
#endif      
    !
    RETURN
    !
#endif 
    !
    !
#if defined(BSSZ4EINSTEIN) || defined(BSSZ4GRMHD) 
    !
    Qx = gradQ(:,1) 
    Qy = gradQ(:,2)
    Qz = gradQ(:,3)

    k1   = EQN%CCZ4k1  
    k2   = EQN%CCZ4k2  
    k3   = EQN%CCZ4k3   
    fff  = EQN%CCZ4f 
    ggg  = EQN%CCZ4g 
    eta  = EQN%CCZ4eta 
    itau = EQN%CCZ4itau  
    e    = EQN%CCZ4e 
    c    = EQN%CCZ4c 
    mu   = EQN%CCZ4mu 
    ds   = EQN%CCZ4ds 
    bs   = EQN%CCZ4bs 
    xi   = EQN%CCZ4xi 
    sk   = EQN%CCZ4sk
    !
    ! These are the tilde quantities, so be careful !    
    g_cov(1,1) = Q(1)
    g_cov(1,2) = Q(2)
    g_cov(1,3) = Q(3)
    g_cov(2,1) = Q(2)
    g_cov(2,2) = Q(4)
    g_cov(2,3) = Q(5)
    g_cov(3,1) = Q(3)
    g_cov(3,2) = Q(5)
    g_cov(3,3) = Q(6)
    ! This determinant should be unity, since we use the conformal decomposition 
    det = (Q(1)*Q(4)*Q(6)-Q(1)*Q(5)**2-Q(2)**2*Q(6)+2*Q(2)*Q(3)*Q(5)-Q(3)**2*Q(4)) 
    g_cov = det**(-1./3.) * g_cov 
    det = (g_cov(1,1)*g_cov(2,2)*g_cov(3,3)-g_cov(1,1)*g_cov(2,3)*g_cov(3,2)-g_cov(2,1)*g_cov(1,2)*g_cov(3,3)+g_cov(2,1)*g_cov(1,3)*g_cov(3,2)+g_cov(3,1)*g_cov(1,2)*g_cov(2,3)-g_cov(3,1)*g_cov(1,3)*g_cov(2,2)) 
    
    g_contr(1,1) =  (g_cov(2,2)*g_cov(3,3)-g_cov(2,3)*g_cov(3,2)) / det 
    g_contr(1,2) = -(g_cov(1,2)*g_cov(3,3)-g_cov(1,3)*g_cov(3,2)) / det
    g_contr(1,3) = -(-g_cov(1,2)*g_cov(2,3)+g_cov(1,3)*g_cov(2,2))/ det 
    g_contr(2,1) = -(g_cov(2,1)*g_cov(3,3)-g_cov(2,3)*g_cov(3,1)) / det 
    g_contr(2,2) = (g_cov(1,1)*g_cov(3,3)-g_cov(1,3)*g_cov(3,1))  / det 
    g_contr(2,3) = -(g_cov(1,1)*g_cov(2,3)-g_cov(1,3)*g_cov(2,1)) / det 
    g_contr(3,1) = -(-g_cov(2,1)*g_cov(3,2)+g_cov(2,2)*g_cov(3,1))/ det 
    g_contr(3,2) = -(g_cov(1,1)*g_cov(3,2)-g_cov(1,2)*g_cov(3,1)) / det 
    g_contr(3,3) = (g_cov(1,1)*g_cov(2,2)-g_cov(1,2)*g_cov(2,1))  / det 
      
    alpha = EXP(MAX(-20.,MIN(20.,Q(17)))) 
    SELECT CASE(EQN%CCZ4LapseType) 
    CASE(0)  ! harmonic 
        fa = 1.0 
        faa = 0.0 
    CASE DEFAULT  ! 1 + log 
        fa = 2.0/alpha
        faa = -2.0/alpha**2   
    END SELECT 
    ! 
    K0    = Q(62)
    dK0   = sk*gradQ(62,:) 
    !  
    Aex(1,1) = Q(7) 
    Aex(1,2) = Q(8) 
    Aex(1,3) = Q(9) 
    Aex(2,1) = Q(8) 
    Aex(2,2) = Q(10) 
    Aex(2,3) = Q(11) 
    Aex(3,1) = Q(9) 
    Aex(3,2) = Q(11) 
    Aex(3,3) = Q(12) 
    !
    traceA = SUM(g_contr*Aex) 
    Aex = Aex - 1./3.*g_cov*traceA 
    !
    dAex(:,1,1) = gradQ(7,:) 
    dAex(:,1,2) = gradQ(8,:) 
    dAex(:,1,3) = gradQ(9,:) 
    dAex(:,2,1) = gradQ(8,:) 
    dAex(:,2,2) = gradQ(10,:) 
    dAex(:,2,3) = gradQ(11,:) 
    dAex(:,3,1) = gradQ(9,:) 
    dAex(:,3,2) = gradQ(11,:) 
    dAex(:,3,3) = gradQ(12,:) 
    !
    Amix = MATMUL(g_contr, Aex)
    Aup  = MATMUL(g_contr, TRANSPOSE(Amix)) 
    !
    Theta = Q(13)
    dTheta = gradQ(13,:) 
    ! 
    Ghat = (/ Q(14), Q(15), Q(16) /)
    dGhat(:,1) = gradQ(14,:)
    dGhat(:,2) = gradQ(15,:)
    dGhat(:,3) = gradQ(16,:)
    !
    XX = (/ Q(59), Q(60), Q(61) /) 
    dXX(:,1) = gradQ(59,:) 
    dXX(:,2) = gradQ(60,:) 
    dXX(:,3) = gradQ(61,:) 
    !
    b = Q(21:23) 
    !
    AA    = (/ Q(24), Q(25), Q(26) /) 
    dAA(:,1) = gradQ(24,:) 
    dAA(:,2) = gradQ(25,:) 
    dAA(:,3) = gradQ(26,:) 
    !
    traceK = Q(54) 
    dtraceK = gradQ(54,:) 
    !
    phi   =  EXP(MAX(-20.,MIN(20.,Q(55))))  

    PP    = Q(56:58) 
    dPP(:,1) = gradQ(56,:) 
    dPP(:,2) = gradQ(57,:) 
    dPP(:,3) = gradQ(58,:) 
    !
    beta = (/ Q(18), Q(19), Q(20) /) 
    BB(1,1) = Q(27) 
    BB(2,1) = Q(28) 
    BB(3,1) = Q(29) 
    BB(1,2) = Q(30) 
    BB(2,2) = Q(31) 
    BB(3,2) = Q(32) 
    BB(1,3) = Q(33) 
    BB(2,3) = Q(34) 
    BB(3,3) = Q(35) 
    !
    dBB(:,1,1) = gradQ(27,:) 
    dBB(:,2,1) = gradQ(28,:) 
    dBB(:,3,1) = gradQ(29,:) 
    dBB(:,1,2) = gradQ(30,:) 
    dBB(:,2,2) = gradQ(31,:) 
    dBB(:,3,2) = gradQ(32,:) 
    dBB(:,1,3) = gradQ(33,:) 
    dBB(:,2,3) = gradQ(34,:) 
    dBB(:,3,3) = gradQ(35,:) 
    !
    dBB = dBB*sk    
    ! 
    DD(1,1,1)=Q(36) 
    DD(1,1,2)=Q(37) 
    DD(1,1,3)=Q(38) 
    DD(1,2,1)=Q(37) 
    DD(1,2,2)=Q(39) 
    DD(1,2,3)=Q(40)
    DD(1,3,1)=Q(38) 
    DD(1,3,2)=Q(40) 
    DD(1,3,3)=Q(41)
    ! 
    DD(2,1,1)=Q(42) 
    DD(2,1,2)=Q(43) 
    DD(2,1,3)=Q(44) 
    DD(2,2,1)=Q(43) 
    DD(2,2,2)=Q(45) 
    DD(2,2,3)=Q(46)
    DD(2,3,1)=Q(44) 
    DD(2,3,2)=Q(46) 
    DD(2,3,3)=Q(47)
    !
    DD(3,1,1)=Q(48) 
    DD(3,1,2)=Q(49) 
    DD(3,1,3)=Q(50) 
    DD(3,2,1)=Q(49) 
    DD(3,2,2)=Q(51) 
    DD(3,2,3)=Q(52)
    DD(3,3,1)=Q(50) 
    DD(3,3,2)=Q(52) 
    DD(3,3,3)=Q(53)
    !
    dDD(:,1,1,1)=gradQ(36,:) 
    dDD(:,1,1,2)=gradQ(37,:) 
    dDD(:,1,1,3)=gradQ(38,:) 
    dDD(:,1,2,1)=gradQ(37,:) 
    dDD(:,1,2,2)=gradQ(39,:) 
    dDD(:,1,2,3)=gradQ(40,:)
    dDD(:,1,3,1)=gradQ(38,:) 
    dDD(:,1,3,2)=gradQ(40,:) 
    dDD(:,1,3,3)=gradQ(41,:)
    dDD(:,2,1,1)=gradQ(42,:) 
    dDD(:,2,1,2)=gradQ(43,:) 
    dDD(:,2,1,3)=gradQ(44,:) 
    dDD(:,2,2,1)=gradQ(43,:) 
    dDD(:,2,2,2)=gradQ(45,:) 
    dDD(:,2,2,3)=gradQ(46,:)
    dDD(:,2,3,1)=gradQ(44,:) 
    dDD(:,2,3,2)=gradQ(46,:) 
    dDD(:,2,3,3)=gradQ(47,:) 
    dDD(:,3,1,1)=gradQ(48,:) 
    dDD(:,3,1,2)=gradQ(49,:) 
    dDD(:,3,1,3)=gradQ(50,:) 
    dDD(:,3,2,1)=gradQ(49,:) 
    dDD(:,3,2,2)=gradQ(51,:) 
    dDD(:,3,2,3)=gradQ(52,:)
    dDD(:,3,3,1)=gradQ(50,:) 
    dDD(:,3,3,2)=gradQ(52,:) 
    dDD(:,3,3,3)=gradQ(53,:)
    !
    dgup = 0.0 
    DO k = 1, 3 
     DO m = 1, 3 
      DO l = 1, 3 
       DO n = 1, 3
        DO j = 1, 3 
           dgup(k,m,l) = dgup(k,m,l)-g_contr(m,n)*g_contr(j,l)*2*DD(k,n,j) 
        ENDDO
       ENDDO 
      ENDDO 
     ENDDO 
    ENDDO         
    !
    Kex  = Aex/phi**2 + 1./3.*traceK*g_cov/phi**2 
    Kmix = MATMUL( phi**2*g_contr, Kex  ) 
    Kup  = MATMUL( phi**2*g_contr, Kmix ) 
    !
    Christoffel_tilde = 0.0  
    Christoffel       = 0.0 
    Gtilde = 0.0 
    !
    DO j = 1, 3
     DO i = 1, 3
      DO k = 1, 3
       !Christoffel_kind1(i,j,k) =  DD(i,j,k)+DD(j,i,k)-DD(k,i,j)     ! this definition does not work ! 
       Christoffel_kind1(i,j,k) = DD(k,i,j)+DD(j,i,k)-DD(i,j,k)      ! this definition seems to work ! 
       DO l = 1, 3
          Christoffel_tilde(i,j,k) = Christoffel_tilde(i,j,k) + g_contr(k,l)*( DD(i,j,l)+DD(j,i,l)-DD(l,i,j) ) 
          Christoffel(i,j,k)       = Christoffel(i,j,k)       + g_contr(k,l)*( DD(i,j,l)+DD(j,i,l)-DD(l,i,j) ) -g_contr(k,l)*( g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l) ) 
          !Gtilde(i)                = Gtilde(i)+2*g_contr(i,j)*g_contr(k,l)*DD(l,j,k) 
        ENDDO 
      ENDDO
     ENDDO
    ENDDO
    !
    DO i = 1, 3
     DO j = 1, 3
      DO l = 1, 3
          Gtilde(i) = Gtilde(i) + g_contr(j,l)*Christoffel_tilde(j,l,i) 
      ENDDO
     ENDDO     
    ENDDO    
    !
    Z   = 0.5*MATMUL( g_cov, Ghat - Gtilde ) 
    Zup = MATMUL(phi**2*g_contr, Z) 
    !
    !dChristoffel    = 0.0 
    dChristoffelNCP = 0.0
    dChristoffelSrc = 0.0 
    dChristoffel_tildeNCP = 0.0
    dChristoffel_tildeSrc = 0.0 
    DO i = 1, 3 
     DO ip = 1, 3 
      DO m = 1, 3 
       DO k = 1, 3
          DO l = 1, 3 
            !dChristoffel(k,i,ip,m) = dChristoffel(k,i,ip,m) + g_contr(m,l)*(0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)))         & 
            !                                                - g_contr(m,l)*(g_cov(ip,l)*0.5*(dPP(k,i)+dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,ip)+dPP(ip,k))-g_cov(i,ip)*0.5*(dPP(k,l)+dPP(l,k)))  & 
            !                                                 +dgup(k,m,l)*(DD(i,ip,l)+DD(ip,i,l)-DD(l,i,ip)) - dgup(k,m,l)*(g_cov(ip,l)*PP(i)+g_cov(i,l)*PP(ip)-g_cov(i,ip)*PP(l)) - g_contr(m,l)*( 2*DD(k,ip,l)*PP(i)+2*DD(k,i,l)*PP(ip)-2*DD(k,i,ip)*PP(l) ) 
            ! 
            dChristoffelNCP(k,i,ip,m) = dChristoffelNCP(k,i,ip,m) + g_contr(m,l)*( 0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)) )         & 
                                                                  - g_contr(m,l)*( g_cov(ip,l)*0.5*(dPP(k,i)+dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,ip)+dPP(ip,k))-g_cov(i,ip)*0.5*(dPP(k,l)+dPP(l,k)) ) 
            !
            dChristoffel_tildeNCP(k,i,ip,m) = dChristoffel_tildeNCP(k,i,ip,m) + g_contr(m,l)*( 0.5*(dDD(k,i,ip,l)+dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)+dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)+dDD(l,k,i,ip)) ) 
            ! 
            !dChristoffelOrd(k,i,ip,m) = dChristoffelOrd(k,i,ip,m) + g_contr(m,l)*( 0.5*(dDD(k,i,ip,l)-dDD(i,k,ip,l))+0.5*(dDD(k,ip,i,l)-dDD(ip,k,i,l))-0.5*(dDD(k,l,i,ip)-dDD(l,k,i,ip)) )         & 
            !                                                      - g_contr(m,l)*( g_cov(ip,l)*0.5*(dPP(k,i)-dPP(i,k))+g_cov(i,l)*0.5*(dPP(k,ip)-dPP(ip,k))-g_cov(i,ip)*0.5*(dPP(k,l)-dPP(l,k)) )             
            !
            dChristoffelSrc(k,i,ip,m) = dChristoffelSrc(k,i,ip,m) + dgup(k,m,l)*(DD(i,ip,l)+DD(ip,i,l)-DD(l,i,ip)) - dgup(k,m,l)*(g_cov(ip,l)*PP(i)+g_cov(i,l)*PP(ip)-g_cov(i,ip)*PP(l)) - g_contr(m,l)*( 2*DD(k,ip,l)*PP(i)+2*DD(k,i,l)*PP(ip)-2*DD(k,i,ip)*PP(l) ) 
            !
            dChristoffel_tildeSrc(k,i,ip,m) = dChristoffel_tildeSrc(k,i,ip,m) + dgup(k,m,l)*(DD(i,ip,l)+DD(ip,i,l)-DD(l,i,ip)) 
            !             
          ENDDO 
       ENDDO
      ENDDO
     ENDDO
    ENDDO
    ! 
    !Riemann = 0.0 
    RiemannSrc = 0.0 
    RiemannNCP = 0.0 
    DO i = 1, 3 
     DO ip = 1, 3 
      DO m = 1, 3 
       DO k = 1, 3
          !Riemann(i,k,ip,m)    = dChristoffel(k,i,ip,m)-dChristoffel(ip,i,k,m)
          RiemannNCP(i,k,ip,m) = dChristoffelNCP(k,i,ip,m)-dChristoffelNCP(ip,i,k,m)
          RiemannSrc(i,k,ip,m) = dChristoffelSrc(k,i,ip,m)-dChristoffelSrc(ip,i,k,m) 
          DO j = 1, 3
           !Riemann(i,k,ip,m)    = Riemann(i,k,ip,m)    + Christoffel(i,ip,j)*Christoffel(j,k,m) - Christoffel(i,k,j)*Christoffel(j,ip,m) 
           RiemannSrc(i,k,ip,m) = RiemannSrc(i,k,ip,m) + Christoffel(i,ip,j)*Christoffel(j,k,m) - Christoffel(i,k,j)*Christoffel(j,ip,m) 
          ENDDO 
       ENDDO
      ENDDO
     ENDDO
    ENDDO    
    ! 
    !Ricci = 0.0 
    RicciNCP = 0.0 
    RicciSrc = 0.0 
    DO m = 1, 3 
     DO n = 1, 3
      DO l = 1, 3    
         !Ricci(m,n) = Ricci(m,n) + Riemann(m,l,n,l)  
         RicciNCP(m,n) = RicciNCP(m,n) + RiemannNCP(m,l,n,l)  
         RicciSrc(m,n) = RicciSrc(m,n) + RiemannSrc(m,l,n,l)  
      ENDDO
     ENDDO
    ENDDO    
    !
    RNCP = phi**2*SUM(g_contr*RicciNCP) 
    RSrc = phi**2*SUM(g_contr*RicciSrc) 
    !
    dGtildeNCP = 0.0
    dGtildeSrc = 0.0
    !DO k = 1, 3 
    ! DO j = 1, 3 
    !  DO l = 1, 3 
    !   DO m = 1, 3
    !    DO n = 1, 3 
    !     dGtildeNCP(j,k) = dGtildeNCP(j,k) + 2.0*g_contr(k,l)*g_contr(m,n)*0.5*(dDD(j,n,l,m)+dDD(n,j,l,m)) 
    !     dGtildeSrc(j,k) = dGtildeSrc(j,k) + 2.0*( g_contr(m,n)*DD(n,l,m)*dgup(j,k,l) + g_contr(k,l)*DD(n,l,m)*dgup(j,m,n) )         
    !    ENDDO
    !   ENDDO 
    !  ENDDO 
    ! ENDDO 
    !ENDDO 
    !    
    ! Here we directly compute the derivative of Gtilde from its original definition as contracted Christoffel symbol,
    ! without assuming unit determinant of the conformal metric. Back to the roots, and as few assumptions as possible... 
    !
    DO i = 1, 3 
     DO k = 1, 3
      DO j = 1, 3
       DO l = 1, 3
           dGtildeSrc(k,i) = dGtildeSrc(k,i) + dgup(k,j,l)*Christoffel_tilde(j,l,i) + g_contr(j,l)*dChristoffel_tildeSrc(k,j,l,i) 
           dGtildeNCP(k,i) = dGtildeNCP(k,i)                                        + g_contr(j,l)*dChristoffel_tildeNCP(k,j,l,i) 
       ENDDO
      ENDDO
     ENDDO
    ENDDO
    !
    ! If you want the original computation of the Ricci tensor according to the CCZ4 paper of Alic et al. 2012, use the following version. 
    ! By default, however, we compute the Ricci tensor ab definitionem from the Riemann tensor and the Christoffel symbols. 
    !
    DO j = 1, 3 
     DO i = 1, 3 
        RiccitildeNCP(i,j) = 0 
        DO l = 1, 3
         DO m = 1, 3
            RiccitildeNCP(i,j) = RiccitildeNCP(i,j)-g_contr(l,m)*0.5*(dDD(l,m,i,j)+dDD(m,l,i,j))
         ENDDO
        ENDDO 
        DO k = 1, 3 
            RiccitildeNCP(i,j) = RiccitildeNCP(i,j) + 0.5*(g_cov(k,i)*dGtildeNCP(j,k)+g_cov(k,j)*dGtildeNCP(i,k)) 
            !RiccitildeNCP(i,j) = RiccitildeNCP(i,j) + 0.5*(g_cov(k,i)*dGhat(j,k)+g_cov(k,j)*dGhat(i,k)) 
        ENDDO
     ENDDO
    ENDDO    
        
    DO j = 1, 3 
     DO i = 1, 3         
        RiccitildeSrc(i,j) = 0
        DO k = 1, 3 
          RiccitildeSrc(i,j) = RiccitildeSrc(i,j) + 0.5*(g_cov(k,i)*dGtildeSrc(j,k)+g_cov(k,j)*dGtildeSrc(i,k)) 
          RiccitildeSrc(i,j) = RiccitildeSrc(i,j) + Ghat(k)*0.5*(Christoffel_kind1(i,j,k)+Christoffel_kind1(j,i,k)) 
          DO l = 1, 3 
           DO m = 1, 3 
            RiccitildeSrc(i,j) = RiccitildeSrc(i,j)+g_contr(l,m)*(Christoffel_tilde(l,i,k)*Christoffel_kind1(j,k,m)+Christoffel_tilde(l,j,k)*Christoffel_kind1(i,k,m)+Christoffel_tilde(i,m,k)*Christoffel_kind1(k,j,l))
           ENDDO
          ENDDO
        ENDDO
     ENDDO
    ENDDO
    
    
    DO j = 1, 3 
     DO i = 1, 3 
       RicciphiNCP(i,j) = 0.5*(dPP(i,j)+dPP(j,i))
       DO k = 1, 3 
        DO l = 1, 3 
           RicciphiNCP(i,j) = RicciphiNCP(i,j)+g_cov(i,j)*g_contr(l,k)*0.5*(dPP(k,l)+dPP(l,k))
        ENDDO
       ENDDO 
     ENDDO
    ENDDO
    
    Pup = MATMUL(g_contr,PP) 
    DO m = 1, 3
        DDcontr(m) = SUM(g_contr*DD(:,m,:)) 
    ENDDO 
    
    DO i = 1, 3 
     DO j = 1, 3 
        RicciphiSrc(i,j) = PP(i)*PP(j) 
        DO k = 1, 3 
         RicciphiSrc(i,j) = RicciphiSrc(i,j) - Christoffel_tilde(i,j,k)*PP(k) - 2*g_cov(i,j)*DDcontr(k)*Pup(k)       
         DO l = 1, 3 
            RicciphiSrc(i,j) = RicciphiSrc(i,j) - g_cov(i,j)*g_contr(l,k)*PP(l)*PP(k) !-g_contr(k,l)*PP(k)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j))   
         ENDDO
        ENDDO
     ENDDO
    ENDDO    
    !
    dZNCP = 0.0 
    dZSrc = 0.0 
    DO i = 1, 3
     DO k = 1, 3    
      DO j = 1, 3 
        dZNCP(k,i) = dZNCP(k,i) + 0.5*g_cov(i,j)*( dGhat(k,j)-dGtildeNCP(k,j) )  
        dZSrc(k,i) = dZSrc(k,i) + DD(k,i,j)*(Ghat(j)-Gtilde(j)) + 0.5*g_cov(i,j)*( -dGtildeSrc(k,j) )
       ENDDO 
      ENDDO 
    ENDDO     
    !
    nablaZNCP = dZNCP 
    nablaXNCP = dXX  
    nablaZSrc = 0.0 
    nablaXSrc = 0.0 
    DO j = 1, 3 
     DO i = 1, 3 
      nablaZSrc(i,j) = dZSrc(i,j)
      DO k = 1, 3 
        nablaZSrc(i,j) = nablaZSrc(i,j) - Christoffel(i,j,k)*Z(k) 
        nablaXSrc(i,j) = nablaXSrc(i,j) - Christoffel(i,j,k)*XX(k) 
      ENDDO 
     ENDDO
    ENDDO    
    !
    RicciPlusNablaZNCP = RiccitildeNCP + RicciphiNCP + ( nablaZNCP + TRANSPOSE(nablaZNCP) ) + ( nablaXNCP + TRANSPOSE(nablaXNCP) ) 
    RicciPlusNablaZSrc = RiccitildeSrc + RicciphiSrc + ( nablaZSrc + TRANSPOSE(nablaZSrc) ) + ( nablaXSrc + TRANSPOSE(nablaXSrc) ) 
    !
    !RicciPlusNablaZNCP = RicciNCP + ( nablaZNCP + TRANSPOSE(nablaZNCP) ) 
    !RicciPlusNablaZSrc = RicciSrc + ( nablaZSrc + TRANSPOSE(nablaZSrc) ) 
    !
    RPlusTwoNablaZNCP = phi**2*SUM(g_contr*RicciPlusNablaZNCP) 
    RPlusTwoNablaZSrc = phi**2*SUM(g_contr*RicciPlusNablaZSrc) 
    !
    !Kupdown = SUM(Kex*Kup) 
    !temp = RPlusTwoNablaZNCP + RPlusTwoNablaZSrc - KupDown + traceK**2 
    !temp = SUM(phi**2*g_contr*Ricci) - KupDown + traceK**2 
    !
    nablaijalphaNCP = 0.0
    nablaijalphaSrc = 0.0
    DO j = 1, 3 
     DO i = 1, 3 
       nablaijalphaNCP(i,j) = alpha*0.5*( dAA(i,j)+dAA(j,i) ) 
       nablaijalphaSrc(i,j) = alpha*AA(j)*AA(i) 
       DO k = 1, 3 
         nablaijalphaSrc(i,j) = nablaijalphaSrc(i,j) - Christoffel(i,j,k)*alpha*AA(k)  
       ENDDO
     ENDDO
    ENDDO 
    nablanablaalphaNCP = phi**2*SUM( g_contr*nablaijalphaNCP ) 
    nablanablaalphaSrc = phi**2*SUM( g_contr*nablaijalphaSrc ) 
!    nablanablaalpha = 0.0
!    DO j = 1, 3 
!     DO i = 1, 3
!         nablanablaalpha = nablanablaalpha + phi**2*( g_contr(i,j)*AA(j)*AA(i)*alpha + alpha*g_contr(i,j)*dAA(i,j) - 2*g_contr(i,j)*SUM(g_contr(:,:)*TRANSPOSE(DD(:,j,:))*AA(i)*alpha)- g_contr(i,j)*PP(i)*AA(j)*alpha )  
!     ENDDO
!    ENDDO 
    !
    SecondOrderTermsNCP = -nablaijalphaNCP + alpha*RicciPlusNablaZNCP 
    SecondOrderTermsSrc = -nablaijalphaSrc + alpha*RicciPlusNablaZSrc 
    traceNCP = SUM( g_contr*SecondOrderTermsNCP ) 
    SecondOrderTermsNCP = SecondOrderTermsNCP - 1./3.*g_cov*traceNCP 
    traceSrc = SUM( g_contr*SecondOrderTermsSrc ) 
    SecondOrderTermsSrc = SecondOrderTermsSrc - 1./3.*g_cov*traceSrc 
    !
    ! Now assemble all this terrible stuff... 
    !
    dtgamma = - 2*alpha*Aex - itau*(det-1.0)*g_cov 
    DO j = 1, 3 
     DO i = 1, 3
      DO k = 1, 3
          dtgamma(i,j) = dtgamma(i,j) + g_cov(k,i)*BB(j,k) + g_cov(k,j)*BB(i,k) - 2./3.*g_cov(i,j)*BB(k,k) + beta(k)*2*DD(k,i,j) 
      ENDDO
     ENDDO
    ENDDO 
    !
    ! Main variables of the CCZ4 system 
    dtK = phi**2*SecondOrderTermsNCP + beta(1)*dAex(1,:,:) + beta(2)*dAex(2,:,:) + beta(3)*dAex(3,:,:)      ! extrinsic curvature
    dtK = dtK + phi**2*SecondOrderTermsSrc + alpha*Aex*(traceK-c*2*Theta) - 2*alpha*MATMUL(Aex,Amix) - itau*g_cov*traceA 
    DO j = 1, 3 
     DO i = 1, 3 
      DO k = 1, 3 
         dtK(i,j) = dtK(i,j) + Aex(k,i)*BB(j,k) + Aex(k,j)*BB(i,k) - 2./3.*Aex(i,j)*BB(k,k) 
      ENDDO
     ENDDO
    ENDDO 
    !
    dtTraceK = - nablanablaalphaNCP - nablanablaalphaSrc + alpha*( RPlusTwoNablaZNCP + RPlusTwoNablaZSrc + traceK**2 - c*2*Theta*traceK ) - 3*alpha*k1*Theta + SUM(beta(:)*dtraceK(:)) 
    !
    traceB = BB(1,1) + BB(2,2) + BB(3,3) 
    dtphi   = beta(1)*PP(1) + beta(2)*PP(2) + beta(3)*PP(3) + 1./3.*alpha*traceK - 1./3.*traceB 
    dtalpha = -alpha*fa*(traceK-K0-c*2*Theta) + beta(1)*AA(1) + beta(2)*AA(2) + beta(3)*AA(3) 

    Aupdown = SUM(Aex*Aup) 
    ! *** original 
    dtTheta = 0.5*alpha*e**2*(RplusTwoNablaZNCP + RplusTwoNablaZSrc) + beta(1)*dTheta(1) + beta(2)*dTheta(2) + beta(3)*dTheta(3)    &            ! temporal Z 
             + 0.5*alpha*e**2*( - Aupdown + 2./3.*traceK**2 ) - c*alpha*Theta*traceK - SUM(Zup*alpha*AA) - alpha*k2*Theta  
    ! *** use turbo cleaning here, i.e. without multiplying with alpha *** 
    !dtTheta = 0.5*e**2*( RplusTwoNablaZNCP + RplusTwoNablaZSrc ) + beta(1)*dTheta(1) + beta(2)*dTheta(2) + beta(3)*dTheta(3)    &                ! temporal Z 
    !         + 0.5*e**2*( - Aupdown + 2./3.*traceK**2 ) - c*alpha*Theta*traceK - SUM(Zup*alpha*AA) - k2*Theta 
    !
    !!divAupNCP = 0.0
    !!divAupSrc = 0.0 
    !!DO i = 1, 3
    !!    DO j = 1, 3
    !!     DO l = 1, 3
    !!      DO k = 1, 3    
    !!        divAupNCP(i) = divAupNCP(i) + g_contr(i,l)*g_contr(j,k)*dAex(j,l,k) 
    !!        divAupSrc(i) = divAupSrc(i) + ( dgup(j,i,l)*g_contr(j,k) + g_contr(i,l)*dgup(j,j,k) )*Aex(l,k) 
    !!      ENDDO
    !!     ENDDO
    !!    ENDDO        
    !!ENDDO 
    !!! 
    !!DO i = 1, 3 
    !!    Mom(i) = SUM(Christoffel_tilde(:,:,i)*Aup(:,:)) - 3.0*SUM(Aup(i,:)*PP(:)) - 2./3.*SUM(g_contr(i,:)*dtraceK(:)) + divAupNCP(i) + divAupSrc(i)  
    !!ENDDO 
    !
    dKex = 0.0
    DO j = 1, 3
     DO i = 1, 3
      DO k = 1, 3
          dKex(k,i,j) = 1.0/phi**2*( dAex(k,i,j) - 2.0*Aex(i,j)*PP(k) + 1./3.*dtraceK(k)*g_cov(i,j) + 2./3.*traceK*DD(k,i,j) - 2./3.*traceK*g_cov(i,j)*PP(k) ) 
      ENDDO
     ENDDO
    ENDDO 
    !
    Mom(:) = 0.0
    DO ii = 1, 3
        DO jj = 1, 3
            DO ll = 1, 3
                Mom(ii) = Mom(ii) + phi**2*g_contr(jj,ll)*(dKex(ll,ii,jj) - dKex(ii,jj,ll))
                DO mm = 1, 3
                    Mom(ii) = Mom(ii) + phi**2*g_contr(jj,ll)*( - Christoffel(jj,ll,mm)*Kex(mm,ii) + Christoffel(jj,ii,mm)*Kex(mm,ll)) 
                ENDDO
            ENDDO     
        ENDDO
    ENDDO   
    !
    dtX = alpha*ds**2*Mom + beta(1)*dXX(1,:) + beta(2)*dXX(2,:) + beta(3)*dXX(3,:) + MATMUL(BB,XX) 
    !
    DO i = 1, 3
        dtGhat(i) = 2*alpha*( SUM(Christoffel_tilde(:,:,i)*Aup(:,:)) - 3.0*SUM(Aup(i,:)*PP(:)) - 2./3.*SUM(g_contr(i,:)*dtraceK(:)) )    &
        !dtGhat(i)  =  2*alpha*( -divAupNCP(i) - divAupSrc(i) + ds**2*Mom(i) )                           & 
                    + 2*alpha*SUM( g_contr(:,i)*( dTheta(:) - Theta*AA(:) - 2./3.*traceK*Z(:)  ) )   & 
                    - 2*SUM( Aup(i,:)*alpha*AA(:) ) + 2./3.*Gtilde(i)*traceB - 2*alpha*k1*SUM(g_contr(i,:)*Z(:)) - SUM(Gtilde(:)*BB(:,i))   &
                   + beta(1)*dGhat(1,i) + beta(2)*dGhat(2,i) + beta(3)*dGhat(3,i) 
        DO l = 1, 3
         DO k = 1, 3
             dtGhat(i) = dtGhat(i) + g_contr(k,l)*0.5*(dBB(k,l,i)+dBB(l,k,i)) + 1./3*g_contr(i,k)*0.5*(dBB(k,l,l)+dBB(l,k,l)) + & 
                                     2*k3*( 2./3.*g_contr(i,l)*Z(l)*BB(k,k) - g_contr(l,k)*Z(l)*BB(k,i) ) 
         ENDDO
        ENDDO         
    ENDDO 
    DO k = 1, 3 
        ov(k) = + 2*alpha*( SUM(g_contr(:,:)*dAex(k,:,:)) + SUM(dgup(k,:,:)*Aex(:,:)) )   ! here we can use the constraint that trace A tilde = 0.         
    ENDDO        
    dtGhat = dtGhat + MATMUL(g_contr,ov)                                                  ! add the ordering constraint "up" (raised by g_contr) 
    !
    dtbb = xi*dtGhat - eta*b  !  <= be careful, this damping term -eta*b may be dangerous for the gamma driver, since it may kill the waves that you want !  
    !
    ! Add the following terms if you want shift convection in the PDE for b^i 
    dtbb = dtbb + bs*( - beta(1)*gradQ(14:16,1) - beta(2)*gradQ(14:16,2) - beta(3)*gradQ(14:16,3)  + beta(1)*gradQ(21:23,1) + beta(2)*gradQ(21:23,2) + beta(3)*gradQ(21:23,3)  ) !     
    dtbb = sk*dtbb 
    !
    QG = ggg*MATMUL( g_contr, AA - Z ) 
    DO j = 1, 3 
     DO i = 1, 3 
        DO n = 1, 3
         DO l = 1, 3 
           QG(i) = QG(i) + ggg*g_contr(i,j)*(-2*g_contr(n,l)*DD(l,n,j)) 
         ENDDO
        ENDDO
     ENDDO
    ENDDO 
    !            
    dtbeta  = + fff*b  
    ! Add the following term if you want to have shift convection in the PDE for beta^i 
    dtbeta = dtbeta + bs*( beta(1)*BB(1,:) + beta(2)*BB(2,:) + beta(3)*BB(3,:) )      
    dtbeta = sk*dtbeta 
    !
    ! Auxiliary variables 
    dtA = -alpha*fa*( dtraceK(:) -dK0(:) - c*2*dTheta(:) ) + beta(1)*dAA(1,:) + beta(2)*dAA(2,:) + beta(3)*dAA(3,:) - alpha*AA*(fa+alpha*faa)*(traceK-K0-c*2*Theta) + MATMUL(BB, AA) 
    DO k = 1, 3 
        dtA(k) = dtA(k) - alpha*fa*( SUM(g_contr(:,:)*dAex(k,:,:)) + SUM(dgup(k,:,:)*Aex(:,:)) )   ! here we can use the constraint that trace A tilde = 0. 
    ENDDO    
    ! 
    ! #xordB2# 
    ! for the ordering constraints, we have to check whether they should be multiplied by alpha**2, or not... 
    dtB = 0.0 
    DO i = 1, 3 
     DO k = 1, 3     
      DO j = 1, 3 
         !dtB(k,i) = +mu*alpha*g_contr(i,j)*( dZNCP(k,j) + dZSrc(k,j) ) + mu*dgup(k,i,j)*Z(j)      
         dtB(k,i) = dtB(k,i) + mu*alpha*g_contr(i,j)*( (dPP(k,j)-dPP(j,k)) )  
         DO n = 1, 3 
          DO l = 1, 3 
            !dtB(k,i) = dtB(k,i) + mu*alpha**2*g_contr(i,j)*( -2*g_contr(n,l)*0.5*(dDD(k,l,n,j)-dDD(l,k,n,j)) ) 
            dtB(k,i) = dtB(k,i) - mu*alpha*g_contr(i,j)*g_contr(n,l)*( dDD(k,l,j,n)-dDD(l,k,j,n) )   
          ENDDO 
         ENDDO 
      ENDDO 
      !
      ENDDO 
    ENDDO 
    dtB = dtB + bs*( beta(1)*dBB(1,:,:) + beta(2)*dBB(2,:,:) + beta(3)*dBB(3,:,:) ) 
    dtB = dtB*sk 
    !
    dtD = -alpha*dAex  
    DO i = 1, 3
      DO j = 1, 3
       DO k = 1, 3 
        DO m = 1, 3
            dtD(k,i,j) = dtD(k,i,j) + ( 0.5*(g_cov(m,i)*0.5*(dBB(k,j,m)+dBB(j,k,m))+g_cov(m,j)*0.5*(dBB(k,i,m)+dBB(i,k,m)) ) - 1./3.*g_cov(i,j)*0.5*(dBB(k,m,m)+dBB(m,k,m)) ) 
            DO n = 1, 3
                dtD(k,i,j) = dtD(k,i,j) + 1./3*alpha*g_cov(i,j)*g_contr(n,m)*dAex(k,n,m) + 1./3.*alpha*g_cov(i,j)*dgup(k,n,m)*Aex(n,m)   ! explicitly remove the trace of tilde A again 
            ENDDO 
        ENDDO        
       ENDDO
      ENDDO
    ENDDO 
    ! 
    DO j = 1, 3 
     DO i = 1, 3
      DO k = 1, 3 
        dtD(k,i,j) = dtD(k,i,j) - alpha*AA(k)*Aex(i,j) 
        DO l = 1, 3 
          dtD(k,i,j) = dtD(k,i,j) + BB(k,l)*DD(l,i,j) + DD(k,l,i)*BB(j,l) + DD(k,l,j)*BB(i,l) - 2./3.*DD(k,i,j)*BB(l,l) 
        ENDDO 
      ENDDO
     ENDDO
    ENDDO         
    !
    dtD = dtD + beta(1)*dDD(1,:,:,:) + beta(2)*dDD(2,:,:,:) + beta(3)*dDD(3,:,:,:)
    !
    dtP = MATMUL(BB,PP) + beta(1)*dPP(1,:) + beta(2)*dPP(2,:) + beta(3)*dPP(3,:)    
    DO k = 1, 3 
     dtP(k) = dtP(k) + 1./3.*alpha*dtraceK(k) + 1./3.*alpha*AA(k)*traceK + 1./3.*alpha*( SUM(g_contr(:,:)*dAex(k,:,:)) + SUM(dgup(k,:,:)*Aex(:,:)) )  
     DO i = 1, 3 
          dtP(k) = dtP(k) - 1./3.*0.5*(dBB(k,i,i)+dBB(i,k,i)) 
     ENDDO
    ENDDO 
    !
    BgradQ(1:6)    = (/ dtgamma(1,1), dtgamma(1,2), dtgamma(1,3), dtgamma(2,2), dtgamma(2,3), dtgamma(3,3) /)          ! \tilde \gamma_ij 
    BgradQ(7:12)   = (/ dtK(1,1), dtK(1,2), dtK(1,3), dtK(2,2), dtK(2,3), dtK(3,3) /)                                  ! \tilde A_ij 
    BgradQ(13)     = dtTheta                                                                                           ! Theta       
    BgradQ(14:16)  = dtGhat(1:3)                                                                                       ! \hat \Gamma^i           
    BgradQ(17)     = dtalpha                                                                                           ! log alpha 
    BgradQ(18:20)  = dtbeta                                                                                            ! beta^i 
    BgradQ(21:23)  = dtbb                                                                                              ! b^i 
    BgradQ(24:26)  = dtA(1:3)                                                                                          ! A_k       
    BgradQ(27:35)  = (/ dtB(1,1), dtB(2,1), dtB(3,1), dtB(1,2), dtB(2,2), dtB(3,2), dtB(1,3), dtB(2,3), dtB(3,3) /)    ! B_k^i 
    BgradQ(36:41)  = (/ dtD(1,1,1), dtD(1,1,2), dtD(1,1,3), dtD(1,2,2), dtD(1,2,3), dtD(1,3,3) /)                      ! D_kij 
    BgradQ(42:47)  = (/ dtD(2,1,1), dtD(2,1,2), dtD(2,1,3), dtD(2,2,2), dtD(2,2,3), dtD(2,3,3) /)                      ! D_kij 
    BgradQ(48:53)  = (/ dtD(3,1,1), dtD(3,1,2), dtD(3,1,3), dtD(3,2,2), dtD(3,2,3), dtD(3,3,3) /)                      ! D_kij 
    BgradQ(54)     = dtTraceK                                                                                          ! traceK 
    BgradQ(55)     = dtphi                                                                                             ! log phi 
    BgradQ(56:58)  = dtP                                                                                               ! P_k 
    BgradQ(59:61)  = dtX                                                                                               ! X_k 
    !
    Src_BgradQ = BgradQ    ! here, we do not have to change sign, since we work on the right hand side in the fused subroutine 
    !
    RETURN
    !
#endif 
    !
    ! Default setting. Call PDENCP and PDESource separately. 
    !
    CALL PDENCP(BgradQ,Q,gradQ) 
    CALL PDESource(src,Q,par,time)  
    src_bgradQ = src - BgradQ 
    ! 
    CONTINUE     
    !            
END SUBROUTINE PDEFusedSrcNCP 


RECURSIVE SUBROUTINE pderefinecriteria(refine_flag, max_luh,min_luh,x) 
  USE MainVariables, ONLY: nVar , nDim  
  IMPLICIT NONE  
	Integer, intent(out) :: refine_flag
	real, intent(in) :: max_luh(nVar),min_luh(nVar),x(nDim)
	
	!if(abs(x(1))<10) then
	!	refine_flag=2
	!	return
	!end if
	!if(abs(x(2))<15) then
	!	refine_flag=2
	!	return
	!end if
 	refine_flag = 0
!!#ifdef CCZ4EINSTEIN


!  if(abs(max_luh(60)-min_luh(60))>1.e-4 .or. abs(max_luh(54)-min_luh(54))>1.e-3) then
!	refine_flag=2
!  else
!	refine_flag=0
!  end if
  
  if(nDim .eq. 3) then
	if(sqrt(x(1)**2+x(2)**2+x(3)**2)<15.0) then
		refine_flag=2
	end if
	else
	        if(sqrt(x(1)**2+x(2)**2)<15.0) then
                refine_flag=2
        end if

  end if
!#endif
 
END SUBROUTINE pderefinecriteria
 
    
RECURSIVE SUBROUTINE maxHyperbolicEigenvalueGRHD(smax,QR,QL,nv)
    USE MainVariables, only : nDim,nVar
    !USE GRMHDmod
    IMPLICIT NONE 
    real, INTENT(OUT) :: smax
    real, INTENT(IN) :: QR(nVar),QL(nVar), nv(3)
    REAL :: LL(19),LR(19),QLGRMHD(19),QRGRMHD(19)
    REAL :: tmp1,tmp2,alpha,phi
    !
    QLGRMHD = 0.0
    QRGRMHD = 0.0
    smax = 0.
#ifdef CCZ4GRHD
    !
    alpha         = EXP(QL(17)) 
    phi           = EXP(QL(55)) 
    QLGRMHD(1:5)   = QL(60:64)        ! hydro variables 
    QLGRMHD(6:9)   = 0.0             ! EM variables 
    QLGRMHD(10)    = alpha           ! lapse 
    QLGRMHD(11:13) = QL(18:20)        ! shift 
    QLGRMHD(14:19) = QL(1:6)/phi**2   ! metric 
    !
    alpha         = EXP(QR(17)) 
    phi           = EXP(QR(55)) 
    QRGRMHD(1:5)   = QR(60:64)        ! hydro variables 
    QRGRMHD(6:9)   = 0.0             ! EM variables 
    QRGRMHD(10)    = alpha           ! lapse 
    QRGRMHD(11:13) = QR(18:20)        ! shift 
    QRGRMHD(14:19) = QR(1:6)/phi**2   ! metric 
    ! 
    CALL PDEEigenvaluesGRMHD(LL,QLGRMHD,nv) 
    CALL PDEEigenvaluesGRMHD(LR,QRGRMHD,nv)  
    tmp1=MAXVAL(LL)
    tmp2=MAXVAL(LR)
    !
    smax=MAX(tmp1,tmp2)
#endif 		
     
END SUBROUTINE maxHyperbolicEigenvalueGRHD


#endif