#ifndef HEADER_PDE
#define HEADER_PDE

!#include "MainVariables.f90"
!#include "SpecificVarEqn99.f90"
!#include "expintegrator_ode.f90"
!#include "expintegrator_type.f90"
!#include "SpecificVarEqn99.f90"

! GRMHD PDE.f90
! Trento (EQNTYPE4)

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
    Kup  = MATMUL( phi**2*g_contr, TRANSPOSE(Kmix)) 
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
    DO l = 1, 3
     DO j = 1, 3
      DO i = 1, 3
          Gtilde(i) = Gtilde(i) + g_contr(j,l)*Christoffel_tilde(j,l,i) 
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
#endif
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
    BgradQ(59)     = 0 
    !
    Src_BgradQ = BgradQ    ! here, we do not have to change sign, since we work on the right hand side in the fused subroutine 
    !
    !
    RETURN
    !
#endif 
    !            
END SUBROUTINE PDEFusedSrcNCP 

#endif
