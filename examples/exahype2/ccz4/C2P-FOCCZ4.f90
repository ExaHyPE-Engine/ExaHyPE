#ifndef HEADER_CPFOCCZ4
#define HEADER_CPFOCCZ4

! The Con2Prim and Prim2Con routines for MHD.
! Should be merged with SRHD's.

 ! #include "MainVariables.f90"
 ! #include "SpecificVarEqn99.f90"
 ! #include "expintegrator_ode.f90"
 ! #include "expintegrator_type.f90"


RECURSIVE SUBROUTINE PDEPrim2Cons(Q,V)
  USE MainVariables, ONLY: nVar, nDim, EQN
  IMPLICIT NONE
  ! Argument list declaration
  REAL :: Q(nVar), V(nVar)
  INTENT(IN)  :: V
  INTENT(OUT) :: Q 
    ! Argument list 
    ! Local variables 
    REAL                  :: rho, vx, vy, vz, p, bx, by, bz, ex, ey, ez, v2, b2, e2    
    REAL                  :: lf, gamma1, w, ww, uem 
    REAL                  :: gp, gm, g_cov(3,3), g_contr(3,3), bv(3), vf_cov(3), psi, lapse, shift(3)
    REAL                  :: vf(3), bv_contr(3), qv_contr(3), qb_contr(3), vxb(3), vb_cov(3), b2_cov, vxb_contr(3) 
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVarGRMHD = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVarGRMHD = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVarGRMHD = 19                           ! The number of variables of the PDE system 
#endif
    REAL                  :: QGRMHD(nVarGRMHD), VGRMHD(nVarGRMHD)
#ifdef VECTOR    
      INTEGER, PARAMETER :: nVarGRGPR = 32                           ! The number of variables of the PDE system 
#else
      INTEGER, PARAMETER :: nVarGRGPR = 30                           ! The number of variables of the PDE system 
#endif
    REAL :: QGRGPR(nVarGRGPR), VGRGPR(nVarGRGPR)
    !
    REAL                  :: A(3,3), G(3,3), Id(3,3), devG(3,3), eh, evv, T, falpha, Temp(3,3)  
    !   

#if defined(CCZ4EINSTEIN) || defined(CCZ4GRMHD) || defined(CCZ4GRHD) || defined(CCZ4GRGPR) 
    Q = V   
    Q(17) = LOG(V(17))
    Q(55) = LOG(V(55)) 
    !
#ifdef CCZ4GRMHD
    VGRMHD(1:9)   = V(60:68)    ! hydro variables 
    VGRMHD(10)    = V(17)       ! lapse 
    VGRMHD(11:13) = V(18:20)    ! shift 
    VGRMHD(14:19) = V(1:6)/( V(55) )**2   ! metric 
    CALL PDEPrim2ConsGRMHD(QGRMHD,VGRMHD) 
    Q(60:68) = QGRMHD(1:9) 
#endif 
#ifdef CCZ4GRHD
    VGRMHD(1:5)   = V(60:64)    ! hydro variables 
    VGRMHD(6:9)   = 0.0
    VGRMHD(10)    = V(17)       ! lapse 
    VGRMHD(11:13) = V(18:20)    ! shift 
    VGRMHD(14:19) = V(1:6)/( V(55) )**2   ! metric 
    CALL PDEPrim2ConsGRMHD(QGRMHD,VGRMHD) 
    Q(60:64) = QGRMHD(1:5) 
#endif 
#ifdef CCZ4GRGPR
    VGRGPR(1:14)   = V(60:73)    ! hydro variables 
    VGRGPR(25:30)   = V(74:79)    ! hydro variables 
    VGRGPR(15)    = V(17)       ! lapse 
    VGRGPR(16:18) = V(18:20)    ! shift 
    VGRGPR(19:24) = V(1:6)/( V(55) )**2   ! metric 
    CALL PDEPrim2ConsGRGPR(QGRGPR,VGRGPR) 
    Q(60:73) = QGRGPR(1:14) 
    Q(74:79) = QGRGPR(25:30) 
#endif 
#endif

#if defined(BSSZ4EINSTEIN) || defined(BSSZ4GRMHD) 
    Q = V   
    Q(17) = LOG(V(17))
    Q(55) = LOG(V(55)) 
    !
#ifdef BSSZ4GRMHD
    VGRMHD(1:9)   = V(63:71)    ! hydro variables 
    VGRMHD(10)    = EXP(V(17))  ! lapse 
    VGRMHD(11:13) = V(18:20)    ! shift 
    VGRMHD(14:19) = V(1:6)/( EXP(V(55)) )**2   ! metric 
    CALL PDEPrim2ConsGRMHD(QGRMHD,VGRMHD) 
    Q(63:71) = QGRMHD(1:9) 
#endif 
#endif

    !
#ifdef SRMHD
    rho    = V(1)
    vx     = V(2)
    vy     = V(3)
    vz     = V(4)
    p      = V(5)
    bx     = V(6)
    by     = V(7)
    bz     = V(8)
    !
    ex     = - (vy*bz - vz*by)
    ey     = - (vz*bx - vx*bz)
    ez     = - (vx*by - vy*bx)
    !
    v2     = vx**2 + vy**2 + vz**2
    b2     = bx**2 + by**2 + bz**2
    e2     = ex**2 + ey**2 + ez**2
    !
    IF (v2 > 1.0) THEN
        WRITE(*,*)'Superluminal velocity in PDEPrim2Cons!!'
        STOP
    ENDIF
    lf     = 1.0 / sqrt(1.0 - v2)
    gamma1 = EQN%gamma/(EQN%gamma-1.0)
    w      = rho + gamma1*p
    ww     = w*lf**2
    uem    = 0.5*(b2+e2)
    !
    Q(1)   = rho*lf
    Q(2)   = ww*vx + (ey*bz - ez*by)
    Q(3)   = ww*vy + (ez*bx - ex*bz)
    Q(4)   = ww*vz + (ex*by - ey*bx)
    Q(5)   = ww - p + uem 
    !
    Q(6)   = bx
    Q(7)   = by
    Q(8)   = bz
    Q(9)   = V(9)  
#endif 
    !
#ifdef GRMHD
  !
  CALL PDEPrim2ConsGRMHD(Q,V) 
  RETURN 
  !
#endif
    !
#ifdef GRGPR
  !
  CALL PDEPrim2ConsGRGPR(Q,V) 
  RETURN 
  !
#endif
    !
#ifdef GPR3D
     
    ! Peshkov-Romenski model in 3D with heat conduction 
    Q(1)   = V(1)        ! rho 
    Q(2:4) = V(1)*V(2:4) ! rho*u, rho*v, rho*w  
    Q(6:14) = V(6:14)    ! A11, A12, A13, A21, A22, A23, A31, A32, A33 
    eh     = (V(5)+EQN%gamma*EQN%p0)/(EQN%gamma-1)/V(1) 
    A(1,:) = (/ V(6),  V(7),  V(8)  /) 
    A(2,:) = (/ V(9),  V(10), V(11) /)
    A(3,:) = (/ V(12), V(13), V(14) /)         
    G      = MATMUL( TRANSPOSE(A), A ) 
    Id     = 0. 
    Id(1,1) = 1.0; Id(2,2) = 1.0; Id(3,3) = 1.0 
    devG   = G - (G(1,1)+G(2,2)+G(3,3))/3.*Id  
    temp   = MATMUL( TRANSPOSE(devG), devG ) 
    evv     = EQN%cs**2/4.*(temp(1,1)+temp(2,2)+temp(3,3)) 
    ! Compute the temperature from the ideal gas law 
    p = V(5) 
    T = p/V(1)/EQN%cv/(EQN%gamma-1)  
    falpha  = EQN%alpha**2  
    
    Q(5)     = V(1)*(eh+evv) + 0.5*V(1)*(V(2)**2+V(3)**2+V(4)**2) + 0.5*V(1)*falpha*(V(15)**2 + V(16)**2 + V(17)**2) ! total energy rhoE     
    Q(15:17) = V(15:17)*V(1) ! rho j 

#endif 
 
END SUBROUTINE PDEPrim2Cons

RECURSIVE SUBROUTINE PDECons2Prim(V,Q)
  USE MainVariables, ONLY: nVar, nDim, EQN

  IMPLICIT NONE
  !--------------------------------------------!
  ! Argument list declaration
  REAL :: Q(nVar), V(nVar)
  INTENT(IN)  :: Q 
  INTENT(OUT) :: V 
    ! Argument list 

    INTEGER:: iErr        ! error flag 
    ! Local variables 
    REAL                 :: p 
    REAL                 :: gamma1, gam, sb, dr, eps, sb2, sx, sy, sz, e, bx, by, bz, s2, b2 
    REAL                 :: x1, x2, x3, v2
    REAL                 :: w, rho, vx, vy, vz, den, vb 
    LOGICAL              :: FAILED
    REAL, PARAMETER      :: tol = 1e-8, third=1.0/3.0 !, p_floor = 1.0e-5, rho_floor = 1.0e-4    
    REAL                 :: RTSAFE_C2P_RMHD1 
    REAL                 :: lapse, shift(3), psi, gammaij(6), g_cov(3,3), g_contr(3,3), gp, gm, dd 
    REAL                 :: Qloc(nVar), bv(3), sm_cov(3), sm(3), bv_contr(3), vf(3), vf_cov(3)   
    REAL                 :: A(3,3), G(3,3), devG(3,3), Id(3,3), tempp(3,3), evv, ehh  
#ifdef VECTOR    
#ifdef AVX512 
  INTEGER, PARAMETER :: nVarGRMHD = 24                           ! The number of variables of the PDE system 
#else   
  INTEGER, PARAMETER :: nVarGRMHD = 20                           ! The number of variables of the PDE system 
#endif 
#else
  INTEGER, PARAMETER :: nVarGRMHD = 19                           ! The number of variables of the PDE system 
#endif
    REAL                 :: QGRMHD(nVarGRMHD), VGRMHD(nVarGRMHD)  
#ifdef VECTOR    
      INTEGER, PARAMETER :: nVarGRGPR = 32                           ! The number of variables of the PDE system 
#else
      INTEGER, PARAMETER :: nVarGRGPR = 30                           ! The number of variables of the PDE system 
#endif
    REAL :: QGRGPR(nVarGRGPR), VGRGPR(nVarGRGPR)
    !
    !    
    iErr = 0     
    !
#ifdef EULER         
    p = (EQN%gamma-1)*( Q(5) - 0.5*SUM(Q(2:4)**2)/Q(1) )    ! fluid pressure 
    V(1) = Q(1)             ! fluid density 
    V(2:4) = Q(2:4)/Q(1)    ! fluid velocity 
    V(5)   = p              ! fluid pressure  
#ifdef NUMENTR
    V(6) = Q(6)
#endif
#ifdef BURGER
     V(1:6) = Q(1:6)
#endif
#endif 
    !
#ifdef MAXWELL
    V(:) = Q(:) 
#endif 
    !
#ifdef HSM
    V(1)   = Q(1)             ! water depth
    V(2:5) = Q(2:5)/Q(1)      ! fluid velocity and aux. variables
    V(6)   = Q(6)             ! still water depth
#endif
    !
#ifdef HGNB
    V(1)   = Q(1)             ! water depth
    V(2:7) = Q(2:7)/Q(1)      ! fluid velocity, pressure, aux. variables 
    V(8)   = Q(8)             ! bottom 
#endif
    !
#ifdef ECHGNB
    V(1) = Q(1)               ! water depth
    V(2:7) = Q(2:7)/Q(1)      ! fluid velocity, sigma, p, pb
    V(8) = Q(8)               ! bottom
#endif
    !
#ifdef HSCHROEDINGER 
    V = Q 
    V(1) = Q(1)
    V(2) = Q(2)/Q(1)     
    V(3) = Q(3)/Q(1)     
    V(4) = Q(4)/Q(1)     
    V(5) = Q(5)/Q(1)     
    V(6) = Q(6)/Q(1) 
#endif
    !
#ifdef THETAMODEL  
    V = Q 
    V(1) = Q(1)                 ! h 
    V(2) = Q(2)/Q(4)            ! u
    V(3) = Q(3)/Q(4)            ! v
    V(4) = Q(4)/Q(1)            ! theta 
    V(5) = Q(5)                 ! b 
#endif
    !
#ifdef ELASTICITY
    V = Q 
#endif 
    !
#ifdef ACOUSTIC 
    V = Q 
#endif 
    !
#if defined(Z4EINSTEIN) || defined(Z4GRMHD) 
    V = Q 
    !
#ifdef Z4GRMHD
    QGRMHD(1:9)   = Q(55:63)    ! hydro variables 
    QGRMHD(10)    = Q(17)       ! lapse 
    QGRMHD(11:13) = Q(18:20)    ! shift 
    QGRMHD(14:19) = Q(1:6)      ! metric 
    CALL PDECons2PrimGRMHD(VGRMHD,QGRMHD,iErr) 
    V(55:63) = VGRMHD(1:9) 
#endif     
#endif
    !
#if defined(CCZ4EINSTEIN) || defined(CCZ4GRMHD) || defined(CCZ4GRHD) || defined(CCZ4GRGPR) 
    V(:) = Q(:) 
    V(17) = EXP(MAX(-20.,MIN(20.,Q(17))))  
    V(55) = EXP(MAX(-20.,MIN(20.,Q(55))))  
    !
#ifdef CCZ4GRMHD
    QGRMHD(1:9)   = Q(60:68)    ! hydro variables 
    QGRMHD(10)    = V(17)       ! lapse 
    QGRMHD(11:13) = Q(18:20)    ! shift 
    QGRMHD(14:19) = Q(1:6)/( V(55) )**2 ! metric 
    CALL PDECons2PrimGRMHD(VGRMHD,QGRMHD,iErr) 
    V(60:68) = VGRMHD(1:9) 
#endif    
#ifdef CCZ4GRHD
    QGRMHD(1:5)   = Q(60:64)    ! hydro variables 
    QGRMHD(6:9)   = 0.0 
    QGRMHD(10)    = V(17)       ! lapse 
    QGRMHD(11:13) = Q(18:20)    ! shift 
    QGRMHD(14:19) = Q(1:6)/( V(55) )**2 ! metric 
    CALL PDECons2PrimGRMHD(VGRMHD,QGRMHD,iErr) 
    V(60:64) = VGRMHD(1:5) 
#endif 
#ifdef CCZ4GRGPR
    QGRGPR(1:14)   = Q(60:73)    ! hydro variables 
    QGRGPR(25:30)   = Q(74:79)    ! hydro variables 
    QGRGPR(15)    = V(17)       ! lapse 
    QGRGPR(16:18) = Q(18:20)    ! shift 
    QGRGPR(19:24) = Q(1:6)/( V(55) )**2 ! metric 
    CALL PDECons2PrimGRGPR(VGRGPR,QGRGPR,iErr) 
    V(60:73) = VGRGPR(1:14) 
    V(74:79) = VGRGPR(25:30) 
#endif     
#endif
    !
#if defined(BSSZ4EINSTEIN) || defined(BSSZ4GRMHD) 
    V = Q
    V(17) = EXP(Q(17))
    V(55) = EXP(Q(55)) 
    !
#ifdef BSSZ4GRMHD
    QGRMHD(1:9)   = Q(63:71)    ! hydro variables 
    QGRMHD(10)    = EXP(Q(17))  ! lapse 
    QGRMHD(11:13) = Q(18:20)    ! shift 
    QGRMHD(14:19) = Q(1:6)/( EXP(Q(55)) )**2 ! metric 
    CALL PDECons2PrimGRMHD(VGRMHD,QGRMHD,iErr) 
    V(63:71) = VGRMHD(1:9) 
#endif     
#endif

    !
#ifdef SRMHD
  gamma1 = EQN%gamma/(EQN%gamma - 1.0)
  gam    = 1.0/gamma1
  dr   = Q(1)
  sx   = Q(2)
  sy   = Q(3)
  sz   = Q(4)
  e    = Q(5) 
  bx   = Q(6)
  by   = Q(7)
  bz   = Q(8)
  !
  s2   = sx*sx + sy*sy + sz*sz
  b2   = bx*bx + by*by + bz*bz
  sb   = sx*bx + sy*by + sz*bz
  sb2  = sb**2
  eps  = 1.e-10
  !
  x1   = 0.
  x2   = 1.-eps
  v2   = RTSAFE_C2P_RMHD1(x1,x2,tol,gam,dr,e,s2,b2,sb2,w,FAILED)
  !
  IF (FAILED) THEN
     iErr = -1
     p    = p_floor
     rho  = rho_floor
     vx   = 0.0
     vy   = 0.0
     vz   = 0.0
     bx   = bx
     by   = by
     bz   = bz
  ELSE
     den  = 1.0/(w+b2)
     vb   = sb/w
     !
     rho  = dr*sqrt(1.-v2)
     vx   = (sx + vb*bx)*den
     vy   = (sy + vb*by)*den
     vz   = (sz + vb*bz)*den
     p    = max(1.e-15, gam*(w*(1.-v2)-rho))
  ENDIF

  V(1:9) = (/ rho, vx, vy, vz, p, bx, by, bz, Q(9) /)
#endif 
    !
#ifdef GRMHD
  !
  CALL PDECons2PrimGRMHD(V,Q,iErr) 
  RETURN 
  !  
#endif
    !
#ifdef GRGPR
  !
  CALL PDECons2PrimGRGPR(V,Q,iErr) 
  RETURN 
  !  
#endif
    !
#ifdef GPR3D
    
    ! Peshkov-Romenski model with heat conduction 
    V(1)   = Q(1)        ! rho 
    V(2:4) = Q(2:4)/Q(1) ! u, v 
    V(6:14) = Q(6:14)      ! A11, A12, A13, A21, A22, A23, A31, A32, A33 
    V(15:17) = Q(15:17)/Q(1) ! j1, j2, j3  
    e      = Q(5)/Q(1) - 0.5*(V(2)**2+V(3)**2+V(4)**2) -0.5*(V(15)**2+V(16)**2+V(17)**2)*EQN%alpha**2      ! e = eh + ev 
    A(1,:) = (/ V( 6), V( 7),  V( 8) /) 
    A(2,:) = (/ V( 9), V(10),  V(11) /)
    A(3,:) = (/ V(12), V(13),  V(14) /)         
    G      = MATMUL( TRANSPOSE(A), A ) 
    Id     = 0. 
    Id(1,1) = 1.0; Id(2,2) = 1.0; Id(3,3) = 1.0 
    devG   = G - (G(1,1)+G(2,2)+G(3,3))/3.*Id  
    tempp  = MATMUL( TRANSPOSE(devG), devG ) 
    evv    = EQN%cs**2/4.*(tempp(1,1)+tempp(2,2)+tempp(3,3))         
    ehh    = e-evv  
    V(5)   = ehh*(EQN%gamma-1.0)*V(1) - EQN%gamma*EQN%p0 

#endif 
END SUBROUTINE PDECons2Prim

#endif

