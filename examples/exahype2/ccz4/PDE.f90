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
  USE MainVariables, ONLY: nVar,nDim,EQN,nParam,d
  IMPLICIT NONE
  ! 11. Oct 21:40: This was a matrix BGradQ(nVar,nDim) but is a vector in spaceTimePredictorNonlinear
  REAL(8),INTENT(OUT) :: BgradQ(nVar)
  REAL(8),INTENT(IN)  :: gradQin(nVar,3)
  REAL(8),INTENT(IN)  :: Q(nVar)
   
  REAL(8) :: gradQ(nVar,3)
  ! Argument list 
  REAL(8) :: par(nParam)  
  ! Local variables 
  INTEGER :: i,j,k,l,m,n
  REAL(8) :: k1,k2,k3,fff,ggg,e,c,mu,ds,xi,sk,bs,eta,itau
  REAL(8) :: fa,faa
  ! Q and their gradients
  REAL(8) :: g_cov(3,3),g_contr(3,3),det                                        ! Q(1-6)    conformal metric
  REAL(8) :: Aex(3,3),dAex(3,3,3),Amix(3,3),Aup(3,3),traceA                     ! Q(7-12)   conformal traceless extrinic curvature
  REAL(8) :: Theta,dTheta(3)                                                    ! Q(13)     time component of Z
  REAL(8) :: Ghat(3),Gtilde(3),dGhat(3,3)                                       ! Q(14-16)  Gamma hat
  REAL(8) :: alpha                                                              ! Q(17)     lapse, exp(Q17)
  REAL(8) :: beta(3)                                                            ! Q(18-20)  shift
  REAL(8) :: b(3)                                                               ! Q(21-23)  gamma driver
  REAL(8) :: AA(3),dAA(3,3)                                                     ! Q(24-26)  auxiliary variables for lapse
  REAL(8) :: BB(3,3),dBB(3,3,3),traceB                                          ! Q(27-35)  auxiliary variables for beta
  REAL(8) :: DD(3,3,3),dDD(3,3,3,3),dgup(3,3,3)                                 ! Q(36-53)  auxiliary variables for metric
  REAL(8) :: traceK,dtraceK(3)                                                  ! Q(54)     trace of extrinic curvature
  REAL(8) :: phi                                                                ! Q(55)     conformal factor, exp(Q55)
  REAL(8) :: PP(3),dPP(3,3)                                                     ! Q(56-58)  auxiliary variables for phi
  REAL(8) :: K0,dK0(3)                                                          ! Q(59)     initial trace of K
  ! time derivatives of Q 
  REAL(8) :: dtgamma(3,3)                                                       ! Q(1-6)
  REAL(8) :: dtK(3,3)                                                           ! Q(7-12)
  REAL(8) :: dtTheta                                                            ! Q(13)
  REAL(8) :: dtGhat(3)                                                          ! Q(14-16)
  REAL(8) :: dtalpha                                                            ! Q(17)
  REAL(8) :: dtbeta(3)                                                          ! Q(18-20)
  REAL(8) :: dtbb(3)                                                            ! Q(21-23)
  REAL(8) :: dtA(3)                                                             ! Q(24-26)
  REAL(8) :: dtB(3,3)                                                           ! Q(27-35)
  REAL(8) :: dtD(3,3,3)                                                         ! Q(36-53)
  REAL(8) :: dtTraceK                                                           ! Q(54)
  REAL(8) :: dtphi                                                              ! Q(55)
  REAL(8) :: dtP(3)                                                             ! Q(56-58)
  !
  ! intermediate quantities
  REAL(8) :: Kex(3,3),Kmix(3,3),Kup(3,3)
  REAL(8) :: Z(3),Zup(3),dZNCP(3,3),nablaZNCP(3,3)
  REAL(8) :: Christoffel(3,3,3),Christoffel_tilde(3,3,3)
  REAL(8) :: dChristoffel(3,3,3,3),dChristoffelNCP(3,3,3,3),dChristoffel_tildeNCP(3,3,3,3)
  REAL(8) :: RiemannNCP(3,3,3,3),RicciNCP(3,3),RNCP
  REAL(8) :: RicciPlusNablaZNCP(3,3),RPlusTwoNablaZNCP
  REAL(8) :: nablaijalphaNCP(3,3),nablanablaalpha,nablanablaalphaNCP
  REAL(8) :: dGtildeNCP(3,3)
  REAL(8) :: SecondOrderTermsNCP(3,3),traceNCP
  REAL(8) :: ov(3)
  !
  BgradQ = 0.0D0
  !
#if defined(CCZ4EINSTEIN) || defined(CCZ4GRMHD) || defined(CCZ4GRHD) || defined(CCZ4GRGPR) 
  !
  IF(nDim.EQ.2) THEN
     gradQ(:,1:2) = gradQin(:,1:2)
     gradQ(:,  3) = 0.0D0
  ELSE
     gradQ = gradQin
  ENDIF 
  !
  k1   = EQN%CCZ4k1                                             ! kappa_1 
  k2   = EQN%CCZ4k2                                             ! kappa_2
  k3   = EQN%CCZ4k3                                             ! kappa_3
  fff  = EQN%CCZ4f                                              ! multiplied to \partial_k(b^i) in the evolution eqn for BB
  ggg  = EQN%CCZ4g                                              !
  e    = EQN%CCZ4e                                              ! e
  itau = EQN%CCZ4itau                                           ! tau^-1
  eta  = EQN%CCZ4eta                                            ! eta
  c    = EQN%CCZ4c                                              ! c
  mu   = EQN%CCZ4mu                                             ! mu
  ds   = EQN%CCZ4ds                                             ! s multiplied to Z_i and \nabla_i(Z_j)
  bs   = EQN%CCZ4bs                                             ! only used in dtbb and dtB
  xi   = EQN%CCZ4xi                                             ! only used in dtbb 
  sk   = EQN%CCZ4sk                                             ! s multiplied to \partial_i(B_j^k) and other places
  !
  ! These are the tilde quantities, so be careful!    
  g_cov(1,1) = Q(1)
  g_cov(1,2) = Q(2)
  g_cov(1,3) = Q(3)
  g_cov(2,1) = Q(2)
  g_cov(2,2) = Q(4)
  g_cov(2,3) = Q(5)
  g_cov(3,1) = Q(3)
  g_cov(3,2) = Q(5)
  g_cov(3,3) = Q(6)
  !
  ! This determinant should be close to unity, since we use the conformal decomposition 
  det = (Q(1)*Q(4)*Q(6)-Q(1)*Q(5)**2-Q(2)**2*Q(6)+2*Q(2)*Q(3)*Q(5)-Q(3)**2*Q(4)) 
  !  
  g_contr(1,1) =  ( g_cov(2,2)*g_cov(3,3)-g_cov(2,3)*g_cov(3,2))/det 
  g_contr(1,2) = -( g_cov(1,2)*g_cov(3,3)-g_cov(1,3)*g_cov(3,2))/det
  g_contr(1,3) = -(-g_cov(1,2)*g_cov(2,3)+g_cov(1,3)*g_cov(2,2))/det 
  g_contr(2,1) = -( g_cov(2,1)*g_cov(3,3)-g_cov(2,3)*g_cov(3,1))/det 
  g_contr(2,2) =  ( g_cov(1,1)*g_cov(3,3)-g_cov(1,3)*g_cov(3,1))/det 
  g_contr(2,3) = -( g_cov(1,1)*g_cov(2,3)-g_cov(1,3)*g_cov(2,1))/det 
  g_contr(3,1) = -(-g_cov(2,1)*g_cov(3,2)+g_cov(2,2)*g_cov(3,1))/det 
  g_contr(3,2) = -( g_cov(1,1)*g_cov(3,2)-g_cov(1,2)*g_cov(3,1))/det 
  g_contr(3,3) =  ( g_cov(1,1)*g_cov(2,2)-g_cov(1,2)*g_cov(2,1))/det 
  !   
  alpha = DEXP(DMAX1(-20.0D0,DMIN1(20.0D0,Q(17))))  
  !
  SELECT CASE(EQN%CCZ4LapseType) 
     CASE(0)  ! harmonic 
        fa  = 1.0D0 
        faa = 0.0D0 
     CASE DEFAULT  ! 1 + log 
        fa  = 2.0D0/alpha
        faa = -2.0D0/alpha**2   
  END SELECT 
  ! 
  K0  = Q(59)
  dK0 = 0.0D0 ! sk*gradQ(59,:) 
  !  
  Aex(1,1) = Q(7 ) 
  Aex(1,2) = Q(8 ) 
  Aex(1,3) = Q(9 ) 
  Aex(2,1) = Q(8 ) 
  Aex(2,2) = Q(10) 
  Aex(2,3) = Q(11) 
  Aex(3,1) = Q(9 ) 
  Aex(3,2) = Q(11) 
  Aex(3,3) = Q(12) 
  !
  traceA = SUM(g_contr*Aex) 
  Aex = Aex-1.0D0/3.0D0*g_cov*traceA 
  !
  dAex(:,1,1) = gradQ(7 ,:) 
  dAex(:,1,2) = gradQ(8 ,:) 
  dAex(:,1,3) = gradQ(9 ,:) 
  dAex(:,2,1) = gradQ(8 ,:) 
  dAex(:,2,2) = gradQ(10,:) 
  dAex(:,2,3) = gradQ(11,:) 
  dAex(:,3,1) = gradQ(9 ,:) 
  dAex(:,3,2) = gradQ(11,:) 
  dAex(:,3,3) = gradQ(12,:) 
  !
  Amix = MATMUL(g_contr,          Aex  )
  Aup  = MATMUL(g_contr,TRANSPOSE(Amix)) 
  !
  Theta  = Q(13)
  dTheta = gradQ(13,:) 
  ! 
  Ghat = (/Q(14),Q(15),Q(16)/)
  dGhat(:,1) = gradQ(14,:)
  dGhat(:,2) = gradQ(15,:)
  dGhat(:,3) = gradQ(16,:)
  !
  b = Q(21:23) 
  !
  AA = (/Q(24),Q(25),Q(26)/) 
  dAA(:,1) = gradQ(24,:) 
  dAA(:,2) = gradQ(25,:) 
  dAA(:,3) = gradQ(26,:) 
  !
  traceK  = Q(54) 
  dtraceK = gradQ(54,:) 
  !
  phi = DEXP(DMAX1(-20.0D0,DMIN1(20.0D0,Q(55))))   

  PP  = Q(56:58) 
  dPP(:,1) = gradQ(56,:) 
  dPP(:,2) = gradQ(57,:) 
  dPP(:,3) = gradQ(58,:) 
  !
  beta = (/Q(18),Q(19),Q(20)/) 
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
  ! note that dBB is already multiplied to s here
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
  dgup = 0.0D0 
  DO k=1,3 
     DO m=1,3 
        DO l=1,3 
           DO n=1,3
              DO j=1,3 
                 dgup(k,m,l) = dgup(k,m,l)-2.0D0*g_contr(m,n)*g_contr(j,l)*DD(k,n,j) 
              ENDDO
           ENDDO 
        ENDDO 
     ENDDO 
  ENDDO         
  !
  Kex  = Aex/phi**2+1.0D0/3.0D0*traceK*g_cov/phi**2 
  Kmix = MATMUL(phi**2*g_contr,          Kex  ) 
  Kup  = MATMUL(phi**2*g_contr,TRANSPOSE(Kmix)) 
  !
  Christoffel_tilde = 0.0D0  
  Christoffel       = 0.0D0 
  Gtilde            = 0.0D0 
  !
  DO j=1,3
     DO i=1,3
        DO k=1,3
           DO l=1,3
              Christoffel_tilde(i,j,k) = Christoffel_tilde(i,j,k)+g_contr(k,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j)) 
              Christoffel(i,j,k)       = Christoffel      (i,j,k)+g_contr(k,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j)) &
                                       &                         -g_contr(k,l)*(g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l)) 
           ENDDO 
        ENDDO
     ENDDO
  ENDDO
  DO l=1,3
     DO j=1,3
        DO i=1,3
           Gtilde(i) = Gtilde(i)+g_contr(j,l)*Christoffel_tilde(j,l,i) 
        ENDDO
     ENDDO     
  ENDDO   
  !
! Z   = 0.5D0*MATMUL(g_cov,Ghat-Gtilde)    ! Baojiu 
  Z   = 0.5D0*ds*MATMUL(g_cov,Ghat-Gtilde) ! Baojiu 
  Zup = MATMUL(phi**2*g_contr,Z) 
  !
  DO i=1,3 
     DO j=1,3 
        DO m=1,3 
           DO k=1,3
              dChristoffelNCP      (k,i,j,m) = 0.0D0 
              dChristoffel_tildeNCP(k,i,j,m) = 0.0D0
              DO l=1,3 
                 ! 
                 dChristoffelNCP(k,i,j,m)       = dChristoffelNCP(k,i,j,m)       + 0.5D0*g_contr(m,l)*((dDD(k,i,j,l)+dDD(i,k,j,l))+    &
                                                &                                                      (dDD(k,j,i,l)+dDD(j,k,i,l))-    &
                                                &                                                      (dDD(k,l,i,j)+dDD(l,k,i,j)))    & 
                                                &                                - 0.5D0*g_contr(m,l)*(g_cov(j,l)*(dPP(k,i)+dPP(i,k))+ &
                                                &                                                      g_cov(i,l)*(dPP(k,j)+dPP(j,k))- &
                                                &                                                      g_cov(i,j)*(dPP(k,l)+dPP(l,k))) 
                 ! 
                 dChristoffel_tildeNCP(k,i,j,m) = dChristoffel_tildeNCP(k,i,j,m) + 0.5D0*g_contr(m,l)*((dDD(k,i,j,l)+dDD(i,k,j,l))+    &
                                                &                                                      (dDD(k,j,i,l)+dDD(j,k,i,l))-    &
                                                &                                                      (dDD(k,l,i,j)+dDD(l,k,i,j)))           
              ENDDO 
           ENDDO
        ENDDO
     ENDDO
  ENDDO
  ! 
  RiemannNCP = 0.0D0 
  DO i=1,3 
     DO j=1,3 
        DO m=1,3 
           DO k=1,3
              RiemannNCP(i,k,j,m) = dChristoffelNCP(k,i,j,m)-dChristoffelNCP(j,i,k,m)
           ENDDO 
        ENDDO
     ENDDO
  ENDDO   
  ! 
  RicciNCP = 0.0D0
  DO m=1,3 
     DO n=1,3
        DO l=1,3    
           RicciNCP(m,n) = RicciNCP(m,n)+RiemannNCP(m,l,n,l)  
        ENDDO
     ENDDO
  ENDDO    
  !
  RNCP = phi**2*SUM(g_contr*RicciNCP) 
  !
  dGtildeNCP = 0.0D0
  !
  ! Here we directly compute the derivative of Gtilde from its original definition as contracted Christoffel symbol,
  ! without assuming unit determinant of the conformal metric. Back to the roots, and as few assumptions as possible...      
  DO i=1,3 
     DO k=1,3
        DO j=1,3
           DO l=1,3
              dGtildeNCP(k,i) = dGtildeNCP(k,i)+g_contr(j,l)*dChristoffel_tildeNCP(k,j,l,i) 
           ENDDO
        ENDDO
     ENDDO
  ENDDO    
  !
  dZNCP = 0.0D0
  DO j=1,3 
     DO i=1,3
        DO k=1,3    
           dZNCP(k,i) = dZNCP(k,i)+0.5D0*ds*g_cov(i,j)*(dGhat(k,j)-dGtildeNCP(k,j))  
        ENDDO 
     ENDDO 
  ENDDO     
  !
  DO j=1,3 
     DO i=1,3 
        nablaZNCP(i,j) = dZNCP(i,j)
     ENDDO
  ENDDO    
  !
  RicciPlusNablaZNCP = RicciNCP+(nablaZNCP+TRANSPOSE(nablaZNCP)) 
  !
  RPlusTwoNablaZNCP  = phi**2*SUM(g_contr*RicciPlusNablaZNCP) 
  !
  nablaijalphaNCP = 0.0D0
  DO j=1,3 
     DO i=1,3 
        nablaijalphaNCP(i,j) = 0.5D0*alpha*(dAA(i,j)+dAA(j,i)) 
     ENDDO
  ENDDO
  ! 
  nablanablaalphaNCP = phi**2*SUM(g_contr*nablaijalphaNCP) 
  !
  SecondOrderTermsNCP = -nablaijalphaNCP+alpha*RicciPlusNablaZNCP 
  !
  traceNCP = SUM(g_contr*SecondOrderTermsNCP) 
  !
  SecondOrderTermsNCP = SecondOrderTermsNCP-1.0D0/3.0D0*g_cov*traceNCP 
  !
  !
  !
  ! Now assemble all this terrible stuff... 
  !
  dtgamma = 0.0D0 
  !
  ! Main variables of the CCZ4 system 
  dtK = phi**2*SecondOrderTermsNCP+beta(1)*dAex(1,:,:)+beta(2)*dAex(2,:,:)+beta(3)*dAex(3,:,:)      ! extrinsic curvature
  !
  dtTraceK = -nablanablaalphaNCP+alpha*RPlusTwoNablaZNCP+SUM(beta(:)*dtraceK(:)) 
  !
  traceB = BB(1,1)+BB(2,2)+BB(3,3) 
  dtphi   = 0.0D0 
  dtalpha = 0.0D0
  !
  dtTheta = 0.5D0*alpha*e**2*RplusTwoNablaZNCP+beta(1)*dTheta(1)+beta(2)*dTheta(2)+beta(3)*dTheta(3)        ! *** original cleaning *** 
  !
  DO i=1,3
     dtGhat(i) = -4.0D0/3.0D0*alpha*SUM(g_contr(i,:)*dtraceK(:))+ &  
               &  2.0D0      *alpha*SUM(g_contr(:,i)*dTheta (:))+ &                    
               &  beta(1)*dGhat(1,i)+beta(2)*dGhat(2,i)+beta(3)*dGhat(3,i) 
     DO l=1,3
        DO k=1,3
           dtGhat(i) = dtGhat(i)+0.5D0*g_contr(k,l)*(dBB(k,l,i)+dBB(l,k,i)) + &
                     &           0.5D0*g_contr(i,k)*(dBB(k,l,l)+dBB(l,k,l))/3.0D0 
        ENDDO
     ENDDO         
  ENDDO 
  ! 
  DO k=1,3 
     ov(k) = 2.0D0*alpha*(SUM(g_contr(:,:)*dAex(k,:,:)))        ! here we can use the constraint that trace A tilde = 0. 
  ENDDO
  dtGhat = dtGhat+sk*MATMUL(g_contr,ov)                         ! Ghat is an "up" vector, so we need to multiply with g_contr 
  !
  dtbb = xi*dtGhat+bs*(beta(1)*gradQ(21:23,1)+beta(2)*gradQ(21:23,2)+beta(3)*gradQ(21:23,3)- &
       &               beta(1)*gradQ(14:16,1)-beta(2)*gradQ(14:16,2)-beta(3)*gradQ(14:16,3)) 
  dtbb = sk*dtbb  
  !
  dtbeta = 0.0D0    
  !
  ! Auxiliary variables 
  dtA = -alpha*fa*(dtraceK(:)-dK0(:)-2.0D0*c*dTheta(:))+ &
      &  beta(1)*dAA(1,:)+beta(2)*dAA(2,:)+beta(3)*dAA(3,:)  
  DO k=1,3 
     dtA(k) = dtA(k)-sk*alpha*fa*(SUM(g_contr(:,:)*dAex(k,:,:)))   ! here we can use the constraint that trace A tilde = 0. 
  ENDDO
  !IF ((dtA(2)/=0) .or. (dtA(3)/=0)) THEN 
  !   write(*,*) "fortran dtA[1]=", dtA(2), "dtA[2]=", dtA(3)
  !   write(*,*) "fortran dtheta[1]=", dtheta(2), "dtheta[2]=", dtheta(3)
  !   write(*,*) "fortran dtraceK[1]= ", dtraceK(2), "dtraceK[2]= ", dtraceK(3)
  !ENDIF
  !
  ! We have removed the conservative fluxes for CCZ4, so put all the stuff into the NCP and FusedNCP 
  dtB(:,1) = fff*gradQ(21,:)  
  dtB(:,2) = fff*gradQ(22,:)  
  dtB(:,3) = fff*gradQ(23,:)  
  ! #ordB1#     
  ! for the ordering constraints, we have to check whether they should be multiplied by alpha**2, or not... 
  DO i=1,3 
     DO k=1,3 
        DO j=1,3 
           dtB(k,i) = dtB(k,i)+mu*alpha**2*g_contr(i,j)*(dPP(k,j)-dPP(j,k))  
           DO n=1,3 
              DO l=1,3 
                 dtB(k,i) = dtB(k,i)-mu*alpha**2*g_contr(i,j)*g_contr(n,l)*(dDD(k,l,j,n)-dDD(l,k,j,n))   
              ENDDO 
           ENDDO
        ENDDO 
     ENDDO 
  ENDDO 
! dtB = dtB+bs*(beta(1)*dBB(1,:,:)+beta(2)*dBB(2,:,:)+beta(3)*dBB(3,:,:))  ! Baojiu
  dtB = dtB*sk                                                             ! Baojiu
  dtB = dtB+bs*(beta(1)*dBB(1,:,:)+beta(2)*dBB(2,:,:)+beta(3)*dBB(3,:,:))  ! Baojiu
  !
  dtD = -alpha*dAex  
  DO i=1,3
     DO j=1,3
        DO k=1,3 
           DO m=1,3
              dtD(k,i,j) = dtD(k,i,j)+(0.5D0*(g_cov(m,i)*0.5D0*(dBB(k,j,m)+dBB(j,k,m))  + &
                         &                    g_cov(m,j)*0.5D0*(dBB(k,i,m)+dBB(i,k,m))) - &
                         &        1.0D0/3.0D0*g_cov(i,j)*0.5D0*(dBB(k,m,m)+dBB(m,k,m))  ) 
              DO n=1,3
                 dtD(k,i,j) = dtD(k,i,j)+1.0D0/3.0D0*alpha*g_cov(i,j)*g_contr(n,m)*dAex(k,n,m)     ! explicitly remove the trace of tilde A again 
              ENDDO 
           ENDDO        
        ENDDO
     ENDDO
  ENDDO 
  dtD = dtD+beta(1)*dDD(1,:,:,:)+beta(2)*dDD(2,:,:,:)+beta(3)*dDD(3,:,:,:)
  !
  dtP = beta(1)*dPP(1,:)+beta(2)*dPP(2,:)+beta(3)*dPP(3,:)    
  DO k=1,3 
     dtP(k) = dtP(k)+1.0D0/3.0D0*alpha*dtraceK(k)+sk*1.0D0/3.0D0*alpha*SUM(g_contr(:,:)*dAex(k,:,:))  ! use the fact that trace A tilde = 0 
     DO i=1,3 
        dtP(k) = dtP(k)-1.0D0/3.0D0*0.5D0*(dBB(k,i,i)+dBB(i,k,i))  
     ENDDO
  ENDDO 
  !
  BgradQ(1:6)   = (/dtgamma(1,1),dtgamma(1,2),dtgamma(1,3),dtgamma(2,2),dtgamma(2,3),dtgamma(3,3)/)       ! \tilde \gamma_ij 
  BgradQ(7:12)  = (/dtK(1,1),dtK(1,2),dtK(1,3),dtK(2,2),dtK(2,3),dtK(3,3)/)                               ! \tilde A_ij 
  BgradQ(13)    = dtTheta                                                                                 ! Theta       
  BgradQ(14:16) = dtGhat(1:3)                                                                             ! \hat \Gamma^i           
  BgradQ(17)    = dtalpha                                                                                 ! log alpha 
  BgradQ(18:20) = dtbeta                                                                                  ! beta^i 
  BgradQ(21:23) = dtbb                                                                                    ! b^i 
  BgradQ(24:26) = dtA(1:3)                                                                                ! A_k       
  BgradQ(27:35) = (/dtB(1,1),dtB(2,1),dtB(3,1),dtB(1,2),dtB(2,2),dtB(3,2),dtB(1,3),dtB(2,3),dtB(3,3)/)    ! B_k^i 
  BgradQ(36:41) = (/dtD(1,1,1),dtD(1,1,2),dtD(1,1,3),dtD(1,2,2),dtD(1,2,3),dtD(1,3,3)/)                   ! D_kij 
  BgradQ(42:47) = (/dtD(2,1,1),dtD(2,1,2),dtD(2,1,3),dtD(2,2,2),dtD(2,2,3),dtD(2,3,3)/)                   ! D_kij 
  BgradQ(48:53) = (/dtD(3,1,1),dtD(3,1,2),dtD(3,1,3),dtD(3,2,2),dtD(3,2,3),dtD(3,3,3)/)                   ! D_kij 
  BgradQ(54)    = dtTraceK                                                                                ! traceK 
  BgradQ(55)    = dtphi                                                                                   ! log phi 
  BgradQ(56:58) = dtP                                                                                     ! P_k 
  BgradQ(59:nVar) = 0.0D0
  !
  BgradQ = -BgradQ ! change sign, since we work on the left hand side in PDENCP 
  !
#endif

END SUBROUTINE PDENCP


RECURSIVE SUBROUTINE PDEEigenvalues(Lambda,Q,nv)
  USE MainVariables, ONLY: nVar,nDim,EQN
  USE iso_c_binding
  IMPLICIT NONE
  REAL(8) :: Lambda(nVar),nv(3),Q(nVar),Vp(nVar)
  INTENT(IN)  :: Q,nv
  INTENT(OUT) :: Lambda 
  ! Local Variables 
  REAL(8) :: lam,mu,irho,VPR(3),cs,c0,uu,alpha

#if defined(CCZ4EINSTEIN)  
  alpha = DMAX1(1.0D0,DEXP(Q(17)))*DMAX1(1.0D0,DEXP(Q(55)))/DMIN1(DSQRT(Q(1)),DSQRT(Q(4)),DSQRT(Q(6)))     
#else
  alpha = 1.0D0
#endif

  Lambda = 0.0D0
  Lambda(1) = -alpha*DMAX1(DSQRT(2.0D0),EQN%CCZ4e,EQN%CCZ4ds,EQN%CCZ4GLMc/alpha,EQN%CCZ4GLMd/alpha)-DOT_PRODUCT(Q(18:20),nv(:))   ! MAX( SQRT(2.0), EQN%CCZ4e, EQN%CCZ4ds ) + SQRT(SUM(Q(18:20)**2)) 
  Lambda(2) =  alpha*DMAX1(DSQRT(2.0D0),EQN%CCZ4e,EQN%CCZ4ds,EQN%CCZ4GLMc/alpha,EQN%CCZ4GLMd/alpha)-DOT_PRODUCT(Q(18:20),nv(:))   ! MAX( SQRT(2.0), EQN%CCZ4e, EQN%CCZ4ds ) + SQRT(SUM(Q(18:20)**2)) 
  Lambda(3) =  Q(1)
  Lambda(4) =  Q(14)
END SUBROUTINE PDEEigenvalues

RECURSIVE SUBROUTINE PDESource(S,Q) 
  USE MainVariables, ONLY:  nVar,nDim,EQN
  USE iso_c_binding
  IMPLICIT NONE
  ! --------------------------------------------
  ! Argument list declaration
  REAL(8),INTENT(IN)  :: Q(nvar) 
  REAL(8),INTENT(OUT) :: S(nvar)
  ! --------------------------------------------
  ! Local variables
  INTEGER :: i,j,k,l,m,n
  REAL(8) :: mu
  REAL(8) :: src(nVar)
  REAL(8) :: k1,k2,k3,fff,ggg,e,c,ds,xi,sk,sknl,bs,dgup(3,3,3),eta,itau
  REAL(8) :: fa,faa
  ! Q and their gradients
  REAL(8) :: g_cov(3,3),g_contr(3,3),det                                ! Q(1-6)
  REAL(8) :: Aex(3,3),Amix(3,3),Aup(3,3),Aupdown,traceA                 ! Q(7-12)
  REAL(8) :: Theta                                                      ! Q(13)
  REAL(8) :: Ghat(3)                                                    ! Q(14-16)
  REAL(8) :: alpha                                                      ! Q(17)
  REAL(8) :: beta(3)                                                    ! Q(18-20)
  REAL(8) :: b(3)                                                       ! Q(21-23)
  REAL(8) :: AA(3)                                                      ! Q(24-26)
  REAL(8) :: BB(3,3),traceB                                             ! Q(27-35)
  REAL(8) :: DD(3,3,3)                                                  ! Q(36-53)
  REAL(8) :: traceK                                                     ! Q(54)
  REAL(8) :: phi                                                        ! Q(55)
  REAL(8) :: PP(3)                                                      ! Q(56-58)
  REAL(8) :: K0                                                         ! Q(59)
  ! time derivatives of Q
  REAL(8) :: dtgamma(3,3)                                               ! Q(1-7)
  REAL(8) :: dtK(3,3)                                                   ! Q(7-12)
  REAL(8) :: dtTheta                                                    ! Q(13)
  REAL(8) :: dtGhat(3)                                                  ! Q(14-16)
  REAL(8) :: dtalpha                                                    ! Q(17)
  REAL(8) :: dtbeta(3)                                                  ! Q(18-20)
  REAL(8) :: dtbb(3)                                                    ! Q(21-23)
  REAL(8) :: dtA(3)                                                     ! Q(24-26)
  REAL(8) :: dtB(3,3)                                                   ! Q(27-35)
  REAL(8) :: dtD(3,3,3)                                                 ! Q(36-53)
  REAL(8) :: dtTraceK                                                   ! Q(54)
  REAL(8) :: dtphi                                                      ! Q(55)
  REAL(8) :: dtP(3)                                                     ! Q(56-58)
  ! intermediate quantities
  REAL(8) :: Gtilde(3)
  REAL(8) :: Kex(3,3),Kmix(3,3),Kup(3,3)
  REAL(8) :: Christoffel(3,3,3),Christoffel_tilde(3,3,3)
  REAL(8) :: dChristoffelSrc(3,3,3,3)
  REAL(8) :: dChristoffel_tildeSrc(3,3,3,3)
  REAL(8) :: RiemannSrc(3,3,3,3)
  REAL(8) :: RicciSrc(3,3)
  REAL(8) :: RSrc
  REAL(8) :: dGtildeSrc(3,3)
  REAL(8) :: Z(3),Zup(3),dZ(3,3),dZSrc(3,3),nablaZSrc(3,3)
  REAL(8) :: RicciPlusNablaZSrc(3,3)
  REAL(8) :: RPlusTwoNablaZSrc
  REAL(8) :: nablaijalphaSrc(3,3)
  REAL(8) :: nablanablaalphaSrc
  REAL(8) :: SecondOrderTermsSrc(3,3),traceSrc
  REAL(8) :: ov(3)

#if defined(CCZ4EINSTEIN) || defined(CCZ4GRHD) || defined(CCZ4GRMHD) || defined(CCZ4GRGPR)
  !
  k1   = EQN%CCZ4k1                             ! kappa_1
  k2   = EQN%CCZ4k2                             ! kappa_2
  k3   = EQN%CCZ4k3                             ! kappa_3
  fff  = EQN%CCZ4f                              ! multiplied to \partial_k(b^i) in the evolution eqn for BB
  ggg  = EQN%CCZ4g                              !
  eta  = EQN%CCZ4eta                            ! eta
  itau = EQN%CCZ4itau                           ! tau^-1
  e    = EQN%CCZ4e                              ! e
  c    = EQN%CCZ4c                              ! c
  mu   = EQN%CCZ4mu                             ! mu
  ds   = EQN%CCZ4ds                             ! only multiplied to Z_i and \nabla_i(Z_j)
  bs   = EQN%CCZ4bs                             ! only used in dtbb and dtB
  xi   = EQN%CCZ4xi                             ! only used in dtbb
  sk   = EQN%CCZ4sk                             ! s multiplied to \partial_i(B_j^k) and other places
  !
  ! these are the tilde quantities, so be careful!
  g_cov(1,1) = Q(1)
  g_cov(1,2) = Q(2)
  g_cov(1,3) = Q(3)
  g_cov(2,1) = Q(2)
  g_cov(2,2) = Q(4)
  g_cov(2,3) = Q(5)
  g_cov(3,1) = Q(3)
  g_cov(3,2) = Q(5)
  g_cov(3,3) = Q(6)
  ! this determinant should be unity, since we use the conformal decomposition
  det = (Q(1)*Q(4)*Q(6)-Q(1)*Q(5)**2-Q(2)**2*Q(6)+2*Q(2)*Q(3)*Q(5)-Q(3)**2*Q(4))
  !
  g_contr(1,1) =  ( g_cov(2,2)*g_cov(3,3)-g_cov(2,3)*g_cov(3,2))/det
  g_contr(1,2) = -( g_cov(1,2)*g_cov(3,3)-g_cov(1,3)*g_cov(3,2))/det
  g_contr(1,3) = -(-g_cov(1,2)*g_cov(2,3)+g_cov(1,3)*g_cov(2,2))/det
  g_contr(2,1) = -( g_cov(2,1)*g_cov(3,3)-g_cov(2,3)*g_cov(3,1))/det
  g_contr(2,2) =  ( g_cov(1,1)*g_cov(3,3)-g_cov(1,3)*g_cov(3,1))/det
  g_contr(2,3) = -( g_cov(1,1)*g_cov(2,3)-g_cov(1,3)*g_cov(2,1))/det
  g_contr(3,1) = -(-g_cov(2,1)*g_cov(3,2)+g_cov(2,2)*g_cov(3,1))/det
  g_contr(3,2) = -( g_cov(1,1)*g_cov(3,2)-g_cov(1,2)*g_cov(3,1))/det
  g_contr(3,3) =  ( g_cov(1,1)*g_cov(2,2)-g_cov(1,2)*g_cov(2,1))/det
  !
  alpha = DEXP(DMAX1(-20.D0,DMIN1(20.D0,Q(17))))
  SELECT CASE(EQN%CCZ4LapseType)
  CASE(0)  ! harmonic
    fa  = 1.0D0
    faa = 0.0D0
  CASE DEFAULT  ! 1 + log
    fa  = 2.0D0/alpha
    faa = -2.0D0/alpha**2
  END SELECT
  !
  K0  = Q(59)
  !
  Aex(1,1) = Q(7 )
  Aex(1,2) = Q(8 )
  Aex(1,3) = Q(9 )
  Aex(2,1) = Q(8 )
  Aex(2,2) = Q(10)
  Aex(2,3) = Q(11)
  Aex(3,1) = Q(9 )
  Aex(3,2) = Q(11)
  Aex(3,3) = Q(12)
  !
  traceA = SUM(g_contr*Aex)
  Aex = Aex-1.0D0/3.0D0*g_cov*traceA
  !
  Amix = MATMUL(g_contr,          Aex  )
  Aup  = MATMUL(g_contr,TRANSPOSE(Amix))
  !
  Theta  = Q(13)
  !
  Ghat = (/Q(14),Q(15),Q(16)/)
  !
  b = Q(21:23)
  !
  AA = (/Q(24),Q(25),Q(26)/)
  !
  traceK = Q(54)
  !
  phi = DEXP(DMAX1(-20.0D0,DMIN1(20.0D0,Q(55))))
  !
  PP  = Q(56:58)
  !
  beta = (/Q(18),Q(19),Q(20)/)
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
  dgup = 0.0D0
  DO n=1,3
    DO j=1,3
      DO l=1,3
        DO m=1,3
          DO k=1,3
            dgup(k,m,l) = dgup(k,m,l)-2.0D0*g_contr(m,n)*g_contr(j,l)*DD(k,n,j)
          ENDDO
        ENDDO
      ENDDO
    ENDDO
  ENDDO
  !
  Kex  = Aex/phi**2+1.0D0/3.0D0*traceK*g_cov/phi**2
  Kmix = MATMUL(phi**2*g_contr,          Kex  )
  Kup  = MATMUL(phi**2*g_contr,TRANSPOSE(Kmix))
  !
  Christoffel_tilde = 0.0D0
  Christoffel       = 0.0D0
  Gtilde = 0.0D0
  !
  DO k=1,3
    DO j=1,3
      DO i=1,3
        DO l=1,3
          Christoffel_tilde(i,j,k) = Christoffel_tilde(i,j,k)+g_contr(k,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j))
          Christoffel(i,j,k)       = Christoffel(i,j,k)      +g_contr(k,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j)) &
                  &                         -g_contr(k,l)*(g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l))
        ENDDO
      ENDDO
    ENDDO
  ENDDO
  !
  DO l=1,3
    DO j=1,3
      DO i=1,3
        Gtilde(i) = Gtilde(i)+g_contr(j,l)*Christoffel_tilde(j,l,i)
      ENDDO
    ENDDO
  ENDDO
  !
  Z   = 0.5D0*ds*MATMUL(g_cov,Ghat-Gtilde)
  Zup = MATMUL(phi**2*g_contr,Z)
  !
  dChristoffelSrc = 0.0D0
  dChristoffel_tildeSrc = 0.0D0
  DO l=1,3
    DO m=1,3
      DO j=1,3
        DO i=1,3
          DO k=1,3
            dChristoffelSrc(k,i,j,m) = dChristoffelSrc(k,i,j,m)+dgup(k,m,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j))                      &
                    &                         -dgup(k,m,l)*(g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l)) &
                    &                         -2.0D0*g_contr(m,l)*(DD(k,j,l)*PP(i)+DD(k,i,l)*PP(j)-DD(k,i,j)*PP(l))
            !
            dChristoffel_tildeSrc(k,i,j,m) = dChristoffel_tildeSrc(k,i,j,m)+dgup(k,m,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j))
          ENDDO
        ENDDO
      ENDDO
    ENDDO
  ENDDO
  !
  RiemannSrc = 0.0D0
  DO m=1,3
    DO j=1,3
      DO k=1,3
        DO i=1,3
          RiemannSrc(i,k,j,m) = dChristoffelSrc(k,i,j,m)-dChristoffelSrc(j,i,k,m)
          DO l=1,3
            RiemannSrc(i,k,j,m) = RiemannSrc(i,k,j,m)+Christoffel(i,j,l)*Christoffel(l,k,m)-Christoffel(i,k,l)*Christoffel(l,j,m)
          ENDDO
        ENDDO
      ENDDO
    ENDDO
  ENDDO
  !
  RicciSrc = 0.0D0
  DO l=1,3
    DO n=1,3
      DO m=1,3
        RicciSrc(m,n) = RicciSrc(m,n)+RiemannSrc(m,l,n,l)
      ENDDO
    ENDDO
  ENDDO
  !
  RSrc = phi**2*SUM(g_contr*RicciSrc)
  !
  ! Here we directly compute the derivative of Gtilde from its original definition as contracted Christoffel symbol,
  ! without assuming unit determinant of the conformal metric. Back to the roots, and as few assumptions as possible...
  !
  dGtildeSrc = 0.0D0
  DO l=1,3
    DO j=1,3
      DO i=1,3
        DO k=1,3
          dGtildeSrc(k,i) = dGtildeSrc(k,i)+dgup(k,j,l)*Christoffel_tilde(j,l,i)+g_contr(j,l)*dChristoffel_tildeSrc(k,j,l,i)
        ENDDO
      ENDDO
    ENDDO
  ENDDO
  !
  dZSrc = 0.0D0
  DO j=1,3
    DO i=1,3
      DO k=1,3
        dZSrc(k,i) = dZSrc(k,i)+ds*(DD(k,i,j)*(Ghat(j)-Gtilde(j))+0.5D0*g_cov(i,j)*(-dGtildeSrc(k,j)))
      ENDDO
    ENDDO
  ENDDO
  !
  nablaZSrc = 0.0D0
  DO j=1,3
    DO i=1,3
      nablaZSrc(i,j) = dZSrc(i,j)
      DO k=1,3
        nablaZSrc(i,j) = nablaZSrc(i,j)-Christoffel(i,j,k)*Z(k)
      ENDDO
    ENDDO
  ENDDO
  !
  RicciPlusNablaZSrc = RicciSrc+(nablaZSrc+TRANSPOSE(nablaZSrc))
  !
  RPlusTwoNablaZSrc = phi**2*SUM(g_contr*RicciPlusNablaZSrc)
  !
  nablaijalphaSrc = 0.0D0
  DO j=1,3
    DO i=1,3
      nablaijalphaSrc(i,j) =      alpha*AA(j)*AA(i)
      DO k=1,3
        nablaijalphaSrc(i,j) = nablaijalphaSrc(i,j)-alpha*Christoffel(i,j,k)*AA(k)
      ENDDO
    ENDDO
  ENDDO
  nablanablaalphaSrc = phi**2*SUM(g_contr*nablaijalphaSrc)
  !
  SecondOrderTermsSrc = -nablaijalphaSrc+alpha*RicciPlusNablaZSrc
  traceSrc = SUM(g_contr*SecondOrderTermsSrc)
  SecondOrderTermsSrc = SecondOrderTermsSrc-1.0D0/3.0D0*g_cov*traceSrc
  !
  ! now assemble all this terrible stuff...
  !
  dtgamma = -2.0D0*alpha*Aex-itau*(det-1.0D0)*g_cov
  DO k=1,3
    DO j=1,3
      DO i=1,3
        dtgamma(i,j) = dtgamma(i,j)+g_cov(k,i)*BB(j,k)+g_cov(k,j)*BB(i,k)-2.0D0/3.0D0*g_cov(i,j)*BB(k,k)+2.0D0*beta(k)*DD(k,i,j)
      ENDDO
    ENDDO
  ENDDO
  !
  ! Main variables of the CCZ4 system
  ! extrinsic curvature
  dtK = phi**2*SecondOrderTermsSrc+alpha*Aex*(traceK-2.0D0*Theta)-2.0D0*alpha*MATMUL(Aex,Amix)-itau*g_cov*traceA
  DO j=1,3
    DO i=1,3
      DO k=1,3
        dtK(i,j) = dtK(i,j)+Aex(k,i)*BB(j,k)+Aex(k,j)*BB(i,k)-2.0D0/3.0D0*Aex(i,j)*BB(k,k)
      ENDDO
    ENDDO
  ENDDO
  !
  ! dtTraceK = -nablanablaalphaNCP-nablanablaalphaSrc+alpha*(RPlusTwoNablaZNCP+RPlusTwoNablaZSrc+traceK**2-2.0D0*Theta*traceK)-3.0D0*alpha*k1*(1.0D0+k2)*Theta+SUM(beta(:)*dtraceK(:))   ! Baojiu
  dtTraceK = -nablanablaalphaSrc+alpha*(RPlusTwoNablaZSrc+traceK**2-2.0D0*c*Theta*traceK)-3.0D0*alpha*k1*(1.0D0+k2)*Theta ! Baojiu
  !
  traceB  = BB(1,1)+BB(2,2)+BB(3,3)
  dtphi   = beta(1)*PP(1)+beta(2)*PP(2)+beta(3)*PP(3)+1.0D0/3.0D0*alpha*traceK-1.0D0/3.0D0*traceB
  dtalpha = -alpha*fa*(traceK-K0-2.0D0*c*Theta)+beta(1)*AA(1)+beta(2)*AA(2)+beta(3)*AA(3)
  !
  Aupdown = SUM(Aex*Aup)
  ! *** original
  dtTheta = 0.5D0*alpha*e**2*RplusTwoNablaZSrc+    &            ! temporal Z
          !         & 0.5D0*alpha*e**2*(-Aupdown+2.0D0/3.0D0*traceK**2)-alpha*Theta*traceK-SUM(Zup*alpha*AA)-alpha*k1*(2.0D0+k2)*Theta    ! Baojiu
          & 0.5D0*alpha*e**2*(-Aupdown+2.0D0/3.0D0*traceK**2)-c*alpha*Theta*traceK-alpha*SUM(Zup*AA)-alpha*k1*(2.0D0+k2)*Theta  ! Baojiu
  !
  dtGhat = 0.0D0
  DO i=1,3
    dtGhat(i) = dtGhat(i)  &
            & +2.0D0*alpha*(SUM(Christoffel_tilde(:,:,i)*Aup(:,:))-3.0D0*SUM(Aup(i,:)*PP(:)))    &
            & +2.0D0*alpha*SUM(g_contr(:,i)*(-Theta*AA(:)-2.0D0/3.0D0*traceK*Z(:)))  &
            & -2.0D0*alpha*SUM(Aup(i,:)*AA(:))-2.0D0*alpha*k1*SUM(g_contr(i,:)*Z(:))-SUM(Gtilde(:)*BB(:,i))   &
            & +2.0D0/3.0D0*Gtilde(i)*traceB
    DO l=1,3
      DO k=1,3
        dtGhat(i) = dtGhat(i)+ &
                &  2.0D0*k3*(2.0D0/3.0D0*g_contr(i,l)*Z(l)*BB(k,k)-g_contr(l,k)*Z(l)*BB(k,i))
      ENDDO
    ENDDO
  ENDDO
  !
  DO k=1,3
    ov(k) = 2.0D0*alpha*SUM(dgup(k,:,:)*Aex(:,:))    ! here we can use the constraint that trace A tilde = 0.
  ENDDO
  !
  dtGhat = dtGhat+sk*MATMUL(g_contr,ov)                                               ! the above ordering constraint is "down", so we must raise the index via g_contr.
  !
  dtbb = xi*dtGhat-eta*b                                                            !  <= be careful, this damping term -eta*b may be dangerous for the gamma driver, since it may kill the waves that you want !
  dtbb = sk*dtbb
  !
  dtbeta  = fff*b
  ! Add the following term if you want to have shift convection in the PDE for beta^i
  ! Do not add it if you want a real Lie derivative for beta. In this case, the advection term cancels out.
  dtbeta = dtbeta+bs*(beta(1)*BB(1,:)+beta(2)*BB(2,:)+beta(3)*BB(3,:))
  dtbeta = sk*dtbeta
  !
  ! Auxiliary variables
  dtA = -alpha*AA*(fa+alpha*faa)*(traceK-K0-2.0D0*c*Theta)+MATMUL(BB,AA)
  DO k=1,3
    dtA(k) = dtA(k)-sk*alpha*fa*SUM(dgup(k,:,:)*Aex(:,:))  ! here we can use the constraint that trace A tilde = 0.
  ENDDO
  !
  dtB = sk*MATMUL(BB,BB)     ! Baojiu
  !
  dtD = 0.0D0
  DO m=1,3
    DO j=1,3
      DO i=1,3
        DO k=1,3
          DO n=1,3
            dtD(k,i,j) = dtD(k,i,j)+1.0D0/3.0D0*alpha*g_cov(i,j)*dgup(k,n,m)*Aex(n,m)   ! explicitly remove the trace of tilde A again
          ENDDO
        ENDDO
      ENDDO
    ENDDO
  ENDDO
  !
  DO j=1,3
    DO i=1,3
      DO k=1,3
        dtD(k,i,j) = dtD(k,i,j)-alpha*AA(k)*Aex(i,j) !trace removing missing here
        DO l=1,3
          dtD(k,i,j) = dtD(k,i,j)+BB(k,l)*DD(l,i,j)+DD(k,l,i)*BB(j,l)+DD(k,l,j)*BB(i,l)-2.0D0/3.0D0*DD(k,i,j)*BB(l,l)
        ENDDO
      ENDDO
    ENDDO
  ENDDO
  !
  dtP = MATMUL(BB,PP)
  DO k=1,3
    dtP(k) = dtP(k)+1.0D0/3.0D0*alpha*AA(k)*traceK+sk*1.0D0/3.0D0*alpha*SUM(dgup(k,:,:)*Aex(:,:))
  ENDDO

  src(1:6)   = (/dtgamma(1,1),dtgamma(1,2),dtgamma(1,3),dtgamma(2,2),dtgamma(2,3),dtgamma(3,3)/)       ! \tilde \gamma_ij
  src(7:12)  = (/dtK(1,1),dtK(1,2),dtK(1,3),dtK(2,2),dtK(2,3),dtK(3,3)/)                               ! \tilde A_ij
  src(13)    = dtTheta                                                                                 ! Theta
  src(14:16) = dtGhat(1:3)                                                                             ! \hat \Gamma^i
  src(17)    = dtalpha                                                                                 ! log alpha
  src(18:20) = dtbeta                                                                                  ! beta^i
  src(21:23) = dtbb                                                                                    ! b^i
  src(24:26) = dtA(1:3)                                                                                ! A_k
  src(27:35) = (/dtB(1,1),dtB(2,1),dtB(3,1),dtB(1,2),dtB(2,2),dtB(3,2),dtB(1,3),dtB(2,3),dtB(3,3)/)    ! B_k^i
  src(36:41) = (/dtD(1,1,1),dtD(1,1,2),dtD(1,1,3),dtD(1,2,2),dtD(1,2,3),dtD(1,3,3)/)                   ! D_kij
  src(42:47) = (/dtD(2,1,1),dtD(2,1,2),dtD(2,1,3),dtD(2,2,2),dtD(2,2,3),dtD(2,3,3)/)                   ! D_kij
  src(48:53) = (/dtD(3,1,1),dtD(3,1,2),dtD(3,1,3),dtD(3,2,2),dtD(3,2,3),dtD(3,3,3)/)                   ! D_kij
  src(54)    = dtTraceK                                                                                ! traceK
  src(55)    = dtphi                                                                                   ! log phi
  src(56:58) = dtP                                                                                     ! P_k
  src(59)    = 0.0D0
  !
  S = src    ! here, we do not have to change sign, since we work on the right hand side in the fused subroutine
  !
  RETURN
#endif

END SUBROUTINE PDESource

RECURSIVE SUBROUTINE PDEFusedSrcNCP(Src_BgradQ,Q,gradQin)
  USE MainVariables, ONLY: nVar,nParam,d,EQN,nDim 
  IMPLICIT NONE
  ! Argument list 
  REAL(8), INTENT(IN)  :: Q(nVar),gradQin(nVar,d)
  REAL(8), INTENT(OUT) :: Src_BgradQ(nVar) 
  ! Local variables 
  REAL(8) :: gradQ(nVar,d)
  INTEGER :: i,j,k,l,m,n
  REAL(8) :: mu
  REAL(8) :: BgradQ(nVar),V(nVar)
  REAL(8) :: k1,k2,k3,fff,ggg,e,c,ds,xi,sk,sknl,bs,dgup(3,3,3),eta,itau
  REAL(8) :: fa,faa
  ! Q and their gradients
  REAL(8) :: g_cov(3,3),g_contr(3,3),det                                ! Q(1-6) 
  REAL(8) :: Aex(3,3),dAex(3,3,3),Amix(3,3),Aup(3,3),Aupdown,traceA     ! Q(7-12) 
  REAL(8) :: Theta,dTheta(3)                                            ! Q(13)
  REAL(8) :: Ghat(3),dGhat(3,3)                                         ! Q(14-16)
  REAL(8) :: alpha                                                      ! Q(17)
  REAL(8) :: beta(3)                                                    ! Q(18-20)
  REAL(8) :: b(3)                                                       ! Q(21-23)
  REAL(8) :: AA(3),dAA(3,3)                                             ! Q(24-26)
  REAL(8) :: BB(3,3),dBB(3,3,3),traceB                                  ! Q(27-35)
  REAL(8) :: DD(3,3,3),dDD(3,3,3,3)                                     ! Q(36-53)
  REAL(8) :: traceK,dtraceK(3),dtraceKNCP(3)                            ! Q(54)
  REAL(8) :: phi                                                        ! Q(55)
  REAL(8) :: PP(3),dPP(3,3)                                             ! Q(56-58)
  REAL(8) :: K0,dK0(3)                                                  ! Q(59)
  ! time derivatives of Q
  REAL(8) :: dtgamma(3,3)                                               ! Q(1-7)
  REAL(8) :: dtK(3,3)                                                   ! Q(7-12)
  REAL(8) :: dtTheta                                                    ! Q(13)
  REAL(8) :: dtGhat(3)                                                  ! Q(14-16)
  REAL(8) :: dtalpha                                                    ! Q(17)
  REAL(8) :: dtbeta(3)                                                  ! Q(18-20)
  REAL(8) :: dtbb(3)                                                    ! Q(21-23)
  REAL(8) :: dtA(3)                                                     ! Q(24-26)
  REAL(8) :: dtB(3,3)                                                   ! Q(27-35)
  REAL(8) :: dtD(3,3,3)                                                 ! Q(36-53)
  REAL(8) :: dtTraceK                                                   ! Q(54)
  REAL(8) :: dtphi                                                      ! Q(55)
  REAL(8) :: dtP(3)                                                     ! Q(56-58)
  ! intermediate quantities    
  REAL(8) :: Gtilde(3)
  REAL(8) :: Kex(3,3),Kmix(3,3),Kup(3,3)
  REAL(8) :: Christoffel(3,3,3),Christoffel_tilde(3,3,3)
  REAL(8) :: dChristoffel(3,3,3,3),dChristoffelNCP(3,3,3,3),dChristoffelSrc(3,3,3,3) 
  REAL(8) :: dChristoffel_tildeNCP(3,3,3,3),dChristoffel_tildeSrc(3,3,3,3)
  REAL(8) :: Riemann(3,3,3,3),RiemannNCP(3,3,3,3), RiemannSrc(3,3,3,3)
  REAL(8) :: Ricci(3,3),RicciNCP(3,3),RicciSrc(3,3) 
  REAL(8) :: R,RNCP,RSrc 
  REAL(8) :: dGtildeSrc(3,3),dGtildeNCP(3,3) 
  REAL(8) :: Z(3),Zup(3),dZ(3,3),dZNCP(3,3),dZSrc(3,3),nablaZNCP(3,3),nablaZSrc(3,3)
  REAL(8) :: RicciPlusNablaZNCP(3,3),RicciPlusNablaZSrc(3,3) 
  REAL(8) :: RplusNablaZNCP,RplusNablaZSrc,RPlusTwoNablaZNCP,RPlusTwoNablaZSrc
  REAL(8) :: nablaijalphaNCP(3,3),nablaijalphaSrc(3,3) 
  REAL(8) :: nablanablaalpha,nablanablaalphaNCP,nablanablaalphaSrc
  REAL(8) :: SecondOrderTermsNCP(3,3),SecondOrderTermsSrc(3,3),traceNCP,traceSrc
  REAL(8) :: ov(3)
  !
  BgradQ = 0.0D0 
  !
#if defined(CCZ4EINSTEIN) || defined(CCZ4GRHD) || defined(CCZ4GRMHD) || defined(CCZ4GRGPR) 
  !
  IF(nDim.EQ.2) THEN
     gradQ(:,1:2)=gradQin(:,1:2)
     gradQ(:,3)=0.0D0
  ELSE
     gradQ=gradQin
  ENDIF 
  !
  k1   = EQN%CCZ4k1                             ! kappa_1 
  k2   = EQN%CCZ4k2                             ! kappa_2 
  k3   = EQN%CCZ4k3                             ! kappa_3 
  fff  = EQN%CCZ4f                              ! multiplied to \partial_k(b^i) in the evolution eqn for BB
  ggg  = EQN%CCZ4g                              !
  eta  = EQN%CCZ4eta                            ! eta
  itau = EQN%CCZ4itau                           ! tau^-1
  e    = EQN%CCZ4e                              ! e
  c    = EQN%CCZ4c                              ! c
  mu   = EQN%CCZ4mu                             ! mu
  ds   = EQN%CCZ4ds                             ! only multiplied to Z_i and \nabla_i(Z_j) 
  bs   = EQN%CCZ4bs                             ! only used in dtbb and dtB
  xi   = EQN%CCZ4xi                             ! only used in dtbb 
  sk   = EQN%CCZ4sk                             ! s multiplied to \partial_i(B_j^k) and other places 
  !
  ! these are the tilde quantities, so be careful!   
  g_cov(1,1) = Q(1)
  g_cov(1,2) = Q(2)
  g_cov(1,3) = Q(3)
  g_cov(2,1) = Q(2)
  g_cov(2,2) = Q(4)
  g_cov(2,3) = Q(5)
  g_cov(3,1) = Q(3)
  g_cov(3,2) = Q(5)
  g_cov(3,3) = Q(6)
  ! this determinant should be unity, since we use the conformal decomposition 
  det = (Q(1)*Q(4)*Q(6)-Q(1)*Q(5)**2-Q(2)**2*Q(6)+2*Q(2)*Q(3)*Q(5)-Q(3)**2*Q(4)) 
  ! 
  g_contr(1,1) =  ( g_cov(2,2)*g_cov(3,3)-g_cov(2,3)*g_cov(3,2))/det 
  g_contr(1,2) = -( g_cov(1,2)*g_cov(3,3)-g_cov(1,3)*g_cov(3,2))/det
  g_contr(1,3) = -(-g_cov(1,2)*g_cov(2,3)+g_cov(1,3)*g_cov(2,2))/det 
  g_contr(2,1) = -( g_cov(2,1)*g_cov(3,3)-g_cov(2,3)*g_cov(3,1))/det 
  g_contr(2,2) =  ( g_cov(1,1)*g_cov(3,3)-g_cov(1,3)*g_cov(3,1))/det 
  g_contr(2,3) = -( g_cov(1,1)*g_cov(2,3)-g_cov(1,3)*g_cov(2,1))/det 
  g_contr(3,1) = -(-g_cov(2,1)*g_cov(3,2)+g_cov(2,2)*g_cov(3,1))/det 
  g_contr(3,2) = -( g_cov(1,1)*g_cov(3,2)-g_cov(1,2)*g_cov(3,1))/det 
  g_contr(3,3) =  ( g_cov(1,1)*g_cov(2,2)-g_cov(1,2)*g_cov(2,1))/det 
  !   
  alpha = DEXP(DMAX1(-20.D0,DMIN1(20.D0,Q(17)))) 
  SELECT CASE(EQN%CCZ4LapseType) 
     CASE(0)  ! harmonic 
        fa  = 1.0D0 
        faa = 0.0D0 
     CASE DEFAULT  ! 1 + log 
        fa  = 2.0D0/alpha
        faa = -2.0D0/alpha**2   
  END SELECT 
  ! 
  K0  = Q(59)
  dK0 = sk*gradQ(59,:) 
  !  
  Aex(1,1) = Q(7 ) 
  Aex(1,2) = Q(8 ) 
  Aex(1,3) = Q(9 ) 
  Aex(2,1) = Q(8 ) 
  Aex(2,2) = Q(10) 
  Aex(2,3) = Q(11) 
  Aex(3,1) = Q(9 ) 
  Aex(3,2) = Q(11) 
  Aex(3,3) = Q(12) 
  !
  traceA = SUM(g_contr*Aex) 
  Aex = Aex-1.0D0/3.0D0*g_cov*traceA 
  !
  dAex(:,1,1) = gradQ(7 ,:) 
  dAex(:,1,2) = gradQ(8 ,:) 
  dAex(:,1,3) = gradQ(9 ,:) 
  dAex(:,2,1) = gradQ(8 ,:) 
  dAex(:,2,2) = gradQ(10,:) 
  dAex(:,2,3) = gradQ(11,:) 
  dAex(:,3,1) = gradQ(9 ,:) 
  dAex(:,3,2) = gradQ(11,:) 
  dAex(:,3,3) = gradQ(12,:) 
  !
  Amix = MATMUL(g_contr,          Aex  )
  Aup  = MATMUL(g_contr,TRANSPOSE(Amix)) 
  !
  Theta  = Q(13)
  dTheta = gradQ(13,:) 
  ! 
  Ghat = (/Q(14),Q(15),Q(16)/)
  dGhat(:,1) = gradQ(14,:)
  dGhat(:,2) = gradQ(15,:)
  dGhat(:,3) = gradQ(16,:)
  !
  b = Q(21:23) 
  !
  AA = (/Q(24),Q(25),Q(26)/) 
  dAA(:,1) = gradQ(24,:) 
  dAA(:,2) = gradQ(25,:) 
  dAA(:,3) = gradQ(26,:) 
  !
  traceK = Q(54) 
  dtraceK = gradQ(54,:) 
  !
  phi = DEXP(DMAX1(-20.0D0,DMIN1(20.0D0,Q(55))))  
  !
  PP  = Q(56:58) 
  dPP(:,1) = gradQ(56,:) 
  dPP(:,2) = gradQ(57,:) 
  dPP(:,3) = gradQ(58,:) 
  !
  beta = (/Q(18),Q(19),Q(20)/) 
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
  dgup = 0.0D0 
  DO n=1,3
     DO j=1,3 
        DO l=1,3 
           DO m=1,3 
              DO k=1,3 
                 dgup(k,m,l) = dgup(k,m,l)-2.0D0*g_contr(m,n)*g_contr(j,l)*DD(k,n,j) 
              ENDDO
           ENDDO 
        ENDDO 
     ENDDO 
  ENDDO         
  !
  Kex  = Aex/phi**2+1.0D0/3.0D0*traceK*g_cov/phi**2 
  Kmix = MATMUL(phi**2*g_contr,          Kex  ) 
  Kup  = MATMUL(phi**2*g_contr,TRANSPOSE(Kmix)) 
  !
  Christoffel_tilde = 0.0D0  
  Christoffel       = 0.0D0 
  Gtilde = 0.0D0 
  !
  DO k=1,3
     DO j=1,3
        DO i=1,3
           DO l=1,3
              Christoffel_tilde(i,j,k) = Christoffel_tilde(i,j,k)+g_contr(k,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j)) 
              Christoffel(i,j,k)       = Christoffel(i,j,k)      +g_contr(k,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j)) &
                                       &                         -g_contr(k,l)*(g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l)) 
           ENDDO 
        ENDDO
     ENDDO
  ENDDO
  !
  DO l=1,3
     DO j=1,3
        DO i=1,3
           Gtilde(i) = Gtilde(i)+g_contr(j,l)*Christoffel_tilde(j,l,i) 
        ENDDO
     ENDDO     
  ENDDO    
  !
  Z   = 0.5D0*ds*MATMUL(g_cov,Ghat-Gtilde) 
  Zup = MATMUL(phi**2*g_contr,Z) 
  !
  dChristoffelNCP = 0.0D0
  dChristoffelSrc = 0.0D0 
  dChristoffel_tildeNCP = 0.0D0
  dChristoffel_tildeSrc = 0.0D0 
  DO l=1,3 
     DO m=1,3 
        DO j=1,3 
           DO i=1,3 
              DO k=1,3
                 dChristoffelNCP(k,i,j,m) = dChristoffelNCP(k,i,j,m)+0.5D0*g_contr(m,l)*((dDD(k,i,j,l)+dDD(i,k,j,l))+    &
                                          &                                              (dDD(k,j,i,l)+dDD(j,k,i,l))-    &
                                          &                                              (dDD(k,l,i,j)+dDD(l,k,i,j)))    & 
                                          &                         -0.5D0*g_contr(m,l)*(g_cov(j,l)*(dPP(k,i)+dPP(i,k))+ &
                                          &                                              g_cov(i,l)*(dPP(k,j)+dPP(j,k))- &
                                          &                                              g_cov(i,j)*(dPP(k,l)+dPP(l,k))) 
                 !
                 dChristoffel_tildeNCP(k,i,j,m) = dChristoffel_tildeNCP(k,i,j,m)+0.5D0*g_contr(m,l)*((dDD(k,i,j,l)+dDD(i,k,j,l))+ &
                                          &                                                          (dDD(k,j,i,l)+dDD(j,k,i,l))- &
                                          &                                                          (dDD(k,l,i,j)+dDD(l,k,i,j))) 
                 ! 
                 dChristoffelSrc(k,i,j,m) = dChristoffelSrc(k,i,j,m)+dgup(k,m,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j))                      &
                                          &                         -dgup(k,m,l)*(g_cov(j,l)*PP(i)+g_cov(i,l)*PP(j)-g_cov(i,j)*PP(l)) &
                                          &                         -2.0D0*g_contr(m,l)*(DD(k,j,l)*PP(i)+DD(k,i,l)*PP(j)-DD(k,i,j)*PP(l)) 
                 !
                 dChristoffel_tildeSrc(k,i,j,m) = dChristoffel_tildeSrc(k,i,j,m)+dgup(k,m,l)*(DD(i,j,l)+DD(j,i,l)-DD(l,i,j)) 
              ENDDO 
           ENDDO
        ENDDO
     ENDDO
  ENDDO
  ! 
  RiemannSrc = 0.0D0
  RiemannNCP = 0.0D0 
  DO m=1,3 
     DO j=1,3 
        DO k=1,3 
           DO i=1,3
              RiemannNCP(i,k,j,m) = dChristoffelNCP(k,i,j,m)-dChristoffelNCP(j,i,k,m)
              RiemannSrc(i,k,j,m) = dChristoffelSrc(k,i,j,m)-dChristoffelSrc(j,i,k,m) 
              DO l=1,3
                 RiemannSrc(i,k,j,m) = RiemannSrc(i,k,j,m)+Christoffel(i,j,l)*Christoffel(l,k,m)-Christoffel(i,k,l)*Christoffel(l,j,m) 
              ENDDO 
           ENDDO
        ENDDO
     ENDDO
  ENDDO    
  ! 
  RicciNCP = 0.0D0
  RicciSrc = 0.0D0 
  DO l=1,3 
     DO n=1,3
        DO m=1,3    
           RicciNCP(m,n) = RicciNCP(m,n)+RiemannNCP(m,l,n,l)  
           RicciSrc(m,n) = RicciSrc(m,n)+RiemannSrc(m,l,n,l)  
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
  dGtildeNCP = 0.0D0
  dGtildeSrc = 0.0D0
  DO l=1,3
     DO j=1,3
        DO i=1,3 
           DO k=1,3
              dGtildeSrc(k,i) = dGtildeSrc(k,i)+dgup(k,j,l)*Christoffel_tilde(j,l,i)+g_contr(j,l)*dChristoffel_tildeSrc(k,j,l,i) 
              dGtildeNCP(k,i) = dGtildeNCP(k,i)                                     +g_contr(j,l)*dChristoffel_tildeNCP(k,j,l,i) 
           ENDDO
        ENDDO
     ENDDO
  ENDDO
  !
  dZNCP = 0.0D0 
  dZSrc = 0.0D0 
  DO j=1,3
     DO i=1,3    
        DO k=1,3 
           dZNCP(k,i) = dZNCP(k,i)+0.5D0*ds*g_cov(i,j)*(dGhat(k,j)-dGtildeNCP(k,j))  
           dZSrc(k,i) = dZSrc(k,i)+ds*(DD(k,i,j)*(Ghat(j)-Gtilde(j))+0.5D0*g_cov(i,j)*(-dGtildeSrc(k,j))) 
        ENDDO 
     ENDDO 
  ENDDO     
  !
  nablaZNCP = dZNCP 
  nablaZSrc = 0.0D0 
  DO j=1,3 
     DO i=1,3 
        nablaZSrc(i,j) = dZSrc(i,j)
        DO k=1,3 
           nablaZSrc(i,j) = nablaZSrc(i,j)-Christoffel(i,j,k)*Z(k) 
        ENDDO 
     ENDDO
  ENDDO    
  !
  RicciPlusNablaZNCP = RicciNCP+(nablaZNCP+TRANSPOSE(nablaZNCP)) 
  RicciPlusNablaZSrc = RicciSrc+(nablaZSrc+TRANSPOSE(nablaZSrc)) 
  !
  RPlusTwoNablaZNCP = phi**2*SUM(g_contr*RicciPlusNablaZNCP) 
  RPlusTwoNablaZSrc = phi**2*SUM(g_contr*RicciPlusNablaZSrc) 
  !
  nablaijalphaNCP = 0.0D0
  nablaijalphaSrc = 0.0D0
  DO j=1,3 
     DO i=1,3 
        nablaijalphaNCP(i,j) = 0.5D0*alpha*(dAA(i,j)+dAA(j,i)) 
        nablaijalphaSrc(i,j) =      alpha*AA(j)*AA(i) 
        DO k=1,3 
           nablaijalphaSrc(i,j) = nablaijalphaSrc(i,j)-alpha*Christoffel(i,j,k)*AA(k)  
        ENDDO
     ENDDO
  ENDDO 
  nablanablaalphaNCP = phi**2*SUM(g_contr*nablaijalphaNCP) 
  nablanablaalphaSrc = phi**2*SUM(g_contr*nablaijalphaSrc) 
  !
  SecondOrderTermsNCP = -nablaijalphaNCP+alpha*RicciPlusNablaZNCP 
  SecondOrderTermsSrc = -nablaijalphaSrc+alpha*RicciPlusNablaZSrc 
  traceNCP = SUM(g_contr*SecondOrderTermsNCP) 
  SecondOrderTermsNCP = SecondOrderTermsNCP-1.0D0/3.0D0*g_cov*traceNCP 
  traceSrc = SUM(g_contr*SecondOrderTermsSrc) 
  SecondOrderTermsSrc = SecondOrderTermsSrc-1.0D0/3.0D0*g_cov*traceSrc 
  !
  !
  !
  ! now assemble all this terrible stuff... 
  !
  dtgamma = -2.0D0*alpha*Aex-itau*(det-1.0D0)*g_cov 
  DO k=1,3 
     DO j=1,3
        DO i=1,3
           dtgamma(i,j) = dtgamma(i,j)+g_cov(k,i)*BB(j,k)+g_cov(k,j)*BB(i,k)-2.0D0/3.0D0*g_cov(i,j)*BB(k,k)+2.0D0*beta(k)*DD(k,i,j) 
        ENDDO
     ENDDO
  ENDDO 
  !
  ! Main variables of the CCZ4 system 
  dtK = phi**2*SecondOrderTermsNCP+beta(1)*dAex(1,:,:)+beta(2)*dAex(2,:,:)+beta(3)*dAex(3,:,:)      ! extrinsic curvature
  dtK = dtK+phi**2*SecondOrderTermsSrc+alpha*Aex*(traceK-2.0D0*Theta)-2.0D0*alpha*MATMUL(Aex,Amix)-itau*g_cov*traceA 
  DO j=1,3 
     DO i=1,3 
        DO k=1,3 
           dtK(i,j) = dtK(i,j)+Aex(k,i)*BB(j,k)+Aex(k,j)*BB(i,k)-2.0D0/3.0D0*Aex(i,j)*BB(k,k) 
        ENDDO
     ENDDO
  ENDDO 
  !
! dtTraceK = -nablanablaalphaNCP-nablanablaalphaSrc+alpha*(RPlusTwoNablaZNCP+RPlusTwoNablaZSrc+traceK**2-2.0D0*Theta*traceK)-3.0D0*alpha*k1*(1.0D0+k2)*Theta+SUM(beta(:)*dtraceK(:))   ! Baojiu 
  dtTraceK = -nablanablaalphaNCP-nablanablaalphaSrc+alpha*(RPlusTwoNablaZNCP+RPlusTwoNablaZSrc+traceK**2-2.0D0*c*Theta*traceK)-3.0D0*alpha*k1*(1.0D0+k2)*Theta+SUM(beta(:)*dtraceK(:)) ! Baojiu 
  !
  traceB  = BB(1,1)+BB(2,2)+BB(3,3) 
  dtphi   = beta(1)*PP(1)+beta(2)*PP(2)+beta(3)*PP(3)+1.0D0/3.0D0*alpha*traceK-1.0D0/3.0D0*traceB 
  dtalpha = -alpha*fa*(traceK-K0-2.0D0*c*Theta)+beta(1)*AA(1)+beta(2)*AA(2)+beta(3)*AA(3) 
  !
  Aupdown = SUM(Aex*Aup) 
  ! *** original 
  dtTheta = 0.5D0*alpha*e**2*(RplusTwoNablaZNCP+RplusTwoNablaZSrc)+beta(1)*dTheta(1)+beta(2)*dTheta(2)+beta(3)*dTheta(3) +    &            ! temporal Z 
!         & 0.5D0*alpha*e**2*(-Aupdown+2.0D0/3.0D0*traceK**2)-alpha*Theta*traceK-SUM(Zup*alpha*AA)-alpha*k1*(2.0D0+k2)*Theta    ! Baojiu  
          & 0.5D0*alpha*e**2*(-Aupdown+2.0D0/3.0D0*traceK**2)-c*alpha*Theta*traceK-alpha*SUM(Zup*AA)-alpha*k1*(2.0D0+k2)*Theta  ! Baojiu 
  ! 
  dtGhat = 0.0D0 
  DO i=1,3
     dtGhat(i) = dtGhat(i)  & 
               & +2.0D0*alpha*(SUM(Christoffel_tilde(:,:,i)*Aup(:,:))-3.0D0*SUM(Aup(i,:)*PP(:))-2.0D0/3.0D0*SUM(g_contr(i,:)*dtraceK(:)))    &
               & +2.0D0*alpha*SUM(g_contr(:,i)*(dTheta(:)-Theta*AA(:)-2.0D0/3.0D0*traceK*Z(:)))  & 
               & -2.0D0*alpha*SUM(Aup(i,:)*AA(:))-2.0D0*alpha*k1*SUM(g_contr(i,:)*Z(:))-SUM(Gtilde(:)*BB(:,i))   &
               & +beta(1)*dGhat(1,i)+beta(2)*dGhat(2,i)+beta(3)*dGhat(3,i)+2.0D0/3.0D0*Gtilde(i)*traceB 
     DO l=1,3
        DO k=1,3
           dtGhat(i) = dtGhat(i)+g_contr(k,l)*0.5D0*(dBB(k,l,i)+dBB(l,k,i))+1.0D0/3.0D0*g_contr(i,k)*0.5D0*(dBB(k,l,l)+dBB(l,k,l)) + & 
                     &         2.0D0*k3*(2.0D0/3.0D0*g_contr(i,l)*Z(l)*BB(k,k)-g_contr(l,k)*Z(l)*BB(k,i)) 
        ENDDO
     ENDDO         
  ENDDO 
  !
  DO k=1,3 
     ov(k) = 2.0D0*alpha*(SUM(g_contr(:,:)*dAex(k,:,:))+SUM(dgup(k,:,:)*Aex(:,:)) )   ! here we can use the constraint that trace A tilde = 0.         
  ENDDO        
  !
  dtGhat = dtGhat+sk*MATMUL(g_contr,ov)                                               ! the above ordering constraint is "down", so we must raise the index via g_contr. 
  !
  dtbb = xi*dtGhat-eta*b                                                            !  <= be careful, this damping term -eta*b may be dangerous for the gamma driver, since it may kill the waves that you want !  
  ! Add the following terms if you want shift convection in the PDE for b^i 
  dtbb = dtbb + bs*(-beta(1)*gradQ(14:16,1)-beta(2)*gradQ(14:16,2)-beta(3)*gradQ(14:16,3) &
       &            +beta(1)*gradQ(21:23,1)+beta(2)*gradQ(21:23,2)+beta(3)*gradQ(21:23,3))   !         
  dtbb = sk*dtbb 
  !
  dtbeta  = fff*b  
  ! Add the following term if you want to have shift convection in the PDE for beta^i 
  ! Do not add it if you want a real Lie derivative for beta. In this case, the advection term cancels out. 
  dtbeta = dtbeta+bs*(beta(1)*BB(1,:)+beta(2)*BB(2,:)+beta(3)*BB(3,:))      
  dtbeta = sk*dtbeta 
  !
  ! Auxiliary variables 
  dtA = -alpha*fa*(dtraceK(:)-dK0(:)-2.0D0*c*dTheta(:))+beta(1)*dAA(1,:)+beta(2)*dAA(2,:)+beta(3)*dAA(3,:)-alpha*AA*(fa+alpha*faa)*(traceK-K0-2.0D0*c*Theta)+MATMUL(BB,AA) 
  DO k=1,3 
     dtA(k) = dtA(k)-sk*alpha*fa*(SUM(g_contr(:,:)*dAex(k,:,:))+SUM(dgup(k,:,:)*Aex(:,:)))   ! here we can use the constraint that trace A tilde = 0. 
  ENDDO    
  ! 
  ! In CCZ4 we have completely removed all the conservative fluxes. 
  dtB(:,1) = fff*gradQ(21,:)  
  dtB(:,2) = fff*gradQ(22,:)  
  dtB(:,3) = fff*gradQ(23,:)  
  ! #ordB2# 
  ! for the ordering constraints, we have to check whether they should be multiplied by alpha**2, or not... 
  DO j=1,3 
     DO i=1,3     
        DO k=1,3 
           dtB(k,i) = dtB(k,i)+mu*alpha**2*g_contr(i,j)*(dPP(k,j)-dPP(j,k))
           DO n=1,3 
              DO l=1,3 
                 dtB(k,i) = dtB(k,i)-mu*alpha**2*g_contr(i,j)*g_contr(n,l)*(dDD(k,l,j,n)-dDD(l,k,j,n))   
              ENDDO 
           ENDDO 
        ENDDO 
     ENDDO 
  ENDDO 
  !
! dtB = dtB+bs*(beta(1)*dBB(1,:,:)+beta(2)*dBB(2,:,:)+beta(3)*dBB(3,:,:)+MATMUL(BB,BB))         ! Baojiu  
  dtB = dtB*sk                                                                                  ! Baojiu 
  dtB = dtB+bs*(beta(1)*dBB(1,:,:)+beta(2)*dBB(2,:,:)+beta(3)*dBB(3,:,:)+sk*MATMUL(BB,BB))      ! Baojiu 
  !
  dtD = -alpha*dAex  
  DO m=1,3
     DO j=1,3
        DO i=1,3
           DO k=1,3 
              dtD(k,i,j) = dtD(k,i,j)+0.5D0*(g_cov(m,i)*0.5D0*(dBB(k,j,m)+dBB(j,k,m))+ &
                         &                   g_cov(m,j)*0.5D0*(dBB(k,i,m)+dBB(i,k,m))) &
                         &           -1.0D0/3.0D0*g_cov(i,j)*0.5D0*(dBB(k,m,m)+dBB(m,k,m))
              DO n=1,3
                 dtD(k,i,j) = dtD(k,i,j)+1.0D0/3.0D0*alpha*g_cov(i,j)*g_contr(n,m)*dAex(k,n,m)+1.0D0/3.0D0*alpha*g_cov(i,j)*dgup(k,n,m)*Aex(n,m)   ! explicitly remove the trace of tilde A again 
              ENDDO 
           ENDDO        
        ENDDO
     ENDDO
  ENDDO 
  ! 
  DO j=1,3 
     DO i=1,3
        DO k=1,3 
           dtD(k,i,j) = dtD(k,i,j)-alpha*AA(k)*Aex(i,j) !trace removing missing here
           DO l=1,3 
              dtD(k,i,j) = dtD(k,i,j)+BB(k,l)*DD(l,i,j)+DD(k,l,i)*BB(j,l)+DD(k,l,j)*BB(i,l)-2.0D0/3.0D0*DD(k,i,j)*BB(l,l) 
           ENDDO 
        ENDDO
     ENDDO
  ENDDO         
  !
  dtD = dtD+beta(1)*dDD(1,:,:,:)+beta(2)*dDD(2,:,:,:)+beta(3)*dDD(3,:,:,:)
  !
  dtP = MATMUL(BB,PP)+beta(1)*dPP(1,:)+beta(2)*dPP(2,:)+beta(3)*dPP(3,:)    
  DO k=1,3 
     dtP(k) = dtP(k)+1.0D0/3.0D0*alpha*dtraceK(k)+1.0D0/3.0D0*alpha*AA(k)*traceK+sk*1.0D0/3.0D0*alpha*(SUM(g_contr(:,:)*dAex(k,:,:))+SUM(dgup(k,:,:)*Aex(:,:)))  
     DO i=1,3 
        dtP(k) = dtP(k)-1.0D0/3.0D0*0.5D0*(dBB(k,i,i)+dBB(i,k,i)) 
     ENDDO
  ENDDO 
  !
  BgradQ(1:6)   = (/dtgamma(1,1),dtgamma(1,2),dtgamma(1,3),dtgamma(2,2),dtgamma(2,3),dtgamma(3,3)/)       ! \tilde \gamma_ij 
  BgradQ(7:12)  = (/dtK(1,1),dtK(1,2),dtK(1,3),dtK(2,2),dtK(2,3),dtK(3,3)/)                               ! \tilde A_ij 
  BgradQ(13)    = dtTheta                                                                                 ! Theta       
  BgradQ(14:16) = dtGhat(1:3)                                                                             ! \hat \Gamma^i           
  BgradQ(17)    = dtalpha                                                                                 ! log alpha 
  BgradQ(18:20) = dtbeta                                                                                  ! beta^i 
  BgradQ(21:23) = dtbb                                                                                    ! b^i 
  BgradQ(24:26) = dtA(1:3)                                                                                ! A_k       
  BgradQ(27:35) = (/dtB(1,1),dtB(2,1),dtB(3,1),dtB(1,2),dtB(2,2),dtB(3,2),dtB(1,3),dtB(2,3),dtB(3,3)/)    ! B_k^i 
  BgradQ(36:41) = (/dtD(1,1,1),dtD(1,1,2),dtD(1,1,3),dtD(1,2,2),dtD(1,2,3),dtD(1,3,3)/)                   ! D_kij 
  BgradQ(42:47) = (/dtD(2,1,1),dtD(2,1,2),dtD(2,1,3),dtD(2,2,2),dtD(2,2,3),dtD(2,3,3)/)                   ! D_kij 
  BgradQ(48:53) = (/dtD(3,1,1),dtD(3,1,2),dtD(3,1,3),dtD(3,2,2),dtD(3,2,3),dtD(3,3,3)/)                   ! D_kij 
  BgradQ(54)    = dtTraceK                                                                                ! traceK 
  BgradQ(55)    = dtphi                                                                                   ! log phi 
  BgradQ(56:58) = dtP                                                                                     ! P_k 
  BgradQ(59)    = 0.0D0
  !
  Src_BgradQ = BgradQ    ! here, we do not have to change sign, since we work on the right hand side in the fused subroutine 
  !
  RETURN
  !
#endif 
  !            
END SUBROUTINE PDEFusedSrcNCP 

#endif
