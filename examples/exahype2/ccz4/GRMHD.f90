#if defined(GRMHD) || defined(CCZ4GRMHD) || defined(CCZ4GRHD) 

!
! Conservative part of the PDE ( flux tensor F(Q) ) 
!
RECURSIVE SUBROUTINE PDEFluxGRMHD(F,Q)
    USE MainVariables, ONLY : d, EQN 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    ! Argument list 
    REAL, INTENT(IN)  :: Q(nVar) 
    REAL, INTENT(OUT) :: F(nVar,d) 
    ! Local variables 
    INTEGER :: iErr 
    INTEGER :: i,j,k,m
    REAL :: V(nVar) 
    REAL :: p, irho, lam, mu 
    REAL :: k1, k2, fff, ggg, e, ds, c, xi, sk, sknl, alpha, fa, k0, beta0(3)  
    REAL :: gamma1, rho, vx, vy, vz, bx, by, bz, ex, ey, ez, v2, b2, e2, lf, w, ww, uem, wwx, wwy, wwz 
    REAL :: gp, gm, g_cov(3,3), g_contr(3,3), vxB_cov(3), vxB_contr(3), B_cov(3), BQ(3), vtr(3), v_contr(3), lapse, shift(3)    
    REAL :: Fij(3,3), v_cov(3), S_contr(3), QB_contr(3), B_contr(3), psi
    REAL :: rhoeps,dd,tau,s2,sm_cov(3),sm(3)
    !
    F = 0.0 
    !
  CALL PDECons2PrimGRMHD(V,Q,iErr)
  !
  gamma1 = EQN%gamma/(EQN%gamma-1.0)
  rho    = V(1)
  !
  DO i=1,3
	  v_cov(i) = V(1+i)
	  B_contr(i) = V(5+i)  		! B is contravariant
	  QB_contr(i) = Q(5+i)  		! B is contravariant
	  shift(i) = V(10+i)
  ENDDO
  p      = V(5)
  psi = V(9)
  lapse = V(10)
  !
  !gammaij = V(14:19) 
  g_cov(1,1) = V(14)
  g_cov(1,2) = V(15)
  g_cov(1,3) = V(16)
  g_cov(2,2) = V(17)
  g_cov(2,3) = V(18)
  g_cov(3,3) = V(19)
  g_cov(2,1) = V(15)
  g_cov(3,1) = V(16)
  g_cov(3,2) = V(18)
  !
  CALL MatrixInverse3x3(g_cov,g_contr,gp)
  gp = SQRT(gp)
  gm = 1./gp
  ! 
  v_contr = MATMUL(g_contr,v_cov)
  S_contr = MATMUL(g_contr,Q(2:4))
  BQ = MATMUL(g_cov,QB_contr)
  B_cov = MATMUL(g_cov,B_contr)
  ! COVARIANT =>
  vxB_cov(1) = v_contr(2)*B_contr(3) - v_contr(3)*B_contr(2)
  vxB_cov(2) = v_contr(3)*B_contr(1) - v_contr(1)*B_contr(3)
  vxB_cov(3) = v_contr(1)*B_contr(2) - v_contr(2)*B_contr(1) 
  DO i=1,3
	  vxB_cov(i) = gp*vxB_cov(i)
  ENDDO
  !
  ! CONTRAVARIANT =>
  vxB_contr(1) = v_cov(2)*B_cov(3) - v_cov(3)*B_cov(2)
  vxB_contr(2) = v_cov(3)*B_cov(1) - v_cov(1)*B_cov(3)
  vxB_contr(3) = v_cov(1)*B_cov(2) - v_cov(2)*B_cov(1) 
  DO i=1,3
	  vxB_contr(i) = gm*vxB_contr(i)
  ENDDO
  !
  v2     = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
  e2     = vxB_contr(1)*vxB_cov(1) + vxB_contr(2)*vxB_cov(2) + vxB_contr(3)*vxB_cov(3)
  b2     = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
  !
  uem    = 0.5*(b2 + e2) 
  !
  lf     = 1.0/sqrt(1.0 - v2)
  !
  dd = gm*Q(1)
  tau = gm*Q(5)
  DO i=1,3 
      sm_cov(i)  = Q(1+i)
  ENDDO
  sm_cov = gm*sm_cov
  sm   = MATMUL (g_contr, sm_cov)  
  s2   = sm_cov(1)*sm(1) + sm_cov(2)*sm(2) + sm_cov(3)*sm(3)
  !
#ifdef GEOS  
  CALL EOS_eps(rhoeps,rho,p,dd,tau,s2) 
  w      = rho + rhoeps + p   ! rho*enthalpy
#else
  w    = rho + gamma1*p  ! this is rho*h
#endif
  !IF(ABS(w-(rho + gamma1*p)).GT.1e-13) THEN
  !    continue
  !ENDIF
  !
  !w      = rho + gamma1*p
  ww     = w*lf**2
  wwx    = ww*v_contr(1)
  wwy    = ww*v_contr(2)
  wwz    = ww*v_contr(3)
  !
  ! transport velocity
  DO i=1,3
	  Vtr(i) = lapse*v_contr(i)-shift(i)
  ENDDO
  !
  !    Fij(1,1:3) =  (/ f1, g1, h1 /) for Q(6)  
  !    Fij(2,1:3) =  (/ f2, g2, h2 /) for Q(7)  ... without divergence cleaning
  !    Fij(3,1:3) =  (/ f3, g3, h3 /) for Q(8)  
  !
  DO m=1,3
      DO i=1,3
          Fij(i,m) = Vtr(i)*QB_contr(m)-Vtr(m)*QB_contr(i)  ! Fij(i,i) = 0 !!!! ! NB: this is contravariant !!!!
      ENDDO
  ENDDO
  !
  F(1,1)   = v_contr(1)*Q(1) 
  F(2,1)   = wwx*v_cov(1) - vxB_contr(1)*vxB_cov(1) - B_contr(1)*B_cov(1) + p + uem
  F(3,1)   = wwx*v_cov(2) - vxB_contr(1)*vxB_cov(2) - B_contr(1)*B_cov(2) 
  F(4,1)   = wwx*v_cov(3) - vxB_contr(1)*vxB_cov(3) - B_contr(1)*B_cov(3) 
  F(5,1)   = S_contr(1)-F(1,1) 
  ! 
  F(6,1)   = Fij(1,1) !+ V(9)  
  F(7,1)   = Fij(1,2) 
  F(8,1)   = Fij(1,3)  
  !  
#ifdef COVCLEAN     
  F(9,1)   = DivCleaning_a**2*QB_contr(1)   ! flat Newtonian, flat coordinates
#else
  F(9,1)   = 0.  
#endif 
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(i,1) = 0.
  ENDDO
  !
  !
  F(1,2)   = v_contr(2)*Q(1) 
  F(2,2)   = wwy*v_cov(1) - vxB_contr(2)*vxB_cov(1) - B_contr(2)*B_cov(1) 
  F(3,2)   = wwy*v_cov(2) - vxB_contr(2)*vxB_cov(2) - B_contr(2)*B_cov(2) + p + uem
  F(4,2)   = wwy*v_cov(3) - vxB_contr(2)*vxB_cov(3) - B_contr(2)*B_cov(3) 
  F(5,2)   = S_contr(2)-F(1,2)  
  ! 
  F(6,2)   = Fij(2,1)  
  F(7,2)   = Fij(2,2) !+ V(9) 
  F(8,2)   = Fij(2,3)  
  ! 
#ifdef COVCLEAN     
  F(9,2)   = DivCleaning_a**2*QB_contr(2)   
#else
  F(9,2)   = 0. !QB_contr(2) 
#endif
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(i,2) = 0.
  ENDDO
  !
  !
  !
  F(1,3)   = v_contr(3)*Q(1) 
  F(2,3)   = wwz*v_cov(1) - vxB_contr(3)*vxB_cov(1) - B_contr(3)*B_cov(1) 
  F(3,3)   = wwz*v_cov(2) - vxB_contr(3)*vxB_cov(2) - B_contr(3)*B_cov(2)   
  F(4,3)   = wwz*v_cov(3) - vxB_contr(3)*vxB_cov(3) - B_contr(3)*B_cov(3) + p + uem
  F(5,3)   = S_contr(3)-F(1,3) 
  ! 
  F(6,3)   = Fij(3,1)
  F(7,3)   = Fij(3,2)
  F(8,3)   = Fij(3,3) !+ V(9) 
  !
#ifdef COVCLEAN     
  F(9,3)   = DivCleaning_a**2*QB_contr(3)   ! flat Newtonian, flat coordinates
#else
  F(9,3)   = 0. !QB_contr(3) 
#endif 
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(i,3) = 0.
  ENDDO
  ! 
  ! - - - - - - - - - 
  DO i=2,4
	  F(i,1)   = F(i,1)*gp
	  F(i,2)   = F(i,2)*gp
	  F(i,3)   = F(i,3)*gp
  ENDDO
  ! Remember that Q(:) below contains already the factor gp, which is ok!
  
  DO i=1,5
	  F(i,1)   = lapse*F(i,1) - shift(1)*Q(i)
	  F(i,2)   = lapse*F(i,2) - shift(2)*Q(i)
	  F(i,3)   = lapse*F(i,3) - shift(3)*Q(i)
  ENDDO 
  !
#ifdef COVCLEAN   
  F(9,1)   = lapse*F(9,1) - shift(1)*Q(9)
  F(9,2)   = lapse*F(9,1) - shift(2)*Q(9)
  F(9,3)   = lapse*F(9,1) - shift(3)*Q(9)
#endif 
  !
  CONTINUE      
  !   
END SUBROUTINE PDEFluxGRMHD
!
    
!
! Conservative part of the PDE ( flux tensor F(Q) ) 
!
RECURSIVE SUBROUTINE PDEFluxPrimGRMHD(F,V,Q)
    USE MainVariables, ONLY : d, EQN 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
    INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
    INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
    INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    ! Argument list 
    REAL, INTENT(IN)  :: Q(nVar), V(nVar) 
    REAL, INTENT(OUT) :: F(nVar,d) 
    ! Local variables 
    INTEGER :: iErr 
    INTEGER :: i,j,k,m 
    REAL :: p, irho, lam, mu 
    REAL :: k1, k2, fff, ggg, e, ds, c, xi, sk, sknl, alpha, fa, k0, beta0(3)  
    REAL :: gamma1, rho, vx, vy, vz, bx, by, bz, ex, ey, ez, v2, b2, e2, lf, w, ww, uem, wwx, wwy, wwz 
    REAL :: gp, gm, g_cov(3,3), g_contr(3,3), vxB_cov(3), vxB_contr(3), B_cov(3), BQ(3), vtr(3), v_contr(3), lapse, shift(3)    
    REAL :: Fij(3,3), v_cov(3), S_contr(3), QB_contr(3), B_contr(3), psi
    REAL :: rhoeps,dd,tau,s2,sm_cov(3),sm(3)
    !
    F = 0.0 
    !
  gamma1 = EQN%gamma/(EQN%gamma-1.0)
  rho    = V(1)
  !
  DO i=1,3
	  v_cov(i) = V(1+i)
	  B_contr(i) = V(5+i)  		! B is contravariant
	  QB_contr(i) = Q(5+i)  		! B is contravariant
	  shift(i) = V(10+i)
  ENDDO
  p      = V(5)
  psi = V(9)
  lapse = V(10)
  !
  !gammaij = V(14:19) 
  g_cov(1,1) = V(14)
  g_cov(1,2) = V(15)
  g_cov(1,3) = V(16)
  g_cov(2,2) = V(17)
  g_cov(2,3) = V(18)
  g_cov(3,3) = V(19)
  g_cov(2,1) = V(15)
  g_cov(3,1) = V(16)
  g_cov(3,2) = V(18)
  !
  CALL MatrixInverse3x3(g_cov,g_contr,gp)
  gp = SQRT(gp)
  gm = 1./gp
  ! 
  v_contr = MATMUL(g_contr,v_cov)
  S_contr = MATMUL(g_contr,Q(2:4))
  BQ = MATMUL(g_cov,QB_contr)
  B_cov = MATMUL(g_cov,B_contr)
  ! COVARIANT =>
  vxB_cov(1) = v_contr(2)*B_contr(3) - v_contr(3)*B_contr(2)
  vxB_cov(2) = v_contr(3)*B_contr(1) - v_contr(1)*B_contr(3)
  vxB_cov(3) = v_contr(1)*B_contr(2) - v_contr(2)*B_contr(1) 
  DO i=1,3
	  vxB_cov(i) = gp*vxB_cov(i)
  ENDDO
  !
  ! CONTRAVARIANT =>
  vxB_contr(1) = v_cov(2)*B_cov(3) - v_cov(3)*B_cov(2)
  vxB_contr(2) = v_cov(3)*B_cov(1) - v_cov(1)*B_cov(3)
  vxB_contr(3) = v_cov(1)*B_cov(2) - v_cov(2)*B_cov(1) 
  DO i=1,3
	  vxB_contr(i) = gm*vxB_contr(i)
  ENDDO
  !
  v2     = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
  e2     = vxB_contr(1)*vxB_cov(1) + vxB_contr(2)*vxB_cov(2) + vxB_contr(3)*vxB_cov(3)
  b2     = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
  !
  uem    = 0.5*(b2 + e2) 
  !
  lf     = 1.0/sqrt(1.0 - v2)
  !
  dd = gm*Q(1)
  tau = gm*Q(5)
  DO i=1,3 
      sm_cov(i)  = Q(1+i)
  ENDDO
  sm_cov = gm*sm_cov
  sm   = MATMUL (g_contr, sm_cov)  
  s2   = sm_cov(1)*sm(1) + sm_cov(2)*sm(2) + sm_cov(3)*sm(3)
  !
#ifdef GEOS  
  CALL EOS_eps(rhoeps,rho,p,dd,tau,s2) 
  w      = rho + rhoeps + p   ! rho*enthalpy
#else
  w    = rho + gamma1*p  ! this is rho*h
#endif
  !IF(ABS(w-(rho + gamma1*p)).GT.1e-13) THEN
  !    continue
  !ENDIF
  !
  !w      = rho + gamma1*p
  ww     = w*lf**2
  wwx    = ww*v_contr(1)
  wwy    = ww*v_contr(2)
  wwz    = ww*v_contr(3)
  !
  ! transport velocity
  DO i=1,3
	  Vtr(i) = lapse*v_contr(i)-shift(i)
  ENDDO
  !
  !    Fij(1,1:3) =  (/ f1, g1, h1 /) for Q(6)  
  !    Fij(2,1:3) =  (/ f2, g2, h2 /) for Q(7)  ... without divergence cleaning
  !    Fij(3,1:3) =  (/ f3, g3, h3 /) for Q(8)  
  !
  DO m=1,3
      DO i=1,3
          Fij(i,m) = Vtr(i)*QB_contr(m)-Vtr(m)*QB_contr(i)  ! Fij(i,i) = 0 !!!! ! NB: this is contravariant !!!!
      ENDDO
  ENDDO
  !
  F(1,1)   = v_contr(1)*Q(1) 
  F(2,1)   = wwx*v_cov(1) - vxB_contr(1)*vxB_cov(1) - B_contr(1)*B_cov(1) + p + uem
  F(3,1)   = wwx*v_cov(2) - vxB_contr(1)*vxB_cov(2) - B_contr(1)*B_cov(2) 
  F(4,1)   = wwx*v_cov(3) - vxB_contr(1)*vxB_cov(3) - B_contr(1)*B_cov(3) 
  F(5,1)   = S_contr(1)-F(1,1) 
  ! 
  F(6,1)   = Fij(1,1) !+ V(9)  
  F(7,1)   = Fij(1,2) 
  F(8,1)   = Fij(1,3)  
  !  
#ifdef COVCLEAN     
  F(9,1)   = DivCleaning_a**2*QB_contr(1)   ! flat Newtonian, flat coordinates
#else
  F(9,1)   = 0.  
#endif 
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(i,1) = 0.
  ENDDO
  !
  !
  F(1,2)   = v_contr(2)*Q(1) 
  F(2,2)   = wwy*v_cov(1) - vxB_contr(2)*vxB_cov(1) - B_contr(2)*B_cov(1) 
  F(3,2)   = wwy*v_cov(2) - vxB_contr(2)*vxB_cov(2) - B_contr(2)*B_cov(2) + p + uem
  F(4,2)   = wwy*v_cov(3) - vxB_contr(2)*vxB_cov(3) - B_contr(2)*B_cov(3) 
  F(5,2)   = S_contr(2)-F(1,2)  
  ! 
  F(6,2)   = Fij(2,1)  
  F(7,2)   = Fij(2,2) !+ V(9) 
  F(8,2)   = Fij(2,3)  
  ! 
#ifdef COVCLEAN     
  F(9,2)   = DivCleaning_a**2*QB_contr(2)   
#else
  F(9,2)   = 0. !QB_contr(2) 
#endif
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(i,2) = 0.
  ENDDO
  !
  !
  !
  F(1,3)   = v_contr(3)*Q(1) 
  F(2,3)   = wwz*v_cov(1) - vxB_contr(3)*vxB_cov(1) - B_contr(3)*B_cov(1) 
  F(3,3)   = wwz*v_cov(2) - vxB_contr(3)*vxB_cov(2) - B_contr(3)*B_cov(2)   
  F(4,3)   = wwz*v_cov(3) - vxB_contr(3)*vxB_cov(3) - B_contr(3)*B_cov(3) + p + uem
  F(5,3)   = S_contr(3)-F(1,3) 
  ! 
  F(6,3)   = Fij(3,1)
  F(7,3)   = Fij(3,2)
  F(8,3)   = Fij(3,3) !+ V(9) 
  !
#ifdef COVCLEAN     
  F(9,3)   = DivCleaning_a**2*QB_contr(3)   ! flat Newtonian, flat coordinates
#else
  F(9,3)   = 0. !QB_contr(3) 
#endif 
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(i,3) = 0.
  ENDDO
  ! 
  ! - - - - - - - - - 
  DO i=2,4
	  F(i,1)   = F(i,1)*gp
	  F(i,2)   = F(i,2)*gp
	  F(i,3)   = F(i,3)*gp
  ENDDO
  ! Remember that Q(:) below contains already the factor gp, which is ok!
  
  DO i=1,5
	  F(i,1)   = lapse*F(i,1) - shift(1)*Q(i)
	  F(i,2)   = lapse*F(i,2) - shift(2)*Q(i)
	  F(i,3)   = lapse*F(i,3) - shift(3)*Q(i)
  ENDDO 
  !
#ifdef COVCLEAN   
  F(9,1)   = lapse*F(9,1) - shift(1)*Q(9)
  F(9,2)   = lapse*F(9,1) - shift(2)*Q(9)
  F(9,3)   = lapse*F(9,1) - shift(3)*Q(9)
#endif 
  !
  CONTINUE      
  !   
END SUBROUTINE PDEFluxPrimGRMHD
!
#ifdef VECTOR 

RECURSIVE SUBROUTINE PDEFluxPrimVectorGRMHD(F,V,Q)
    USE MainVariables, ONLY : d, EQN, VECTORLENGTH  
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    ! Argument list 
    REAL(8), INTENT(IN)  :: V(VECTORLENGTH,nVar),  Q(VECTORLENGTH,nVar), par(VECTORLENGTH,nParam)  
    REAL(8), INTENT(OUT) :: F(VECTORLENGTH,nVar,d) 
    ! Local variables 
    INTEGER :: iErr 
    INTEGER :: i,j,k,m 
    REAL(8) :: p(VECTORLENGTH),rhoeps(VECTORLENGTH),DD(VECTORLENGTH),tau(VECTORLENGTH),s2(VECTORLENGTH)
    REAL(8) :: e(VECTORLENGTH), ds(VECTORLENGTH), c(VECTORLENGTH), xi(VECTORLENGTH), sk(VECTORLENGTH), sknl(VECTORLENGTH)  
    REAL(8) :: alpha(VECTORLENGTH), fa(VECTORLENGTH), k0(VECTORLENGTH), beta0(VECTORLENGTH,3), det(VECTORLENGTH), idet(VECTORLENGTH)  
    REAL(8) :: gamma1(VECTORLENGTH), rho(VECTORLENGTH), vx(VECTORLENGTH), vy(VECTORLENGTH), vz(VECTORLENGTH), bx(VECTORLENGTH)
    REAL(8) :: by(VECTORLENGTH), bz(VECTORLENGTH), ex(VECTORLENGTH), ey(VECTORLENGTH), ez(VECTORLENGTH), v2(VECTORLENGTH), b2(VECTORLENGTH)
    REAL(8) :: e2(VECTORLENGTH), lf(VECTORLENGTH), w(VECTORLENGTH), ww(VECTORLENGTH), uem(VECTORLENGTH), wwx(VECTORLENGTH), wwy(VECTORLENGTH), wwz(VECTORLENGTH)  
    REAL(8) :: gp(VECTORLENGTH), gm(VECTORLENGTH), g_cov(VECTORLENGTH,3,3), g_contr(VECTORLENGTH,3,3), vxB_cov(VECTORLENGTH,3), vxB_contr(VECTORLENGTH,3), B_cov(VECTORLENGTH,3),vtr(VECTORLENGTH,3), v_contr(VECTORLENGTH,3), lapse(VECTORLENGTH), shift(VECTORLENGTH,3)    
    REAL(8) :: Fij(VECTORLENGTH,3,3),v_cov(VECTORLENGTH,3), S_contr(VECTORLENGTH,3), gS_contr(VECTORLENGTH,3), S_cov(VECTORLENGTH,3), QB_contr(VECTORLENGTH,3), B_contr(VECTORLENGTH,3), psi(VECTORLENGTH)  
#ifdef AVX512
    !dir$ attributes align:  64 :: p,e,ds,c,xi,sk,sknl,alpha,fa,k0,beta0,det,idet,gamma1,rho,vx,vy,vz,bx,by,bz,ex,ey,ez,v2,b2,e2,lf,w,ww,uem,wwx,wwy,wwz
    !dir$ attributes align:  64 :: gp,gm,g_cov,g_contr,vxB_cov,vxB_contr, B_cov, vtr, v_contr, lapse, shift, Fij, v_cov, S_contr,  gS_contr,  S_cov, QB_contr, B_contr, psi,rhoeps,DD,tau,s2  
    !DIR$ ASSUME_ALIGNED F : 64  
    !DIR$ ASSUME_ALIGNED V : 64  
    !DIR$ ASSUME_ALIGNED Q : 64  
    !DIR$ ASSUME_ALIGNED par : 64  
#else
    !dir$ attributes align:  32 :: p,e,ds,c,xi,sk,sknl,alpha,fa,k0,beta0,det,idet,gamma1,rho,vx,vy,vz,bx,by,bz,ex,ey,ez,v2,b2,e2,lf,w,ww,uem,wwx,wwy,wwz
    !dir$ attributes align:  32 :: gp,gm,g_cov,g_contr,vxB_cov,vxB_contr,B_cov,vtr,v_contr,lapse,shift,Fij,v_cov,S_contr, gS_contr, S_cov,QB_contr,B_contr,psi,rhoeps,DD,tau,s2 
    !DIR$ ASSUME_ALIGNED F : 32 
    !DIR$ ASSUME_ALIGNED V : 32 
    !DIR$ ASSUME_ALIGNED Q : 32 
    !DIR$ ASSUME_ALIGNED par : 32 
#endif 
  !
  gamma1 = EQN%gamma/(EQN%gamma-1.0)
  rho    = V(:,1)
  !
  DO i=1,3
	  v_cov(:,i)    = V(:,1+i)
	  B_contr(:,i)  = V(:,5+i)  		! B is contravariant
	  QB_contr(:,i) = Q(:,5+i)  		! B is contravariant
	  shift(:,i)    = V(:,10+i)
  ENDDO
  !
  p(:)     = V(:,5)
  psi(:)   = V(:,9)
  lapse(:) = V(:,10)
  !
  !gammaij = V(14:19) 
  g_cov(:,1,1) = V(:,14)
  g_cov(:,1,2) = V(:,15)
  g_cov(:,1,3) = V(:,16)
  g_cov(:,2,2) = V(:,17)
  g_cov(:,2,3) = V(:,18)
  g_cov(:,3,3) = V(:,19)
  g_cov(:,2,1) = V(:,15)
  g_cov(:,3,1) = V(:,16)
  g_cov(:,3,2) = V(:,18)
  !
  det(:) = (g_cov(:,1,1)*g_cov(:,2,2)*g_cov(:,3,3)-g_cov(:,1,1)*g_cov(:,2,3)**2-g_cov(:,1,2)**2*g_cov(:,3,3)+2*g_cov(:,1,2)*g_cov(:,1,3)*g_cov(:,2,3)-g_cov(:,1,3)**2*g_cov(:,2,2))  
  idet(:) = 1./det(:)  
  !
  g_contr(:,1,1) =  ( g_cov(:,2,2)*g_cov(:,3,3)-g_cov(:,2,3)*g_cov(:,3,2))*idet(:) 
  g_contr(:,1,2) = -( g_cov(:,1,2)*g_cov(:,3,3)-g_cov(:,1,3)*g_cov(:,3,2))*idet(:)
  g_contr(:,1,3) = -(-g_cov(:,1,2)*g_cov(:,2,3)+g_cov(:,1,3)*g_cov(:,2,2))*idet(:) 
  g_contr(:,2,1) = -( g_cov(:,2,1)*g_cov(:,3,3)-g_cov(:,2,3)*g_cov(:,3,1))*idet(:) 
  g_contr(:,2,2) =  ( g_cov(:,1,1)*g_cov(:,3,3)-g_cov(:,1,3)*g_cov(:,3,1))*idet(:) 
  g_contr(:,2,3) = -( g_cov(:,1,1)*g_cov(:,2,3)-g_cov(:,1,3)*g_cov(:,2,1))*idet(:) 
  g_contr(:,3,1) = -(-g_cov(:,2,1)*g_cov(:,3,2)+g_cov(:,2,2)*g_cov(:,3,1))*idet(:) 
  g_contr(:,3,2) = -( g_cov(:,1,1)*g_cov(:,3,2)-g_cov(:,1,2)*g_cov(:,3,1))*idet(:) 
  g_contr(:,3,3) =  ( g_cov(:,1,1)*g_cov(:,2,2)-g_cov(:,1,2)*g_cov(:,2,1))*idet(:)       
  !
  gp(:) = SQRT(det)
  gm(:) = 1./gp(:) 
  !
  DD(:)         = Q(:,1)*gm(:) 
  S_cov(:,1)    = Q(:,2)*gm(:)
  S_cov(:,2)    = Q(:,3)*gm(:)
  S_cov(:,3)    = Q(:,4)*gm(:)
  tau(:)        = Q(:,5)*gm(:)
  !
  !v_contr = MATMUL(g_contr,v_cov)
  !S_contr = MATMUL(g_contr,S_cov)
  !B_cov = MATMUL(g_cov,B_contr)
  ! 
  v_contr(:,1) = g_contr(:,1,1)*v_cov(:,1) + g_contr(:,1,2)*v_cov(:,2) + g_contr(:,1,3)*v_cov(:,3) 
  v_contr(:,2) = g_contr(:,2,1)*v_cov(:,1) + g_contr(:,2,2)*v_cov(:,2) + g_contr(:,2,3)*v_cov(:,3) 
  v_contr(:,3) = g_contr(:,3,1)*v_cov(:,1) + g_contr(:,3,2)*v_cov(:,2) + g_contr(:,3,3)*v_cov(:,3) 
  gS_contr(:,1) = g_contr(:,1,1)*Q(:,2) + g_contr(:,1,2)*Q(:,3) + g_contr(:,1,3)*Q(:,4) 
  gS_contr(:,2) = g_contr(:,2,1)*Q(:,2) + g_contr(:,2,2)*Q(:,3) + g_contr(:,2,3)*Q(:,4) 
  gS_contr(:,3) = g_contr(:,3,1)*Q(:,2) + g_contr(:,3,2)*Q(:,3) + g_contr(:,3,3)*Q(:,4) 
  S_contr(:,1) = g_contr(:,1,1)*S_cov(:,1) + g_contr(:,1,2)*S_cov(:,2) + g_contr(:,1,3)*S_cov(:,3) 
  S_contr(:,2) = g_contr(:,2,1)*S_cov(:,1) + g_contr(:,2,2)*S_cov(:,2) + g_contr(:,2,3)*S_cov(:,3) 
  S_contr(:,3) = g_contr(:,3,1)*S_cov(:,1) + g_contr(:,3,2)*S_cov(:,2) + g_contr(:,3,3)*S_cov(:,3) 
  !   
  ! COVARIANT =>
  vxB_cov(:,1) = gp(:)*( v_contr(:,2)*B_contr(:,3) - v_contr(:,3)*B_contr(:,2) )
  vxB_cov(:,2) = gp(:)*( v_contr(:,3)*B_contr(:,1) - v_contr(:,1)*B_contr(:,3) )
  vxB_cov(:,3) = gp(:)*( v_contr(:,1)*B_contr(:,2) - v_contr(:,2)*B_contr(:,1) )
  !
  ! CONTRAVARIANT =>
  vxB_contr(:,1) = gm(:)*( v_cov(:,2)*B_cov(:,3) - v_cov(:,3)*B_cov(:,2) ) 
  vxB_contr(:,2) = gm(:)*( v_cov(:,3)*B_cov(:,1) - v_cov(:,1)*B_cov(:,3) ) 
  vxB_contr(:,3) = gm(:)*( v_cov(:,1)*B_cov(:,2) - v_cov(:,2)*B_cov(:,1) ) 
  !
  v2(:) = v_contr(:,1)*v_cov(:,1) + v_contr(:,2)*v_cov(:,2) + v_contr(:,3)*v_cov(:,3)
  e2(:) = vxB_contr(:,1)*vxB_cov(:,1) + vxB_contr(:,2)*vxB_cov(:,2) + vxB_contr(:,3)*vxB_cov(:,3)
  b2(:) = B_contr(:,1)*B_cov(:,1) + B_contr(:,2)*B_cov(:,2) + B_contr(:,3)*B_cov(:,3)
  s2(:) = S_contr(:,1)*S_cov(:,1) + S_contr(:,2)*S_cov(:,2) + S_contr(:,3)*S_cov(:,3)
  !
  uem(:) = 0.5*(b2(:)+e2(:))
  lf(:)  = 1.0/SQRT(1.0-v2(:))
  ! 
#ifdef GEOS  
  CALL EOS_eps_vector(rhoeps(:),rho(:),p(:),DD(:),tau(:),s2(:))
  w(:) = rho(:) + rhoeps(:) + p(:)          ! rho*enthalpy
#else
  w(:)   = rho(:) + gamma1(:)*p(:)
#endif
!
  !IF(SQRT(SUM((w-(rho + gamma1*p))**2)).GT.1e-10) THEN
  !    continue
  !ENDIF
  !
  ! 
  ww(:)  = w(:)*lf(:)**2
  wwx(:) = ww(:)*v_contr(:,1)
  wwy(:) = ww(:)*v_contr(:,2)
  wwz(:) = ww(:)*v_contr(:,3)
  !
  ! transport velocity
  DO i=1,3
	  Vtr(:,i) = lapse(:)*v_contr(:,i)-shift(:,i)
  ENDDO
  !
  !    Fij(1,1:3) =  (/ f1, g1, h1 /) for Q(6)  
  !    Fij(2,1:3) =  (/ f2, g2, h2 /) for Q(7)  ... without divergence cleaning
  !    Fij(3,1:3) =  (/ f3, g3, h3 /) for Q(8)  
  !
  DO m=1,3
      DO i=1,3
          Fij(:,i,m) = Vtr(:,i)*QB_contr(:,m)-Vtr(:,m)*QB_contr(:,i)  ! Fij(i,i) = 0 !!!! ! NB: this is contravariant !!!!
      ENDDO
  ENDDO
  !
  F(:,1,1)   = v_contr(:,1)*Q(:,1) 
  F(:,2,1)   = wwx(:)*v_cov(:,1) - vxB_contr(:,1)*vxB_cov(:,1) - B_contr(:,1)*B_cov(:,1) + p(:) + uem(:) 
  F(:,3,1)   = wwx(:)*v_cov(:,2) - vxB_contr(:,1)*vxB_cov(:,2) - B_contr(:,1)*B_cov(:,2) 
  F(:,4,1)   = wwx(:)*v_cov(:,3) - vxB_contr(:,1)*vxB_cov(:,3) - B_contr(:,1)*B_cov(:,3) 
  F(:,5,1)   = gS_contr(:,1)-F(:,1,1) 
  ! 
  F(:,6,1)   = Fij(:,1,1) !+ V(9)  
  F(:,7,1)   = Fij(:,1,2) 
  F(:,8,1)   = Fij(:,1,3)  
  !  
#ifdef COVCLEAN     
  F(:,9,1)   = DivCleaning_a**2*QB_contr(:,1)   ! flat Newtonian, flat coordinates
#else
  F(:,9,1)   = 0.  
#endif 
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(:,i,1) = 0.
  ENDDO
  !
  !
  F(:,1,2)   = v_contr(:,2)*Q(:,1) 
  F(:,2,2)   = wwy(:)*v_cov(:,1) - vxB_contr(:,2)*vxB_cov(:,1) - B_contr(:,2)*B_cov(:,1) 
  F(:,3,2)   = wwy(:)*v_cov(:,2) - vxB_contr(:,2)*vxB_cov(:,2) - B_contr(:,2)*B_cov(:,2) + p(:) + uem(:) 
  F(:,4,2)   = wwy(:)*v_cov(:,3) - vxB_contr(:,2)*vxB_cov(:,3) - B_contr(:,2)*B_cov(:,3) 
  F(:,5,2)   = gS_contr(:,2)-F(:,1,2)  
  ! 
  F(:,6,2)   = Fij(:,2,1)  
  F(:,7,2)   = Fij(:,2,2) !+ V(9) 
  F(:,8,2)   = Fij(:,2,3)  
  ! 
#ifdef COVCLEAN     
  F(:,9,2)   = DivCleaning_a**2*QB_contr(:,2)   
#else
  F(:,9,2)   = 0. !QB_contr(2) 
#endif
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(:,i,2) = 0.
  ENDDO
  !
  !
  !
  F(:,1,3)   = v_contr(:,3)*Q(:,1) 
  F(:,2,3)   = wwz(:)*v_cov(:,1) - vxB_contr(:,3)*vxB_cov(:,1) - B_contr(:,3)*B_cov(:,1) 
  F(:,3,3)   = wwz(:)*v_cov(:,2) - vxB_contr(:,3)*vxB_cov(:,2) - B_contr(:,3)*B_cov(:,2)   
  F(:,4,3)   = wwz(:)*v_cov(:,3) - vxB_contr(:,3)*vxB_cov(:,3) - B_contr(:,3)*B_cov(:,3) + p(:) + uem(:) 
  F(:,5,3)   = gS_contr(:,3)-F(:,1,3) 
  ! 
  F(:,6,3)   = Fij(:,3,1)
  F(:,7,3)   = Fij(:,3,2)
  F(:,8,3)   = Fij(:,3,3) !+ V(9) 
  !
#ifdef COVCLEAN     
  F(:,9,3)   = DivCleaning_a**2*QB_contr(:,3)   ! flat Newtonian, flat coordinates
#else
  F(:,9,3)   = 0. !QB_contr(3) 
#endif 
  !  lapse&shift&metric fluxes 
  DO i=10,nVar
	F(:,i,3) = 0.
  ENDDO
  ! 
  ! - - - - - - - - - 
  DO i=2,4
	  F(:,i,1)   = F(:,i,1)*gp(:) 
	  F(:,i,2)   = F(:,i,2)*gp(:) 
	  F(:,i,3)   = F(:,i,3)*gp(:) 
  ENDDO
  ! Remember that Q(:) below contains already the factor gp, which is ok!
  
  DO i=1,5
	  F(:,i,1)   = lapse(:)*F(:,i,1) - shift(:,1)*Q(:,i)
	  F(:,i,2)   = lapse(:)*F(:,i,2) - shift(:,2)*Q(:,i)
	  F(:,i,3)   = lapse(:)*F(:,i,3) - shift(:,3)*Q(:,i)
  ENDDO 
  !
#ifdef COVCLEAN   
  F(:,9,1)   = lapse(:)*F(:,9,1) - shift(:,1)*Q(:,9)
  F(:,9,2)   = lapse(:)*F(:,9,1) - shift(:,2)*Q(:,9)
  F(:,9,3)   = lapse(:)*F(:,9,1) - shift(:,3)*Q(:,9)
#endif 
  !
  !
  CONTINUE    
    !  
  !
END SUBROUTINE PDEFluxPrimVectorGRMHD           
!
#endif 
!    
! Nonconservative part of the PDE ( B(Q) * gradQ ) 
!    
RECURSIVE SUBROUTINE PDENCPGRMHD(BgradQ,Q,gradQ)
    USE MainVariables, ONLY : d, EQN 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
  ! Argument list 
  REAL, INTENT(IN)  :: Q(nVar), gradQ(nVar,d) 
  REAL, INTENT(OUT) :: BgradQ(nVar) 
  ! Local variables 
  REAL :: p, irho
  REAL :: rhoeps,dd,tau,s2,sm_cov(3),sm(3)
  REAL :: Qx(nVar), Qy(nVar), Qz(nVar) 
  REAL :: e,g_cov(3,3),g_contr(3,3)
  REAL :: AQx(nVar), BQy(nVar), CQz(nVar) 
  REAL :: lapse, shift(3), gammaij(6), delta(3,3), B_cov(3), vxB_cov(3), vxb_contr(3), psi, S_contr(3), qb_contr(3), B_contr(3) 
  REAL :: v2,v_contr(3),uem,b2,e2,gp,gm,vc(nVar),lf,w,ww,gamma1,rho,v_cov(3), w_ij, wim    
  INTEGER :: i,j,k,l,m,iErr, ccount    
  !
  !BgradQ = 0.0 
  DO i=1,nVar
  	Qx(i) = gradQ(i,1) 
  	Qy(i) = gradQ(i,2)
  	Qz(i) = gradQ(i,3)
  	AQx(i) = 0.
  	BQy(i) = 0.
  	CQz(i) = 0.
  ENDDO
  !
  lapse = Q(10)
  !
  shift(1) = Q(11)
  shift(2) = Q(12)
  shift(3) = Q(13)
  !
  DO i=1,6
  	gammaij(i) = Q(13+i) 
  ENDDO
  g_cov(1,1) = Q(14)
  g_cov(1,2) = Q(15)
  g_cov(1,3) = Q(16)
  g_cov(2,2) = Q(17)
  g_cov(2,3) = Q(18)
  g_cov(3,3) = Q(19)
  g_cov(2,1) = Q(15)
  g_cov(3,1) = Q(16)
  g_cov(3,2) = Q(18)
  !
  DO i=1,3
    DO j=1,3
  	  delta(i,j) = 0.0
    ENDDO 
  ENDDO 
  DO i=1,3
     delta(i,i) = 1.0 
  ENDDO 
  !
  CALL MatrixInverse3x3(g_cov,g_contr,gp)
  gp = SQRT(gp)
  gm = 1./gp
  ! 
  CALL PDECons2PrimGRMHD(Vc,Q,iErr)
  gamma1 = EQN%gamma/(EQN%gamma-1.0)
  rho    = Vc(1)
  v_cov = Vc(2:4)
  p      = Vc(5)
  !
  !B_cov(1:3) = Vc(6:8)
  DO i=1,3
    B_contr(i) = Vc(5+i) ! contravariant!
    QB_contr(i) = Q(5+i)
  ENDDO
  !
  B_cov = MATMUL(g_cov,B_contr)
  psi = Vc(9) 
  ! 
  v_contr       = MATMUL(g_contr,v_cov)
  S_contr = MATMUL(g_contr,Q(2:4))
  !
  !! COVARIANT =>
  vxB_cov(1) = v_contr(2)*B_contr(3) - v_contr(3)*B_contr(2)
  vxB_cov(2) = v_contr(3)*B_contr(1) - v_contr(1)*B_contr(3)
  vxB_cov(3) = v_contr(1)*B_contr(2) - v_contr(2)*B_contr(1) 
  DO i=1,3
    vxB_cov(i) = gp*vxB_cov(i)
  ENDDO
  !
  ! CONTRAVARIANT =>
  vxB_contr(1) = v_cov(2)*B_cov(3) - v_cov(3)*B_cov(2)
  vxB_contr(2) = v_cov(3)*B_cov(1) - v_cov(1)*B_cov(3)
  vxB_contr(3) = v_cov(1)*B_cov(2) - v_cov(2)*B_cov(1)
  DO i=1,3
  	vxB_contr(i) = gm*vxB_contr(i)
  ENDDO
  !v2     = vx**2 + vy**2 + vz**2
  !b2     = bx**2 + by**2 + bz**2
  !e2     = ex**2 + ey**2 + ez**2 
  v2     = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
  e2     = vxB_contr(1)*vxB_cov(1) + vxB_contr(2)*vxB_cov(2) + vxB_contr(3)*vxB_cov(3)
  b2     = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
  !
  uem    = 0.5*(b2 + e2) 
  !
   S_contr = MATMUL(g_contr,Q(2:4))
  !
  lf     = 1.0/sqrt(1.0 - v2)
  !
  dd = gm*Q(1)
  tau = gm*Q(5)
  DO i=1,3 
      sm_cov(i)  = Q(1+i)
  ENDDO
  sm_cov = gm*sm_cov
  sm   = MATMUL (g_contr, sm_cov)  
  s2   = sm_cov(1)*sm(1) + sm_cov(2)*sm(2) + sm_cov(3)*sm(3)
  !
#ifdef GEOS  
  CALL EOS_eps(rhoeps,rho,p,dd,tau,s2) 
  w      = rho + rhoeps + p   ! rho*enthalpy
#else
  w   = rho + gamma1*p
#endif
!
  !IF(ABS(w-(rho + gamma1*p)).GT.1e-13) THEN
  !    continue
  !ENDIF
  !
  !w      = rho + gamma1*p   ! rho*hentalpy
  ww     = w*lf**2          ! rho*hentalpy*Lorentz^2 
  !
	ccount=0
      DO i=1,3 
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=1
                !------ 
			W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
                !
                AQx(1+j) = AQx(1+j) - Q(1+i)*Qx(10+i)  ! Q(11:13)  shift(i) or shift_contr(i)
                AQx(5) = AQx(5) - gp*W_ij*Qx(10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=2
                !------ 
			W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
                !
                BQy(1+j) = BQy(1+j) - Q(1+i)*Qy(10+i)   ! Q(11:13)  shift(i) or shift_contr(i)
                BQy(5) = BQy(5) - gp*W_ij*Qy(10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=3
                !------
			W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
                !
                CQz(1+j) = CQz(1+j) - Q(1+i)*Qz(10+i)   ! Q(11:13)  shift(i) or shift_contr(i)
                CQz(5) = CQz(5) - gp*W_ij*Qz(10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
          DO m=1,3
            IF(m.GE.i) THEN  
			ccount=ccount+1
                !
			Wim = ww*v_contr(i)*v_contr(m)-vxB_contr(i)*vxB_contr(m)-B_contr(i)*B_contr(m)+(p+uem)*g_contr(i,m)
			!Wim = Wim + (1.0 - delta(i,m))*(ww*v_contr(m)*v_contr(i)-vxB_contr(m)*vxB_contr(i)-B_contr(m)*B_contr(i)+(p+uem)*g_contr(m,i))  ! account also of the remaining symmetric components of gamma for i.NE.m.
			IF(i.NE.m) THEN !TWICE! account also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    Wim = 2.0*Wim   
                ENDIF
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=1
                !------ 
			AQx(1+j) = AQx(1+j) - 0.5*gp*lapse*Wim*Qx(13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			AQx(5) = AQx(5) - 0.5*gp*Wim*shift(j)*Qx(13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=2
                !------ 
			BQy(1+j) = BQy(1+j) - 0.5*gp*lapse*Wim*Qy(13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			BQy(5) = BQy(5) - 0.5*gp*Wim*shift(j)*Qy(13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=3
                !------
			CQz(1+j) = CQz(1+j) - 0.5*gp*lapse*Wim*Qz(13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			CQz(5) = CQz(5) - 0.5*gp*Wim*shift(j)*Qz(13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
#ifdef COVCLEAN     
				CQz(9) = CQz(9) + 0.5*Q(9)*gammaij(ccount)*shift(j)*Qz(13+ccount)   ! D.C.
                CQz(6:8) = CQz(6:8) - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount)*Qz(13+ccount)
                !
                IF(i.NE.m) THEN !TWICE!  account  also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    CQz(9) = CQz(9) + 0.5*Q(9)*gammaij(ccount)*shift(j)*Qz(13+ccount)   ! D.C.
                    CQz(6:8) = CQz(6:8) - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount)*Qz(13+ccount)
                ENDIF
#endif  		
                !
            ENDIF
          ENDDO
      ENDDO
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=1
    !------ 
    AQx(1+j) = AQx(1+j) + (Q(5)+Q(1))*Qx(10)    ! Q(10) or lapse
    AQx(5) = AQx(5) + S_contr(j)*Qx(10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	AQx(9) = AQx(9) - Q(5+j)*Qx(10)             ! D.C.
    AQx(9) = AQx(9) + Q(9)*Qx(10+j)             ! D.C.   phi * d(shift^x)/dx
    AQx(6:8) = AQx(6:8) - Q(11:13)*Qx(5+j)             ! D.C.   shift * d(gp*B^x)/dx
    AQx(6:8) = AQx(6:8) + lapse*g_contr(1:3,j)*Qx(9)  ! D.C.  
#else
    AQx(6:8) = AQx(6:8) - Q(11:13)*Qx(5+j)             ! D.C.   shift * d(gp*B^x)/dx 
    AQx(6:8) = AQx(6:8) + lapse*gp*g_contr(1:3,j)*Qx(9)  ! D.C.   
    AQx(9) = AQx(9) + gm*lapse*EQN%DivCleaning_a**2*Qx(5+j)             ! D.C.   shift * d(gp*B^x)/dx 
    AQx(9) = AQx(9) - Q(10+j)*Qx(9)             ! D.C.   shift * d(gp*B^x)/dx  
#endif
	!
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=2
    !------ 
    BQy(1+j) = BQy(1+j) + (Q(5)+Q(1))*Qy(10)    ! Q(10) or lapse
    BQy(5) = BQy(5) + S_contr(j)*Qy(10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	BQy(9) = BQy(9) - Q(5+j)*Qy(10)             ! D.C.
    BQy(9) = BQy(9) + Q(9)*Qy(10+j)             ! D.C.   phi * d(shift^y)/dy
    BQy(6:8) = BQy(6:8) - Q(11:13)*Qy(5+j)                  ! D.C.   shift * d(gp*B^y)/dy
    BQy(6:8) = BQy(6:8) + lapse*g_contr(1:3,j)*Qy(9)        ! D.C.  
#else
    BQy(6:8) = BQy(6:8) - Q(11:13)*Qy(5+j)                  ! D.C.   shift * d(gp*B^x)/dx 
    BQy(6:8) = BQy(6:8) + lapse*gp*g_contr(1:3,j)*Qy(9)     ! D.C.  
    BQy(9) = BQy(9) + gm*lapse*EQN%DivCleaning_a**2*Qy(5+j)             ! D.C.   shift * d(gp*B^x)/dx 
    BQy(9) = BQy(9) - Q(10+j)*Qy(9)             ! D.C.   shift * d(gp*B^x)/dx  
#endif
	!
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=3
    !------
    CQz(1+j) = CQz(1+j) + (Q(5)+Q(1))*Qz(10)    ! Q(10) or lapse
    CQz(5) = CQz(5) + S_contr(j)*Qz(10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	CQz(9) = CQz(9) - Q(5+j)*Qz(10)             ! D.C.
    CQz(9) = CQz(9) + Q(9)*Qz(10+j)             ! D.C.   phi *d(shift^z)/dz
    CQz(6:8) = CQz(6:8) - Q(11:13)*Qz(5+j)                  ! D.C.   shift * d(gp*B^z)/dz
    CQz(6:8) = CQz(6:8) + lapse*g_contr(1:3,j)*Qz(9)        ! D.C.  
#else
    CQz(6:8) = CQz(6:8) - Q(11:13)*Qz(5+j)                  ! D.C.   shift * d(gp*B^x)/dx 
    CQz(6:8) = CQz(6:8) + lapse*gp*g_contr(1:3,j)*Qz(9)     ! D.C.  
    CQz(9) = CQz(9) + gm*lapse*EQN%DivCleaning_a**2*Qz(5+j)             ! D.C.   shift * d(gp*B^x)/dx 
    CQz(9) = CQz(9) - Q(10+j)*Qz(9)             ! D.C.   shift * d(gp*B^x)/dx  
#endif
	!
	DO i=1,nVar
		BgradQ(i) = AQx(i) + BQy(i) + CQz(i)  
	ENDDO
	!
		!IF( ANY( ISNAN(BgradQ(:)) )) THEN
		!	PRINT *,'gradQ, PDENCPGRMHD NAN :' 
		!	WRITE(*,*) i
		!	WRITE(*,*) Q(1:5)
		!	WRITE(*,*) Q(6:10)
		!	WRITE(*,*) Q(11:15)
		!	WRITE(*,*) Q(16:19)
		!	PRINT *,'--------gradQ-------' 
		!	WRITE(*,*) gradQ(1:5,1)
		!	WRITE(*,*) gradQ(6:10,1)
		!	WRITE(*,*) gradQ(11:15,1)
		!	WRITE(*,*) gradQ(16:19,1)
		!	PRINT *,'---------------' 
		!	WRITE(*,*) gradQ(1:5,2)
		!	WRITE(*,*) gradQ(6:10,2)
		!	WRITE(*,*) gradQ(11:15,2)
		!	WRITE(*,*) gradQ(16:19,2)
		!	PRINT *,'---------------' 
		!	WRITE(*,*) gradQ(1:5,3)
		!	WRITE(*,*) gradQ(6:10,3)
		!	WRITE(*,*) gradQ(11:15,3)
		!	WRITE(*,*) gradQ(16:19,3)
		!	PRINT *,'--------BgradQ-------' 
		!	WRITE(*,*) BgradQ(1:5)
		!	WRITE(*,*) BgradQ(6:10)
		!	WRITE(*,*) BgradQ(11:15)
		!	WRITE(*,*) BgradQ(16:19)
		!	PRINT *,'--------AQx-------' 
		!	WRITE(*,*) AQx(1:5)
		!	WRITE(*,*) AQx(6:10)
		!	WRITE(*,*) AQx(11:15)
		!	WRITE(*,*) AQx(16:19)
		!	PRINT *,'--------BQy-------' 
		!	WRITE(*,*) BQy(1:5)
		!	WRITE(*,*) BQy(6:10)
		!	WRITE(*,*) BQy(11:15)
		!	WRITE(*,*) BQy(16:19)
		!	PRINT *,'--------CQz-------' 
		!	WRITE(*,*) CQz(1:5)
		!	WRITE(*,*) CQz(6:10)
		!	WRITE(*,*) CQz(11:15)
		!	WRITE(*,*) CQz(16:19)
		!	PRINT *,'---------------' 
		!	STOP
		!ENDIF 
    CONTINUE
    !            
END SUBROUTINE PDENCPGRMHD      
!
!    
RECURSIVE SUBROUTINE PDENCPPrimGRMHD(BgradQ,Vc,Q,gradQ)
    USE MainVariables, ONLY : d, EQN 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
  ! Argument list 
  REAL, INTENT(IN)  :: Q(nVar), Vc(nVar), gradQ(nVar,d)
  REAL, INTENT(OUT) :: BgradQ(nVar) 
  ! Local variables 
  REAL :: p, irho
  REAL :: rhoeps,dd,tau,s2,sm_cov(3),sm(3)
  REAL :: Qx(nVar), Qy(nVar), Qz(nVar) 
  REAL :: e,g_cov(3,3),g_contr(3,3)
  REAL :: AQx(nVar), BQy(nVar), CQz(nVar) 
  REAL :: lapse, shift(3), gammaij(6), delta(3,3), B_cov(3), vxB_cov(3), vxb_contr(3), psi, S_contr(3), qb_contr(3), B_contr(3) 
  REAL :: v2,v_contr(3),uem,b2,e2,gp,gm,lf,w,ww,gamma1,rho,v_cov(3), w_ij, wim    
  INTEGER :: i,j,k,l,m,iErr, ccount    
  !
  !BgradQ = 0.0 
  DO i=1,nVar
  	Qx(i) = gradQ(i,1) 
  	Qy(i) = gradQ(i,2)
  	Qz(i) = gradQ(i,3)
  	AQx(i) = 0.
  	BQy(i) = 0.
  	CQz(i) = 0.
  ENDDO
  !
  lapse = Q(10)
  !
  shift(1) = Q(11)
  shift(2) = Q(12)
  shift(3) = Q(13)
  !
  DO i=1,6
  	gammaij(i) = Q(13+i) 
  ENDDO
  g_cov(1,1) = Q(14)
  g_cov(1,2) = Q(15)
  g_cov(1,3) = Q(16)
  g_cov(2,2) = Q(17)
  g_cov(2,3) = Q(18)
  g_cov(3,3) = Q(19)
  g_cov(2,1) = Q(15)
  g_cov(3,1) = Q(16)
  g_cov(3,2) = Q(18)
  !
  DO i=1,3
    DO j=1,3
  	  delta(i,j) = 0.0
    ENDDO 
  ENDDO 
  DO i=1,3
     delta(i,i) = 1.0 
  ENDDO 
  !
  CALL MatrixInverse3x3(g_cov,g_contr,gp)
  gp = SQRT(gp)
  gm = 1./gp
  ! 
  gamma1 = EQN%gamma/(EQN%gamma-1.0)
  rho    = Vc(1)
  v_cov = Vc(2:4)
  p      = Vc(5)
  !
  !B_cov(1:3) = Vc(6:8)
  DO i=1,3
    B_contr(i) = Vc(5+i) ! contravariant!
    QB_contr(i) = Q(5+i)
  ENDDO
  !
  B_cov = MATMUL(g_cov,B_contr)
  psi = Vc(9) 
  ! 
  v_contr       = MATMUL(g_contr,v_cov)
  S_contr = MATMUL(g_contr,Q(2:4))
  !
  !! COVARIANT =>
  vxB_cov(1) = v_contr(2)*B_contr(3) - v_contr(3)*B_contr(2)
  vxB_cov(2) = v_contr(3)*B_contr(1) - v_contr(1)*B_contr(3)
  vxB_cov(3) = v_contr(1)*B_contr(2) - v_contr(2)*B_contr(1) 
  DO i=1,3
    vxB_cov(i) = gp*vxB_cov(i)
  ENDDO
  !
  ! CONTRAVARIANT =>
  vxB_contr(1) = v_cov(2)*B_cov(3) - v_cov(3)*B_cov(2)
  vxB_contr(2) = v_cov(3)*B_cov(1) - v_cov(1)*B_cov(3)
  vxB_contr(3) = v_cov(1)*B_cov(2) - v_cov(2)*B_cov(1)
  DO i=1,3
  	vxB_contr(i) = gm*vxB_contr(i)
  ENDDO
  !v2     = vx**2 + vy**2 + vz**2
  !b2     = bx**2 + by**2 + bz**2
  !e2     = ex**2 + ey**2 + ez**2 
  v2     = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
  e2     = vxB_contr(1)*vxB_cov(1) + vxB_contr(2)*vxB_cov(2) + vxB_contr(3)*vxB_cov(3)
  b2     = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
  !
  uem    = 0.5*(b2 + e2) 
  !
   S_contr = MATMUL(g_contr,Q(2:4))
  !
  lf     = 1.0/sqrt(1.0 - v2)
  !
  dd = gm*Q(1)
  tau = gm*Q(5)
  DO i=1,3 
      sm_cov(i)  = Q(1+i)
  ENDDO
  sm_cov = gm*sm_cov
  sm   = MATMUL (g_contr, sm_cov)  
  s2   = sm_cov(1)*sm(1) + sm_cov(2)*sm(2) + sm_cov(3)*sm(3)
  !
#ifdef GEOS  
  CALL EOS_eps(rhoeps,rho,p,dd,tau,s2) 
  w      = rho + rhoeps + p   ! rho*enthalpy
#else
  w   = rho + gamma1*p
#endif
!
  !IF(ABS(w-(rho + gamma1*p)).GT.1e-13) THEN
  !    continue
  !ENDIF
  !
  !w      = rho + gamma1*p   ! rho*hentalpy
  ww     = w*lf**2          ! rho*hentalpy*Lorentz^2 
  !
	ccount=0
      DO i=1,3 
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=1
                !------ 
			W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
                !
                AQx(1+j) = AQx(1+j) - Q(1+i)*Qx(10+i)  ! Q(11:13)  shift(i) or shift_contr(i)
                AQx(5) = AQx(5) - gp*W_ij*Qx(10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=2
                !------ 
			W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
                !
                BQy(1+j) = BQy(1+j) - Q(1+i)*Qy(10+i)   ! Q(11:13)  shift(i) or shift_contr(i)
                BQy(5) = BQy(5) - gp*W_ij*Qy(10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=3
                !------
			W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
                !
                CQz(1+j) = CQz(1+j) - Q(1+i)*Qz(10+i)   ! Q(11:13)  shift(i) or shift_contr(i)
                CQz(5) = CQz(5) - gp*W_ij*Qz(10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
          DO m=1,3
            IF(m.GE.i) THEN  
			ccount=ccount+1
                !
			Wim = ww*v_contr(i)*v_contr(m)-vxB_contr(i)*vxB_contr(m)-B_contr(i)*B_contr(m)+(p+uem)*g_contr(i,m)
			!Wim = Wim + (1.0 - delta(i,m))*(ww*v_contr(m)*v_contr(i)-vxB_contr(m)*vxB_contr(i)-B_contr(m)*B_contr(i)+(p+uem)*g_contr(m,i))  ! account also of the remaining symmetric components of gamma for i.NE.m.
			IF(i.NE.m) THEN !TWICE! account also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    Wim = 2.0*Wim   
                ENDIF
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=1
                !------ 
			AQx(1+j) = AQx(1+j) - 0.5*gp*lapse*Wim*Qx(13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			AQx(5) = AQx(5) - 0.5*gp*Wim*shift(j)*Qx(13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=2
                !------ 
			BQy(1+j) = BQy(1+j) - 0.5*gp*lapse*Wim*Qy(13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			BQy(5) = BQy(5) - 0.5*gp*Wim*shift(j)*Qy(13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=3
                !------
			CQz(1+j) = CQz(1+j) - 0.5*gp*lapse*Wim*Qz(13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			CQz(5) = CQz(5) - 0.5*gp*Wim*shift(j)*Qz(13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
#ifdef COVCLEAN     
				CQz(9) = CQz(9) + 0.5*Q(9)*gammaij(ccount)*shift(j)*Qz(13+ccount)   ! D.C.
                CQz(6:8) = CQz(6:8) - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount)*Qz(13+ccount)
                !
                IF(i.NE.m) THEN !TWICE!  account  also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    CQz(9) = CQz(9) + 0.5*Q(9)*gammaij(ccount)*shift(j)*Qz(13+ccount)   ! D.C.
                    CQz(6:8) = CQz(6:8) - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount)*Qz(13+ccount)
                ENDIF
#endif  		
                !
            ENDIF
          ENDDO
      ENDDO
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=1
    !------ 
    AQx(1+j) = AQx(1+j) + (Q(5)+Q(1))*Qx(10)    ! Q(10) or lapse
    AQx(5) = AQx(5) + S_contr(j)*Qx(10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	AQx(9) = AQx(9) - Q(5+j)*Qx(10)             ! D.C.
    AQx(9) = AQx(9) + Q(9)*Qx(10+j)             ! D.C.   phi * d(shift^x)/dx
    AQx(6:8) = AQx(6:8) - Q(11:13)*Qx(5+j)             ! D.C.   shift * d(gp*B^x)/dx
    AQx(6:8) = AQx(6:8) + lapse*g_contr(1:3,j)*Qx(9)  ! D.C.  
#else
    AQx(6:8) = AQx(6:8) - Q(11:13)*Qx(5+j)             ! D.C.   shift * d(gp*B^x)/dx 
    AQx(6:8) = AQx(6:8) + lapse*gp*g_contr(1:3,j)*Qx(9)  ! D.C.   
    AQx(9) = AQx(9) + gm*lapse*EQN%DivCleaning_a**2*Qx(5+j)             ! D.C.   shift * d(gp*B^x)/dx 
    AQx(9) = AQx(9) - Q(10+j)*Qx(9)             ! D.C.   shift * d(gp*B^x)/dx  
#endif
	!
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=2
    !------ 
    BQy(1+j) = BQy(1+j) + (Q(5)+Q(1))*Qy(10)    ! Q(10) or lapse
    BQy(5) = BQy(5) + S_contr(j)*Qy(10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	BQy(9) = BQy(9) - Q(5+j)*Qy(10)             ! D.C.
    BQy(9) = BQy(9) + Q(9)*Qy(10+j)             ! D.C.   phi * d(shift^y)/dy
    BQy(6:8) = BQy(6:8) - Q(11:13)*Qy(5+j)                  ! D.C.   shift * d(gp*B^y)/dy
    BQy(6:8) = BQy(6:8) + lapse*g_contr(1:3,j)*Qy(9)        ! D.C.  
#else
    BQy(6:8) = BQy(6:8) - Q(11:13)*Qy(5+j)                  ! D.C.   shift * d(gp*B^x)/dx 
    BQy(6:8) = BQy(6:8) + lapse*gp*g_contr(1:3,j)*Qy(9)     ! D.C.  
    BQy(9) = BQy(9) + gm*lapse*EQN%DivCleaning_a**2*Qy(5+j)             ! D.C.   shift * d(gp*B^x)/dx 
    BQy(9) = BQy(9) - Q(10+j)*Qy(9)             ! D.C.   shift * d(gp*B^x)/dx  
#endif
	!
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=3
    !------
    CQz(1+j) = CQz(1+j) + (Q(5)+Q(1))*Qz(10)    ! Q(10) or lapse
    CQz(5) = CQz(5) + S_contr(j)*Qz(10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	CQz(9) = CQz(9) - Q(5+j)*Qz(10)             ! D.C.
    CQz(9) = CQz(9) + Q(9)*Qz(10+j)             ! D.C.   phi *d(shift^z)/dz
    CQz(6:8) = CQz(6:8) - Q(11:13)*Qz(5+j)                  ! D.C.   shift * d(gp*B^z)/dz
    CQz(6:8) = CQz(6:8) + lapse*g_contr(1:3,j)*Qz(9)        ! D.C.  
#else
    CQz(6:8) = CQz(6:8) - Q(11:13)*Qz(5+j)                  ! D.C.   shift * d(gp*B^x)/dx 
    CQz(6:8) = CQz(6:8) + lapse*gp*g_contr(1:3,j)*Qz(9)     ! D.C.  
    CQz(9) = CQz(9) + gm*lapse*EQN%DivCleaning_a**2*Qz(5+j)             ! D.C.   shift * d(gp*B^x)/dx 
    CQz(9) = CQz(9) - Q(10+j)*Qz(9)             ! D.C.   shift * d(gp*B^x)/dx  
#endif
	!
	DO i=1,nVar
		BgradQ(i) = AQx(i) + BQy(i) + CQz(i)  
	ENDDO
	!
		!IF( ANY( ISNAN(BgradQ(:)) )) THEN
		!	PRINT *,'gradQ, PDENCPGRMHD NAN :' 
		!	WRITE(*,*) i
		!	WRITE(*,*) Q(1:5)
		!	WRITE(*,*) Q(6:10)
		!	WRITE(*,*) Q(11:15)
		!	WRITE(*,*) Q(16:19)
		!	PRINT *,'--------gradQ-------' 
		!	WRITE(*,*) gradQ(1:5,1)
		!	WRITE(*,*) gradQ(6:10,1)
		!	WRITE(*,*) gradQ(11:15,1)
		!	WRITE(*,*) gradQ(16:19,1)
		!	PRINT *,'---------------' 
		!	WRITE(*,*) gradQ(1:5,2)
		!	WRITE(*,*) gradQ(6:10,2)
		!	WRITE(*,*) gradQ(11:15,2)
		!	WRITE(*,*) gradQ(16:19,2)
		!	PRINT *,'---------------' 
		!	WRITE(*,*) gradQ(1:5,3)
		!	WRITE(*,*) gradQ(6:10,3)
		!	WRITE(*,*) gradQ(11:15,3)
		!	WRITE(*,*) gradQ(16:19,3)
		!	PRINT *,'--------BgradQ-------' 
		!	WRITE(*,*) BgradQ(1:5)
		!	WRITE(*,*) BgradQ(6:10)
		!	WRITE(*,*) BgradQ(11:15)
		!	WRITE(*,*) BgradQ(16:19)
		!	PRINT *,'--------AQx-------' 
		!	WRITE(*,*) AQx(1:5)
		!	WRITE(*,*) AQx(6:10)
		!	WRITE(*,*) AQx(11:15)
		!	WRITE(*,*) AQx(16:19)
		!	PRINT *,'--------BQy-------' 
		!	WRITE(*,*) BQy(1:5)
		!	WRITE(*,*) BQy(6:10)
		!	WRITE(*,*) BQy(11:15)
		!	WRITE(*,*) BQy(16:19)
		!	PRINT *,'--------CQz-------' 
		!	WRITE(*,*) CQz(1:5)
		!	WRITE(*,*) CQz(6:10)
		!	WRITE(*,*) CQz(11:15)
		!	WRITE(*,*) CQz(16:19)
		!	PRINT *,'---------------' 
		!	STOP
		!ENDIF 
    CONTINUE
    !            
END SUBROUTINE PDENCPPrimGRMHD      
!

#ifdef VECTOR

RECURSIVE SUBROUTINE PDENCPPrimVectorGRMHD(BgradQ,Vc,Q,Qx,Qy,Qz)
    USE MainVariables, ONLY : d, EQN, VECTORLENGTH 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    ! Argument list 
    REAL(8), INTENT(IN)  :: Vc(VECTORLENGTH,nVar), Q(VECTORLENGTH,nVar), Qx(VECTORLENGTH,nVar), Qy(VECTORLENGTH,nVar), Qz(VECTORLENGTH,nVar) 
    REAL(8), INTENT(OUT) :: BgradQ(VECTORLENGTH,nVar) 
    ! Local variables 
    INTEGER :: i,j,k,l,m,n,ip,iq,ii,jj,kk,ll,iErr,count,ccount  
    REAL(8) :: rhoeps(VECTORLENGTH)
    REAL(8) :: p(VECTORLENGTH), irho(VECTORLENGTH), det(VECTORLENGTH), idet(VECTORLENGTH) , DD(VECTORLENGTH) , tau(VECTORLENGTH) , s2(VECTORLENGTH) 
    REAL(8) :: e(VECTORLENGTH),g_cov(VECTORLENGTH,3,3),g_contr(VECTORLENGTH,3,3)
    REAL(8) :: AQx(VECTORLENGTH,nVar), BQy(VECTORLENGTH,nVar), CQz(VECTORLENGTH,nVar) 
    REAL(8) :: lapse(VECTORLENGTH), shift(VECTORLENGTH,3), gammaij(VECTORLENGTH,6), delta(VECTORLENGTH,3,3), B_cov(VECTORLENGTH,3), vxB_cov(VECTORLENGTH,3), vxb_contr(VECTORLENGTH,3), psi(VECTORLENGTH), gS_contr(VECTORLENGTH,3),S_contr(VECTORLENGTH,3), qb_contr(VECTORLENGTH,3), B_contr(VECTORLENGTH,3) 
    REAL(8) :: v2(VECTORLENGTH),v_contr(VECTORLENGTH,3),S_cov(VECTORLENGTH,3),uem(VECTORLENGTH),b2(VECTORLENGTH),e2(VECTORLENGTH),gp(VECTORLENGTH),gm(VECTORLENGTH),lf(VECTORLENGTH),w(VECTORLENGTH),ww(VECTORLENGTH),gamma1(VECTORLENGTH),rho(VECTORLENGTH),v_cov(VECTORLENGTH,3), w_ij(VECTORLENGTH), wim(VECTORLENGTH)    
    !
#ifdef AVX512
    !dir$ attributes align:  64 :: rhoeps,p,irho,det,idet,DD,tau,s2,e,g_cov,g_contr,AQx,BQy,CQz,lapse,shift,gammaij,delta,B_cov,vxB_cov,vxb_contr,psi,S_contr,gS_contr,qb_contr,B_contr
    !dir$ attributes align:  64 :: v2,v_contr,S_cov,uem,b2,e2,gp,gm,lf,w,ww,gamma1,rho,v_cov,w_ij,wim
    !DIR$ ASSUME_ALIGNED BgradQ : 64  
    !DIR$ ASSUME_ALIGNED Vc : 64  
    !DIR$ ASSUME_ALIGNED Q  : 64  
    !DIR$ ASSUME_ALIGNED Qx : 64  
    !DIR$ ASSUME_ALIGNED Qy : 64  
    !DIR$ ASSUME_ALIGNED Qz : 64  
    !DIR$ ASSUME_ALIGNED par : 64  
#else
    !dir$ attributes align:  32 :: rhoeps,p,irho,det,idet,DD,tau,s2,e,g_cov,g_contr,AQx,BQy,CQz,lapse,shift,gammaij,delta,B_cov,vxB_cov,vxb_contr,psi,S_contr,gS_contr,qb_contr,B_contr
    !dir$ attributes align:  32 :: v2,v_contr,S_cov,uem,b2,e2,gp,gm,lf,w,ww,gamma1,rho,v_cov,w_ij,wim
    !DIR$ ASSUME_ALIGNED BgradQ : 32 
    !DIR$ ASSUME_ALIGNED Vc : 32 
    !DIR$ ASSUME_ALIGNED Q  : 32  
    !DIR$ ASSUME_ALIGNED Qx : 32  
    !DIR$ ASSUME_ALIGNED Qy : 32  
    !DIR$ ASSUME_ALIGNED Qz : 32  
    !DIR$ ASSUME_ALIGNED par : 32  
#endif 
    !
    !BgradQ = 0.0 
    AQx = 0.0
    BQy = 0.0
    CQz = 0.0 
    !
    lapse(:) = Q(:,10)
    !
	shift(:,1) = Q(:,11)
	shift(:,2) = Q(:,12)
	shift(:,3) = Q(:,13)
	!
	DO i=1,6
		gammaij(:,i) = Q(:,13+i) 
	ENDDO
    g_cov(:,1,1) = Q(:,14)
    g_cov(:,1,2) = Q(:,15)
    g_cov(:,1,3) = Q(:,16)
    g_cov(:,2,2) = Q(:,17)
    g_cov(:,2,3) = Q(:,18)
    g_cov(:,3,3) = Q(:,19)
    g_cov(:,2,1) = Q(:,15)
    g_cov(:,3,1) = Q(:,16)
    g_cov(:,3,2) = Q(:,18)
    !
    delta = 0.0
    DO i=1,3
        delta(:,i,i) = 1.0 
    ENDDO 
    !
    det(:) = (g_cov(:,1,1)*g_cov(:,2,2)*g_cov(:,3,3)-g_cov(:,1,1)*g_cov(:,2,3)**2-g_cov(:,1,2)**2*g_cov(:,3,3)+2*g_cov(:,1,2)*g_cov(:,1,3)*g_cov(:,2,3)-g_cov(:,1,3)**2*g_cov(:,2,2))  
    idet(:) = 1./det(:)  
    !
    g_contr(:,1,1) =  ( g_cov(:,2,2)*g_cov(:,3,3)-g_cov(:,2,3)*g_cov(:,3,2)) * idet(:) 
    g_contr(:,1,2) = -( g_cov(:,1,2)*g_cov(:,3,3)-g_cov(:,1,3)*g_cov(:,3,2)) * idet(:)
    g_contr(:,1,3) = -(-g_cov(:,1,2)*g_cov(:,2,3)+g_cov(:,1,3)*g_cov(:,2,2)) * idet(:) 
    g_contr(:,2,1) = -( g_cov(:,2,1)*g_cov(:,3,3)-g_cov(:,2,3)*g_cov(:,3,1)) * idet(:) 
    g_contr(:,2,2) =  ( g_cov(:,1,1)*g_cov(:,3,3)-g_cov(:,1,3)*g_cov(:,3,1)) * idet(:) 
    g_contr(:,2,3) = -( g_cov(:,1,1)*g_cov(:,2,3)-g_cov(:,1,3)*g_cov(:,2,1)) * idet(:) 
    g_contr(:,3,1) = -(-g_cov(:,2,1)*g_cov(:,3,2)+g_cov(:,2,2)*g_cov(:,3,1)) * idet(:) 
    g_contr(:,3,2) = -( g_cov(:,1,1)*g_cov(:,3,2)-g_cov(:,1,2)*g_cov(:,3,1)) * idet(:) 
    g_contr(:,3,3) =  ( g_cov(:,1,1)*g_cov(:,2,2)-g_cov(:,1,2)*g_cov(:,2,1)) * idet(:)       

    gp(:) = SQRT(det(:))
    gm(:) = 1./gp(:) 
    !  
    DD(:)         = Q(:,1)*gm(:) 
    S_cov(:,1)    = Q(:,2)*gm(:)
    S_cov(:,2)    = Q(:,3)*gm(:)
    S_cov(:,3)    = Q(:,4)*gm(:)
    tau(:)        = Q(:,5)*gm(:)
    !
    gamma1 = EQN%gamma/(EQN%gamma-1.0)
    rho(:) = Vc(:,1)
    v_cov(:,:) = Vc(:,2:4)
    p(:)       = Vc(:,5)
    !
    DO i=1,3
	    B_contr(:,i) = Vc(:,5+i) ! contravariant!
	    QB_contr(:,i) = Q(:,5+i)
    ENDDO
    !
    !B_cov = MATMUL(g_cov,B_contr)
    B_cov(:,1) = g_cov(:,1,1)*B_contr(:,1) + g_cov(:,1,2)*B_contr(:,2) + g_cov(:,1,3)*B_contr(:,3) 
    B_cov(:,2) = g_cov(:,2,1)*B_contr(:,1) + g_cov(:,2,2)*B_contr(:,2) + g_cov(:,2,3)*B_contr(:,3) 
    B_cov(:,3) = g_cov(:,3,1)*B_contr(:,1) + g_cov(:,3,2)*B_contr(:,2) + g_cov(:,3,3)*B_contr(:,3) 
    !
    psi(:) = Vc(:,9) 
    ! 
    !v_contr       = MATMUL(g_contr,v_cov)
    !gS_contr = MATMUL(g_contr,Q(2:4))
    !
    v_contr(:,1) = g_contr(:,1,1)*v_cov(:,1) + g_contr(:,1,2)*v_cov(:,2) + g_contr(:,1,3)*v_cov(:,3) 
    v_contr(:,2) = g_contr(:,2,1)*v_cov(:,1) + g_contr(:,2,2)*v_cov(:,2) + g_contr(:,2,3)*v_cov(:,3) 
    v_contr(:,3) = g_contr(:,3,1)*v_cov(:,1) + g_contr(:,3,2)*v_cov(:,2) + g_contr(:,3,3)*v_cov(:,3) 
    S_contr(:,1) = g_contr(:,1,1)*S_cov(:,1) + g_contr(:,1,2)*S_cov(:,2) + g_contr(:,1,3)*S_cov(:,3) 
    S_contr(:,2) = g_contr(:,2,1)*S_cov(:,1) + g_contr(:,2,2)*S_cov(:,2) + g_contr(:,2,3)*S_cov(:,3) 
    S_contr(:,3) = g_contr(:,3,1)*S_cov(:,1) + g_contr(:,3,2)*S_cov(:,2) + g_contr(:,3,3)*S_cov(:,3)   
    gS_contr(:,1) = g_contr(:,1,1)*Q(:,2) + g_contr(:,1,2)*Q(:,3) + g_contr(:,1,3)*Q(:,4) 
    gS_contr(:,2) = g_contr(:,2,1)*Q(:,2) + g_contr(:,2,2)*Q(:,3) + g_contr(:,2,3)*Q(:,4) 
    gS_contr(:,3) = g_contr(:,3,1)*Q(:,2) + g_contr(:,3,2)*Q(:,3) + g_contr(:,3,3)*Q(:,4)   
    !
    !! COVARIANT =>
    vxB_cov(:,1) = gp(:)*( v_contr(:,2)*B_contr(:,3) - v_contr(:,3)*B_contr(:,2) ) 
    vxB_cov(:,2) = gp(:)*( v_contr(:,3)*B_contr(:,1) - v_contr(:,1)*B_contr(:,3) ) 
    vxB_cov(:,3) = gp(:)*( v_contr(:,1)*B_contr(:,2) - v_contr(:,2)*B_contr(:,1) ) 
    !
    ! CONTRAVARIANT   
    vxB_contr(:,1) = gm(:)* ( v_cov(:,2)*B_cov(:,3) - v_cov(:,3)*B_cov(:,2) ) 
    vxB_contr(:,2) = gm(:)* ( v_cov(:,3)*B_cov(:,1) - v_cov(:,1)*B_cov(:,3) ) 
    vxB_contr(:,3) = gm(:)* ( v_cov(:,1)*B_cov(:,2) - v_cov(:,2)*B_cov(:,1) ) 
    !v2     = vx**2 + vy**2 + vz**2
    !b2     = bx**2 + by**2 + bz**2
    !e2     = ex**2 + ey**2 + ez**2 
    v2     = v_contr(:,1)*v_cov(:,1) + v_contr(:,2)*v_cov(:,2) + v_contr(:,3)*v_cov(:,3)
    e2     = vxB_contr(:,1)*vxB_cov(:,1) + vxB_contr(:,2)*vxB_cov(:,2) + vxB_contr(:,3)*vxB_cov(:,3)
    b2     = B_contr(:,1)*B_cov(:,1) + B_contr(:,2)*B_cov(:,2) + B_contr(:,3)*B_cov(:,3)
    s2     = S_contr(:,1)*S_cov(:,1) + S_contr(:,2)*S_cov(:,2) + S_contr(:,3)*S_cov(:,3)
    !
    uem(:)    = 0.5*(b2(:) + e2(:) ) 
    !
    lf(:)     = 1.0/sqrt(1.0 - v2(:) ) 
    !
#ifdef GEOS  
    CALL EOS_eps_vector(rhoeps(:),rho(:),p(:),DD(:),tau(:),s2(:))
    w(:)      = rho(:) + rhoeps(:) + p(:)   ! rho*enthalpy
#else
    w(:)     = rho(:) + gamma1(:)*p(:)   ! rho*hentalpy
#endif
    !IF(SQRT(SUM((w-(rho + gamma1*p))**2)).GT.1e-10) THEN
    !    continue
    !ENDIF
    !
    !w(:)     = rho(:) + gamma1(:)*p(:)   ! rho*hentalpy
    ww(:)     = w(:)*lf(:)**2          ! rho*hentalpy*Lorentz^2 
    !
	ccount=0
      DO i=1,3 
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=1
                !------ 
			W_ij(:) = ww(:)*v_cov(:,i)*v_contr(:,j)-vxB_cov(:,i)*vxB_contr(:,j)-B_cov(:,i)*B_contr(:,j)+(p(:)+uem(:))*delta(:,i,j)
                !
                AQx(:,1+j) = AQx(:,1+j) - Q(:,1+i)*Qx(:,10+i)          ! Q(11:13)  shift(i) or shift_contr(i)
                AQx(:,5)   = AQx(:,5)   - gp(:)*W_ij(:)*Qx(:,10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=2
                !------ 
			W_ij(:) = ww(:)*v_cov(:,i)*v_contr(:,j)-vxB_cov(:,i)*vxB_contr(:,j)-B_cov(:,i)*B_contr(:,j)+(p(:)+uem(:))*delta(:,i,j)
                !
                BQy(:,1+j) = BQy(:,1+j) - Q(:,1+i)*Qy(:,10+i)   ! Q(11:13)  shift(i) or shift_contr(i)
                BQy(:,5) = BQy(:,5) - gp(:)*W_ij(:)*Qy(:,10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=3
                !------
			W_ij(:) = ww(:)*v_cov(:,i)*v_contr(:,j)-vxB_cov(:,i)*vxB_contr(:,j)-B_cov(:,i)*B_contr(:,j)+(p(:)+uem(:))*delta(:,i,j)
                !
                CQz(:,1+j) = CQz(:,1+j) - Q(:,1+i)*Qz(:,10+i)   ! Q(11:13)  shift(i) or shift_contr(i)
                CQz(:,5) = CQz(:,5) - gp(:)*W_ij(:)*Qz(:,10+i)     ! Q(11:13)  shift(i) or shift_contr(i)
                !
          DO m=1,3
            IF(m.GE.i) THEN  
			ccount=ccount+1
                !
			Wim(:) = ww(:)*v_contr(:,i)*v_contr(:,m)-vxB_contr(:,i)*vxB_contr(:,m)-B_contr(:,i)*B_contr(:,m)+(p(:)+uem(:))*g_contr(:,i,m)
			!Wim = Wim + (1.0 - delta(i,m))*(ww*v_contr(m)*v_contr(i)-vxB_contr(m)*vxB_contr(i)-B_contr(m)*B_contr(i)+(p+uem)*g_contr(m,i))  ! account also of the remaining symmetric components of gamma for i.NE.m.
			IF(i.NE.m) THEN !TWICE! account also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    Wim(:) = 2.0*Wim(:)    
                ENDIF
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=1
                !------ 
			AQx(:,1+j) = AQx(:,1+j) - 0.5*gp(:)*lapse(:)*Wim(:)*Qx(:,13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			AQx(:,5) = AQx(:,5) - 0.5*gp(:)*Wim(:)*shift(:,j)*Qx(:,13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=2
                !------ 
			BQy(:,1+j) = BQy(:,1+j) - 0.5*gp(:)*lapse(:)*Wim(:)*Qy(:,13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			BQy(:,5)  = BQy(:,5) - 0.5*gp(:)*Wim(:)*shift(:,j)*Qy(:,13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=3
                !------
			CQz(:,1+j) = CQz(:,1+j) - 0.5*gp(:)*lapse(:)*Wim(:)*Qz(:,13+ccount)  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
			CQz(:,5)  = CQz(:,5) - 0.5*gp(:)*Wim(:)*shift(:,j)*Qz(:,13+ccount)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                !
#ifdef COVCLEAN     
				CQz(:,9) = CQz(:,9) + 0.5*Q(:,9)*gammaij(:,ccount)*shift(:,j)*Qz(:,13+ccount)   ! D.C.
                CQz(:,6:8) = CQz(:,6:8) - 0.5*lapse(:)*Q(:,9)*g_contr(:,1:3,j)*gammaij(:,ccount)*Qz(:,13+ccount)
                !
                IF(i.NE.m) THEN !TWICE!  account  also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    CQz(:,9) = CQz(:,9) + 0.5*Q(:,9)*gammaij(:,ccount)*shift(:,j)*Qz(:,13+ccount)   ! D.C.
                    CQz(:,6:8) = CQz(:,6:8) - 0.5*lapse(:)*Q(:,9)*g_contr(:,1:3,j)*gammaij(:,ccount)*Qz(:,13+ccount)
                ENDIF
#endif  		
                !
            ENDIF
          ENDDO
      ENDDO
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=1
    !------ 
    AQx(:,1+j) = AQx(:,1+j) + (Q(:,5)+Q(:,1))*Qx(:,10)    ! Q(10) or lapse
    AQx(:,5) = AQx(:,5) + gS_contr(:,j)*Qx(:,10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	AQx(:,9) = AQx(:,9) - Q(:,5+j)*Qx(:,10)             ! D.C.
    AQx(:,9) = AQx(:,9) + Q(:,9)*Qx(:,10+j)             ! D.C.   phi * d(shift^x)/dx
    AQx(:,6:8) = AQx(:,6:8) - Q(:,11:13)*Qx(:,5+j)             ! D.C.   shift * d(gp*B^x)/dx
    AQx(:,6:8) = AQx(:,6:8) + lapse(:)*g_contr(:,1:3,j)*Qx(:,9)  ! D.C.  
#else
    AQx(:,6) = AQx(:,6)  - Q(:,11)*Qx(:,5+j) + lapse(:)*gp(:)*g_contr(:,1,j)*Qx(:,9)              ! D.C.   shift * d(gp*B^x)/dx 
    AQx(:,7) = AQx(:,7)  - Q(:,12)*Qx(:,5+j) + lapse(:)*gp(:)*g_contr(:,2,j)*Qx(:,9)              ! D.C.   shift * d(gp*B^x)/dx 
    AQx(:,8) = AQx(:,8)  - Q(:,13)*Qx(:,5+j) + lapse(:)*gp(:)*g_contr(:,3,j)*Qx(:,9)              ! D.C.   shift * d(gp*B^x)/dx 
    AQx(:,9) = AQx(:,9)  + gm(:)*lapse(:)*EQN%DivCleaning_a**2*Qx(:,5+j) - Q(:,10+j)*Qx(:,9)             ! D.C.   shift * d(gp*B^x)/dx 
#endif
	!
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=2
    !------ 
    BQy(:,1+j) = BQy(:,1+j) + (Q(:,5)+Q(:,1))*Qy(:,10)    ! Q(10) or lapse
    BQy(:,5)   = BQy(:,5) + gS_contr(:,j)*Qy(:,10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	BQy(:,9) = BQy(:,9) - Q(:,5+j)*Qy(:,10)             ! D.C.
    BQy(:,9) = BQy(:,9) + Q(:,9)*Qy(:,10+j)             ! D.C.   phi * d(shift^y)/dy
    BQy(:,6:8) = BQy(:,6:8) - Q(:,11:13)*Qy(:,5+j)                  ! D.C.   shift * d(gp*B^y)/dy
    BQy(:,6:8) = BQy(:,6:8) + lapse(:)*g_contr(:,1:3,j)*Qy(:,9)        ! D.C.  
#else
    BQy(:,6) = BQy(:,6) - Q(:,11)*Qy(:,5+j)  + lapse(:)*gp(:)*g_contr(:,1,j)*Qy(:,9)                 ! D.C.   shift * d(gp*B^x)/dx 
    BQy(:,7) = BQy(:,7) - Q(:,12)*Qy(:,5+j)  + lapse(:)*gp(:)*g_contr(:,2,j)*Qy(:,9)                 ! D.C.   shift * d(gp*B^x)/dx 
    BQy(:,8) = BQy(:,8) - Q(:,13)*Qy(:,5+j)  + lapse(:)*gp(:)*g_contr(:,3,j)*Qy(:,9)                 ! D.C.   shift * d(gp*B^x)/dx 
    BQy(:,9) = BQy(:,9) + gm(:)*lapse(:)*EQN%DivCleaning_a**2*Qy(:,5+j) - Q(:,10+j)*Qy(:,9)          ! D.C.   shift * d(gp*B^x)/dx 
#endif
	!
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=3
    !------
    CQz(:,1+j) = CQz(:,1+j) + (Q(:,5)+Q(:,1))*Qz(:,10)    ! Q(10) or lapse
    CQz(:,5) = CQz(:,5) + gS_contr(:,j)*Qz(:,10)         !  Q(10) or lapse
    !
#ifdef COVCLEAN     
	CQz(:,9) = CQz(:,9) - Q(:,5+j)*Qz(:,10)             ! D.C.
    CQz(:,9) = CQz(:,9) + Q(:,9)*Qz(:,10+j)             ! D.C.   phi *d(shift^z)/dz
    CQz(:,6:8) = CQz(:,6:8) - Q(:,11:13)*Qz(:,5+j)                  ! D.C.   shift * d(gp*B^z)/dz
    CQz(:,6:8) = CQz(:,6:8) + lapse(:)*g_contr(:,1:3,j)*Qz(:,9)        ! D.C.  
#else
    CQz(:,6) = CQz(:,6) - Q(:,11)*Qz(:,5+j) + lapse(:)*gp(:)*g_contr(:,1,j)*Qz(:,9)                  ! D.C.   shift * d(gp*B^x)/dx  
    CQz(:,7) = CQz(:,7) - Q(:,12)*Qz(:,5+j) + lapse(:)*gp(:)*g_contr(:,2,j)*Qz(:,9)                  ! D.C.   shift * d(gp*B^x)/dx  
    CQz(:,8) = CQz(:,8) - Q(:,13)*Qz(:,5+j) + lapse(:)*gp(:)*g_contr(:,3,j)*Qz(:,9)                  ! D.C.   shift * d(gp*B^x)/dx  
    CQz(:,9) = CQz(:,9) + gm(:)*lapse(:)*EQN%DivCleaning_a**2*Qz(:,5+j)  - Q(:,10+j)*Qz(:,9)         ! D.C.   shift * d(gp*B^x)/dx 
#endif
	!
	DO i=1,nVar
		BgradQ(:,i) = AQx(:,i) + BQy(:,i) + CQz(:,i)  
    ENDDO
    !    
    !WHERE(Vc(:,1) < 1e-7 ) 
    !    BgradQ(:,1) = 0.0
    !    BgradQ(:,2) = 0.0
    !    BgradQ(:,3) = 0.0
    !    BgradQ(:,4) = 0.0
    !    BgradQ(:,5) = 0.0
    !    BgradQ(:,6) = 0.0
    !    BgradQ(:,7) = 0.0
    !    BgradQ(:,8) = 0.0
    !    BgradQ(:,9) = 0.0
    !ENDWHERE    
    !
    continue
    !
END SUBROUTINE PDENCPPrimVectorGRMHD           
!
RECURSIVE SUBROUTINE PDECons2PrimVectorGRMHD(V,Q,iErr) 
    USE MainVariables, ONLY : d, VECTORLENGTH, EQN, p_floor, rho_floor ,NSTOV_rho_atmo,NSTOV_p_atmo
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    ! Argument list 
    REAL(8), INTENT(INOUT) :: V(VECTORLENGTH,nVar) 
    REAL(8), INTENT(IN)    :: Q(VECTORLENGTH,nVar) 
    ! Local variables 
    INTEGER :: iErr 
    INTEGER :: i,j,k,m
    INTEGER(8) :: iErrV(VECTORLENGTH)
    LOGICAL(8)    :: FAILED(VECTORLENGTH) 
    REAL(8)    :: p0(VECTORLENGTH), tau(VECTORLENGTH),dprho(VECTORLENGTH),dpeps(VECTORLENGTH), Lor(VECTORLENGTH), h(VECTORLENGTH)
    REAL(8)    :: rhoeps(VECTORLENGTH)
    REAL(8)    :: gp(VECTORLENGTH), gm(VECTORLENGTH), g_cov(VECTORLENGTH,3,3), g_contr(VECTORLENGTH,3,3), det(VECTORLENGTH), idet(VECTORLENGTH) 
    REAL(8)    :: psi(VECTORLENGTH), lapse(VECTORLENGTH), shift(VECTORLENGTH,3), Qloc(VECTORLENGTH,nVar), dd(VECTORLENGTH)
    REAL(8)    :: B_contr(VECTORLENGTH,3), B_cov(VECTORLENGTH,3), sm_cov(VECTORLENGTH,3), sm(VECTORLENGTH,3),gamma1(VECTORLENGTH),gam(VECTORLENGTH) 
    REAL(8)    :: s2(VECTORLENGTH), b2(VECTORLENGTH), sb(VECTORLENGTH), sb2(VECTORLENGTH), e(VECTORLENGTH), x1(VECTORLENGTH), x2(VECTORLENGTH), w(VECTORLENGTH)   
    REAL(8)    :: eps(VECTORLENGTH), v2(VECTORLENGTH), p(VECTORLENGTH), rho(VECTORLENGTH), vx(VECTORLENGTH), vy(VECTORLENGTH), vz(VECTORLENGTH), v_cov(VECTORLENGTH,3)  
    REAL(8)    :: den(VECTORLENGTH), vb(VECTORLENGTH), v_contr(VECTORLENGTH,3), ww(VECTORLENGTH)   
    !
#ifdef AVX512
    !dir$ attributes align: 64:: gp,gm,g_cov,g_contr,det,idet,psi,lapse,shift,Qloc,B_contr,B_cov,sm_cov,sm,dd,gamma1,gam,s2,b2,sb,sb2,e
    !dir$ attributes align: 64:: x1,x2,w,eps,v2,p,rho,vx,vy,vz,v_cov,v_contr,den,vb,p0,tau,dprho,dpeps,Lor,h,rhoeps,iErrV,FAILED
    !DIR$ ASSUME_ALIGNED V : 64 
    !DIR$ ASSUME_ALIGNED Q : 64 
#else
    !dir$ attributes align: 32:: gp,gm,g_cov,g_contr,det,idet,psi,lapse,shift,Qloc,B_contr,B_cov,sm_cov,sm,dd,gamma1,gam,s2,b2,sb,sb2,e 
    !dir$ attributes align: 32:: x1,x2,w,eps,v2,p,rho,vx,vy,vz,v_cov,v_contr,den,vb,p0,tau,dprho,dpeps,Lor,h,rhoeps,iErrV,FAILED
    !DIR$ ASSUME_ALIGNED V : 32 
    !DIR$ ASSUME_ALIGNED Q : 32 
#endif 
    !
    !RETURN 
    !
    psi(:)   = Q(:,9)
    lapse(:) = Q(:,10)
	DO i=1,3
		shift(:,i) = Q(:,10+i)
	ENDDO
	DO i=1,6
		V(:,13+i) = Q(:,13+i)
	ENDDO
    !
    g_cov(:,1,1) = Q(:,14)
    g_cov(:,1,2) = Q(:,15)
    g_cov(:,1,3) = Q(:,16)
    g_cov(:,2,2) = Q(:,17)
    g_cov(:,2,3) = Q(:,18)
    g_cov(:,3,3) = Q(:,19)
    g_cov(:,2,1) = Q(:,15)
    g_cov(:,3,1) = Q(:,16)
    g_cov(:,3,2) = Q(:,18)
    !
    det(:) = (g_cov(:,1,1)*g_cov(:,2,2)*g_cov(:,3,3)-g_cov(:,1,1)*g_cov(:,2,3)**2-g_cov(:,1,2)**2*g_cov(:,3,3)+2*g_cov(:,1,2)*g_cov(:,1,3)*g_cov(:,2,3)-g_cov(:,1,3)**2*g_cov(:,2,2))  
    idet(:) = 1./det(:)  
    !
    g_contr(:,1,1) =  ( g_cov(:,2,2)*g_cov(:,3,3)-g_cov(:,2,3)*g_cov(:,3,2)) * idet(:) 
    g_contr(:,1,2) = -( g_cov(:,1,2)*g_cov(:,3,3)-g_cov(:,1,3)*g_cov(:,3,2)) * idet(:)
    g_contr(:,1,3) = -(-g_cov(:,1,2)*g_cov(:,2,3)+g_cov(:,1,3)*g_cov(:,2,2)) * idet(:) 
    g_contr(:,2,1) = -( g_cov(:,2,1)*g_cov(:,3,3)-g_cov(:,2,3)*g_cov(:,3,1)) * idet(:) 
    g_contr(:,2,2) =  ( g_cov(:,1,1)*g_cov(:,3,3)-g_cov(:,1,3)*g_cov(:,3,1)) * idet(:) 
    g_contr(:,2,3) = -( g_cov(:,1,1)*g_cov(:,2,3)-g_cov(:,1,3)*g_cov(:,2,1)) * idet(:) 
    g_contr(:,3,1) = -(-g_cov(:,2,1)*g_cov(:,3,2)+g_cov(:,2,2)*g_cov(:,3,1)) * idet(:) 
    g_contr(:,3,2) = -( g_cov(:,1,1)*g_cov(:,3,2)-g_cov(:,1,2)*g_cov(:,3,1)) * idet(:) 
    g_contr(:,3,3) =  ( g_cov(:,1,1)*g_cov(:,2,2)-g_cov(:,1,2)*g_cov(:,2,1)) * idet(:)       
    ! 
    gp(:) = SQRT(det)
    gm(:) = 1./gp(:) 
    ! 
	DO i=1,8
		Qloc(:,i)  = gm(:)*Q(:,i)
	ENDDO
    !
	dd(:) = Qloc(:,1)
	tau(:) = Qloc(:,5)
	DO i=1,3
		B_contr(:,i) = Qloc(:,5+i)  
		sm_cov(:,i)  = Qloc(:,1+i)
	ENDDO
    !
    gamma1 = EQN%gamma/(EQN%gamma - 1.0)
    gam    = 1.0/gamma1
    ! Solve for p
    FAILED  = .FALSE.
    !
    !sm   = MATMUL (g_contr, sm_cov)
	!B_cov   = MATMUL (g_cov, B_contr)
    ! 
    sm(:,1) = g_contr(:,1,1)*sm_cov(:,1) + g_contr(:,1,2)*sm_cov(:,2) + g_contr(:,1,3)*sm_cov(:,3) 
    sm(:,2) = g_contr(:,2,1)*sm_cov(:,1) + g_contr(:,2,2)*sm_cov(:,2) + g_contr(:,2,3)*sm_cov(:,3) 
    sm(:,3) = g_contr(:,3,1)*sm_cov(:,1) + g_contr(:,3,2)*sm_cov(:,2) + g_contr(:,3,3)*sm_cov(:,3) 
	!
    B_cov(:,1) = g_cov(:,1,1)*B_contr(:,1) + g_cov(:,1,2)*B_contr(:,2) + g_cov(:,1,3)*B_contr(:,3) 
    B_cov(:,2) = g_cov(:,2,1)*B_contr(:,1) + g_cov(:,2,2)*B_contr(:,2) + g_cov(:,2,3)*B_contr(:,3) 
    B_cov(:,3) = g_cov(:,3,1)*B_contr(:,1) + g_cov(:,3,2)*B_contr(:,2) + g_cov(:,3,3)*B_contr(:,3) 
    
    s2(:)  = sm_cov(:,1)*sm(:,1) + sm_cov(:,2)*sm(:,2) + sm_cov(:,3)*sm(:,3)
	b2(:)  = B_contr(:,1)*B_cov(:,1) + B_contr(:,2)*B_cov(:,2) + B_contr(:,3)*B_cov(:,3)
	sb(:)  = sm_cov(:,1)*B_contr(:,1) + sm_cov(:,2)*B_contr(:,2) + sm_cov(:,3)*B_contr(:,3)
    sb2(:) = sb(:)*sb(:)  
	!
    ! First option [Del Zanna et al. (2007) A&A, 473, 11-30 (method 3)]
	e(:)    = Qloc(:,5) + dd(:)      ! Q(5) = gamma^1/2 ( U - dd )
    x1(:)   = 0.         ! 
	eps(:)  = 1.0e-10
	x2(:)   = 1.0-eps    !tol ! 
    w(:)    = 0 
    !
    ! THIS IS ONLY FOR THE GRHD.
#ifdef GEOS
  IF(ANY(v(:,5).GT.0)) THEN
    p0 = v(:,5)
  ELSE
    p0 = NSTOV_p_atmo  ! initial guess 
  ENDIF
  !p0(:) = NSTOV_p_atmo  ! initial guess 
  CALL C2P_GEOS_vector(p,rho,eps,p0,tau,DD,s2,FAILED)  !  (tau,D,s2) => (p,rho,eps)
  !
    WHERE(FAILED(:)) 
	    ! 
        iErrV(:)    = -1
        p(:)    = p_floor
        rho(:)  = rho_floor
        vx(:)   = 0.0
        vy(:)   = 0.0
        vz(:)   = 0.0
	    v_cov(:,1) = 0. 
	    v_cov(:,2) = 0. 
	    v_cov(:,3) = 0. 
	    !
    ELSEWHERE 
        Lor(:) = DD(:)/rho(:)
        h(:) = (tau(:) + DD(:) + p(:))  ! rho*enthampy*Lor**2 
        v_cov(:,1) = sm_cov(:,1)/h(:) 
        v_cov(:,2) = sm_cov(:,2)/h(:) 
        v_cov(:,3) = sm_cov(:,3)/h(:) 
    
        !WHERE(rho(:)<1e-5)    ! for the first successfull TOV star simulations, this stuff was computed before p. 
        WHERE(rho(:)<NSTOV_rho_atmo*(1.0+0.02))     
            v_cov(:,1)=0.0
            v_cov(:,2)=0.0
            v_cov(:,3)=0.0
        ENDWHERE
        ! 
        WHERE( rho(:) < NSTOV_rho_atmo ) 
            rho(:) = NSTOV_rho_atmo
        END WHERE                    
        ! 
        !WHERE(p(:)<1e-12)
        WHERE(p(:)<NSTOV_p_atmo*(1.0+0.02))
            v_cov(:,1)=0.0
            v_cov(:,2)=0.0
            v_cov(:,3)=0.0
        ENDWHERE        
        WHERE(p(:)<NSTOV_p_atmo)
            p(:)   = NSTOV_p_atmo
        ENDWHERE        
        !
    ENDWHERE
  !
    IF(ANY(p.LT.NSTOV_p_atmo)) THEN
        continue
    ENDIF
    !
#else
    v2 = 0.0 
    !
    v_cov(:,1) = V(:,2) 
    v_cov(:,2) = V(:,3) 
    v_cov(:,3) = V(:,4) 
    !
    v_contr(:,1) = g_contr(:,1,1)*V(:,2) + g_contr(:,1,2)*V(:,3) + g_contr(:,1,3)*V(:,4) 
    v_contr(:,2) = g_contr(:,2,1)*V(:,2) + g_contr(:,2,2)*V(:,3) + g_contr(:,2,3)*V(:,4) 
    v_contr(:,3) = g_contr(:,3,1)*V(:,2) + g_contr(:,3,2)*V(:,3) + g_contr(:,3,3)*V(:,4) 
    !
    v2 = v_cov(:,1)*v_contr(:,1) + v_cov(:,2)*v_contr(:,2) + v_cov(:,3)*v_contr(:,3) 
    !
	CALL RTSAFE_C2P_RMHD1_VECTOR(v2,x1,x2,eps,gam,dd,e,s2,b2,sb2,w,FAILED,VECTORLENGTH) 
    !
    WHERE(FAILED(:)) 
	    ! 
        iErrV(:)    = -1
        p(:)    = p_floor
        rho(:)  = rho_floor
        vx(:)   = 0.0
        vy(:)   = 0.0
        vz(:)   = 0.0
	    v_cov(:,1) = 0. 
	    v_cov(:,2) = 0. 
	    v_cov(:,3) = 0. 
	    !
    ELSEWHERE 
        
        iErrV(:)    = 0 

        den(:)  = 1.0/(w(:)+b2(:))
        vb(:)   = sb(:)/w(:)
        !
        rho(:)  = dd(:)*sqrt(1.-v2(:))
	    v_cov(:,1) = (sm_cov(:,1) + vb(:)*B_cov(:,1))*den(:)
	    v_cov(:,2) = (sm_cov(:,2) + vb(:)*B_cov(:,2))*den(:)
	    v_cov(:,3) = (sm_cov(:,3) + vb(:)*B_cov(:,3))*den(:)
        ! 
        WHERE(rho(:)<1e-5)    ! for the first successfull TOV star simulations, this stuff was computed before p. 
        !WHERE(rho(:)<NSTOV_rho_atmo*(1.0+0.02))     
            v_cov(:,1)=0.0
            v_cov(:,2)=0.0
            v_cov(:,3)=0.0
        ENDWHERE
        ! 
               
       !! WHERE( rho(:) < 1e-10 ) 
       !!     rho(:) = 1e-10 
       !! !WHERE( rho(:) < NSTOV_rho_atmo ) 
       !! !    rho(:) = NSTOV_rho_atmo
       !!  END WHERE                    
        !
        p(:) = gam(:)*(w(:)*(1.-v2(:))-rho(:)) 
        !p(:) = gam(:)*(w(:)*(1.-v2(:))-rho(:))
        !
        WHERE(p(:)<1e-12)
        !WHERE(p(:)<NSTOV_p_atmo*(1.0+0.02))
            v_cov(:,1)=0.0
            v_cov(:,2)=0.0
            v_cov(:,3)=0.0
        ENDWHERE        
        !WHERE(p(:)<NSTOV_p_atmo)
        !    p(:)   = NSTOV_p_atmo
        !ENDWHERE        
        !
        ! new 
        !
        WHERE( dd(:)<1e-6 ) 
           rho(:)     = dd(:)   
           v_cov(:,1) = 0.0
           v_cov(:,2) = 0.0
           v_cov(:,3) = 0.0
           p(:)  = gam(:)*tau(:) 
        END WHERE   
        
        WHERE(dd(:)<=0.0)
           rho(:)     = 0.0     
           v_cov(:,1) = 0.0
           v_cov(:,2) = 0.0
           v_cov(:,3) = 0.0
           p(:)       = 0.0     
        END WHERE
        
        WHERE(tau(:)<=0.0) 
           rho(:)     = 0.0     
           v_cov(:,1) = 0.0
           v_cov(:,2) = 0.0
           v_cov(:,3) = 0.0
           p(:)       = 0.0               
        END WHERE 
          
        
        !
    ENDWHERE 
    !
#endif
    !
    !WHERE(Q(:,1)<1e-6)  
    !    rho(:) = Q(:,1)   
    !    p(:)   = (EQN%gamma-1.0)*Q(:,5)  
    !    ww(:)  = rho(:) + EQN%gamma/(EQN%gamma-1.0)*p(:)   
    !    v_cov(:,1) = Q(:,2)*ww(:)/(ww(:)*ww(:)+1e-3) 
    !    v_cov(:,2) = Q(:,3)*ww(:)/(ww(:)*ww(:)+1e-3) 
    !    v_cov(:,3) = Q(:,4)*ww(:)/(ww(:)*ww(:)+1e-3) 
    !END WHERE 
    !     
	V(:,1)  = rho(:)
	V(:,2)  = v_cov(:,1)
	V(:,3)  = v_cov(:,2)
	V(:,4)  = v_cov(:,3)
	V(:,5)  = p(:)
	V(:,6)  = B_contr(:,1)
	V(:,7)  = B_contr(:,2)
	V(:,8)  = B_contr(:,3)
	V(:,9)  = psi(:)
	V(:,10) = lapse(:)
	V(:,11) = shift(:,1)
	V(:,12) = shift(:,2)
	V(:,13) = shift(:,3)
    !
    iErr = MINVAL(iErrV)
    !
END SUBROUTINE PDECons2PrimVectorGRMHD        
!    
#endif 
!
! Conservative part of the PDE ( flux tensor F(Q) ) 
!
RECURSIVE SUBROUTINE PDESourcePrimGRMHD(S,V,Q,time)
    USE MainVariables, ONLY : d, EQN 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
    ! Argument list 
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    REAL, INTENT(IN)  :: Q(nVar), V(nVar), time   
    REAL, INTENT(OUT) :: S(nVar) 
    ! Local variables 
    REAL :: p, irho, lam, mu 
    REAL :: k1, k2, fff, ggg, e, ds, c, xi, sk, sknl, alpha, fa, k0, beta0(3), eta 
    REAL :: g_cov(3,3), det, g_contr(3,3), Christoffel(3,3,3), dgup(3,3,3), faa, b0(3), dChristoffelSrc(3,3,3,3), RiemannSrc(3,3,3,3)
    REAL :: RicciSrc(3,3), gammaup(3), dtmetric(3,3), dtgup(3,3), dtChristoffelSrc(3,3,3), dtGammaUpSrc(3)
    REAL :: s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15,s16,s17,s18,s19,s20,s21,s22,s23,s24,s25
    !
    S = 0.0     
    !
END SUBROUTINE PDESourcePrimGRMHD
!

RECURSIVE SUBROUTINE PDESourceGRMHD(S,Q,time)
    USE MainVariables, ONLY : d, EQN 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
    ! Argument list 
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    REAL, INTENT(IN)  :: Q(nVar), time   
    REAL, INTENT(OUT) :: S(nVar) 
    ! Local variables 
    REAL :: p, irho, lam, mu 
    REAL :: k1, k2, fff, ggg, e, ds, c, xi, sk, sknl, alpha, fa, k0, beta0(3), eta 
    REAL :: g_cov(3,3), det, g_contr(3,3), Christoffel(3,3,3), dgup(3,3,3), faa, b0(3), dChristoffelSrc(3,3,3,3), RiemannSrc(3,3,3,3)
    REAL :: RicciSrc(3,3), gammaup(3), dtmetric(3,3), dtgup(3,3), dtChristoffelSrc(3,3,3), dtGammaUpSrc(3)
    REAL :: s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15,s16,s17,s18,s19,s20,s21,s22,s23,s24,s25
    !
    S = 0.0     
    !
END SUBROUTINE PDESourceGRMHD
!

RECURSIVE SUBROUTINE PDEEigenvaluesGRMHD(L,Q,n)
    USE MainVariables, ONLY : EQN,d 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    !USE MainVariables, ONLY : d, EQN,DivCleaning_a
	!USE AstroMod
	!USE recipies_mod
    IMPLICIT NONE
    ! Argument list 
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    INTEGER, PARAMETER :: nParam=0 
    REAL, INTENT(IN)  :: Q(nVar),n(3)
    REAL, INTENT(OUT) :: L(nVar) 
    ! Local variables 
    REAL ::  Vp(nVar), gradQ(nVar,d)
    REAL :: p, irho
    REAL :: V(nVar),Qx(nVar), Qy(nVar), Qz(nVar) , Qp(nVar), Qm(nVar) 
    REAL :: e,g_cov(3,3),g_contr(3,3),nv(3)
    REAL :: AQx(nVar), BQy(nVar), CQz(nVar), FFp(nVar,d), FFm(nVar,d) ,dfdQ(nVar,nVar),AA(nVar,nVar)
    REAL :: lapse, shift(3),shift_cov(3), gammaij(6), delta(3,3), B_cov(3), vxB_cov(3), vxb_contr(3), psi, S_contr(3), qb_contr(3), B_contr(3) 
    REAL :: v2,v_contr(3),uem,b2,e2,gp,gm,vc(nVar),lf,w,ww,gamma1,rho,v_cov(3), w_ij, wim,eps,u,xg(3)
	REAL :: b2_4,cs2,sft,VdotB,vn,den,gg,lf2m,ImLambda(nVar), rtemp(nVar) ,R(nVar,nVar), iR(nVar,nVar),fmm(nVar),gmm(nVar),hmm(nVar),fpp(nVar),gpp(nVar),hpp(nVar),gg2,sft2,vn2
    INTEGER :: i,j,k,m,iErr, ccount, itemp(nVar) 
    LOGICAL :: FLAG
    REAL :: rhoeps,dd,tau,s2,sm_cov(3),sm(3)
    !
    !
    xg(1) = 0.
    xg(2) = 0.
    xg(3) = 0.
   !
   !****************************************************
    ! Eigenvalues as given by Olindo 
    CALL PDECons2PrimGRMHD(V,Q,iErr)
    rho            = V(1)
    DO i=1,3
	    v_cov(i)   = V(1+i)
    ENDDO
    p              = V(5)
    DO i=1,3
	    B_contr(i) = V(5+i)
    ENDDO
    psi            = V(9)
    lapse          = V(10)
    DO i=1,3
	    shift(i)   = V(10+i)
    ENDDO
    !
    g_cov(1,1)   = V(14)
    g_cov(1,2)   = V(15)
    g_cov(1,3)   = V(16)
    g_cov(2,2)   = V(17)
    g_cov(2,3)   = V(18)
    g_cov(3,3)   = V(19)
    g_cov(2,1)   = V(15)
    g_cov(3,1)   = V(16)
    g_cov(3,2)   = V(18)
    !
    CALL MatrixInverse3x3(g_cov,g_contr,gp)
    gp = SQRT(gp)
    gm = 1./gp
    !  evaluate contr. cov. variables
    v_contr      = MATMUL(g_contr,v_cov)
    B_cov      = MATMUL(g_cov,B_contr)
    !  evaluate useful quantities
    v2     = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
    lf     = 1.0/sqrt(1.0 - v2)
    lf2m   = 1.0 - v2
    b2     = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
    VdotB     = v_contr(1)*B_cov(1) + v_contr(2)*B_cov(2) + v_contr(3)*B_cov(3)
    b2_4 = b2*lf2m + VdotB  ! this is b^2
    gamma1 = EQN%gamma/(EQN%gamma-1.0)  
    !
    !
    dd = gm*Q(1)
    tau = gm*Q(5)
    DO i=1,3 
        sm_cov(i)  = Q(1+i)
    ENDDO
    sm_cov = gm*sm_cov
    sm   = MATMUL (g_contr, sm_cov)  
    s2   = sm_cov(1)*sm(1) + sm_cov(2)*sm(2) + sm_cov(3)*sm(3)
    !
#ifdef GEOS  
    CALL EOS_eps(rhoeps,rho,p,dd,tau,s2) 
    w      = rho + rhoeps + p  + b2_4  ! rho*enthalpy
#else
    w      = rho + gamma1*p + b2_4 ! this is rho*h + b^2
#endif
    !IF(ABS(w-(rho + gamma1*p + b2_4)).GT.1e-13) THEN
    !    continue
    !ENDIF
    !
    !
    !w      = rho + gamma1*p + b2_4 ! this is rho*h + b^2
    cs2    = (EQN%gamma*p + b2_4)/w
    !
    vn     = v_contr(1)*n(1) + v_contr(2)*n(2) + v_contr(3)*n(3)
    sft    = shift(1)*n(1) + shift(2)*n(2) + shift(3)*n(3) 
    gg     = g_contr(1,1)*ABS(n(1)) + g_contr(2,2)*ABS(n(2)) + g_contr(3,3)*ABS(n(3))
    den    = 1.0/(1.0 - v2*cs2)
    IF(SUM(n**2).EQ.0.) THEN  
        u = SQRT( v2) 
        WRITE(*,*)'Impossible error!'
        STOP
    ELSE
        u = vn
    ENDIF
    L(1)   = ( u*(1.0-cs2) - SQRT( cs2*lf2m*( (1.0-v2*cs2)*gg - u**2*(1.0-cs2) )) )*den
    L(2:4) = u
    L(5)   = ( u*(1.0-cs2) + SQRT( cs2*lf2m*( (1.0-v2*cs2)*gg - u**2*(1.0-cs2) )) )*den 
	DO i=1,5
		L(i)   = lapse*L(i) - sft
	ENDDO
    !
	DO i=6,nVar
		L(i)   = 0.
	ENDDO
    !
    ! SAFE MODE: define also 'covariant' eigenvalues! (we may use the remaining free slots in L, L(6:8), L(10:19)
    !
    shift_cov = MATMUL(g_cov,shift)
    !
    vn2  = v_cov(1)*n(1) + v_cov(2)*n(2) + v_cov(3)*n(3)
    sft2 = shift_cov(1)*n(1) + shift_cov(2)*n(2) + shift_cov(3)*n(3) 
    gg2  = g_cov(1,1)*ABS(n(1)) + g_cov(2,2)*ABS(n(2)) + g_cov(3,3)*ABS(n(3))
    den  = 1.0/(1.0 - v2*cs2)
    !
    u = vn2 
    !
    L(10)  = ( u*(1.0-cs2) - SQRT( cs2*lf2m*( (1.0-v2*cs2)*gg2 - u**2*(1.0-cs2) )) )*den
    L(6:8) = u
    L(11)  = ( u*(1.0-cs2) + SQRT( cs2*lf2m*( (1.0-v2*cs2)*gg2 - u**2*(1.0-cs2) )) )*den 
    L(6:11)   = lapse*L(6:11) - sft2
    ! 
    L(9) =  EQN%DivCleaning_a   ! 1.  !EQN%ch
    !
    FLAG = .FALSE.
    IF(MAXVAL(ABS(L)).GT.1.01) THEN
        FLAG = .TRUE.
        continue
    ENDIF
    ! 
    !
    !
	!
	continue
	!
END SUBROUTINE

RECURSIVE SUBROUTINE PDEMatrixBGRMHD(Bn,Q,nv)
    USE MainVariables, ONLY : d, EQN 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
  ! Argument list 
  REAL, INTENT(IN)  :: Q(nVar), nv(d)   
  REAL, INTENT(OUT) :: Bn(nVar,nVar) 
  ! Local variables 
  INTEGER :: i 
  REAL    :: p, irho, lam, mu, ialpha  
  REAL    :: B1(nVar,nVar), B2(nVar,nVar), B3(nVar,nVar)  
  REAL    :: A(nVar,nVar), B(nVar,nVar), C(nVar,nVar)  
  REAL    :: k1, k2, fff, ggg, e, ds, cs, xi, sk, sknl, alpha, fa, k0, beta0(3), b0(3)   
  REAL    :: g_cov(3,3), g_contr(3,3), det, Christoffel(3,3,3), dgup(3,3,3), uv(3), bb2   
  REAL    :: lapse, shift(3), gammaij(6), delta(3,3), B_cov(3), vxB_cov(3), vxb_contr(3), psi, S_contr(3),  B_contr(3) 
  REAL    :: v2,v_contr(3),uem,e2,gp,gm,vc(nVar),lf,w,ww,gamma1,rho,v_cov(3), w_ij, wim    
  INTEGER :: j,k,l,m,iErr, ccount    
  REAL :: rhoeps,dd,tau,s2,sm_cov(3),sm(3)
  !
  Bn = 0.0
  !
  lapse = Q(10)
  DO i=1,3
	shift(i) = Q(10+i)
  ENDDO
  DO i=1,6
	gammaij(i) = Q(13+i) 
  ENDDO
  !
  g_cov(1,1) = Q(14)
  g_cov(1,2) = Q(15)
  g_cov(1,3) = Q(16)
  g_cov(2,2) = Q(17)
  g_cov(2,3) = Q(18)
  g_cov(3,3) = Q(19)
  g_cov(2,1) = Q(15)
  g_cov(3,1) = Q(16)
  g_cov(3,2) = Q(18)
  !
  DO j=1,3
	  DO i=1,3
		delta(i,j) = 0.
	  ENDDO 
  ENDDO
  !  
  DO i=1,3
     delta(i,i) = 1.0
  ENDDO 
  !
  CALL MatrixInverse3x3(g_cov,g_contr,gp)
  gp = SQRT(gp)
  gm = 1./gp
  ! 
  CALL PDECons2PrimGRMHD(Vc,Q,iErr)
  gamma1 = EQN%gamma/(EQN%gamma-1.0)
  rho    = Vc(1)
  DO i=1,3
	v_cov(i) = Vc(1+i)
	B_contr(i) = Vc(5+i)
  ENDDO
  p      = Vc(5)
  !
  B_cov = MATMUL(g_cov,B_contr(1:3))
  psi = Vc(9) 
  !
  v_contr     = MATMUL(g_contr,v_cov)
  S_contr = MATMUL(g_contr,Q(2:4))
  !
  ! COVARIANT =>
  vxB_cov(1) = v_contr(2)*B_contr(3) - v_contr(3)*B_contr(2)
  vxB_cov(2) = v_contr(3)*B_contr(1) - v_contr(1)*B_contr(3)
  vxB_cov(3) = v_contr(1)*B_contr(2) - v_contr(2)*B_contr(1)
  vxB_cov(1:3) = gp*vxB_cov(1:3)
  ! 
  ! CONTRAVARIANT =>
  vxB_contr(1) = v_cov(2)*B_cov(3) - v_cov(3)*B_cov(2)
  vxB_contr(2) = v_cov(3)*B_cov(1) - v_cov(1)*B_cov(3)
  vxB_contr(3) = v_cov(1)*B_cov(2) - v_cov(2)*B_cov(1)
  vxB_contr(1:3) = gm*vxB_contr(1:3)
  !
  v2     = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
  e2     = vxB_contr(1)*vxB_cov(1) + vxB_contr(2)*vxB_cov(2) + vxB_contr(3)*vxB_cov(3)
  bb2    = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
  !
  uem    = 0.5*(bb2 + e2) 
  !
  lf     = 1.0/sqrt(1.0 - v2) 
  !
  !
  dd = gm*Q(1)
  tau = gm*Q(5)
  DO i=1,3 
      sm_cov(i)  = Q(1+i)
  ENDDO
  sm_cov = gm*sm_cov
  sm   = MATMUL (g_contr, sm_cov)  
  s2   = sm_cov(1)*sm(1) + sm_cov(2)*sm(2) + sm_cov(3)*sm(3)
  !
#ifdef GEOS  
  CALL EOS_eps(rhoeps,rho,p,dd,tau,s2) 
  w      = rho + rhoeps + p   ! rho*enthalpy
#else
  w      = rho + gamma1*p  ! this is rho*h
#endif
    !IF(ABS(w-(rho + gamma1*p)).GT.1e-13) THEN
    !    continue
    !ENDIF
    !
  !
  !w      = rho + gamma1*p   ! rho*hentalpy
  ww     = w*lf**2          ! rho*hentalpy*Lorentz^2 
  !
  !DO j=1,3
  A = 0.
  B = 0.
  C = 0.
    !
    ccount=0
    ! 
    DO i=1,3
        ! shift
        !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        j=1
        !------ 
        W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
        !
        A(1+j,10+i) = - Q(1+i)  ! Q(11:13)  shift(i) or shift_contr(i)
        A(5,10+i) = - gp*W_ij    ! Q(11:13)  shift(i) or shift_contr(i)
        !
        !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        j=2
        !------ 
        W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
        !
        B(1+j,10+i) = - Q(1+i)  ! Q(11:13)  shift(i) or shift_contr(i)
        B(5,10+i) = - gp*W_ij    ! Q(11:13)  shift(i) or shift_contr(i)
        ! 
        !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        j=3
        !------
        W_ij = ww*v_cov(i)*v_contr(j)-vxB_cov(i)*vxB_contr(j)-B_cov(i)*B_contr(j)+(p+uem)*delta(i,j)
        !
        C(1+j,10+i) = - Q(1+i)  ! Q(11:13)  shift(i) or shift_contr(i)
        C(5,10+i) = - gp*W_ij    ! Q(11:13)  shift(i) or shift_contr(i)
        ! 
          DO m=1,3
            IF(m.GE.i) THEN  
                !metric
                ccount=ccount+1
                !
                Wim = ww*v_contr(i)*v_contr(m)-vxB_contr(i)*vxB_contr(m)-B_contr(i)*B_contr(m)+(p+uem)*g_contr(i,m)
                !Wim = Wim + (1.0 - delta(i,m))*(ww*v_contr(m)*v_contr(i)-vxB_contr(m)*vxB_contr(i)-B_contr(m)*B_contr(i)+(p+uem)*g_contr(m,i))  ! account also of the remaining symmetric components of gamma for i.NE.m.
                IF(i.NE.m) THEN !TWICE! account also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    Wim = 2.0*Wim   
                ENDIF
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=1
                !------ 
                A(1+j,13+ccount) = - 0.5*gp*lapse*Wim  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                A(5,13+ccount) = - 0.5*gp*Wim*shift(j)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)

#ifdef COVCLEAN     
				A(9,13+ccount) =  + 0.5*Q(9)*gammaij(ccount)*shift(j)    ! D.C.
                A(6:8,13+ccount) =  - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount) 
                !
                IF(i.NE.m) THEN !TWICE!  account  also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    A(9,13+ccount) = A(9,13+ccount) + 0.5*Q(9)*gammaij(ccount)*shift(j)  ! D.C.
                    A(6:8,13+ccount) = A(6:8,13+ccount) - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount) 
                ENDIF
#endif  	
                !
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=2
                !------ 
                B(1+j,13+ccount) = - 0.5*gp*lapse*Wim  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                B(5,13+ccount) = - 0.5*gp*Wim*shift(j)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)

#ifdef COVCLEAN     
				B(9,13+ccount) =  + 0.5*Q(9)*gammaij(ccount)*shift(j)   ! D.C.
                B(6:8,13+ccount) =  - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount)
                !
                IF(i.NE.m) THEN !TWICE!  account  also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    B(9,13+ccount) = B(9,13+ccount) + 0.5*Q(9)*gammaij(ccount)*shift(j)   ! D.C.
                    B(6:8,13+ccount) = B(6:8,13+ccount) - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount)
                ENDIF
#endif  		
                ! 
                !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
                j=3
                !------
                C(1+j,13+ccount) = - 0.5*gp*lapse*Wim  ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
                C(5,13+ccount) = - 0.5*gp*Wim*shift(j)   ! Q(14:19) gammaij(ccount) or  g_cov(i,m)
#ifdef COVCLEAN     
				C(9,13+ccount) =  + 0.5*Q(9)*gammaij(ccount)*shift(j)   ! D.C.
                C(6:8,13+ccount) =  - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount)
                !
                IF(i.NE.m) THEN !TWICE!  account  also of the remaining symmetric components of gamma §(memo: g_contr and Wim are symmetric)
                    C(9,13+ccount) = C(9,13+ccount) + 0.5*Q(9)*gammaij(ccount)*shift(j)   ! D.C.
                    C(6:8,13+ccount) = C(6:8,13+ccount) - 0.5*lapse*Q(9)*g_contr(1:3,j)*gammaij(ccount)
                ENDIF
#endif  		
                ! 
            ENDIF
          ENDDO
      ENDDO
    !lapse
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=1
    !------ 
    A(1+j,10) = + (Q(5)+Q(1))   ! Q(10) or lapse
    A(5,10) =  S_contr(j)     !  Q(10) or lapse

#ifdef COVCLEAN     
	A(9,10) = - Q(5+j)             ! D.C.
    A(9,10+j) = + Q(9)             ! D.C.   phi * d(shift^x)/dx
    A(6:8,5+j) = - Q(11:13)             ! D.C.   shift * d(gp*B^x)/dx
    A(6:8,9) = lapse*g_contr(1:3,j)  ! D.C.  
#else
    A(6:8,5+j) = - Q(11:13)             ! D.C.   shift * d(gp*B^x)/dx 
    A(6:8,9) = lapse*gp*g_contr(1:3,j)  ! D.C.   
    A(9,5+j) = gm*lapse*EQN%DivCleaning_a**2             ! D.C.   shift * d(gp*B^x)/dx 
    A(9,9) = - Q(10+j)            ! D.C.   shift * d(gp*B^x)/dx  
#endif
    !
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=2
    !------ 
    B(1+j,10) = + Q(5)+Q(1)   ! Q(10) or lapse
    B(5,10) =  S_contr(j)     !  Q(10) or lapse
!
#ifdef COVCLEAN     
	B(9,10) = - Q(5+j)            ! D.C.
    B(9,10+j) = + Q(9)            ! D.C.   phi * d(shift^y)/dy
    B(6:8,5+j) = - Q(11:13)                  ! D.C.   shift * d(gp*B^y)/dy
    B(6:8,9) = + lapse*g_contr(1:3,j)       ! D.C.  
#else
    B(6:8,5+j) =  - Q(11:13)                  ! D.C.   shift * d(gp*B^x)/dx 
    B(6:8,9) =  + lapse*gp*g_contr(1:3,j)    ! D.C.  
    B(9,5+j) = + gm*lapse*EQN%DivCleaning_a**2            ! D.C.   shift * d(gp*B^x)/dx 
    B(9,9) = - Q(10+j)             ! D.C.   shift * d(gp*B^x)/dx  
#endif
    ! 
    !xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    j=3
    !------
    C(1+j,10) = + Q(5)+Q(1)   ! Q(10) or lapse
    C(5,10) =  S_contr(j)     !  Q(10) or lapse
	!
#ifdef COVCLEAN     
	C(9,10) = - Q(5+j)             ! D.C.
    C(9,10+j) = + Q(9)             ! D.C.   phi *d(shift^z)/dz
    C(6:8,5+j) = - Q(11:13)                  ! D.C.   shift * d(gp*B^z)/dz
    C(6:8,9) = + lapse*g_contr(1:3,j)        ! D.C.  
#else
    C(6:8,5+j) = - Q(11:13)                  ! D.C.   shift * d(gp*B^x)/dx 
    C(6:8,9) = + lapse*gp*g_contr(1:3,j)     ! D.C.  
    C(9,5+j) = + gm*lapse*EQN%DivCleaning_a**2             ! D.C.   shift * d(gp*B^x)/dx 
    C(9,9) = - Q(10+j)             ! D.C.   shift * d(gp*B^x)/dx  
#endif
  !
  Bn = A*nv(1) + B*nv(2) + C*nv(3) 
  !  
END SUBROUTINE PDEMatrixBGRMHD  
    

RECURSIVE SUBROUTINE PDECons2PrimGRMHD(V,Q,iErr)
    USE MainVariables, ONLY :  EQN,p_floor,rho_floor,NSTOV_rho_atmo,NSTOV_p_atmo
#ifdef GEOS 	
    USE EOS_mod
#endif 	
	!USE AstroMod
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
    ! Argument list 
    REAL, INTENT(IN)     :: Q(nVar)     ! vector of conserved quantities 
    REAL, INTENT(OUT)    :: V(nVar)     ! primitive variables 
    INTEGER, INTENT(OUT) :: iErr        ! error flag 
    ! Local variables 
    REAL :: rho,p,psi,lapse,gp,gm
    REAL :: shift(3),v_cov(3),v_contr(3),vxB_cov(3),vxB_contr(3),B_cov(3),B_contr(3)
    REAL :: sm(3),sm_cov(3),Qtmp(nVar),Qloc(nVar)
    REAL :: g_cov(3,3),g_contr(3,3),gammaij(6)
    REAL :: vb,v2,e2,b2,s2,sb,sb2,e,x1,x2,eps,uem,LF,gamma1,gam,w,ww,depsdrho,depsdp
    REAL :: vx,vy,vz,den,dd
    REAL :: p0, tau,dprho,dpeps, Lor, h
    REAL :: tol
    INTEGER :: i
    LOGICAL              :: FAILED
    !    
    iErr = 0 
    tol = 1.0e-14
    !
    !V = Q
    !RETURN
	!V=0.
    psi = Q(9)
    lapse = Q(10)
    DO i=1,3
    	shift(i) = Q(10+i)
    ENDDO
    DO i=1,6
    	gammaij(i) = Q(13+i)
    ENDDO
    !
    g_cov(1,1) = Q(14)
    g_cov(1,2) = Q(15)
    g_cov(1,3) = Q(16)
    g_cov(2,2) = Q(17)
    g_cov(2,3) = Q(18)
    g_cov(3,3) = Q(19)
    g_cov(2,1) = Q(15)
    g_cov(3,1) = Q(16)
    g_cov(3,2) = Q(18)
    !
    IF(SUM(g_cov**2).LT.1e-9) THEN
    	print *, 'FATAL ERROR: g_cov = 0'
    	WRITE(*,*) g_cov(1,1:3)
    	WRITE(*,*) g_cov(2,1:3)
    	WRITE(*,*) g_cov(3,1:3)
    ENDIF
    CALL MatrixInverse3x3(g_cov,g_contr,gp)
    !
    gp = SQRT(gp)
    gm = 1./gp
    ! 
    DO i=1,8
        Qloc(i)  = gm*Q(i)
    ENDDO
    !
    dd = Qloc(1)
    tau = Qloc(5)
    DO i=1,3
        B_contr(i) = Qloc(5+i) ! !wrong: magnetic field is contravariant
        sm_cov(i)  = Qloc(1+i)
    ENDDO
    !
    gamma1 = EQN%gamma/(EQN%gamma - 1.0)
    gam    = 1.0/gamma1
    ! Solve for p
    FAILED  = .FALSE.
    !
    sm   = MATMUL (g_contr, sm_cov)
    !
    B_cov   = MATMUL (g_cov, B_contr)
    s2   = sm_cov(1)*sm(1) + sm_cov(2)*sm(2) + sm_cov(3)*sm(3)
    b2   = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
    sb   = sm_cov(1)*B_contr(1) + sm_cov(2)*B_contr(2) + sm_cov(3)*B_contr(3)
    sb2  = sb**2
    !
    ! First option [Del Zanna et al. (2007) A&A, 473, 11-30 (method 3)]
    e = tau + dd  ! Q(5) = gamma^1/2 ( U - dd )
    ! THIS IS ONLY FOR THE GRHD.
#ifdef GEOS
  IF(v(5).GT.0) THEN
    p0 = v(5)
  ELSE
    p0 = 0.5*(Q(5)+Q(1)) !1000000000*NSTOV_p_atmo  ! initial guess 
  ENDIF
  CALL C2P_GEOS(p,rho,eps,p0,tau,DD,s2,FAILED)  !  (tau,D,s2) => (p,rho,eps)  
  !
  !rho = V(1)      
  !v_cov(1) = V(2) 
  !v_cov(2) = V(3) 
  !v_cov(3) = V(4) 
  !p = V(5)        
  !!
  !IF(V(1).EQ.0) THEN
  !  p = NSTOV_p_atmo  ! initial guess 
  !  w = 0.            ! initial guess 
  !  v2=0.
  !  rho = dd*SQRT(1.0-v2)            ! initial guess 
  !  w = (rho*(1.0+0.5)+0.01)
  !  den  = 1.0/(w+b2)
  !  vb   = sb/w
  !  v_cov(1) = (sm_cov(1) + vb*B_cov(1))*den
  !  v_cov(2) = (sm_cov(2) + vb*B_cov(2))*den
  !  v_cov(3) = (sm_cov(3) + vb*B_cov(3))*den
  !  p = max(NSTOV_p_atmo, gam*(w*(1.-v2)-rho)) 
  !ENDIF
  !!
  !v2 =      g_contr(1,1)*v_cov(1)*v_cov(1)+g_contr(1,2)*v_cov(2)*v_cov(1)+g_contr(1,3)*v_cov(3)*v_cov(1)
  !v2 = v2 + g_contr(2,1)*v_cov(1)*v_cov(2)+g_contr(2,2)*v_cov(2)*v_cov(2)+g_contr(2,3)*v_cov(3)*v_cov(2)
  !v2 = v2 + g_contr(3,1)*v_cov(1)*v_cov(3)+g_contr(3,2)*v_cov(2)*v_cov(3)+g_contr(3,3)*v_cov(3)*v_cov(3)
  !! 
  !Lor = 1.0/SQRT(1.0-v2)
  !CALL EOS_eps_depsdrho_depsdp(eps,depsdrho,depsdp,rho,p) 
  !w = (rho*(1.0+eps)+p)*Lor**2
  !!  
  !CALL C2P_GEOS_3D(w,p,v2,gam,dd,e,s2,b2,sb2,FAILED)  !  (tau,D,s2) => (p,rho,eps)
  !!
  IF(FAILED) THEN
     IF(Q(1).GT.1e-10.AND.Q(5).GT.1e-10) iErr = +1
     rho = NSTOV_rho_atmo
     CALL EOS_p_dprho_dpeps(p,dprho,dpeps,rho,eps)
     rho  = rho_floor
     vx   = 0.0
     vy   = 0.0
     vz   = 0.0
     v_cov = 0.0  
     V(1:19) = (/ rho, v_cov(1:3), p, B_contr(1:3), psi , lapse, shift(1:3), gammaij(1:6)/)
     !CALL PDEPrim2Cons(Qtmp,V,x0_000,0.) 
     !CALL PDECons2Prim(V,Qtmp,x0_000,0.,iErrTMP)   
     continue
     RETURN
  ELSE
       ! C2P_GEOS pure hydro
      Lor = DD/rho
      h = (tau + DD + p)  ! rho*enthampy*Lor**2
      v_cov(1:3) = sm_cov(1:3)/h
      !
      ! C2P_GEOS_3D
      !den  = 1.0/(w+b2)
      !vb   = sb/w
      !!
      !rho  = dd*sqrt(1.-v2)
      !v_cov(1) = (sm_cov(1) + vb*B_cov(1))*den
      !v_cov(2) = (sm_cov(2) + vb*B_cov(2))*den
      !v_cov(3) = (sm_cov(3) + vb*B_cov(3))*den
      !!p = max(1.e-15, gam*(w*(1.-v2)-rho))
      !!p = gam*(w*(1.-v2)-rho)
  ENDIF
  !

#else
    x1   = 0.      ! 
    eps = 1.0e-10
    x2   = 1.0-eps !tol ! 
    w=0 
    !
    CALL RTSAFE_C2P_RMHD1(v2,x1,x2,tol,gam,dd,e,s2,b2,sb2,w,FAILED) 
    !
    IF (FAILED) THEN
        ! 
        iErr = -1
        p    = p_floor
        rho  = rho_floor
        vx   = 0.0
        vy   = 0.0
        vz   = 0.0
        v_cov(1) = 0. 
        v_cov(2) = 0. 
        v_cov(3) = 0. 
        !
    ELSE
        den  = 1.0/(w+b2)
        vb   = sb/w
        !
        rho  = dd*sqrt(1.-v2)
        v_cov(1) = (sm_cov(1) + vb*B_cov(1))*den
        v_cov(2) = (sm_cov(2) + vb*B_cov(2))*den
        v_cov(3) = (sm_cov(3) + vb*B_cov(3))*den
        !p = max(1.e-15, gam*(w*(1.-v2)-rho))
        p = gam*(w*(1.-v2)-rho)
    ENDIF
#endif  
  !
    IF(rho<NSTOV_rho_atmo*1.01) THEN   ! for the first successfull TOV star simulations, this stuff was computed before p. 
        v_cov(:)= 0.0
        !rho = 1e-12 
        !p   = 1e-12 
    ENDIF
    IF(rho<1e-11) THEN 
        rho = 1e-11
    !IF(rho<NSTOV_rho_atmo) THEN 
    !    rho = NSTOV_rho_atmo
    ENDIF 
    !
    IF(p<1e-12) THEN 
        v_cov(:)= 0.0
        !PRINT *,'I"m here!'
        !pause
        !rho = 1e-12 
        p   = 1e-12  
    !IF(p<NSTOV_p_atmo*1.01) THEN 
    !    v_cov(:)= 0.0 
    ENDIF   
    !IF(p<NSTOV_p_atmo) THEN 
    !    p=NSTOV_p_atmo
    !ENDIF   
    !
    IF( dd <1e-6 ) THEN  
        rho      = dd   
        v_cov(1) = 0.0
        v_cov(2) = 0.0
        v_cov(3) = 0.0
        p        = gam*tau  
    ENDIF 
    !
	V(1) = rho
	V(2) = v_cov(1)
	V(3) = v_cov(2)
	V(4) = v_cov(3)
	V(5) = p
	V(6) = B_contr(1)
	V(7) = B_contr(2)
	V(8) = B_contr(3)
	V(9) = psi
	V(10) = lapse
	V(11) = shift(1)
	V(12) = shift(2)
	V(13) = shift(3)
	DO i=1,6
		V(13+i) = gammaij(i)
    ENDDO
    V(19:nVar)=Q(19:nVar)
	!
	!CALL PDEPrim2ConsGRMHD(Qtmp,V)
	!
	!IF(MAXVAL(ABS(Qtmp-Q)).GT.1e-8) THEN
	!  iErr = +10   
	!		print *, 'FATAL ERROR: C2P o P2C'
	!		WRITE(*,*) MAXVAL(ABS(Qtmp-Q))  
	!		STOP
	!  continue
	!  !
	!ENDIF
	!  
  !  
    !Qtmp =Qtmp
    continue
    !
END SUBROUTINE PDECons2PrimGRMHD  

    
    
    
RECURSIVE SUBROUTINE PDEPrim2ConsGRMHD(Q,V)
    USE MainVariables, ONLY : d, EQN 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
	!USE AstroMod
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
  ! Argument list 
  REAL, INTENT(IN)      :: V(nVar)     ! primitive variables 
  REAL, INTENT(OUT)     :: Q(nVar)     ! vector of conserved quantities 
  ! Local variables 
  ! Local variable declaration
  REAL :: rho,p,psi,lapse,gp,gm
  REAL :: shift(3),v_cov(3),v_contr(3),vxB_cov(3),vxB_contr(3),B_cov(3),B_contr(3)
  REAL :: g_cov(3,3),g_contr(3,3)
  REAL :: vb,v2,e2,b2,uem,LF,gamma1,w,ww
  REAL :: rhoeps,b2_4
  INTEGER :: i
  ! 
  REAL :: Dd,gam,sb2,s_cov(3),s_contr(3),e,s2,f(3),df(3,3),x(3)
  !Q=V
  !RETURN
  rho     = V(1)
  v_cov  = V(2:4)
  p       = V(5)
  !
  psi = V(9)
  lapse = V(10)
  DO i=1,3
	  B_contr(i) = V(5+i)  !wrong 
	  shift(i) = V(10+i)          ! NB: we choose V() and Q() being the shift_controvariant!
  ENDDO
  !
  !gammaij = V(14:19) 
  g_cov(1,1) = V(14)
  g_cov(1,2) = V(15)
  g_cov(1,3) = V(16)
  g_cov(2,2) = V(17)
  g_cov(2,3) = V(18)
  g_cov(3,3) = V(19)
  g_cov(2,1) = V(15)
  g_cov(3,1) = V(16)
  g_cov(3,2) = V(18)
  !
  CALL MatrixInverse3x3(g_cov,g_contr,gp)
  gp = SQRT(gp)
  gm = 1./gp
  !  
  v_contr      = MATMUL(g_contr,v_cov)
  B_cov = MATMUL(g_cov,B_contr(1:3))
  !
  ! COVARIANT =>
  vxB_cov(1) = v_contr(2)*B_contr(3) - v_contr(3)*B_contr(2)
  vxB_cov(2) = v_contr(3)*B_contr(1) - v_contr(1)*B_contr(3)
  vxB_cov(3) = v_contr(1)*B_contr(2) - v_contr(2)*B_contr(1)
  vxB_cov(1) = gp*vxB_cov(1)
  vxB_cov(2) = gp*vxB_cov(2)
  vxB_cov(3) = gp*vxB_cov(3)
  !
  ! CONTRAVARIANT =>
  vxB_contr(1) = v_cov(2)*B_cov(3) - v_cov(3)*B_cov(2)
  vxB_contr(2) = v_cov(3)*B_cov(1) - v_cov(1)*B_cov(3)
  vxB_contr(3) = v_cov(1)*B_cov(2) - v_cov(2)*B_cov(1)
  vxB_contr(1) = gm*vxB_contr(1)
  vxB_contr(2) = gm*vxB_contr(2)
  vxB_contr(3) = gm*vxB_contr(3)
  !
  vb      = v_cov(1)*B_contr(1) + v_cov(2)*B_contr(2) + v_cov(3)*B_contr(3)      
  v2     = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
  e2     = vxB_contr(1)*vxB_cov(1) + vxB_contr(2)*vxB_cov(2) + vxB_contr(3)*vxB_cov(3)
  b2     = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
  !
  uem    = 0.5*(b2 + e2) 
  !
  !IF (v2 > 1.0) THEN
  !     ! IF(MAXVAL(ABS(xGP)).LE.Exc_radius) THEN  ! RETURN
  !   WRITE(*,*)'Superluminal velocity in PDEPrim2Cons!!'
  !   STOP
  !ENDIF
  lf     = 1.0 / sqrt(1.0 - v2)
  gamma1 = EQN%gamma/(EQN%gamma-1.0)  
  !
#ifdef GEOS  
  CALL EOS_eps_P2C(rhoeps,rho,p) 
  w      = rho + rhoeps + p  ! rho*enthalpy
#else
  w      = rho + gamma1*p  ! this is rho*h
#endif
    !IF(ABS(w-(rho + gamma1*p)).GT.1e-13) THEN
    !    continue
    !ENDIF
  !
  !w      = rho + gamma1*p
  ww     = w*lf**2
  !
  Q(1)    = rho*lf
  Q(2)  = ww*v_cov(1) + b2*v_cov(1) - vb*B_cov(1)
  Q(3)  = ww*v_cov(2) + b2*v_cov(2) - vb*B_cov(2)
  Q(4)  = ww*v_cov(3) + b2*v_cov(3) - vb*B_cov(3) 
  Q(5)  = ww - p + uem - Q(1)     !!!!! we subtract PDE(Q(1))!!!!
  Q(6) = V(6)
  Q(7) = V(7)
  Q(8) = V(8)
  !
  !! test functional
  !gam = 1.0/gamma1
  !dd=Q(1)
  !s_cov(1) = Q(2)
  !s_cov(2) = Q(3)
  !s_cov(3) = Q(4)
  !e=(Q(5)+Q(1))
  !s_contr = MATMUL(g_contr,s_cov)
  !s2 = s_cov(1)*s_contr(1) + s_cov(2)*s_contr(2) + s_cov(3)*s_contr(3)
  !sb2= (s_cov(1)*B_contr(1) + s_cov(2)*B_contr(2) + s_cov(3)*B_contr(3))**2
  !x(1) = v2
  !x(2) = ww
  !x(3) = p
  !CALL FUNC_C2P_GEOS_3D(x,f,df,gam,dd,e,s2,b2,sb2)
  !!
  !IF(SUM(ABS(f)).GT.1e-13)THEN
  !      continue
  !ENDIF
  !
  DO i=1,8
		Q(i) = gp*Q(i)  
  ENDDO
  DO i=9,nVar
		Q(i) = V(i)
  ENDDO
  ! 
  continue
    !
END SUBROUTINE PDEPrim2ConsGRMHD  
    
    
#ifdef VECTOR 
    
RECURSIVE SUBROUTINE PDEPrim2ConsGRMHDvector(Q,V)
    USE MainVariables, ONLY : d, EQN, VECTORLENGTH 
#ifdef GEOS 	
    USE EOS_mod
#endif 	
	!USE AstroMod
    IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
  ! Argument list
  REAL(8), INTENT(IN)  :: V(VECTORLENGTH,nVar)     ! primitive variables
  REAL(8), INTENT(OUT) :: Q(VECTORLENGTH,nVar)     ! vector of conserved quantities
  ! Local variables
  REAL(8), DIMENSION(VECTORLENGTH)     :: rho,p,psi,lapse,gp,gm,vb,v2,e2,b2,uem,LF,gamma1,w,ww,rhoeps,b2_4,Dd,gam,sb2,e,s2,det,idet
  REAL(8), DIMENSION(VECTORLENGTH,3)   :: shift,v_cov,v_contr,vxB_cov,vxB_contr,B_cov,B_contr,s_cov,s_contr,x,f
  REAL(8), DIMENSION(VECTORLENGTH,3,3) :: g_cov,g_contr,df
  INTEGER :: i
#ifdef AVX512
    !dir$ attributes align: 64:: rho,p,psi,lapse,gp,gm,vb,v2,e2,b2,uem,LF,gamma1,w,ww,rhoeps,b2_4,Dd,gam,sb2,e,s2
    !dir$ attributes align: 64:: shift,v_cov,v_contr,vxB_cov,vxB_contr,B_cov,B_contr,s_cov,s_contr,x,f
    !dir$ attributes align: 64:: g_cov,g_contr,df
    !DIR$ ASSUME_ALIGNED Q     : 64
    !DIR$ ASSUME_ALIGNED V     : 64
#else
    !dir$ attributes align: 32:: rho,p,psi,lapse,gp,gm,vb,v2,e2,b2,uem,LF,gamma1,w,ww,rhoeps,b2_4,Dd,gam,sb2,e,s2
    !dir$ attributes align: 32:: shift,v_cov,v_contr,vxB_cov,vxB_contr,B_cov,B_contr,s_cov,s_contr,x,f
    !dir$ attributes align: 32:: g_cov,g_contr,df
    !DIR$ ASSUME_ALIGNED Q     : 32
    !DIR$ ASSUME_ALIGNED V     : 32
#endif
  !Q=V
  !RETURN
  DO i=1,3
    v_cov(:,i) = V(:,i+1)
	B_contr(:,i) = V(:,5+i)  !wrong 
	shift(:,i) = V(:,10+i)          ! NB: we choose V() and Q() being the shift_controvariant!
  ENDDO
  rho(:)    = V(:,1)
  p(:)      = V(:,5)
  psi(:)    = V(:,9)
  lapse(:)  = V(:,10)
  !
  !gammaij = V(14:19) 
  g_cov(:,1,1) = V(:,14)
  g_cov(:,1,2) = V(:,15)
  g_cov(:,1,3) = V(:,16)
  g_cov(:,2,2) = V(:,17)
  g_cov(:,2,3) = V(:,18)
  g_cov(:,3,3) = V(:,19)
  g_cov(:,2,1) = V(:,15)
  g_cov(:,3,1) = V(:,16)
  g_cov(:,3,2) = V(:,18)
  !
  det(:) = (g_cov(:,1,1)*g_cov(:,2,2)*g_cov(:,3,3)-g_cov(:,1,1)*g_cov(:,2,3)**2-g_cov(:,1,2)**2*g_cov(:,3,3)+2*g_cov(:,1,2)*g_cov(:,1,3)*g_cov(:,2,3)-g_cov(:,1,3)**2*g_cov(:,2,2))  
  idet(:) = 1./det(:)  
  !
  g_contr(:,1,1) =  ( g_cov(:,2,2)*g_cov(:,3,3)-g_cov(:,2,3)*g_cov(:,3,2)) * idet(:) 
  g_contr(:,1,2) = -( g_cov(:,1,2)*g_cov(:,3,3)-g_cov(:,1,3)*g_cov(:,3,2)) * idet(:)
  g_contr(:,1,3) = -(-g_cov(:,1,2)*g_cov(:,2,3)+g_cov(:,1,3)*g_cov(:,2,2)) * idet(:) 
  g_contr(:,2,1) = -( g_cov(:,2,1)*g_cov(:,3,3)-g_cov(:,2,3)*g_cov(:,3,1)) * idet(:) 
  g_contr(:,2,2) =  ( g_cov(:,1,1)*g_cov(:,3,3)-g_cov(:,1,3)*g_cov(:,3,1)) * idet(:) 
  g_contr(:,2,3) = -( g_cov(:,1,1)*g_cov(:,2,3)-g_cov(:,1,3)*g_cov(:,2,1)) * idet(:) 
  g_contr(:,3,1) = -(-g_cov(:,2,1)*g_cov(:,3,2)+g_cov(:,2,2)*g_cov(:,3,1)) * idet(:) 
  g_contr(:,3,2) = -( g_cov(:,1,1)*g_cov(:,3,2)-g_cov(:,1,2)*g_cov(:,3,1)) * idet(:) 
  g_contr(:,3,3) =  ( g_cov(:,1,1)*g_cov(:,2,2)-g_cov(:,1,2)*g_cov(:,2,1)) * idet(:)       
  ! 
  gp(:) = SQRT(det(:))
  gm(:) = 1./gp(:)
  !  
  v_contr(:,1) = g_contr(:,1,1)*v_cov(:,1) + g_contr(:,1,2)*v_cov(:,2) + g_contr(:,1,3)*v_cov(:,3) 
  v_contr(:,2) = g_contr(:,2,1)*v_cov(:,1) + g_contr(:,2,2)*v_cov(:,2) + g_contr(:,2,3)*v_cov(:,3) 
  v_contr(:,3) = g_contr(:,3,1)*v_cov(:,1) + g_contr(:,3,2)*v_cov(:,2) + g_contr(:,3,3)*v_cov(:,3) 
  B_cov(:,1) = g_cov(:,1,1)*B_contr(:,1) + g_cov(:,1,2)*B_contr(:,2) + g_cov(:,1,3)*B_contr(:,3) 
  B_cov(:,2) = g_cov(:,2,1)*B_contr(:,1) + g_cov(:,2,2)*B_contr(:,2) + g_cov(:,2,3)*B_contr(:,3) 
  B_cov(:,3) = g_cov(:,3,1)*B_contr(:,1) + g_cov(:,3,2)*B_contr(:,2) + g_cov(:,3,3)*B_contr(:,3)   
  !
  ! COVARIANT =>
  vxB_cov(:,1) = v_contr(:,2)*B_contr(:,3) - v_contr(:,3)*B_contr(:,2)
  vxB_cov(:,2) = v_contr(:,3)*B_contr(:,1) - v_contr(:,1)*B_contr(:,3)
  vxB_cov(:,3) = v_contr(:,1)*B_contr(:,2) - v_contr(:,2)*B_contr(:,1)
  vxB_cov(:,1) = gp(:)*vxB_cov(:,1)
  vxB_cov(:,2) = gp(:)*vxB_cov(:,2)
  vxB_cov(:,3) = gp(:)*vxB_cov(:,3)
  !
  ! CONTRAVARIANT =>
  vxB_contr(:,1) = v_cov(:,2)*B_cov(:,3) - v_cov(:,3)*B_cov(:,2)
  vxB_contr(:,2) = v_cov(:,3)*B_cov(:,1) - v_cov(:,1)*B_cov(:,3)
  vxB_contr(:,3) = v_cov(:,1)*B_cov(:,2) - v_cov(:,2)*B_cov(:,1)
  vxB_contr(:,1) = gm(:)*vxB_contr(:,1)
  vxB_contr(:,2) = gm(:)*vxB_contr(:,2)
  vxB_contr(:,3) = gm(:)*vxB_contr(:,3)
  !
  vb(:) = v_cov(:,1)*B_contr(:,1) + v_cov(:,2)*B_contr(:,2) + v_cov(:,3)*B_contr(:,3)      
  v2(:) = v_contr(:,1)*v_cov(:,1) + v_contr(:,2)*v_cov(:,2) + v_contr(:,3)*v_cov(:,3)
  e2(:) = vxB_contr(:,1)*vxB_cov(:,1) + vxB_contr(:,2)*vxB_cov(:,2) + vxB_contr(:,3)*vxB_cov(:,3)
  b2(:) = B_contr(:,1)*B_cov(:,1) + B_contr(:,2)*B_cov(:,2) + B_contr(:,3)*B_cov(:,3)
  !
  uem(:) = 0.5*(b2(:) + e2(:)) 
  !
  lf(:)     = 1.0 / sqrt(1.0 - v2(:))
  gamma1(:) = EQN%gamma/(EQN%gamma-1.0)  
  !
#ifdef GEOS  
  CALL EOS_eps_P2C_vector(rhoeps(:),rho(:),p(:)) 
  w(:) = rho(:) + rhoeps(:) + p(:)  ! rho*enthalpy
#else
  w(:)    = rho(:) + gamma1(:)*p(:)  ! this is rho*h
#endif
  !IF(SUM(ABS(w-(rho + gamma1*p))).GT.1e-13) THEN
  !    continue
  !ENDIF
  !
  !w      = rho + gamma1*p
  ww(:) = w(:)*lf(:)**2
  !
  Q(:,1) = rho(:)*lf(:)
  Q(:,2) = ww(:)*v_cov(:,1) + b2(:)*v_cov(:,1) - vb(:)*B_cov(:,1)
  Q(:,3) = ww(:)*v_cov(:,2) + b2(:)*v_cov(:,2) - vb(:)*B_cov(:,2)
  Q(:,4) = ww(:)*v_cov(:,3) + b2(:)*v_cov(:,3) - vb(:)*B_cov(:,3) 
  Q(:,5) = ww(:) - p(:) + uem(:) - Q(:,1)     !!!!! we subtract PDE(:,Q(:,1))!!!!
  Q(:,6) = V(:,6)
  Q(:,7) = V(:,7)
  Q(:,8) = V(:,8)
  !
  !! test functional
  !gam = 1.0/gamma1
  !dd=Q(:,1)
  !s_cov(:,1) = Q(:,2)
  !s_cov(:,2) = Q(:,3)
  !s_cov(:,3) = Q(:,4)
  !e=(:,Q(:,5)+Q(:,1))
  !s_contr = MATMUL(:,g_contr,s_cov)
  !s2 = s_cov(:,1)*s_contr(:,1) + s_cov(:,2)*s_contr(:,2) + s_cov(:,3)*s_contr(:,3)
  !sb2= (:,s_cov(:,1)*B_contr(:,1) + s_cov(:,2)*B_contr(:,2) + s_cov(:,3)*B_contr(:,3))**2
  !x(:,1) = v2
  !x(:,2) = ww
  !x(:,3) = p
  !CALL FUNC_C2P_GEOS_3D(:,x,f,df,gam,dd,e,s2,b2,sb2)
  !!
  !IF(:,SUM(:,ABS(:,f)).GT.1e-13)THEN
  !      continue
  !ENDIF
  !
  DO i=1,8
		Q(:,i) = gp(:)*Q(:,i)  
  ENDDO
  DO i=9,nVar
		Q(:,i) = V(:,i)
  ENDDO
  ! 
  continue
    !
END SUBROUTINE PDEPrim2ConsGRMHDvector      
    
#endif 
    
            
RECURSIVE SUBROUTINE RTSAFE_C2P_RMHD1(v2,X1,X2,XACC,gam,d,e,s2,b2,sb2,w,FAILED)
  USE MainVariables, ONLY : nVar
  IMPLICIT NONE
  ! argument variables
  REAL, INTENT(INOUT)       :: v2,X1,X2,XACC,w 
  REAL, INTENT(IN)          :: gam,d,e,s2,b2,sb2
  LOGICAL, INTENT(INOUT)    :: FAILED
  ! auxiliary variables
  INTEGER, PARAMETER   :: MAXIT=400
  INTEGER              :: J,i
  REAL                 :: FL,FH,DF,XH,XL,SWAP,DXOLD,DX,F,TEMP,Xtmp,Vtmp(7),Ftmp,dFtmp,dXtmp
  !
	!
  FAILED = .FALSE.
  !IF(DebugPrint) THEN 
  !    WRITE(DebugFile,'(a)') 'DebugFile.dat' 
  !    OPEN(UNIT=4321,FILE=TRIM(DebugFile), RECL=800) 
  !    WRITE(4321,*) '  VARIABLES = "i"   "Xtmp"   "Ftmp"   "dFtmp"   "gam" "d" "e" "s2" "b2" "sb2" "w" '
  !    WRITE(4321,*)
  !    Vtmp(1:7) = (/ gam,d,e,s2,b2,sb2,w /)
  !    dXtmp = (1.0-1.0e-12)/300
  !    DO i =1, 350
  !        Xtmp = 0.5*dXtmp + real(i)*dXtmp
  !      CALL FUNC_C2P_RMHD1(Xtmp,Ftmp,dFtmp,Vtmp(1),Vtmp(2),Vtmp(3),Vtmp(4),Vtmp(5),Vtmp(6),Vtmp(7))
  !      WRITE(4321,'(i9.4,10(E16.6))') i,Xtmp,Ftmp,dFtmp,Vtmp(1:7)
  !    ENDDO
  !    CLOSE(4321)   
  !ENDIF
  !
  !PRINT *,e
  CALL FUNC_C2P_RMHD1(X1,FL,DF,gam,d,e,s2,b2,sb2,w)
  IF(FL.EQ.0.) THEN
     v2=X1
     RETURN
  ENDIF
  CALL FUNC_C2P_RMHD1(X2,FH,DF,gam,d,e,s2,b2,sb2,w)
#ifdef C2PGRMHD  
  IF(ABS(FH).LT.1e-22) THEN
      continue
        IF(FH.NE.0.) THEN
            continue
        ENDIF
        !
#else
  IF(FH.EQ.0.) THEN
#endif
     v2=X2
     RETURN
  ENDIF
  IF(FL*FH.GT.0.) THEN
     FAILED = .TRUE.
     v2 = 0. ! assign value even if it fails
     RETURN
  ENDIF
  IF(FL.LT.0.)THEN
     XL=X1
     XH=X2
  ELSE
     XH=X1
     XL=X2
     SWAP=FL
     FL=FH
     FH=SWAP
  ENDIF
  v2=.5*(X1+X2)
  DXOLD=ABS(X2-X1)
  DX=DXOLD
  CALL FUNC_C2P_RMHD1(v2,F,DF,gam,d,e,s2,b2,sb2,w)
  DO 11 J=1,MAXIT
     !IF(((v2-XH)*DF-F)*((v2-XL)*DF-F).GE.0. &
     !     .OR. ABS(2.*F).GT.ABS(DXOLD*DF) ) THEN
     !   DXOLD=DX
     !   DX=0.5*(XH-XL)
     !   v2=XL+DX
     !   IF(XL.EQ.v2) THEN
     !       continue
     !       RETURN
     !   ENDIF
     !ELSE
        DXOLD=DX
        DX=F/DF
        TEMP=v2
        v2=v2-DX
        IF(TEMP.EQ.v2) THEN
            continue
            RETURN
        ENDIF
     !ENDIF
     IF(ABS(DX).LT.XACC) THEN
            !PRINT *,j,DX
            continue
            RETURN
     ENDIF
     CALL FUNC_C2P_RMHD1(v2,F,DF,gam,d,e,s2,b2,sb2,w)
     IF(F.LT.0.) THEN
        XL=v2
        FL=F
     ELSE
        XH=v2
        FH=F
     ENDIF
11   CONTINUE
     FAILED = .TRUE.
     v2 = 0. ! assign value even if it fails
     RETURN
  END SUBROUTINE RTSAFE_C2P_RMHD1
   !
#ifdef VECTOR   
  !
RECURSIVE SUBROUTINE RTSAFE_C2P_RMHD1_VECTOR(v2,X1,X2,XACC,gam,d,e,s2,b2,sb2,w,FAILED,VECTORLENGTH)
  !USE VSMM_mod 
  IMPLICIT NONE
  INTEGER               :: VECTORLENGTH 
  INTEGER, PARAMETER    :: MAXIT=400
  INTEGER               :: i,j 
  REAL(8)               :: v2(VECTORLENGTH)  
  REAL(8)               :: X1(VECTORLENGTH), X2(VECTORLENGTH), XACC(VECTORLENGTH), gam(VECTORLENGTH)
  REAL(8)               :: d(VECTORLENGTH),e(VECTORLENGTH),s2(VECTORLENGTH),b2(VECTORLENGTH),sb2(VECTORLENGTH),w(VECTORLENGTH) 
  REAL(8)               :: FL(VECTORLENGTH),FH(VECTORLENGTH),DF(VECTORLENGTH),XH(VECTORLENGTH),XL(VECTORLENGTH),SWAP(VECTORLENGTH)
  REAL(8)               :: DXOLD(VECTORLENGTH),DX(VECTORLENGTH),F(VECTORLENGTH),TEMP(VECTORLENGTH),Xtmp(VECTORLENGTH),Ftmp(VECTORLENGTH),dFtmp(VECTORLENGTH),dXtmp(VECTORLENGTH) 
  LOGICAL(8)               :: FAILED(VECTORLENGTH) 
  !
#ifdef AVX512
    !dir$ attributes align: 64:: FL,FH,DF,XH,XL,SWAP,DXOLD,DX,F,TEMP,Xtmp,Ftmp,dFtmp,dXtmp
    !DIR$ ASSUME_ALIGNED v2 : 64 
    !DIR$ ASSUME_ALIGNED X1 : 64 
    !DIR$ ASSUME_ALIGNED X2 : 64 
    !DIR$ ASSUME_ALIGNED XACC : 64 
    !DIR$ ASSUME_ALIGNED gam  : 64 
    !DIR$ ASSUME_ALIGNED d    : 64 
    !DIR$ ASSUME_ALIGNED e    : 64 
    !DIR$ ASSUME_ALIGNED s2   : 64 
    !DIR$ ASSUME_ALIGNED b2   : 64 
    !DIR$ ASSUME_ALIGNED sb2  : 64 
    !DIR$ ASSUME_ALIGNED w    : 64 
    !DIR$ ASSUME_ALIGNED FAILED    : 64 
#else
    !dir$ attributes align: 32 :: FL,FH,DF,XH,XL,SWAP,DXOLD,DX,F,TEMP,Xtmp,Ftmp,dFtmp,dXtmp
    !DIR$ ASSUME_ALIGNED v2   : 32 
    !DIR$ ASSUME_ALIGNED X1   : 32 
    !DIR$ ASSUME_ALIGNED X2   : 32 
    !DIR$ ASSUME_ALIGNED XACC : 32 
    !DIR$ ASSUME_ALIGNED gam  : 32 
    !DIR$ ASSUME_ALIGNED d    : 32 
    !DIR$ ASSUME_ALIGNED e    : 32 
    !DIR$ ASSUME_ALIGNED s2   : 32 
    !DIR$ ASSUME_ALIGNED b2   : 32 
    !DIR$ ASSUME_ALIGNED sb2  : 32 
    !DIR$ ASSUME_ALIGNED w    : 32
    !DIR$ ASSUME_ALIGNED FAILED    : 32  
#endif 
  !
  FAILED(:) = .FALSE.
  !
  ! First try four Newton iterations. 
  !   
  DO j = 1 , 5 
      CALL FUNC_C2P_RMHD1_VECTOR(v2,F,DF,gam,d,e,s2,b2,sb2,w)
      WHERE( ISNAN(F) )
          F = 1e20
      ENDWHERE 
      IF( ALL(ABS(F).LT.XACC) ) THEN
          RETURN
      ENDIF 
      v2(:) = v2(:) - F(:)/DF(:)  
  ENDDO 
  !
  CALL FUNC_C2P_RMHD1_VECTORF(X1,FL,DF,gam,d,e,s2,b2,sb2,w)
  WHERE(ABS(FL(:)).LT.XACC(:))  
     v2(:) = X1(:) 
     X2(:) = X1(:) 
  ENDWHERE 
  CALL FUNC_C2P_RMHD1_VECTORF(X2,FH,DF,gam,d,e,s2,b2,sb2,w)
  WHERE(ABS(FH(:)).LT.XACC(:))  
     v2(:) = X2(:) 
     X1(:) = X2(:) 
  ENDWHERE 
  WHERE(FL(:)*FH(:).GT.0.0)  
     FAILED(:) = .TRUE.
     v2(:)     = 0. ! assign value even if it fails     
  ENDWHERE 
  WHERE(FL(:).LT.0.0)  
     XL(:) = X1(:) 
     XH(:) = X2(:)
  ELSEWHERE 
     XH(:) = X1(:)
     XL(:) = X2(:) 
     SWAP(:) = FL(:) 
     FL(:) = FH(:) 
     FH(:) = SWAP(:) 
  ENDWHERE 
  v2(:)=0.5*(X1(:)+X2(:))
  DXOLD(:)=ABS(X2(:)-X1(:))
  DX(:)=DXOLD(:)
  CALL FUNC_C2P_RMHD1_VECTORF(v2,F,DF,gam,d,e,s2,b2,sb2,w)  
  DO j = 1, MAXIT 
     WHERE( ((v2-XH)*DF-F)*((v2-XL)*DF-F).GE.0. .OR. ABS(2.*F).GT.ABS(DXOLD*DF) )  
        DXOLD(:) = DX(:)
        DX(:) = 0.5*(XH(:)-XL(:)) 
        v2(:)=XL(:)+DX(:) 
     ELSEWHERE 
        DXOLD(:) = DX(:)
        DX(:)=F(:)/DF(:) 
        TEMP(:)=v2(:)
        v2(:)=v2(:)-DX(:) 
     ENDWHERE 
     IF( ALL(ABS(DX).LT.XACC) ) THEN
        CONTINUE 
        RETURN
     ENDIF
     CALL FUNC_C2P_RMHD1_VECTORF(v2,F,DF,gam,d,e,s2,b2,sb2,w)
     WHERE(F.LT.0.)  
        XL(:) = v2(:)
        FL(:) = F(:) 
     ELSEWHERE 
        XH(:)=v2(:)
        FH(:)=F(:)
     ENDWHERE 
    ! 
  ENDDO 
  !
  FAILED = .TRUE.
  v2 = 0. ! assign value even if it fails
  RETURN
  ! 
END SUBROUTINE   RTSAFE_C2P_RMHD1_VECTOR
   !
#endif    
   !
FUNCTION RTSAFE_C2P_RMHD2(X1,X2,XACC,g1, D, k2, B2, kB, E, H, FAILED)
  IMPLICIT NONE
  INTEGER, PARAMETER    :: MAXIT=200
  INTEGER               :: J
  REAL                  :: RTSAFE_C2P_RMHD2
  REAL                  :: X1,X2,XACC, g1, D, k2, B2, kB, E, H
  REAL                  :: FL,FH,DF,XH,XL,SWAP,DXOLD,DX,F,TEMP
  LOGICAL               :: FAILED
  !
  FAILED = .FALSE.
  CALL FUNC_C2P_RMHD2(X1, FL, DF, g1, D, k2, B2, kB, E, H)
  IF(FL.EQ.0.) THEN
     RTSAFE_C2P_RMHD2=X1
     RETURN
  ENDIF
  CALL FUNC_C2P_RMHD2(X2, FH, DF, g1, D, k2, B2, kB, E, H)
  IF(FH.EQ.0.) THEN
     RTSAFE_C2P_RMHD2=X2
     RETURN
  ENDIF
  IF(FL*FH.GT.0.) THEN
     FAILED = .TRUE.
     RETURN
  ENDIF
  IF(FL.LT.0.)THEN
     XL=X1
     XH=X2
  ELSE
     XH=X1
     XL=X2
     SWAP=FL
     FL=FH
     FH=SWAP
  ENDIF
  RTSAFE_C2P_RMHD2=.5*(X1+X2)
  DXOLD=ABS(X2-X1)
  DX=DXOLD
  CALL FUNC_C2P_RMHD2(RTSAFE_C2P_RMHD2, F, DF, g1, D, k2, B2, kB, E, H)
  DO 11 J=1,MAXIT
     IF(((RTSAFE_C2P_RMHD2-XH)*DF-F)*((RTSAFE_C2P_RMHD2-XL)*DF-F).GE.0. &
          .OR. ABS(2.*F).GT.ABS(DXOLD*DF) ) THEN
        DXOLD=DX
        DX=0.5*(XH-XL)
        RTSAFE_C2P_RMHD2=XL+DX
        IF(XL.EQ.RTSAFE_C2P_RMHD2)RETURN
     ELSE
        DXOLD=DX
        DX=F/DF
        TEMP=RTSAFE_C2P_RMHD2
        RTSAFE_C2P_RMHD2=RTSAFE_C2P_RMHD2-DX
        IF(TEMP.EQ.RTSAFE_C2P_RMHD2)RETURN
     ENDIF
     IF(ABS(DX).LT.XACC) RETURN
     CALL FUNC_C2P_RMHD2(RTSAFE_C2P_RMHD2,F, DF, g1, D, k2, B2, kB, E, H)
     IF(F.LT.0.) THEN
        XL=RTSAFE_C2P_RMHD2
        FL=F
     ELSE
        XH=RTSAFE_C2P_RMHD2
        FH=F
     ENDIF
11   CONTINUE
     FAILED = .TRUE.
     RETURN
END FUNCTION RTSAFE_C2P_RMHD2
!    
FUNCTION RTSAFE_C2P_RHD1(X1,X2,XACC,d,e,s2,FAILED)
  IMPLICIT NONE
  INTEGER, PARAMETER    :: MAXIT=200
  INTEGER               :: J
  REAL                  :: RTSAFE_C2P_RHD1
  REAL                  :: X1,X2,XACC,gam,d,e,s2
  REAL                  :: FL,FH,DF,XH,XL,SWAP,DXOLD,DX,F,TEMP
  LOGICAL               :: FAILED
  FAILED = .FALSE.
  CALL FUNC_C2P_RHD1(X1,FL,DF,d,e,s2,FAILED)
  IF (FAILED) RETURN
  IF(FL.EQ.0.) THEN
     RTSAFE_C2P_RHD1=X1
     RETURN
  ENDIF
  CALL FUNC_C2P_RHD1(X2,FH,DF,d,e,s2,FAILED)
  IF (FAILED) RETURN
  IF(FH.EQ.0.) THEN
     RTSAFE_C2P_RHD1=X2
     RETURN
  ENDIF
  IF(FL*FH.GT.0.) THEN
     FAILED = .TRUE.
     RETURN
  ENDIF
  IF(FL.LT.0.)THEN
     XL=X1
     XH=X2
  ELSE
     XH=X1
     XL=X2
     SWAP=FL
     FL=FH
     FH=SWAP
  ENDIF
  RTSAFE_C2P_RHD1=.5*(X1+X2)
  DXOLD=ABS(X2-X1)
  DX=DXOLD
  CALL FUNC_C2P_RHD1(RTSAFE_C2P_RHD1,F,DF,d,e,s2,FAILED)
  IF (FAILED) RETURN
  DO 11 J=1,MAXIT
     IF(((RTSAFE_C2P_RHD1-XH)*DF-F)*((RTSAFE_C2P_RHD1-XL)*DF-F).GE.0. &
          .OR. ABS(2.*F).GT.ABS(DXOLD*DF) ) THEN
        DXOLD=DX
        DX=0.5*(XH-XL)
        RTSAFE_C2P_RHD1=XL+DX
        IF(XL.EQ.RTSAFE_C2P_RHD1)RETURN
     ELSE
        DXOLD=DX
        DX=F/DF
        TEMP=RTSAFE_C2P_RHD1
        RTSAFE_C2P_RHD1=RTSAFE_C2P_RHD1-DX
        IF(TEMP.EQ.RTSAFE_C2P_RHD1)RETURN
     ENDIF
     IF(ABS(DX).LT.XACC) RETURN
     CALL FUNC_C2P_RHD1(RTSAFE_C2P_RHD1,F,DF,d,e,s2,FAILED)
     IF (FAILED) RETURN
     IF(F.LT.0.) THEN
        XL=RTSAFE_C2P_RHD1
        FL=F
     ELSE
        XH=RTSAFE_C2P_RHD1
        FH=F
     ENDIF
11   CONTINUE
     FAILED = .TRUE.
     write(*,*)'RTSAFE exceeding maximum iterations'
     stop
     RETURN
   END FUNCTION RTSAFE_C2P_RHD1
   !
FUNCTION RTSAFE_C2P_RHD2(X1,X2,XACC,qi,ri,kappa,FAILED)
  IMPLICIT NONE
  INTEGER, PARAMETER    :: MAXIT=200
  INTEGER               :: J
  REAL                  :: RTSAFE_C2P_RHD2
  REAL                  :: X1,X2,XACC,qi,ri,kappa
  REAL                  :: FL,FH,DF,XH,XL,SWAP,DXOLD,DX,F,TEMP
  LOGICAL               :: FAILED
  FAILED = .FALSE.
  CALL FUNC_C2P_RHD2(X1,FL,DF,qi,ri,kappa,FAILED)
  IF (FAILED) RETURN
  IF(FL.EQ.0.) THEN
     RTSAFE_C2P_RHD2=X1
     RETURN
  ENDIF
  CALL FUNC_C2P_RHD2(X2,FH,DF,qi,ri,kappa,FAILED)
  IF (FAILED) RETURN
  IF(FH.EQ.0.) THEN
     RTSAFE_C2P_RHD2=X2
     RETURN
  ENDIF
  IF(FL*FH.GT.0.) THEN
     FAILED = .TRUE.
     RETURN
  ENDIF
  IF(FL.LT.0.)THEN
     XL=X1
     XH=X2
  ELSE
     XH=X1
     XL=X2
     SWAP=FL
     FL=FH
     FH=SWAP
  ENDIF
  RTSAFE_C2P_RHD2=.5*(X1+X2)
  DXOLD=ABS(X2-X1)
  DX=DXOLD
  CALL FUNC_C2P_RHD2(RTSAFE_C2P_RHD2,F,DF,qi,ri,kappa,FAILED)
  IF (FAILED) RETURN
  DO 11 J=1,MAXIT
     IF(((RTSAFE_C2P_RHD2-XH)*DF-F)*((RTSAFE_C2P_RHD2-XL)*DF-F).GE.0. &
          .OR. ABS(2.*F).GT.ABS(DXOLD*DF) ) THEN
        DXOLD=DX
        DX=0.5*(XH-XL)
        RTSAFE_C2P_RHD2=XL+DX
        IF(XL.EQ.RTSAFE_C2P_RHD2)RETURN
     ELSE
        DXOLD=DX
        DX=F/DF
        TEMP=RTSAFE_C2P_RHD2
        RTSAFE_C2P_RHD2=RTSAFE_C2P_RHD2-DX
        IF(TEMP.EQ.RTSAFE_C2P_RHD2)RETURN
     ENDIF
     IF(ABS(DX).LT.XACC) RETURN
     CALL FUNC_C2P_RHD2(RTSAFE_C2P_RHD2,F,DF,qi,ri,kappa,FAILED)
     IF (FAILED) RETURN
     IF(F.LT.0.) THEN
        XL=RTSAFE_C2P_RHD2
        FL=F
     ELSE
        XH=RTSAFE_C2P_RHD2
        FH=F
     ENDIF
11   CONTINUE
     write(*,*)'RTSAFE exceeding maximum iterations'
     stop
     RETURN
   END FUNCTION RTSAFE_C2P_RHD2
   !
   !--------------------------------------------------------
FUNCTION ZBRENT_C2P_RHD2 ( x1, x2, tol, qi, ri, kappa, FAILED)
  IMPLICIT NONE
  REAL,    INTENT(IN) :: x1,x2,tol,qi,ri,kappa
  LOGICAL, INTENT(OUT):: FAILED
  REAL :: ZBRENT_C2P_RHD2, fo
  INTEGER, PARAMETER :: ITMAX=100
  REAL, PARAMETER :: EPS=epsilon(x1)
  INTEGER :: iter
  REAL    :: a,b,c,d,e,fa,fb,fc,p,q,r,s,tol1,xm,df
  INTERFACE
     PURE SUBROUTINE FUNC_C2P_RHD2(z,f,df,qi,ri,kappa,FAILED)
     USE MainVariables, ONLY: EQN 
     IMPLICIT NONE
     INTEGER    :: iter
     REAL       :: z,f,df
     REAL       :: qi,ri,kappa
     LOGICAL    :: FAILED
     REAL       :: gamma_mo, z2, w, epsilon, h, a
     INTENT(IN) :: z,qi,ri,kappa
     INTENT(OUT):: f,df,FAILED
     END SUBROUTINE FUNC_C2P_RHD2
  END INTERFACE
  FAILED = .FALSE.
  a=x1
  b=x2
  CALL FUNC_C2P_RHD2(a,fo,df,qi,ri,kappa,FAILED)
  fa = fo
  CALL FUNC_C2P_RHD2(b,fo,df,qi,ri,kappa,FAILED)
  fb = fo
  IF ((fa > 0.0 .and. fb > 0.0) .or. (fa < 0.0 .and. fb < 0.0)) THEN
    FAILED = .TRUE.
    RETURN
  ENDIF
  c=b
  fc=fb
  do iter=1,ITMAX
     if ((fb > 0.0 .and. fc > 0.0) .or. (fb < 0.0 .and. fc < 0.0)) then
        c=a
        fc=fa
        d=b-a
        e=d
     end if
     if (abs(fc) < abs(fb)) then
        a=b
        b=c
        c=a
        fa=fb
        fb=fc
        fc=fa
     end if
     tol1=2.0*EPS*abs(b)+0.5*tol
     xm=0.5*(c-b)
     if (abs(xm) <= tol1 .or. fb == 0.0) then
        ZBRENT_C2P_RHD2=b
        RETURN
     end if
     if (abs(e) >= tol1 .and. abs(fa) > abs(fb)) then
        s=fb/fa
        if (a == c) then
           p=2.0*xm*s
           q=1.0-s
        else
           q=fa/fc
           r=fb/fc
           p=s*(2.0*xm*q*(q-r)-(b-a)*(r-1.0))
           q=(q-1.0)*(r-1.0)*(s-1.0)
        end if
        if (p > 0.0) q=-q
        p=abs(p)
        if (2.0*p  <  min(3.0*xm*q-abs(tol1*q),abs(e*q))) then
           e=d
           d=p/q
        else
           d=xm
           e=d
        end if
     else
        d=xm
        e=d
     end if
     a=b
     fa=fb
     b=b+merge(d,sign(tol1,xm), abs(d) > tol1 )
     CALL FUNC_C2P_RHD2(b,fo,df,qi,ri,kappa,FAILED)
     fb = fo
  end do
  !call nrerror('zbrent_rout: exceeded maximum iterations')
  ZBRENT_C2P_RHD2=b
END FUNCTION ZBRENT_C2P_RHD2
   !
RECURSIVE SUBROUTINE FUNC_C2P_RMHD1(x,f,df,gam,d,e,s2,b2,sb2,w)
  !
  ! This is the CONS2PRIM strategy adopted by Del Zanna et al. (2007) A&A, 473, 11-30
  ! and it corresponds to their choice 3 in Section 3.2
  !
  IMPLICIT NONE
  REAL, PARAMETER :: third=1./3.
  INTEGER    :: iter
  REAL       :: x,f,df,v2,rho,c0,c2,c3,dw,dc2,dc3,dlogw,wb,vb2
  REAL       :: gam,d,e,s2,b2,sb2,w
  REAL, PARAMETER :: tolerance = 1e-14
  INTENT(IN) :: x,gam,d,e,s2,b2,sb2
  INTENT(OUT):: f,df,w
  ! ... we actually know the root of a cubic!!!!
  REAL :: sqrtdet,q,w0,wtest,p,c2ic3,c2ic33
  REAL, PARAMETER ::  i27=1./27.
  v2=x
  rho=d*sqrt(1.-v2)
  
  c3=1.-gam*(1.-v2)
  c2=gam*rho+.5*b2*(1.+v2)-e
  c0=-0.5*sb2
  
  ! For every x=v2, we solve a cubic in W of the form:
  ! c3*W^3+c2*W^2+c0=0 (c3>0, c0<=0)
  ! W=y of the paper. If sb=0 ( meaning c0 = 0),
  ! w = -c2/c3 > 0 and dw = 0 in the do loop below.
  ! If -c2/c3 < 0 when sb=0, which makes w=0,
  ! this is a signature that something was wrong before. 
  c2ic3 = c2/c3 
  if ( abs ( c0) < 1.0d-20) then
     w = -c2ic3
  else 
     c2ic33 = c2ic3**3
     q = c0/c3 + 2.0*i27*c2ic33
     p = -third*c2ic3**2
     sqrtdet = SQRT(0.25*q**2+i27*p**3)
     !wtest = -third*c2ic3 + (-0.5*q+sqrtdet)**third + (-0.5*q-sqrtdet)**third
     w = -third*c2ic3 + (-0.5*q+sqrtdet)**third + (-0.5*q-sqrtdet)**third 
     continue
     !w = max ( - c2ic3, ( -c0 / c3)**third)
     !do iter = 1,100
     !   dw = -((c3*w + c2)*w**2 + c0)/((3*c3*w + 2*c2)*w)
     !   if (abs(dw/w).LT.tolerance) THEN
     !           exit
     !   ENDIF
     !   !
     !   w = w + dw
     !end do
  endif
  !
  ! then the NR step, solving the 1D problem F=F(w(v2),v2)=0
  dc3   = gam
  dc2   = 0.5 * ( b2 - gam * rho / (1.0 - v2))
  dlogw = -( dc3 * w + dc2 ) / ( 3.0 * c3 * w + 2.0 * c2)
  wb    = w + b2
  vb2   = sb2 / w**2
  f     = wb**2 * v2 - ( 2.0 * w + b2) * vb2 - s2
  df    = wb * ( wb + 2.0 * dlogw * ( w * v2 + vb2))
  !
END SUBROUTINE FUNC_C2P_RMHD1

#ifdef VECTOR 

RECURSIVE SUBROUTINE FUNC_C2P_RMHD1_VECTORF(x,f,df,gam,d,e,s2,b2,sb2,w)
  !
  ! This is the CONS2PRIM strategy adopted by Del Zanna et al. (2007) A&A, 473, 11-30
  ! and it corresponds to their choice 3 in Section 3.2
  !
  use MainVariables, ONLY : VECTORLENGTH  
  IMPLICIT NONE
  INTEGER       :: iter
  LOGICAL(8)       :: MASK(VECTORLENGTH) 
  REAL(8)       :: x(VECTORLENGTH),f(VECTORLENGTH),df(VECTORLENGTH),v2(VECTORLENGTH),rho(VECTORLENGTH),c0(VECTORLENGTH),tmp(VECTORLENGTH),c2(VECTORLENGTH),c3(VECTORLENGTH),dw(VECTORLENGTH),dc2(VECTORLENGTH),dc3(VECTORLENGTH),dlogw(VECTORLENGTH),wb(VECTORLENGTH),vb2(VECTORLENGTH) 
  REAL(8)       :: gam(VECTORLENGTH),d(VECTORLENGTH),e(VECTORLENGTH),s2(VECTORLENGTH),b2(VECTORLENGTH),sb2(VECTORLENGTH),w(VECTORLENGTH),wtmp(VECTORLENGTH)
  REAL(8), PARAMETER :: tolerance = 1e-10, third=1./3.,tolerance1 = 1e-12
  INTENT(IN) :: x,gam,d,e,s2,b2,sb2  
  INTENT(OUT):: f,df,w
#ifdef AVX512
    !dir$ attributes align: 64:: v2, rho, c0, tmp, c2, c3, dw, dc2, dc3, dlogw, wb, vb2  ,MASK,wtmp
    !DIR$ ASSUME_ALIGNED x   : 64 
    !DIR$ ASSUME_ALIGNED f   : 64 
    !DIR$ ASSUME_ALIGNED df  : 64 
    !DIR$ ASSUME_ALIGNED gam : 64 
    !DIR$ ASSUME_ALIGNED d   : 64 
    !DIR$ ASSUME_ALIGNED e   : 64 
    !DIR$ ASSUME_ALIGNED s2  : 64 
    !DIR$ ASSUME_ALIGNED b2  : 64 
    !DIR$ ASSUME_ALIGNED sb2 : 64 
    !DIR$ ASSUME_ALIGNED w   : 64 
#else
    !dir$ attributes align: 32:: v2, rho, c0, tmp, c2, c3, dw, dc2, dc3, dlogw, wb, vb2  ,MASK,wtmp
    !DIR$ ASSUME_ALIGNED x   : 32 
    !DIR$ ASSUME_ALIGNED f   : 32 
    !DIR$ ASSUME_ALIGNED df  : 32 
    !DIR$ ASSUME_ALIGNED gam : 32 
    !DIR$ ASSUME_ALIGNED d   : 32 
    !DIR$ ASSUME_ALIGNED e   : 32 
    !DIR$ ASSUME_ALIGNED s2  : 32 
    !DIR$ ASSUME_ALIGNED b2  : 32 
    !DIR$ ASSUME_ALIGNED sb2 : 32 
    !DIR$ ASSUME_ALIGNED w   : 32 
#endif 
  
  v2(:)  = x(:)
  rho(:) = d(:)*sqrt(1.-v2(:))
  
  c3(:) = 1.0-gam*(1.-v2(:))
  c2(:) = gam(:)*rho(:) + 0.5*b2(:)*(1.0+v2(:))-e(:) 
  c0(:) = -0.5*sb2(:)
  
  ! For every x=v2, we solve a cubic in W of the form: 
  ! c3*W^3+c2*W^2+c0=0 (c3>0, c0<=0)
  ! W=y of the paper. If sb=0 ( meaning c0 = 0), 
  ! w = -c2/c3 > 0 and dw = 0 in the do loop below. 
  ! If -c2/c3 < 0 when sb=0, which makes w=0, 
  ! this is a signature that something was wrong before.
  !
        w(:) = max ( - c2(:) / c3(:), ( -c0(:) / c3(:) )**third )
        ! unroll the first six Newton iterations. Flops are for free.  
        tmp(:) = ((3*c3(:)*w(:) + 2*c2(:))*w(:))/(((3*c3(:)*w(:) + 2*c2(:))*w(:))**2+tolerance1)
        dw(:) = -((c3(:)*w(:) + c2(:))*w(:)**2 + c0(:))*tmp(:)
        w(:) = w(:) + dw(:)
        tmp(:) = ((3*c3(:)*w(:) + 2*c2(:))*w(:))/(((3*c3(:)*w(:) + 2*c2(:))*w(:))**2+tolerance1)
        dw(:) = -((c3(:)*w(:) + c2(:))*w(:)**2 + c0(:))*tmp(:) 
        w(:) = w(:) + dw(:)
        tmp(:) = ((3*c3(:)*w(:) + 2*c2(:))*w(:))/(((3*c3(:)*w(:) + 2*c2(:))*w(:))**2+tolerance1)
        dw(:) = -((c3(:)*w(:) + c2(:))*w(:)**2 + c0(:))*tmp(:)
        w(:) = w(:) + dw(:)
        tmp(:) = ((3*c3(:)*w(:) + 2*c2(:))*w(:))/(((3*c3(:)*w(:) + 2*c2(:))*w(:))**2+tolerance1)
        dw(:) = -((c3(:)*w(:) + c2(:))*w(:)**2 + c0(:))*tmp(:)
        w(:) = w(:) + dw(:)
        tmp(:) = ((3*c3(:)*w(:) + 2*c2(:))*w(:))/(((3*c3(:)*w(:) + 2*c2(:))*w(:))**2+tolerance1)
        dw(:) = -((c3(:)*w(:) + c2(:))*w(:)**2 + c0(:))*tmp(:)
        w(:) = w(:) + dw(:)
        tmp(:) = ((3*c3(:)*w(:) + 2*c2(:))*w(:))/(((3*c3(:)*w(:) + 2*c2(:))*w(:))**2+tolerance1)
        dw(:) = -((c3(:)*w(:) + c2(:))*w(:)**2 + c0(:))*tmp(:) 
        w(:) = w(:) + dw(:)
  !
  WHERE ( abs ( c0(:) ) < 1.0d-20) 
        w(:) = -c2(:) / c3(:)
        wtmp(:) = w(:)    ! save results
  ENDWHERE
  !
  IF(MAXVAL(ABS(dw(:)/w(:)))>tolerance) THEN 
      DO iter = 1, 100
            tmp(:) = ((3*c3(:)*w(:) + 2*c2(:))*w(:))/(((3*c3(:)*w(:) + 2*c2(:))*w(:))**2+tolerance1)
            dw(:) = -((c3(:)*w(:) + c2(:))*w(:)**2 + c0(:))*tmp(:)
            IF(MAXVAL(abs(dw(:)/w(:))).LT.tolerance) THEN
                  EXIT 
            ENDIF
            w(:) = w(:) + dw(:)
      ENDDO   
      WHERE ( abs ( c0(:)) < 1.0d-20) 
            w(:) = wtmp(:)  ! get correct values
      ENDWHERE 
  ENDIF          
  !
  dc3(:)   = gam(:)
  dc2(:)   = 0.5 * ( b2(:) - gam(:) * rho(:) / (1.0 - v2(:)))
  dlogw(:) = -( dc3(:) * w(:) + dc2(:) ) / ( 3.0 * c3(:) * w(:) + 2.0 * c2(:) )
  wb(:)    = w(:) + b2(:)
  vb2(:)   = sb2(:) / w(:)**2
  f(:)     = wb(:)**2 * v2(:) - ( 2.0 * w(:) + b2(:) ) * vb2(:) - s2(:) 
  df(:)    = wb(:) * ( wb(:) + 2.0 * dlogw(:) * ( w(:) * v2(:) + vb2(:) ))
  !
END SUBROUTINE FUNC_C2P_RMHD1_VECTORF 

#endif 

RECURSIVE SUBROUTINE FUNC_C2P_RMHD2(x, f, df, g1, D, k2, B2, kB, E, H)
  !
  ! This is the CONS2PRIM strategy adopted by Del Zanna et al. (2003) A&A, 400, 397-413
  !                                    and by Dumbser et al. (2008) JCP, 227, 8209-8253
  !
  IMPLICIT NONE
  REAL       :: x, g1, D, k2, B2, kB, E
  REAL       :: f, df
  REAL       :: a1, b1, c1, d1, p1, rho, v2, T2
  REAL       :: dv2, sqrvc, c0, c2, c3, H, dH, drho
  COMPLEX    :: CH
  INTENT(IN) :: x, g1, D, k2, B2, kB, E
  INTENT(OUT):: f, df, H
  REAL, PARAMETER    :: sqr3 = 1.7320508075688772935274463415059
  
  v2    = x
  sqrvc = SQRT((1-v2))
  rho  = D*sqrvc
  T2   = B2*k2-kB*kB
  a1   = (1-(1-v2)/g1)
  b1   = (-E+rho/g1+0.5*B2+2*(1-(1-v2)/g1)*B2)
  c1   = (2*(-E+rho/g1+0.5*B2)*B2+(1-(1-v2)/g1)*B2**2)
  d1   = (-E+rho/g1+0.5*B2)*B2**2+0.5*T2
  !                    
  p1  = c1/a1 - b1*b1/a1/a1/3.
  CH   = 1/a1*(36*c1*b1*a1-108*d1*a1**2-8*b1**3+12*sqr3*csqrt(CMPLX(4*c1**3*a1-c1**2*b1**2-18*c1*b1*a1*d1+27*d1**2*a1**2+4*d1*b1**3))*a1)**(1.D0/3.D0)/6-2.D0/3.D0*(3*c1*a1-b1**2)/a1/(36*c1*b1*a1-108*d1*a1**2-8*b1**3+12*sqr3*csqrt(CMPLX(4*c1**3*a1-c1**2*b1**2-18*c1*b1*a1*d1+27*d1**2*a1**2+4*d1*b1**3))*a1)**(1.D0/3.D0)-b1/a1/3
  H    = REAL(CH)
  !
  f    = H**2*v2 + (2*H+B2)*T2/(H+B2)**2 - k2           

  drho = -0.5*D/SQRT((1-v2))
  dH   = (H**2+H*B2+drho*H+drho*B2)/(-3*H*g1-2*B2*g1-3*H*v2-v2*B2+3*H+B2+2*E*g1-2*rho)
  
  df  = H**2 + 2*H*dH*(v2 - T2/(H+B2)**3 )
  
END SUBROUTINE FUNC_C2P_RMHD2
    
RECURSIVE SUBROUTINE FUNC_C2P_RMHD3(x,gam,sb2,b2,s2,d,e,eps,alpha,beta)

  implicit none
  REAL :: x(2),gam,sb2,b2,s2,d,e,eps
  REAL :: alpha(2,2),beta(2)
  REAL :: v2,w,w2,vv,rho,p,ssbb

  v2=x(1)
  w =x(2)

  w2=w**2
  vv=max(eps,1.-v2)
  rho=d*sqrt(vv)
  p=max(eps,gam*(w*vv-rho))

  ssbb=sb2/w2

  beta(1)=-((w+b2)**2*v2-(2.*w+b2)*ssbb-s2)
  beta(2)=-(w-p+.5*(1.+v2)*b2-.5*ssbb-e)

  alpha(1,2)=2.*(w+b2)*v2+2.*(1.+b2/w)*ssbb
  alpha(1,1)=(w+b2)**2
  alpha(2,2)=1.-gam*vv+ssbb/w
  alpha(2,1)=-gam*(-w+.5*rho/vv)+.5*b2

END SUBROUTINE FUNC_C2P_RMHD3


RECURSIVE SUBROUTINE FUNC_C2P_RHD1(x,f,df,d,e,s2,FAILED)
  !
  ! This is the CONS2PRIM strategy adopted in Whisky and originally described in Baiotti's PhD thesis. 
  ! See also Appendix D of the book "Relativistic hydrodynamics" by Rezzolla & Zanotti (2013)
  !
  USE MainVariables, ONLY: EQN
  IMPLICIT NONE
  INTEGER    :: iter
  REAL       :: x,f,df
  REAL       :: d,e,s2
  LOGICAL    :: FAILED
  REAL       :: gamma_mo, tmp, Z, rho, epsilon, drhodp, depsilondp
  INTENT(IN) :: x,d,e,s2
  INTENT(OUT):: f,df,FAILED
  
  ! x = pressure, the unknown
  FAILED   = .FALSE.
  gamma_mo = EQN%gamma - 1.0
  Z       = e + x + d
  IF ((Z**2 - s2) .GT. 0.0) THEN
     tmp     = SQRT(Z**2 - s2)
  ELSE
     FAILED = .TRUE.
     RETURN
  ENDIF
  rho     = d / Z * tmp
  epsilon = ( tmp - x*Z/tmp - d) / d
  
  drhodp     = d*s2 / ( Z**2 * tmp) 
  depsilondp = x*s2 / ( rho * tmp**2 * Z)
  
  f     = x - rho*epsilon*gamma_mo
  df    = 1.0 - epsilon*gamma_mo*drhodp - rho*gamma_mo*depsilondp 
  
END SUBROUTINE FUNC_C2P_RHD1

!
RECURSIVE SUBROUTINE FUNC_C2P_RHD2(z,f,df,qi,ri,kappa,FAILED)
  !
  ! This is the CONS2PRIM strategy adopted by Galeazzi et al. (2013), Phys Rev D 88 064009, Appendix C
  ! See also the book "Relativistic hydrodynamics" by Rezzolla & Zanotti (2013).
  !
  USE MainVariables, ONLY: EQN
  IMPLICIT NONE
  INTEGER    :: iter
  REAL       :: z,f,df
  REAL       :: qi,ri,kappa
  LOGICAL    :: FAILED
  REAL       :: z2, lf, epsilon, h
  INTENT(IN) :: z,qi,ri,kappa
  INTENT(OUT):: f,df,FAILED
  
  ! z = (Lorentz factor)*v , the unknown
  
  FAILED   = .FALSE.
  z2      = z**2  
  lf      = SQRT(1.0 + z2)
  epsilon = lf*(qi+1.0) - z*ri - 1.0
  h       = 1.0 + EQN%gamma*epsilon
  
  f  = z - ri/h
  df = 1.0 + ri/h**2 * EQN%gamma * (z/lf*(qi + 1.0) - ri)
  
END SUBROUTINE FUNC_C2P_RHD2
!
!    
FUNCTION ZBRENT_LF_POLY(x1,x2,tol,gamma,kpol,dd,b22,ss,sb)
  ! This is used in the isentropic case
  !
  IMPLICIT NONE
  REAL             :: x1,x2,tol,dd,b22,ss,sb
  REAL             :: zbrent_lf_poly,fo,gamma,kpol
  INTEGER, PARAMETER :: ITMAX=100
  REAL, PARAMETER  :: EPS=epsilon(x1)
  INTEGER          :: iter
  REAL             :: a,b,c,d,e,fa,fb,fc,p,q,r,s,tol1,xm
  REAL             :: gamma_comb,hh,gamma_mo
  
  INTENT(IN) :: x1,x2,tol,gamma,kpol,dd,b22,ss,sb
  
  a=x1
  b=x2
  gamma_mo   = gamma - 1.0
  gamma_comb = kpol*gamma/gamma_mo
  
  !  hh is just to compute fa, the value of the function at the left 
  !  bracketting. Note that hh = Z/D at W=1
  hh = 1.0d0 + gamma_comb*dd**(gamma - 1.0d0)
  
  fa = ss + (sb/dd/hh)**2*(2.0d0*dd*hh + b22)
  
111 fb = (ss + sb*sb/(dd*(1.0d0 + gamma_comb*(dd/b)**(gamma - 1.0d0))*        &
  & b)**2*(2.0d0*dd*(1.0d0 + gamma_comb*(dd/b)**(gamma - 1.0d0))*      &
  & b + b22))*b**2 - ((1.0d0 + gamma_comb*(dd/b)**(gamma - 1.0d0))*    &
  & dd*b + b22)**2*(b**2 - 1.0d0)
  
  if ( fa*fb > 0.0 ) then
     ! update the bracketing interval: increase b !
     b = b*10.0
     !     write(*,*)'from zbrent_lf_poly',b
     goto 111
  endif
  c=b
  fc=fb
  do iter=1,ITMAX
     if ( fb*fc > 0.0 ) then
        c=a
        fc=fa
        d=b-a
        e=d
     end if
     if (abs(fc) < abs(fb)) then
        a=b
        b=c
        c=a
        fa=fb
        fb=fc
        fc=fa
     end if
     tol1=2.0*EPS*abs(b)+0.5*tol
     xm=0.5*(c-b)
     if (abs(xm) <= tol1 .or. fb == 0.0) then
        zbrent_lf_poly=b
        RETURN
     end if
     if (abs(e) >= tol1 .and. abs(fa) > abs(fb)) then
        s=fb/fa
        if (a == c) then
           p=2.0*xm*s
           q=1.0-s
        else
           q=fa/fc
           r=fb/fc
           p=s*(2.0*xm*q*(q-r)-(b-a)*(r-1.0))
           q=(q-1.0)*(r-1.0)*(s-1.0)
        end if
        if (p > 0.0) q=-q
        p=abs(p)
        if (2.0*p  <  min(3.0*xm*q-abs(tol1*q),abs(e*q))) then
           e=d
           d=p/q
        else
           d=xm 
           e=d
        end if
     else
        d=xm
        e=d
     end if
     a=b
     fa=fb
     b=b+merge(d,sign(tol1,xm), abs(d) > tol1 )
     
     fb = (ss + sb*sb/(dd*(1.0d0 + gamma_comb*(dd/b)**(gamma - 1.0d0))*       &
     & b)**2*(2.0d0*dd*(1.0d0 + gamma_comb*(dd/b)**(gamma - 1.0d0))* &
     & b + b22))*b**2 - ((1.0d0 + gamma_comb*(dd/b)**                &
     & (gamma - 1.0d0))* dd*b + b22)**2*(b**2 - 1.0d0)
     
     
  end do
  zbrent_lf_poly=b
END FUNCTION ZBRENT_LF_POLY

RECURSIVE SUBROUTINE LUDCMP(A,N,NP,INDX,D)
      !
      IMPLICIT NONE
      INTEGER         :: NP, N
      INTEGER, PARAMETER :: NMAX=100
      INTEGER            :: INDX(N)
      REAL,    PARAMETER :: TINY=1.0E-20
      REAL               :: A(NP,NP), VV(NMAX)
      ! Local Variables
      INTEGER            :: I, J, K, IMAX
      REAL               :: D, AAMAX, SUM, DUM
      !
      D=1.
      DO I=1,N
        AAMAX=0.
        DO J=1,N
          IF (ABS(A(I,J)).GT.AAMAX) AAMAX=ABS(A(I,J))
        ENDDO
        IF (AAMAX.EQ.0.) PAUSE 'Singular matrix.'
        VV(I)=1./AAMAX
      ENDDO  
      DO J=1,N
        IF (J.GT.1) THEN
          DO I=1,J-1
            SUM=A(I,J)
            IF (I.GT.1)THEN
              DO K=1,I-1
                SUM=SUM-A(I,K)*A(K,J)
              ENDDO
              A(I,J)=SUM
            ENDIF
          ENDDO
        ENDIF
        AAMAX=0.
        DO I=J,N
          SUM=A(I,J)
          IF (J.GT.1)THEN
            DO K=1,J-1
              SUM=SUM-A(I,K)*A(K,J)
            ENDDO
            A(I,J)=SUM
          ENDIF
          DUM=VV(I)*ABS(SUM)
          IF (DUM.GE.AAMAX) THEN
            IMAX=I
            AAMAX=DUM
          ENDIF
        ENDDO
        IF (J.NE.IMAX)THEN
          DO K=1,N
            DUM=A(IMAX,K)
            A(IMAX,K)=A(J,K)
            A(J,K)=DUM
          ENDDO
          D=-D
          VV(IMAX)=VV(J)
        ENDIF
        INDX(J)=IMAX
        IF(J.NE.N)THEN
          IF(A(J,J).EQ.0.)A(J,J)=TINY
          DUM=1./A(J,J)
          DO I=J+1,N
            A(I,J)=A(I,J)*DUM
          ENDDO
        ENDIF
      ENDDO
      IF(A(N,N).EQ.0.)A(N,N)=TINY
      RETURN
END SUBROUTINE LUDCMP

! ***************************************************************************

RECURSIVE SUBROUTINE LUBKSB(A,N,NP,INDX,B)
      !
      IMPLICIT NONE
      INTEGER   :: NP, N
      INTEGER   :: INDX(N)
      REAL      :: A(NP,NP), B(N)
      ! Local Variables
      INTEGER   :: II, I, J, LL
      REAL      :: SUM
      !
      II=0
      DO I=1,N
        LL=INDX(I)
        SUM=B(LL)
        B(LL)=B(I)
        IF (II.NE.0)THEN
          DO J=II,I-1
            SUM=SUM-A(I,J)*B(J)
          ENDDO
        ELSE IF (SUM.NE.0.) THEN
          II=I
        ENDIF
        B(I)=SUM
      ENDDO
      DO I=N,1,-1
        SUM=B(I)
        IF(I.LT.N)THEN
          DO J=I+1,N
            SUM=SUM-A(I,J)*B(J)
          ENDDO
        ENDIF
        B(I)=SUM/A(I,I)
      ENDDO
      RETURN
END SUBROUTINE

RECURSIVE SUBROUTINE exact_quartic(solution,coefficients)
  ! ---------------------------------------------------
  IMPLICIT NONE 
  ! ---------------------------------------------------
  REAL    :: coefficients(5)
  COMPLEX :: solution(4) 
  ! ---------------------------------------------------
  ! Variables associated to the solution of the quartic
  ! ---------------------------------------------------
  REAL smallroot
  REAL, PARAMETER  :: smallnum = 1.0e-8
  COMPLEX ec1, ec2
  REAL  temparr_01, temparr_02, temparr_03, temparr_04
  REAL  temparr_05, temparr_06, temparr_07, temparr_08
  REAL  temparr_09, eta
  REAL ::  cubic_a, cubic_b, cubic_c, cubic_d, cubic_p, cubic_q
  REAL :: cubic_disc
  REAL :: quartic_a, quartic_b, quartic_c, quartic_d
  REAL :: quartic_e, quartic_p, quartic_q, quartic_r
  COMPLEX :: cubic_u, cubic_v, cubic_y1, cubic_y2, cubic_y3
  COMPLEX :: ctemparr_01, ctemparr_02, ctemparr_03, ctemparr_04 
  COMPLEX :: quartic_y1, quartic_y2, quartic_y3, quartic_y4
  ! ---------------------------------------------------
  !  Compute the coefficients of the quartic
  ! ---------------------------------------------------
  ! Coefficient of 4-th degree
  quartic_a  = coefficients(1) 
  ! Coefficient of 3-th degree
  quartic_b  = coefficients(2) 
  ! Coefficient of 2-th degree
  quartic_c  = coefficients(3) 
  ! Coefficient of 1-th degree
  quartic_d  = coefficients(4) 
  ! Coefficient of 0-th degree
  quartic_e  = coefficients(5) 
  !     *****************************************************************
  !     * Beginning of solving the quartic.
  !     *****************************************************************
  ec1 = CMPLX ( -0.5, 0.5 * SQRT ( 3.0) )
  ec2 = CONJG ( ec1)
  smallroot = 1.0e-30
  !     * Load the variables and transform the quartic into the corresponding
  !     * reducing cubic.
  quartic_p  = - 3.0 * quartic_b**2 / ( 8.0 * quartic_a**2)    &
  &    + quartic_c  / quartic_a 
  quartic_q  = quartic_b**3 / ( 8.0 * quartic_a**3)            & 
  &     - quartic_b  * quartic_c / ( 2.0 * quartic_a**2 ) &
  &     + quartic_d  / quartic_a 
  quartic_r  = - 3.0 * quartic_b**4                            &
  &     / ( 256.0 * quartic_a**4 )                        &
  &     + quartic_b**2 * quartic_c                        &
  &     / ( 16.0 * quartic_a**3 )                         &
  &     - quartic_b  * quartic_d                          &
  &     / ( 4.0 * quartic_a**2 )                          &
  &     + quartic_e  / quartic_a 
  cubic_a  = 1.0
  cubic_b  = 0.5 * quartic_p 
  cubic_c  = 0.0625 * quartic_p**2 - 0.25 * quartic_r 
  cubic_d  = - 0.015625 * quartic_q**2
  !
  !     *****************************************************************
  !     * Beginning of solving the cubic.
  !     *****************************************************************
  !
  cubic_p  = ( 3.0 * cubic_a  * cubic_c  - cubic_b**2 )     &
  &     / ( 9.0 * cubic_a**2)
  
  cubic_q  = 2.0 * cubic_b**3                               &
  &     / ( 27.0 * cubic_a**3) - cubic_b  * cubic_c    &
  &     / ( 3.0 * cubic_a**2) + cubic_d  / cubic_a 
  !
  cubic_q  = 0.5 * cubic_q 
  !
  cubic_disc  = - cubic_p**3 - cubic_q**2
  !
  IF ( cubic_disc  .GE. 0.0 ) THEN
     !        * Roots are real
     temparr_01  = SQRT ( - cubic_p**3 )
     temparr_02  = - cubic_q / AMAX1 ( smallroot, temparr_01  )
     temparr_02  = AMAX1 ( temparr_02 , -1.0)
     temparr_02  = AMIN1 ( temparr_02 , 1.0)
     temparr_02  = ACOS ( temparr_02  ) / 3.0
     temparr_01  = AMAX1 ( smallroot, temparr_01  )**0.33333333333333
     !
     cubic_u  = CMPLX ( temparr_01 * COS ( temparr_02  ),     &
     &             temparr_01 * SIN ( temparr_02  ) )
     cubic_v  = CONJG ( cubic_u  )
  ELSE
     !        * Roots are complex
     temparr_01  = SQRT ( - cubic_disc )
     temparr_02  = temparr_01 
     temparr_01  = - cubic_q  + temparr_01 
     temparr_02  = - cubic_q  - temparr_02 
     !    
     cubic_u  = SIGN ( 1.0, temparr_01  )
     cubic_u  = cubic_u * AMAX1 ( smallroot,                    &
     &     ABS ( temparr_01  ) )**0.33333333333333
     !
     cubic_v  = SIGN ( 1.0, temparr_02  )
     cubic_v  = cubic_v * AMAX1 ( smallroot,                    &
     &     ABS ( temparr_02  ) )**0.33333333333333
  END IF
  !
  !     * Note that "cubic_y1" will always carry a real root.
  !
  cubic_y1  = cubic_u  + cubic_v 
  cubic_y2  = ec1 * cubic_u  + ec2 * cubic_v 
  cubic_y3  = ec2 * cubic_u  + ec1 * cubic_v 
  temparr_01  = - cubic_b  / ( 3.0 * cubic_a  )
  ctemparr_01  = CMPLX ( temparr_01 , 0.0)
  cubic_y1  = cubic_y1  + ctemparr_01 
  cubic_y2  = cubic_y2  + ctemparr_01 
  cubic_y3  = cubic_y3  + ctemparr_01 
  !     *****************************************************************
  !     * End of solving the cubic.
  !     *****************************************************************
  !     * The (complex) square roots of the roots of the reducing cubic need
  !     * to be chosen so that they satisfy a consistency condition. Here we
  !     * find that specific choice that satisfies the consistency constraint.
  cubic_y1  = CSQRT ( cubic_y1  )
  cubic_y2  = CSQRT ( cubic_y2  )
  cubic_y3  = CSQRT ( cubic_y3  )
  !
  ctemparr_01  = - cubic_y1 
  ctemparr_02  = - cubic_y2 
  ctemparr_03  = - cubic_y3 
  !
  ctemparr_04  = CMPLX ( - 0.125 * quartic_q , 0.0)
  !    
  IF ( CABS ( ctemparr_01  * cubic_y2 * cubic_y3       &
     &        - ctemparr_04  ) .LE. smallnum) THEN
     cubic_y1  = ctemparr_01 
  ELSE IF ( CABS ( cubic_y1  * ctemparr_02 * cubic_y3     &
     &       - ctemparr_04  ) .LE. smallnum) THEN
     cubic_y2  = ctemparr_02 
  ELSE IF ( CABS ( cubic_y1  * cubic_y2 * ctemparr_03     &
     &       - ctemparr_04  ) .LE. smallnum) THEN
     cubic_y3  = ctemparr_03 
  ELSE IF ( CABS ( ctemparr_01  * ctemparr_02 * cubic_y3  &
     &        - ctemparr_04  ) .LE. smallnum) THEN
     cubic_y1  = ctemparr_01 
     cubic_y2  = ctemparr_02 
  ELSE IF ( CABS ( cubic_y1  * ctemparr_02 * ctemparr_03   &
     &       - ctemparr_04  ) .LE. smallnum) THEN
     cubic_y2  = ctemparr_02 
     cubic_y3  = ctemparr_03 
  ELSE IF ( CABS ( ctemparr_01  * cubic_y2 * ctemparr_03   &
     &        - ctemparr_04  ) .LE. smallnum) THEN
     cubic_y1  = ctemparr_01 
     cubic_y3  = ctemparr_03 
  ELSE IF ( CABS ( ctemparr_01  * ctemparr_02 * ctemparr_03 &
     &       - ctemparr_04  ) .LE. smallnum) THEN
     cubic_y1  = ctemparr_01 
     cubic_y2  = ctemparr_02 
     cubic_y3  = ctemparr_03 
  END IF
  !
  temparr_01  = - quartic_b   / ( 4.0 * quartic_a  )
  ctemparr_01  = CMPLX ( temparr_01 , 0.0)
  quartic_y1  = cubic_y1  + cubic_y2 + cubic_y3  + ctemparr_01 
  quartic_y2  = cubic_y1  - cubic_y2 - cubic_y3  + ctemparr_01 
  quartic_y3  = - cubic_y1  + cubic_y2 - cubic_y3  + ctemparr_01 
  quartic_y4  = - cubic_y1  - cubic_y2 + cubic_y3  + ctemparr_01 
  !     * Now sort the four roots in increasing order.
  IF ( REAL ( quartic_y1 ) .GT. REAL ( quartic_y2 ) ) THEN
     ctemparr_01  = quartic_y1 
     quartic_y1  = quartic_y2 
     quartic_y2  = ctemparr_01 
  END IF
  !
  IF ( REAL ( quartic_y2 ) .GT. REAL ( quartic_y3 ) ) THEN
     ctemparr_01  = quartic_y2 
     quartic_y2  = quartic_y3 
     quartic_y3  = ctemparr_01 
  END IF
  !
  IF ( REAL ( quartic_y3 ) .GT. REAL ( quartic_y4 ) ) THEN
     ctemparr_01  = quartic_y3 
     quartic_y3  = quartic_y4 
     quartic_y4  = ctemparr_01 
  END IF
  !
  IF ( REAL ( quartic_y1 ) .GT. REAL ( quartic_y2 ) ) THEN
     ctemparr_01  = quartic_y1 
     quartic_y1  = quartic_y2 
     quartic_y2  = ctemparr_01 
  END IF
  !
  IF ( REAL ( quartic_y2 ) .GT. REAL ( quartic_y3 ) ) THEN
     ctemparr_01  = quartic_y2 
     quartic_y2  = quartic_y3 
     quartic_y3  = ctemparr_01 
  END IF
  !
  IF ( REAL ( quartic_y1 ) .GT. REAL ( quartic_y2 ) ) THEN
     ctemparr_01  = quartic_y1 
     quartic_y1  = quartic_y2 
     quartic_y2  = ctemparr_01 
  END IF
  !
  solution(1) = quartic_y1 
  solution(2) = quartic_y2 
  solution(3) = quartic_y3 
  solution(4) = quartic_y4 
  !
  
END SUBROUTINE exact_quartic

#ifdef GEOS 
RECURSIVE SUBROUTINE FUNC_C2P_RMHD_1D_2D(x,f,df,gam,d,e,s2,b2,sb2,w)
  !
  ! This is the CONS2PRIM strategy adopted by Del Zanna et al. (2007) A&A, 473, 11-30
  ! and it corresponds to their choice 3 in Section 3.2
  !
#ifdef GEOS 	
    USE EOS_mod
#endif 	
  IMPLICIT NONE
  REAL, PARAMETER :: third=1./3.
  INTEGER    :: iter
  REAL       :: x,f,df,v2,rho,c0,c2,c3,dw,dc2,dc3,dlogw,wb,vb2
  REAL       :: gam,d,e,s2,b2,sb2,w,dfdx,ydfdy,dydx,ydfdp,dpdx,dydp
  REAL       :: eps,depsdrho,depsdp,den
  REAL, PARAMETER :: tolerance = 1e-14
  INTENT(IN) :: x,gam,d,e,s2,b2,sb2
  INTENT(OUT):: f,df,w 
  REAL :: sqrtdet,q,w0,wtest,p,c2ic3,c2ic33
  REAL, PARAMETER ::  i27=1./27.
  LOGICAL :: FAILED
  v2=x
  rho=d*sqrt(1.-v2) 
  c3=1.-gam*(1.-v2)
  c2=gam*rho+.5*b2*(1.+v2)-e
  c0=-0.5*sb2
  
  ! For every x=v2, we solve a cubic in W of the form:
  ! c3*W^3+c2*W^2+c0=0 (c3>0, c0<=0)
  ! W=y of the paper. If sb=0 ( meaning c0 = 0),
  ! w = -c2/c3 > 0 and dw = 0 in the do loop below.
  ! If -c2/c3 < 0 when sb=0, which makes w=0,
  ! this is a signature that something was wrong before.
  !
  ! given w=rho*h*W^2, solve the 2d nonlinear system for x=(w,p) 
  CALL RTSolve_V2_P_C2P_RMHD(w,p,v2,gam,d,e,s2,b2,sb2,FAILED)
  ! then compute f and df for the 1D NR iteration F=F(w(v2),p(v2),v2)=0
  !
  dc3   = gam
  dc2   = 0.5 * ( b2 - gam * rho / (1.0 - v2))
  dlogw = -( dc3 * w + dc2 ) / ( 3.0 * c3 * w + 2.0 * c2)
  wb    = w + b2
  vb2   = sb2 / w**2
  !
  f     = wb**2 * v2 - ( 2.0 * w + b2) * vb2 - s2
  !
  dfdx = wb*wb
  ydfdy = 2.0*wb*(w*v2+vb2)
  dlogw = ( dc3 * w + dc2 ) / ( 3.0 * c3 * w + 2.0 * c2)
  !
  CALL  EOS_eps_depsdrho_depsdp(eps,depsdrho,depsdp,rho,p)
  !
  ydfdp = w*D*D/rho*depsdp + w/(1.0-x)
  den = (1.0 + rho*depsdp)
  !dp/dx = 
  
  df    = wb * ( wb + 2.0 * dlogw * ( w * v2 + vb2)) 
  !
  ! compute df = df/dx + df/dy*(dy/dx + dy/dp*dp/dx) + df/dp*dp/dx .... ???
  !  df/dp = 0 ...
END SUBROUTINE FUNC_C2P_RMHD_1D_2D

RECURSIVE SUBROUTINE RTSolve_V2_P_C2P_RMHD(w,p,v2,gam,d,e,s2,b2,sb2,FAILED)
  !***********************************************************
  ! given w=rho*h*W^2, solve the 2d nonlinear system for x=(w,p) 
  !*****
  IMPLICIT NONE
  ! arguments
  REAL :: w,p,v2,gam,d,e,s2,b2,sb2
  LOGICAL :: FAILED
  INTENT (INOUT) :: w,p
  INTENT (IN)    :: v2,gam,d,e,s2,b2,sb2
  ! local variables
  INTEGER:: i
  REAL :: dx(2),x(2),f(2),df(2,2),idf(2,2),det,drhodx,dydx,depsdrho,depsdp,eps
  !  
  REAL, PARAMETER :: third=1./3.
  INTEGER, PARAMETER :: MaxNewton=100
  REAL, PARAMETER :: tolerance = 1e-10 
  FAILED = .FALSE.
  !v2=x0
  ! check the value fo 
  x(1) = w
  x(2) = p
  CALL FUNC_C2P_RMHD_F2F3(x,f,df,v2,gam,d,e,s2,b2,sb2)
  det =df(1,1)*df(2,2)-df(1,2)*df(2,1)
  idf(1,1) = df(2,2)/det
  idf(1,2) =-df(1,2)/det
  idf(2,1) =-df(2,1)/det
  idf(2,2) =-df(1,1)/det
  !
  dx(1) = -idf(1,1)*f(1)-idf(1,2)*f(2)     ! = dv2
  dx(2) = -idf(2,1)*f(1)-idf(2,2)*f(2)     ! = dp
  !
  IF(SUM(ABS(dx)).LT.tolerance) THEN
      continue
      RETURN
  ENDIF
  !
  DO i=1,MaxNewton
        x=x+dx   
        CALL FUNC_C2P_RMHD_F2F3(x,f,df,v2,gam,d,e,s2,b2,sb2)
        det =df(1,1)*df(2,2)-df(1,2)*df(2,1)
        idf(1,1) = df(2,2)/det
        idf(1,2) =-df(1,2)/det
        idf(2,1) =-df(2,1)/det
        idf(2,2) =-df(1,1)/det
        !
        dx(1) = -idf(1,1)*f(1)-idf(1,2)*f(2)     ! = dv2
        dx(2) = -idf(2,1)*f(1)-idf(2,2)*f(2)     ! = dp
        !
        IF(SUM(ABS(dx)).LT.tolerance) THEN
            w=x(1)
            p=x(2)
            continue
            RETURN
        ENDIF
  ENDDO
  !
  FAILED = .TRUE.
  w=x(1)
  p=x(2)
  !  
  continue
  !
END SUBROUTINE RTSolve_V2_P_C2P_RMHD


RECURSIVE SUBROUTINE FUNC_C2P_RMHD_F2F3(x,f,df,v2,gam,d,e,s2,b2,sb2)
  USE EOS_mod
  IMPLICIT NONE
  ! arguments
  REAL :: x(2),f(2),df(2,2)
  REAL :: v2,gam,d,e,s2,b2,sb2 
  INTENT (INOUT) :: x,f,df
  INTENT (IN)    :: v2,gam,d,e,s2,b2,sb2
  ! local variables
  INTEGER    :: i
  REAL :: det,drhodx,dydx,depsdrho,depsdp,eps ,w
  REAL, PARAMETER :: third=1./3.
  !
  REAL :: rho,c0,c2,c3,dw,dc2,dc3,dlogw,wb,vb2 
  ! ... we actually know the root of a cubic!!!!
  REAL :: sqrtdet,q,w0,wtest,p,c2ic3,c2ic33
  REAL, PARAMETER ::  i27=1./27. 
  w=x(1)
  p=x(2)
  rho=d*sqrt(1.-v2) 
  c3=1. !-gam*(1.-v2)
  c2=.5*b2*(1.+v2)-e-p
  c0=-0.5*sb2
  !
  f(1) = c3*w**3 + c2*w**2 + c0
  !
  CALL EOS_eps_depsdrho_depsdp(eps,depsdrho,depsdp,rho,p) 
  !
  f(2) = p + eps*rho + rho - (1.-v2)*w 
  !
  ! compute the jacobian: (d/dw, d/dp)[ (/ f(1), f(2) /) ]
  dc3   = 1.0
  dc2   = 0.5 * ( b2 - gam * rho / (1.0 - v2))
  dlogw = -( dc3 * w + dc2 ) / ( 3.0 * c3 * w + 2.0 * c2)
  wb    = w + b2
  vb2   = sb2 / w**2
  f     = wb**2 * v2 - ( 2.0 * w + b2) * vb2 - s2
  df    = wb * ( wb + 2.0 * dlogw * ( w * v2 + vb2))
  
  df(1,1) = (3*c3*w + 2*c2)*w
  df(1,2) = -w**2
  !
  df(2,1) = -(1-v2)
  df(2,2) = 1.0+rho*depsdp
  !
  continue
  !
END SUBROUTINE FUNC_C2P_RMHD_F2F3

#endif 
    


RECURSIVE SUBROUTINE PDEEigenvectorsGRMHD(R,L,iR,Q,nv) 
  USE MainVariables, ONLY: EQN, d ,ndim
	!USE AstroMod
    !USE EOS_mod
  IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
  ! Argument list 
  REAL :: R(nVar,nVar), L(nVar,nVar), iR(nVar,nVar)
  REAL :: Q(nVar), normal(nDim)
  REAL :: nv(d)
  ! 
  ! LOCAL VARIABLES
  ! Local variables
  INTEGER :: i,j,k, zero,iErr, itemp(nVar), info     
  REAL    :: rho,u,v,w,p,c,c2,H,v2,M,r2c,Pi,gmo,den,lf,lf2,kk,hrho
  REAL    :: Cplus, Cminus, Aplus, Aminus, Lplus, Lminus, Delta, ftr, vs
  REAL    :: sv(3),tv(3),Lambda(nVar)  
  REAL    :: VPR(3),BVR(3),kappa(2),beta(2)
  REAL    :: TM(nVar,nVar),iTM(nVar,nVar),A(nVar,nVar),RM(nVar,nVar),iRM(nVar,nVar) 
  REAL    :: TestMatrix(nVar,nVar) 
  REAL    :: R7(7,7), iR7(7,7)
  REAL    :: VP(nVar), QPR(nVar),dudw(nVar,nVar),dwdu(nVar,nVar)
  REAL    :: dfdQ(nVar,nVar), ImLambda(nVar), rtemp(nVar)    
  REAL    :: t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t13,t19,t21,t16,t18,t30,t31,t35,t37,t43,t47,t48,t11
  REAL    :: t12,t14,t15,t17,t20,t22,t24,t26,t27,t28,t29,t32,t33,t36,t38,t39,t40,t41,t42,t44,t46,t49,t65,t70,t23,t25,t34
  REAL    :: t51,t53,t55,t56,t60,t61,t68,t69,t75,t76,t78,t45,t50,t52,t54,t57,t63,t64,t58,t62 
  REAL    :: rho0, k0, uu, vv, ww, eps, dist, alpha, cp, cs, nx, ny, uv(3), lam, mu, irho, ialpha,radalpha
  REAL    :: Qp(nVar),Qm(nVar),fpp(nVar),gpp(nVar),hpp(nVar),fmm(nVar),gmm(nVar),hmm(nVar),tempGJ(nVar,nVar), Qrot(nVar)   
  REAL    :: dQdV(nVar,nVar), dVdQ(nVar,nVar),rhov(2), av(2), ux(2), vy(2), pv(2), alphav(2), curv, sigma, gamma1, gamma2, pi1, pi2, g    
  REAL    :: AM(3,3), GT(3,3), devG(3,3), Id(3,3), evv, tempp(3,3), rhok, R1(nVar,nVar), L1(nVar), R2(nVar,nVar), L2(nVar)    
  REAL    :: s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15,s16,s17  
  REAL    :: S(nVar),dSdQ(nVar,nVar),dQdS(nVar,nVar), dfdS(nVar,nVar), dPdS(nVar,nVar), iL1(nVar,nVar), dSdP(nVar,nVar), checkmat(nVar,nVar)    
  REAL    :: alpham(3,3), B(nVar,nVar), HA(nVar,nVar), L1M(nVar,nVar), sqrtM(nVar,nVar), sqrtiM(nVar,nVar), param, mix, cv, AA(nVar,nVar)    
  REAL    :: zm, q0  
  INTEGER, PARAMETER :: LWORK=2*nVar*nVar+6*nVar+1, LIWORK=5*nVar+3
  REAL    :: WORK(LWORK), FWORK(8*nVar) 
  INTEGER :: IWORK(LIWORK), IWORK2(nVar)  
  ! GRMHD
  REAL :: Lambda_ia(nVar)
  REAL :: reivec1(nvar),reivec2(nvar),reivec3(nvar),reivecm(nvar),reivecp(nvar)
  REAL :: leivec1(nvar),leivec2(nvar),leivec3(nvar),leivecm(nvar),leivecp(nvar)
  REAL :: v_cov(3),B_contr(3),B_cov(3),shift(3),v_contr(3),vn_contr(3),vn_cov(3)
  REAL :: g_cov(3,3),g_contr(3,3)
  REAL :: psi,lapse,gp,gm,lf2M,b2,vdotb,b2_4,vn,gg,sft,vxp,vxm,enthalpy,dpdeps,xsi,cs2,tmp2,tmp1,gam,dlt,cxx,cxy,cxz,axp,axm,kappa0
  LOGICAL, PARAMETER :: ANALYTICAL = .TRUE.

  
  REAL, PARAMETER :: epsilon = 1e-11   
  !
  Pi = ACOS(-1.0)
  R = 0. 
  L = 0. 
  iR = 0. 
  ! 
    ! 
    CALL PDECons2PrimGRMHD(VP,Q,iErr)
    rho    = VP(1)
	DO i=1,3
		v_cov(i) = VP(1+i)
		B_contr(i) = VP(5+i)
		shift(i) = VP(10+i)
	ENDDO
    p      = VP(5)
    !
    psi = VP(9)
    lapse = VP(10)
    !
    g_cov(1,1) = VP(14)
    g_cov(1,2) = VP(15)
    g_cov(1,3) = VP(16)
    g_cov(2,2) = VP(17)
    g_cov(2,3) = VP(18)
    g_cov(3,3) = VP(19)
    g_cov(2,1) = VP(15)
    g_cov(3,1) = VP(16)
    g_cov(3,2) = VP(18)
    !  
    CALL MatrixInverse3x3(g_cov,g_contr,gp)
    gp = SQRT(gp)
    gm = 1./gp
    !  evaluate contr. cov. variables
    v_contr      = MATMUL(g_contr,v_cov)
    B_cov      = MATMUL(g_cov,B_contr)
    !  evaluate useful quantities
    v2     = v_contr(1)*v_cov(1) + v_contr(2)*v_cov(2) + v_contr(3)*v_cov(3)
    lf     = 1.0/sqrt(1.0 - v2)
    lf2m   = 1.0 - v2
    b2     = B_contr(1)*B_cov(1) + B_contr(2)*B_cov(2) + B_contr(3)*B_cov(3)
    VdotB     = v_contr(1)*B_cov(1) + v_contr(2)*B_cov(2) + v_contr(3)*B_cov(3)
    b2_4 = b2*lf2m + VdotB  ! this is b^2
    gamma1 = EQN%gamma/(EQN%gamma-1.0) 
    w      = rho + gamma1*p + b2_4 ! this is rho*h + b^2
    cs2    = (EQN%gamma*p + b2_4)/w
    !
    vn     = v_contr(1)*nv(1) + v_contr(2)*nv(2) + v_contr(3)*nv(3)
    sft    = shift(1)*nv(1) + shift(2)*nv(2) + shift(3)*nv(3) 
    gg     = g_contr(1,1)*ABS(nv(1)) + g_contr(2,2)*ABS(nv(2)) + g_contr(3,3)*ABS(nv(3))
    den    = 1.0/(1.0 - v2*cs2)
    IF(SUM(nv**2).EQ.0.) THEN  
        u = SQRT( v2) 
        WRITE(*,*)'Impossible error! nv:',nv
        STOP
    ELSE
        u = vn 
    ENDIF
    !
    ! to be sure that we use the same routine
    !CALL PDEEigenvaluesGRMHD(lambda,Q,nv) 
    
    Lambda(1)   = ( u*(1.0-cs2) - SQRT( cs2*lf2m*( (1.0-v2*cs2)*gg - u**2*(1.0-cs2) )) )*den 
    Lambda(2:4) = u
    Lambda(5)   = ( u*(1.0-cs2) + SQRT( cs2*lf2m*( (1.0-v2*cs2)*gg - u**2*(1.0-cs2) )) )*den
    Lambda_ia(1:5)   = Lambda(1:5)
    Lambda(1:5)   = lapse*Lambda(1:5) - sft
    !
    Lambda(6:) = 0.
     
  !
  !  sv = (/ 1., 1., 1. /) - ABS(nv)
  !  sv = sv/SQRT(SUM(sv(:)**2))  
  !  CALL Kreuzprodukt(tv,nv,sv)  
  !
  IF(ABS(ABS(nv(1))-1.0).LE.1e-14) THEN
     sv = (/ 0., 1., 0. /) 
     tv = (/ 0., 0., 1. /) 
  ENDIF
  IF(ABS(ABS(nv(2))-1.0).LE.1e-14) THEN
     !sv = (/ 1., 0., 0. /) 
     !tv = (/ 0., 0., 1. /) 
     sv = (/ 0., 0., 1. /) 
     tv = (/ 1., 0., 0. /) 
  ENDIF
  IF(ABS(ABS(nv(3))-1.0).LE.1e-14) THEN
     sv = (/ 1., 0., 0. /) 
     tv = (/ 0., 1., 0. /) 
  ENDIF
  !
  TM(:,:)    = 0.
  TM(1,1)    = 1.
  TM(5,5)    = 1.
  TM(2:4,2)  = nv(:)
  TM(2:4,3)  = sv(:) 
  TM(2:4,4)  = tv(:) 
  !
  !! velocity field rotated in normal direction
  vn_contr(1)  = DOT_PRODUCT(v_contr(1:3),nv(1:3))   ! nv(1)*VP(2) + nv(2)*VP(3) + nv(3)*VP(4) )
  vn_contr(2)  = DOT_PRODUCT(v_contr(1:3),sv(1:3))   ! sv(1)*VP(2) + sv(2)*VP(3) + sv(3)*VP(4) )
  vn_contr(3)  = DOT_PRODUCT(v_contr(1:3),tv(1:3))   ! tv(1)*VP(2) + tv(2)*VP(3) + tv(3)*VP(4) )
  !! 
  vn_cov      = MATMUL(g_cov,vn_contr)
  !  if (cs2<0) cs2=0 ! this does not modify the roe crashing problem with shocktube
  !enthalpy = one + eps + press / rho
  enthalpy = (w-b2_4)/rho
  !vx_cov =  vn_cov(1)
  !vy_cov =  vn_cov(2)
  !vz_cov =  vn_cov(3) 
!!$Calculate eigenvalues and put them in conventional order
  !
  !lam1 = vn_contr(1) - sft/lapse
  !lam2 = vn_contr(1) - sft/lapse
  !lam3 = vn_contr(1) - sft/lapse
  !
  !Lambda_ia(5) = (vn_contr(1)*(1.0-cs2) + sqrt(cs2*(1.0-v2)*&
  !     (gg*(1.0-v2*cs2) - vn_contr(1)**2*(1.0-cs2))))/(1.0-v2*cs2)
  !Lambda_ia(1) = (vn_contr(1)*(1.0-cs2) - sqrt(cs2*(1.0-v2)*&
  !     (gg*(1.0-v2*cs2) - vn_contr(1)**2*(1.0-cs2))))/(1.0-v2*cs2)
  !
  !lamp = Lambda_ia(5) - sft/lapse
  !lamm = Lambda_ia(1) - sft/lapse

!!$  lam(1) = lamm
!!$  lam(2) = lam1
!!$  lam(3) = lam2
!!$  lam(4) = lam3
!!$  lam(5) = lamp
 
!!$Compute some auxiliary quantities

  axp = (gg - vn_contr(1)*vn_contr(1))/(gg - vn_contr(1)*Lambda_ia(5))
  axm = (gg - vn_contr(1)*vn_contr(1))/(gg - vn_contr(1)*Lambda_ia(1))
  vxp = (vn_contr(1) - Lambda_ia(5))/(gg - vn_contr(1) * Lambda_ia(5))
  vxm = (vn_contr(1) - Lambda_ia(1))/(gg - vn_contr(1) * Lambda_ia(1))

!!$Calculate associated right eigenvectors
  dpdeps = rho*(EQN%gamma-1.0)
  kappa0 = dpdeps / (dpdeps - rho * cs2)

  reivec1(1) = kappa0 / (enthalpy * lf)
  reivec1(2) = vn_cov(1)
  reivec1(3) = vn_cov(2)
  reivec1(4) = vn_cov(3)
  reivec1(5) = 1.0 - reivec1(1)

  reivec2(1) = lf * vn_cov(2)
  reivec2(2) = enthalpy * (g_cov(1,2) + 2.0 * lf * lf * vn_cov(1) * vn_cov(2))
  reivec2(3) = enthalpy * (g_cov(2,2) + 2.0 * lf * lf * vn_cov(2) * vn_cov(2))
  reivec2(4) = enthalpy * (g_cov(2,3) + 2.0 * lf * lf * vn_cov(2) * vn_cov(3))
  reivec2(5) = vn_cov(2) * lf * (2.0 * lf * enthalpy - 1.0)

  reivec3(1) = lf * vn_cov(3)
  reivec3(2) = enthalpy * (g_cov(1,3) + 2.0 * lf * lf * vn_cov(1) * vn_cov(3))
  reivec3(3) = enthalpy * (g_cov(2,3) + 2.0 * lf * lf * vn_cov(2) * vn_cov(3))
  reivec3(4) = enthalpy * (g_cov(3,3) + 2.0 * lf * lf * vn_cov(3) * vn_cov(3))
  reivec3(5) = vn_cov(3) * lf * (2.0 * lf * enthalpy - 1.0)

  reivecp(1) = 1.0
  reivecp(2) = enthalpy * lf * (vn_cov(1) - vxp)
  reivecp(3) = enthalpy * lf * vn_cov(2)
  reivecp(4) = enthalpy * lf * vn_cov(3)
  reivecp(5) = enthalpy * lf * axp - 1.0

  reivecm(1) = 1.0
  reivecm(2) = enthalpy * lf * (vn_cov(1) - vxm)
  reivecm(3) = enthalpy * lf * vn_cov(2)
  reivecm(4) = enthalpy * lf * vn_cov(3)
  reivecm(5) = enthalpy * lf * axm - 1.0

!!$Calculate associated left eigenvectors if requested

 ! if (ANALYTICAL) then

    cxx = g_cov(2,2) * g_cov(3,3) - g_cov(2,3) * g_cov(2,3)
    cxy = g_cov(1,3) * g_cov(2,3) - g_cov(1,2) * g_cov(3,3)
    cxz = g_cov(1,2) * g_cov(2,3) - g_cov(1,3) * g_cov(2,2)
    gam = g_cov(1,1) * cxx + g_cov(1,2) * cxy + g_cov(1,3) * cxz
    xsi = cxx - gam * vn_contr(1) * vn_contr(1)
    dlt = enthalpy**3 * lf * (kappa0 - 1.0) * (vxm - vxp) * xsi

    tmp1 = lf / (kappa0 - 1.0)

    leivec1(1) = tmp1 * (enthalpy - lf)
    leivec1(2) = tmp1 * lf * vn_contr(1)
    leivec1(3) = tmp1 * lf * vn_contr(2)
    leivec1(4) = tmp1 * lf * vn_contr(3)
    leivec1(5) =-tmp1 * lf

    tmp1 = 1.0 / (xsi * enthalpy)

    leivec2(1) = (g_cov(2,3) * vn_cov(3) - g_cov(3,3) * vn_cov(2)) * tmp1
    leivec2(2) = (g_cov(3,3) * vn_cov(2) - g_cov(2,3) * vn_cov(3)) * tmp1 * vn_contr(1)
    leivec2(3) = (g_cov(3,3) * (1.0 - vn_contr(1) * vn_cov(1)) + g_cov(1,3) * vn_cov(3) * vn_contr(1)) * tmp1
    leivec2(4) = (g_cov(2,3) * (vn_contr(1) * vn_cov(1) - 1.0) - g_cov(1,3) * vn_contr(1) * vn_cov(2)) * tmp1
    leivec2(5) = (g_cov(2,3) * vn_cov(3) - g_cov(3,3) * vn_cov(2)) * tmp1

    leivec3(1) = (g_cov(2,3) * vn_cov(2) - g_cov(2,2) * vn_cov(3)) * tmp1
    leivec3(2) = (g_cov(2,2) * vn_cov(3) - g_cov(2,3) * vn_cov(2)) * tmp1 * vn_contr(1)
    leivec3(3) = (g_cov(2,3) * (vn_contr(1) * vn_cov(1) - 1.0) - g_cov(1,2) * vn_contr(1) * vn_cov(3)) * tmp1
    leivec3(4) = (g_cov(2,2) * (1.0 - vn_contr(1) * vn_cov(1)) + g_cov(1,2) * vn_contr(1) * vn_cov(2)) * tmp1
    leivec3(5) = (g_cov(2,3) * vn_cov(2) - g_cov(2,2) * vn_cov(3)) * tmp1

    tmp1 = enthalpy * enthalpy / dlt
    tmp2 = lf * lf * xsi

    leivecp(1) = - (enthalpy * lf * vxm * xsi + (1.0 - kappa0) * (vxm * &
      (tmp2 - cxx) - gam * vn_contr(1)) - kappa0 * tmp2 * vxm) * tmp1
    leivecp(2) = - (cxx * (1.0 - kappa0 * axm) + (2.0 * kappa0 - 1.0) * vxm * &
      (tmp2 * vn_contr(1) - cxx * vn_contr(1))) * tmp1
    leivecp(3) = - (cxy * (1.0 - kappa0 * axm) + (2.0 * kappa0 - 1.0) * vxm * &
      (tmp2 * vn_contr(2) - cxy * vn_contr(1))) * tmp1
    leivecp(4) = - (cxz * (1.0 - kappa0 * axm) + (2.0 * kappa0 - 1.0) * vxm * &
      (tmp2 * vn_contr(3) - cxz * vn_contr(1))) * tmp1
    leivecp(5) = - ((1.0 - kappa0) * (vxm * (tmp2 - cxx) - gam * vn_contr(1)) - &
      kappa0 * tmp2 * vxm) * tmp1

    leivecm(1) = (enthalpy * lf * vxp * xsi + (1.0 - kappa0) * (vxp * &
      (tmp2 - cxx) - gam * vn_contr(1)) - kappa0 * tmp2 * vxp) * tmp1
    leivecm(2) = (cxx * (1.0 - kappa0 * axp) + (2.0 * kappa0 - 1.0) * vxp * &
      (tmp2 * vn_contr(1) - cxx * vn_contr(1))) * tmp1
    leivecm(3) = (cxy * (1.0 - kappa0 * axp) + (2.0 * kappa0 - 1.0) * vxp * &
      (tmp2 * vn_contr(2) - cxy * vn_contr(1))) * tmp1
    leivecm(4) = (cxz * (1.0 - kappa0 * axp) + (2.0 * kappa0 - 1.0) * vxp * &
      (tmp2 * vn_contr(3) - cxz * vn_contr(1))) * tmp1
    leivecm(5) = ((1.0 - kappa0) * (vxp * (tmp2 - cxx) - gam * vn_contr(1)) - &
      kappa0 * tmp2 * vxp) * tmp1
!  endif
  !
  !IF(Q(1).LT.1e-9) THEN
  !    Lambda = 1.0
  !ENDIF
  
  ! Eigenvalues
  L = 0.0
  L(1,1) = Lambda(1)                      !( u*(1.0-c2)+c*SQRT( (1.0-v2)*( (1.0-v2*c2) - u**2*(1.0-c2) )) )*den 
  L(2,2) = Lambda(2)                      !u
  L(3,3) = Lambda(3)                      !u
  L(4,4) = Lambda(4)                      !u
  L(5,5) = Lambda(5)                      !( u*(1.0-c2)-c*SQRT( (1.0-v2)*( (1.0-v2*c2) - u**2*(1.0-c2) )) )*den
  DO i=6,nVar
    L(i,i) = 0.0    
  ENDDO
  !
  !
  RM = 0.
  ! Right eigenvector matrix
  RM(1:5,1)  = reivecm(1:5)   !   (/ 1.0/lf,     lf*v,                 lf*w,                1.0,              1.0                 /)
  RM(1:5,2)  = reivec1(1:5)   ! (/ u,          2.*h*lf2*u*v,         2.*h*lf2*u*w,        h*lf*Cplus,       h*lf*Cminus         /)
  RM(1:5,3)  = reivec2(1:5)   ! (/ v,          h*(1.0+2.0*lf2*v*v),  2.*h*lf2*v*w,        h*lf*v,           h*lf*v              /)
  RM(1:5,4)  = reivec3(1:5)   ! (/ w,          2.*h*lf2*w*v,         h*(1.0+2.0*lf2*w*w), h*lf*w,           h*lf*w              /)
  RM(1:5,5)  = reivecp(1:5)   ! (/ 1.0-1.0/lf, 2.*h*lf2*v-lf*v,      2.*h*lf2*w-lf*w,     h*lf*Aplus - 1.0, h*lf*Aminus - 1.0   /)
  !
  iRM = 0.
  ! Left eigenvector matrix (inverse of R)
  iRM(1,1:5)  = leivecm(1:5)   != (/ h - lf,                           lf*u,                                 lf*v,                                   lf*w,                                   -lf    /)
  iRM(2,1:5)  = leivec1(1:5)   != (/ -v,                               u*v,                                  1.0 - u*u,                              0.0,                                    -v     /)
  iRM(3,1:5)  = leivec2(1:5)   != (/ -w,                               u*w,                                  0.0,                                    1.0 - u*u,                              -w     /)
  iRM(4,1:5)  = leivec3(1:5)   != (/ h*lf*(Aminus*u-Cminus) + Lminus,  1.0+ftr*(1.0-Aminus) - kk*Aminus,  lf2*v*(2*kk-1.0)*(Aminus*u-Cminus),  lf2*w*(2*kk-1.0)*(Aminus*u-Cminus),  Lminus /)
  iRM(5,1:5)  = leivecp(1:5)   != (/ h*lf*(Aplus*u-Cplus) + Lplus,     1.0+ftr*(1.0-Aplus)  - kk*Aplus,   lf2*v*(2*kk-1.0)*(Aplus*u-Cplus),    lf2*w*(2*kk-1.0)*(Aplus*u-Cplus),    Lplus  /)
  !
  DO i=6,nVar
      RM(i,i) = 1.0
      iRM(i,i) = 1.0
  ENDDO
  !
  !PRINT *,"RM"
  ! DO i=1,nVar
  !      PRINT *,RM(:,i)
  ! ENDDO
  !PRINT *,"iRM"
  ! DO i=1,nVar
  !      PRINT *,iRM(i,:)
  ! ENDDO
  
   !CALL MatrixInverse(nVar,RM,iRM)
   !TestMatrix = MATMUL(RM,iRM) 
   !DO i=1,nVar
   !    DO j=1,nVar
   !         IF(i.NE.j) THEN
   !             IF(ABS(TestMatrix(i,j)).GT.1e-12) THEN 
   !                 WRITE(*,*) ' ERROR in ConsJacobian: Transformation matrix checksum error! ',ABS(TestMatrix(i,j))
   !                 !STOP
   !             ENDIF 
   !         ELSE
   !             IF(ABS(TestMatrix(i,j)-1.0).GT.1e-12) THEN 
   !                 WRITE(*,*) ' ERROR in ConsJacobian: Transformation matrix checksum error! ',ABS(TestMatrix(i,j)-1.0)
   !                 !STOP
   !             ENDIF 
   !         ENDIF
   !         !
   !    ENDDO
   !ENDDO
   !
   !IF( (SUM(TestMatrix).GT.(5.+epsilon)).OR.(SUM(TestMatrix).LT.(5.-epsilon)) ) THEN
   ! WRITE(*,*) ' ERROR in ConsJacobian: Transformation matrix checksum error! '
   ! !STOP
   !ENDIF
   !
  !R = RM
  !iR = iRM
  !!
        ! Final Matrices including the rotation 
        R  = 0.
        iR = 0. 
        DO j = 1, 5
           DO k = 1, 5
              R(1:5,j) = R(1:5,j) + TM(1:5,k)*RM(k,j)
           ENDDO
        ENDDO
        DO k = 1, 5
           DO j = 1, 5
              iR(1:5,j) = iR(1:5,j) + iRM(1:5,k)*TM(j,k)
           ENDDO
        ENDDO
        DO i =6,nVar
            R(i,i) = 1.0
            iR(i,i) = 1.0
        ENDDO
         
  !!
  CONTINUE
  !
END SUBROUTINE PDEEigenvectorsGRMHD

RECURSIVE SUBROUTINE PDEIntermediateFieldsGRMHD(RL,LL,iRL,Q,nv) 
  USE Mainvariables, ONLY: EQN, nDim, d 
  IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
  INTEGER, PARAMETER :: nLin=3  
  ! Argument list 
  REAL :: RL(nVar,nLin), LL(nLin,nLin), iRL(nLin,nVar)
  REAL :: Q(nVar), nv(d)
  REAL :: x(nDim), time
  INTENT(IN)  :: Q,nv
  INTENT(OUT) :: RL,LL,iRL 
  ! Local variables
  INTEGER :: i,j,k, zero,iErr, minl(1), maxl(1) 
  REAL :: R(nVar,nVar), L(nVar,nVar), Lv(nVar), iR(nVar,nVar)
  ! 
 ! PRINT *, "HEY, I'm in the IntermediateFields subroutine!" 
  !
   
  CALL PDEEigenvectorsGRMHD(R,L,iR,Q,nv)
    
  DO i = 1, nVar
      Lv(i) = L(i,i)     
  ENDDO
  
  !minl = MINLOC(Lv) 
  !maxl = MAXLOC(Lv)
  !
  !i = 0 
  !LL = 0. 
  !   
  !DO j = 1, nVar
  !    IF( (j.NE.minl(1)).AND.(j.NE.maxl(1)) ) THEN 
  !        i = i + 1 
  !        RL(:,i) = R(:,j) 
  !        iRL(i,:) = iR(j,:) 
  !        LL(i,i)  = L(j,j) 
  !    ENDIF      
  !ENDDO 
     
      RL(:,1)  = R(:,2) 
      iRL(1,:) = iR(2,:) 
      LL(1,1)  = L(2,2) 

      RL(:,2)  = R(:,3)
      iRL(2,:) = iR(3,:)
      LL(2,2)  = L(3,3)

      RL(:,3)  = R(:,4)
      iRL(3,:) = iR(4,:)
      LL(3,3)  = L(4,4)
   
  !
    END SUBROUTINE PDEIntermediateFieldsGRMHD    
    
RECURSIVE SUBROUTINE HLLEMFluxFVGRMHD(FL,FR,QL,QR,nv) 
  USE MainVariables, ONLY : nDim,d
  USE iso_c_binding 
  IMPLICIT NONE
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVar = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVar = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVar = 19                           ! The number of variables of the PDE system 
#endif
  ! Local variables
  REAL, INTENT(IN)     :: nv(d) 
  REAL, INTENT(IN)     :: QL(nVar)
  REAL, INTENT(IN)     :: QR(nVar)
  REAL, INTENT(INOUT)  :: FL(nVar)
  REAL, INTENT(INOUT)  :: FR(nVar)
    ! Local variables 
  INTEGER, PARAMETER :: nLin = 3  
  INTEGER           :: i,j,k,l, ml(1)  
  REAL              :: smax, Qav(nVar),sL,sR
  REAL              ::  flattener(nLin)
  REAL    :: absA(nVar,nVar), amax, minL, maxR, minM, maxM   
  REAL    :: QM(nVar),LL(nVar),LR(nVar),LM(nVar)
  REAL    :: deltaL(nLin,nLin),Lam(nLin,nLin),Lap(nLin,nLin) 
  REAL    :: RL(nVar,nLin),iRL(nLin,nVar), temp(nLin,nVar), LLin(nLin,nLin) 
  REAL    :: Aroe(nVar,nVar),Aroep(nVar,nVar), Aroem(nVar,nVar), Dm(nVar), Dp(nVar), dQ(nVar)
  REAL :: fRtmp(nVar),f1R(nVar), g1R(nVar), h1R(nVar),FoneR(nVar,d)
  REAL :: fLtmp(nVar),f1L(nVar), g1L(nVar), h1L(nVar),FoneL(nVar,d)
  !  
  !
  flattener=1.
  !
  !PRINT *, 'nv=', nv(1:3)
  IF(ANY(ISNAN(QL))) THEN
  	PRINT *,'QL is NaN!'
  	continue
  ENDIF
  IF(ANY(ISNAN(QR))) THEN
        PRINT *,'QRL is NaN!'
        continue
  ENDIF

  !
  CALL PDEFluxGRMHD(FoneL,QL) !f1L,g1L,h1L,QL)
  CALL PDEFluxGRMHD(FoneR,QR) !f1R,g1R,h1R,QR)
  !
  fRtmp = FoneR(:,1)*nv(1)+FoneR(:,2)*nv(2)+FoneR(:,3)*nv(3)
  fLtmp = FoneL(:,1)*nv(1)+FoneL(:,2)*nv(2)+FoneL(:,3)*nv(3)
  ! 
  
  IF(ANY(ISNAN(fLtmp))) THEN
        PRINT *,'fLtmp is NaN!'
        continue
  ENDIF
  IF(ANY(ISNAN(fRtmp))) THEN
        PRINT *,'fRtmp is NaN!'
        continue
  ENDIF

  !USE Parameters, ONLY : d,nVar,ndim 
  QM = 0.5*(QL+QR) 
  !CALL PDECons2PrimGRMHD(VM,QM,iErr)
  CALL PDEEigenvaluesGRMHD(LL,QL,nv)  
  CALL PDEEigenvaluesGRMHD(LR,QR,nv)  
!IF(ANY(QM(6:8).NE.0)) THEN
!    PRINT *, "HLLEMFluxFV QM(6:8)",QM(6:8)
!    STOP
!ENDIF
  CALL PDEEigenvaluesGRMHD(LM,QM,nv)  
  !PRINT *, 'EigenGRMHD DONE'
  minL = MINVAL(LL)
  minM = MINVAL(LM)
  maxR = MAXVAL(LR) 
  maxM = MAXVAL(LM) 
  sL = MIN( 0., minL, minM ) 
  sR = MAX( 0., maxR, maxM )   
  CALL PDEIntermediateFieldsGRMHD(RL,LLin,iRL,QM,nv) 
  !PRINT *, "PDEIntermediateFields finished"
  Lam = 0.5*(LLin-ABS(LLin))
  Lap = 0.5*(LLin+ABS(LLin)) 
  deltaL = 0.0
  DO i = 1, nLin
      deltaL(i,i) = (1. - Lam(i,i)/(sL-1e-14) - Lap(i,i)/(sR+1e-14) )*flattener(i)  
  ENDDO  
  !
  !amax = 0. 
  !
  absA = 0. 
  DO i = 1, nVar
      absA(i,i) = sR*sL/((sR-sL)+1e-14) ! - 0.5*amax ! regular HLL diffusion, only on the diagonal 
  ENDDO  
  !
  IF(QR(1).LT.1e-6.OR.QL(1).LT.1e-6) THEN
        deltaL = 0.
  ELSE 
     temp = MATMUL( deltaL, iRL ) 
     absA = absA - sR*sL/((sR-sL)+1e-14)*MATMUL( RL, temp )  ! HLLEM anti-diffusion  
  ENDIF    
  !    
  !CALL RoeMatrixGRMHD(ARoe,QL,QR,nv)
  !!
  !ARoem = -sL*ARoe*(sR-sL)/((sR-sL)+1e-14)
  !ARoep = +sR*ARoe*(sR-sL)/((sR-sL)+1e-14)
  !! 
  !!DR = ARoep
  !!DL = ARoem
  !!!!!FL(:) = 0.5*( FR(:) + FL(:) ) + MATMUL(absA, QR(:) - QL(:) )    ! purely conservative flux 
  !!!!!FR(:) = FL(:) - 0.5*ncp(:)                                                              ! subtract the non-conservative product 
  !!!!!FL(:) = FL(:) + 0.5*ncp(:)
  !!
  dQ = QR - QL
  fL = (sR*fLtmp - sL*fRtmp)/((sR-sL)+1e-14) + MATMUL( absA, dQ ) 
  !!
  !!Dp = -MATMUL(Aroep,dQ)
  !!Dm = -MATMUL(Aroem,dQ)        ! these are the path integral of the MatrixB from QL to QR. (the NCP as a first approximation)
  !!
  !Dp = MATMUL(Aroep,dQ)
  !Dm = MATMUL(Aroem,dQ)        ! these are the path integral of the MatrixB from QL to QR. (the NCP as a first approximation)
  !!
  fR = fL !- Dp
  fL = fL !+ Dm
  !!  
  IF(ANY(ISNAN(fL))) THEN
        PRINT *,'hllemgrmhd fL is NaN!', QL, QR 
        pause 
        continue
  ENDIF
  IF(ANY(ISNAN(fR))) THEN
        PRINT *,'hllemgrmhd fR is NaN!', QL, QR
        pause  
        continue
  ENDIF

  !PRINT *, 'HLLEMFluxFVGRMHD DONE'
  ! REMEMBER THE FOLLOWING: we are recursively updating qh as
  ! q_i^{n+1} = q_i^n - FL              .... i.e. F_=F_{i+1/2}_ right flux
  ! q_{i+1}^{n+1} = q_i^n + FR             .... i.e. FR=F_{i+1/2} left flux
  ! see musclhancock.cpph after "// 4. Solve Riemann problems"
  ! 
    END SUBROUTINE HLLEMFluxFVGRMHD
 
    
#endif     


 



