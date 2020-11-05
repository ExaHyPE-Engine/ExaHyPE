SUBROUTINE METRIC ( xc, lapse, gp, gm, shift, Kex, g_cov, g_contr, phi )
  USE MainVariables, ONLY: ICType, aom, Mbh, P_eps,NSTOVVar,NSTOV_nODE
#if defined(RNSTOV) 
  USE NSTOV_mod
#endif     
#ifdef TWOPUNCTURES  
	USE TwoPunctures_mod, ONLY : TwoPunctures_Interpolate
#endif 
#ifdef RNSID 
    USE RNSID_mod, ONLY : RNSID_Interpolate
#endif
  IMPLICIT NONE 
  !
  REAL, DIMENSION(3), intent(IN) :: xc
  REAL                           :: lapse,gp,gm, phi 
  REAL,dimension(3)              :: shift
  REAL,dimension(3,3)            :: Kex, g_cov, g_contr 

  REAL :: x, y, z, r, z2, r2, aom2, det 
  REAL :: lx, ly, lz, HH, SS, detg, rho, r_hz, sig, theta, a, M    
  REAL :: st, st2, delta, rho2, sigma, zz, V0Z4(54),V070(70) 
  REAL :: transl(3), xGP(3),Mbh_loc,rbar
#if defined(RNSTOV) 
  REAL :: qloc(NSTOV_nODE) 
#endif    
  REAL, DIMENSION(3) :: xGP_loc,xGP_sph    
  !
  Kex = 0.0 
  phi = 1.0
  !
  SELECT CASE(ICType)
  CASE('CCZ4Puncture') 
    !   
    r = P_eps + SQRT( SUM(xc**2) ) 
    g_cov = 0.0
    g_cov(1,1) = 1.0 
    g_cov(2,2) = 1.0
    g_cov(3,3) = 1.0 
    g_cov = g_cov * (1.0+1./r)**4 
    g_contr = 0.0 
    g_contr(1,1) = 1./g_cov(1,1)  
    g_contr(2,2) = 1./g_cov(2,2)  
    g_contr(3,3) = 1./g_cov(3,3)  
    lapse = 1.0
    shift = 0.0 
    det = g_cov(1,1)*g_cov(2,2)*g_cov(3,3) 
    phi = det**(-1./6.) 
    ! 
  CASE('CCZ4TwoPunctures') 
    !
    transl = (/ -1.0, 0.0, 0.0 /) 
    xGP = xc - transl 
    !  
#ifdef TWOPUNCTURES  
        CALL TwoPunctures_Interpolate(xGP, V0Z4) 
#else
        PRINT *, ' TwoPunctures not available. Please compile with -DTWOPUNCTURES flag. '
#endif      
    !
    g_cov(1,1) = V0Z4(1) 
    g_cov(1,2) = V0Z4(2) 
    g_cov(2,1) = V0Z4(2) 
    g_cov(1,3) = V0Z4(3) 
    g_cov(3,1) = V0Z4(3) 
    g_cov(2,2) = V0Z4(4) 
    g_cov(2,3) = V0Z4(5) 
    g_cov(3,2) = V0Z4(5) 
    g_cov(3,3) = V0Z4(6) 
    !
    Kex(1,1) = V0Z4(7) 
    Kex(1,2) = V0Z4(8) 
    Kex(2,1) = V0Z4(8) 
    Kex(1,3) = V0Z4(9) 
    Kex(3,1) = V0Z4(9) 
    Kex(2,2) = V0Z4(10) 
    Kex(2,3) = V0Z4(11) 
    Kex(3,2) = V0Z4(11) 
    Kex(3,3) = V0Z4(12) 
    !
    CALL  MatrixInverse3x3(g_cov,g_contr,det)  
    lapse = V0Z4(17) 
    shift = V0Z4(18:20) 
    phi = det**(-1./6.) 
    ! 
  CASE('CCZ4GRMHDAccretion') 
    !
    ! Rotating black hole in Kerr-Schild Cartesian coordinates. See De Felice & Clarke Sect. 11.4
    x    = xc(1)
    y    = xc(2)
    z    = xc(3)
  
    z2   = z**2
    aom2 = aom**2
    !r    = MAX( 1e-1, SQRT( (x**2 + y**2 + z**2 - aom2)/2.0 + SQRT(((x**2 + y**2 + z**2 - aom2)/2.0)**2 + z2*aom2)) )  
    r    = SQRT( (x**2 + y**2 + z**2 - aom2)/2.0 + SQRT(((x**2 + y**2 + z**2 - aom2)/2.0)**2 + z2*aom2))
  
    r2   = r**2
  
    HH = Mbh*r2*r / (r2*r2 + aom2*z2)
    SS = 1.0 + 2.0*HH
    lx = (r*x + aom*y)/(r2 + aom2)  
    ly = (r*y - aom*x)/(r2 + aom2)
    lz = z/r  
  
    lapse   = 1.0/SQRT(SS)
    shift(1) = 2.0*HH/SS*lx         
    shift(2) = 2.0*HH/SS*ly         
    shift(3) = 2.0*HH/SS*lz        
     
    g_cov( 1, 1:3) = (/ 1.0 + 2.0*HH*lx**2, 2.0*HH*lx*ly,        2.0*HH*lx*lz       /)
    g_cov( 2, 1:3) = (/ 2.0*HH*lx*ly,       1.0 + 2.0*HH*ly**2,  2.0*HH*ly*lz       /)
    g_cov( 3, 1:3) = (/ 2.0*HH*lx*lz,       2.0*HH*ly*lz,        1.0 + 2.0*HH*lz**2 /)
  
    g_contr( 1, 1:3) = (/  1.0 + 2.0*HH*ly**2 + 2.0*HH*lz**2,   -2.0*HH*lx*ly,                       -2.0*HH*lx*lz                     /)
    g_contr( 2, 1:3) = (/ -2.0*HH*lx*ly,                         1.0 + 2.0*HH*lx**2 + 2.0*HH*lz**2,  -2.0*HH*ly*lz                     /)
    g_contr( 3, 1:3) = (/ -2.0*HH*lx*lz,                        -2.0*HH*ly*lz,                        1.0 + 2.0*HH*lx**2 + 2.0*HH*ly**2 /)
  
    g_contr = g_contr/SS
  
    gp = SQRT(SS)
    gm = 1.0/gp
    !
    phi = SS**(-1./6.) 
    !
  CASE('CCZ4RNSID','GRMHDRNSID')
    !
    transl = (/ 0.0, 0.0, 0.0 /) 
    xGP = xc - transl 
    !  
#ifdef RNSID   
        CALL RNSID_Interpolate(xGP, V070) 
#else
        PRINT *, ' RNSID not available. Please compile with -DRNSID flag. '
        STOP 
#endif      

    !
    g_cov(1,1) = V070(1) 
    g_cov(1,2) = V070(2) 
    g_cov(2,1) = V070(2) 
    g_cov(1,3) = V070(3) 
    g_cov(3,1) = V070(3) 
    g_cov(2,2) = V070(4) 
    g_cov(2,3) = V070(5) 
    g_cov(3,2) = V070(5) 
    g_cov(3,3) = V070(6) 
    !
    Kex(1,1) = V070(7) 
    Kex(1,2) = V070(8) 
    Kex(2,1) = V070(8) 
    Kex(1,3) = V070(9) 
    Kex(3,1) = V070(9) 
    Kex(2,2) = V070(10) 
    Kex(2,3) = V070(11) 
    Kex(3,2) = V070(11) 
    Kex(3,3) = V070(12) 
    !
    CALL  MatrixInverse3x3(g_cov,g_contr,det)  
    lapse = V070(17)
    !lapse = 1.0-0.1*EXP( -0.5*(xGP(1)**2+xGP(2)**2+xGP(3)**2)/5.0**2 ) 
    shift = V070(18:20) 
    phi = det**(-1./6.)       
    !  
  CASE('CCZ4Kerr2D')  
      !
      ! Rotating black hole in Kerr-Schild spherical coordinates. See Appendix B of Komissarov (2004) MNRAS, 350, 427
      !
      r  = xc(1)
      r2 = r*r
      !    
      st = SIN(xc(2))
      IF (st < 1.e-6) st = 1.
      st2 = st**2
  
      aom2 = aom**2
  
      delta = r2 - 2.0 * Mbh * r + aom2
      rho2  = r2 + aom2*(1.0 - st2)
      sigma = (r2 + aom2)**2 - aom2*delta*st2
      zz    = 2.0*r/rho2
  
      lapse    = 1.0 / sqrt(1.0 + zz)
      shift(1) = zz/(1.0 + zz)       
      shift(2) = 0.0         
      shift(3) = 0.0  
  
      g_cov( 1, 1:3) = (/ 1.0 + zz,             0.0,        -aom*st2*(1.0 + zz)   /)
      g_cov( 2, 1:3) = (/ 0.0,                  rho2,       0.0                   /)
      g_cov( 3, 1:3) = (/ -aom*st2*(1.0 + zz),  0.0,        (sigma/rho2)*st2      /)
      !
      det = (g_cov(1,1)*g_cov(2,2)*g_cov(3,3)-g_cov(1,1)*g_cov(2,3)*g_cov(3,2)-g_cov(2,1)*g_cov(1,2)*g_cov(3,3)+g_cov(2,1)*g_cov(1,3)*g_cov(3,2)+g_cov(3,1)*g_cov(1,2)*g_cov(2,3)-g_cov(3,1)*g_cov(1,3)*g_cov(2,2)) 
      phi = det**(-1./6.) 
      !
      g_contr( 1, 1:3) = (/  lapse**2 + aom2*st2/rho2,   0.0,                       aom/rho2         /)
      g_contr( 2, 1:3) = (/  0.0,                        1.0/rho2,                  0.0              /)
      g_contr( 3, 1:3) = (/  aom/rho2,                   0.0,                       1.0/(rho2*st2)   /)
  
      gp = rho2*st*sqrt(1.0 + zz)
      gm = 1.0/gp    
      !
  CASE('Z4Kerr2D')
      !
       r      = xc(1)   ! SQRT( xGP(1)**2 + xGP(2)**2 + xGP(3)**2)
       theta  = xc(2)   ! 
       a      = aom 
       M      = Mbh  
       r_hz   = Mbh + SQRT(Mbh**2 - aom**2) 
       !
       rho   = SQRT(r**2 + a**2*COS(theta)**2) 
       Delta = r**2 - 2*Mbh*r+a**2 
       Sig   = (r**2+a**2)**2 - a**2*Delta*SIN(theta)**2  
       zz    = 2.*M*r/rho**2
       !
       g_cov( 1, 1:3) = (/ 1.0+zz,                     0.0,     -a*SIN(theta)**2*(1.0+zz)   /) 
       g_cov( 2, 1:3) = (/ 0.0,                        rho**2,   0.0                        /) 
       g_cov( 3, 1:3) = (/ -a*SIN(theta)**2*(1.0+zz),  0.0,      sig*SIN(theta)**2/rho**2   /)       
       !
       CALL  MatrixInverse3x3(g_cov,g_contr,det)  
       !
       phi = det**(-1./6.)         
       !
       gp = SQRT(det)
       gm = 1.0/gp 
       !
       lapse = 1.0/SQRT(1+zz)                            ! alpha (lapse) 
       ! 
       shift(1) = zz/(1+zz)                              ! beta_1 (shift) 
       shift(2) = 0.0                                    ! beta_2 
       shift(3) = 0.0                                    ! beta_3        
       !
  CASE('GRMHDAccretion','GRMHDTorus','Z4Kerr3D','CCZ4Kerr3D')
      !
#ifndef SPHERICAL
  ! Rotating black hole in Kerr-Schild Cartesian coordinates. See De Felice & Clarke Sect. 11.4
  x    = xc(1)
  y    = xc(2)
  z    = xc(3)
  
  z2   = z**2
  aom2 = aom**2
  r    = SQRT( (x**2 + y**2 + z**2 - aom2)/2.0 + SQRT(((x**2 + y**2 + z**2 - aom2)/2.0)**2 + z2*aom2))
  
  r2   = r**2
  
  HH = Mbh*r2*r / (r2*r2 + aom2*z2)
  SS = 1.0 + 2.0*HH
  lx = (r*x + aom*y)/(r2 + aom2)  
  ly = (r*y - aom*x)/(r2 + aom2)
  lz = z/r  
  
  lapse   = 1.0/SQRT(SS)
  shift(1) = 2.0*HH/SS*lx         
  shift(2) = 2.0*HH/SS*ly         
  shift(3) = 2.0*HH/SS*lz        
     
  g_cov( 1, 1:3) = (/ 1.0 + 2.0*HH*lx**2, 2.0*HH*lx*ly,        2.0*HH*lx*lz       /)
  g_cov( 2, 1:3) = (/ 2.0*HH*lx*ly,       1.0 + 2.0*HH*ly**2,  2.0*HH*ly*lz       /)
  g_cov( 3, 1:3) = (/ 2.0*HH*lx*lz,       2.0*HH*ly*lz,        1.0 + 2.0*HH*lz**2 /)
  
  det = (g_cov(1,1)*g_cov(2,2)*g_cov(3,3)-g_cov(1,1)*g_cov(2,3)*g_cov(3,2)-g_cov(2,1)*g_cov(1,2)*g_cov(3,3)+g_cov(2,1)*g_cov(1,3)*g_cov(3,2)+g_cov(3,1)*g_cov(1,2)*g_cov(2,3)-g_cov(3,1)*g_cov(1,3)*g_cov(2,2)) 
  phi = det**(-1./6.) 

  g_contr( 1, 1:3) = (/  1.0 + 2.0*HH*ly**2 + 2.0*HH*lz**2,   -2.0*HH*lx*ly,                       -2.0*HH*lx*lz                     /)
  g_contr( 2, 1:3) = (/ -2.0*HH*lx*ly,                         1.0 + 2.0*HH*lx**2 + 2.0*HH*lz**2,  -2.0*HH*ly*lz                     /)
  g_contr( 3, 1:3) = (/ -2.0*HH*lx*lz,                        -2.0*HH*ly*lz,                        1.0 + 2.0*HH*lx**2 + 2.0*HH*ly**2 /)
  
  g_contr = g_contr/SS
  
  gp = SQRT(SS)
  gm = 1.0/gp
  ! 
#else
  ! Rotating black hole in Kerr-Schild spherical coordinates. See Appendix B of Komissarov (2004) MNRAS, 350, 427
  r  = xc(1)
  r2 = r*r
  
  st = SIN(xc(2))
  IF (st < 1.e-6) st = 1.
  st2 = st**2
  
  aom2 = aom**2
  
  delta = r2 - 2.0 * Mbh * r + aom2
  rho2  = r2 + aom2*(1.0 - st2)
  sigma = (r2 + aom2)**2 - aom2*delta*st2
  zz    = 2.0*r/rho2
  
  lapse    = 1.0 / sqrt(1.0 + zz)
  shift(1) = zz/(1.0 + zz)       
  shift(2) = 0.0         
  shift(3) = 0.0  
  
  g_cov( 1, 1:3) = (/ 1.0 + zz,             0.0,        -aom*st2*(1.0 + zz)   /)
  g_cov( 2, 1:3) = (/ 0.0,                  rho2,       0.0                   /)
  g_cov( 3, 1:3) = (/ -aom*st2*(1.0 + zz),  0.0,        (sigma/rho2)*st2      /)
  
  g_contr( 1, 1:3) = (/  lapse**2 + aom2*st2/rho2,   0.0,                       aom/rho2         /)
  g_contr( 2, 1:3) = (/  0.0,                        1.0/rho2,                  0.0              /)
  g_contr( 3, 1:3) = (/  aom/rho2,                   0.0,                       1.0/(rho2*st2)   /)
  
  gp = rho2*st*sqrt(1.0 + zz)
  gm = 1.0/gp  
  !
#endif         
    !
  CASE('GRMHDAlfven')
      !
      lapse    = 1.0  
      shift(1) = 0.0
      shift(2) = 0.0         
      shift(3) = 0.0  
      !
      g_cov = 0.
      g_contr = 0. 
      g_cov( 1, 1) = 1.0
      g_cov( 2, 2) = 1.0
      g_cov( 3, 3) = 1.0
      g_contr( 1, 1) = 1.0
      g_contr( 2, 2) = 1.0
      g_contr( 3, 3) = 1.0 
      !
      gp = 1.0
      gm = 1.0
      !
    CASE('GRMHDTOV','CCZ4TOV','GRMHDTOV_perturbed') 
#ifdef RNSTOV 
        CALL Curved2Cartesian(xGP_loc,xc)          ! no effects if you are in Cartesian coordiantes
        CALL Cartesian2Spherical(xGP_sph,xGP_loc)   ! here we convert Cartesian to Spherical, independently on the chosen coordiantes
        r=XGP_sph(1)
        !
#ifdef SPHERICAL  ! then we are in the original coordinate system        
        CALL NSTOV_x(r,qloc)
#elif CYLINDRICAL
        PRINT *, 'CYLINDRICAL COORDINATES NOT TESTED FOR RNSTOV'
#else  
        CALL NSTOV_rbar(r,qloc)
		!print *, r,NSTOVVar%qloc
		!pause
#endif
        !
        lapse = EXP(qloc(2))
        shift(1:3) = 0.
        !gammaij(1:6) = 0.
        !gammaij(1) = 1.0
        !gammaij(4) = 1.0
        !gammaij(6) = 1.0
        g_cov = 0.
#ifdef SPHERICAL
        g_cov(1,1)  = 1.0/(1.0-2.0*qloc(1)/r)
        g_cov(2,2)  = r**2
        g_cov(3,3)  = SIN(xGP_sph(2))**2*r**2
        !
#else   
        !rbar = r !*NSTOVVar%C*exp(NSTOVVar%q(4,n))
        !rbar = 0.5*r/NSTOVVar%radius*(SQRT(NSTOVVar%radius**2-2*NSTOVVar%q(1,NSTOVVar%iradius)*NSTOVVar%radius)+NSTOVVar%radius-NSTOVVar%q(1,NSTOVVar%iradius))*EXP(NSTOVVar%qloc(4)-NSTOVVar%q(4,NSTOVVar%iradius)) 
        g_cov(1,1) = 1.0/(NSTOVVar%C**2*exp(2.0*qloc(4))) ! rbar**2/(r+1e-14)**2
        g_cov(2,2) = 1.0/(NSTOVVar%C**2*exp(2.0*qloc(4))) ! rbar**2/(r+1e-14)**2
        g_cov(3,3) = 1.0/(NSTOVVar%C**2*exp(2.0*qloc(4))) ! rbar**2/(r+1e-14)**2
        !
#endif 
        g_contr=0.
        g_contr( 1, 1) = 1.0/g_cov(1,1)
        g_contr( 2, 2) = 1.0/g_cov(2,2)
        g_contr( 3, 3) = 1.0/g_cov(3,3)
        gp = SQRT(g_cov(1,1)*g_cov(2,2)*g_cov(3,3))
        gm = 1/gp

        det = g_cov(1,1)*g_cov(2,2)*g_cov(3,3) 
        phi = det**(-1./6.)        
        
#endif

   END SELECT 
   ! 
END SUBROUTINE METRIC


SUBROUTINE DMETRIC (xc, dalpha, BB, DD, dphi)

  IMPLICIT NONE 
  !
  REAL, DIMENSION(3),     intent(IN)  :: xc
  REAL, DIMENSION(3),     intent(OUT) :: dalpha
  REAL, DIMENSION(3,3),   intent(OUT) :: BB
  REAL, DIMENSION(3,3,3), intent(OUT) :: DD
  REAL, DIMENSION(3),     intent(OUT) :: dphi
  INTEGER                             :: i 
  REAL                                :: xp(3), xm(3), gp(3,3), gm(3,3), ap, am, betap(3), betam(3), tempp, tempm, temp(3,3)  
  REAL                                :: xp1(3), xm1(3), gp1(3,3), gm1(3,3), ap1, am1, betap1(3), betam1(3) 
  REAL                                :: xp2(3), xm2(3), gp2(3,3), gm2(3,3), ap2, am2, betap2(3), betam2(3) 
  REAL                                :: alpha, beta(3), g_cov(3,3), phip, phim, phip1, phip2, phim1, phim2, Kex(3,3)  
  REAL, PARAMETER                     :: epsilon = 1e-6, eps4 = 1e-4   
  !
  ! Metric derivative computed with a simple second order central finite difference 
  !
  DO i = 1, 3
    xp = xc
    xp(i) = xp(i)+epsilon 
    xm = xc
    xm(i) = xm(i)-epsilon 
    CALL METRIC ( xp, ap, tempp, tempm, betap, Kex, gp, temp, phip)
    CALL METRIC ( xm, am, tempp, tempm, betam, Kex, gm, temp, phim)
    dalpha(i) = (ap-am)/(2*epsilon) 
    BB(i,:)   = (betap(:)-betam(:))/(2*epsilon) 
    DD(i,:,:) = (gp(:,:)-gm(:,:))/(2*epsilon) 
    dphi(i)   = (phip - phim)/(2*epsilon) 
    CONTINUE 
  ENDDO
  !
  ! Metric derivative computed with a fourth order central finite difference 
  !
  !DO i = 1, 3
  !  xp1 = xc
  !  xp1(i) = xp1(i)+eps4 
  !  xm1 = xc
  !  xm1(i) = xm1(i)-eps4 
  !  xp2 = xc
  !  xp2(i) = xp2(i)+2*eps4 
  !  xm2 = xc
  !  xm2(i) = xm2(i)-2*eps4 
  !  CALL METRIC ( xp1, ap1, tempp, tempm, betap1, Kex, gp1, temp, phip1)
  !  CALL METRIC ( xm1, am1, tempp, tempm, betam1, Kex, gm1, temp, phim1)
  !  CALL METRIC ( xp2, ap2, tempp, tempm, betap2, Kex, gp2, temp, phip2)
  !  CALL METRIC ( xm2, am2, tempp, tempm, betam2, Kex, gm2, temp, phim2)
  !  dalpha(i) = ( 8.0*ap1      -8.0*am1       +am2       -ap2       )/(12.0*eps4) 
  !  BB(i,:)   = ( 8.0*betap1(:)-8.0*betam1(:) +betam2(:) -betap2(:) )/(12.0*eps4) 
  !  DD(i,:,:) = ( 8.0*gp1(:,:) -8.0*gm1(:,:)  +gm2(:,:)  -gp2(:,:)  )/(12.0*eps4) 
  !  dphi(i)   = ( 8.0*phip1    -8.0*phim1     +phim2     -phip2     )/(12.0*eps4) 
  !ENDDO
  !
 END SUBROUTINE DMETRIC  
       
 SUBROUTINE MatrixInverse3x3(M,iM,det) 
    !---------------
    ! compute the determinant det of the NxN-matrix M
    !---------------
    IMPLICIT NONE
    ! input variables 
    REAL, INTENT(IN)   :: M(3,3)
    ! output variables
    REAL, INTENT(OUT)    :: iM(3,3)
    REAL, INTENT(OUT)    :: det
    ! output variables
    REAL    :: ComputeDet,Id(3,3)
    INTEGER :: i,j
    ! 
    det = M(1,1)*M(2,2)*M(3,3)-M(1,1)*M(2,3)*M(3,2)-M(2,1)*M(1,2)*M(3,3)+M(2,1)*M(1,3)*M(3,2)+M(3,1)*M(1,2)*M(2,3)-M(3,1)*M(1,3)*M(2,2) !ComputeDet(M,3)
    !
    iM(1,1) =M(2,2)*M(3,3)-M(2,3)*M(3,2)
    iM(1,2) =M(1,3)*M(3,2)-M(1,2)*M(3,3)
    iM(1,3) =M(1,2)*M(2,3)-M(1,3)*M(2,2)
    iM(2,1) =M(2,3)*M(3,1)-M(2,1)*M(3,3)
    iM(2,2) =M(1,1)*M(3,3)-M(1,3)*M(3,1)
    iM(2,3) =M(1,3)*M(2,1)-M(1,1)*M(2,3)
    iM(3,1) =M(2,1)*M(3,2)-M(2,2)*M(3,1)
    iM(3,2) =M(1,2)*M(3,1)-M(1,1)*M(3,2)
    iM(3,3) =M(1,1)*M(2,2)-M(1,2)*M(2,1)
    IF(det*det.LT.1e-20) THEN
        print *, 'FATAL ERROR: det = 0'
        STOP
    ENDIF
    !
    iM = iM/det
    !
    !Id = MATMUL(M,iM)
    !DO i=1,3
    !    DO j=1,3
    !        IF(i.eq.j) THEN
    !            IF((Id(i,j)-1.)**2..GT.1e-18) THEN
    !                print *, 'FATAL ERROR: iM*M !=1'
    !                STOP
    !            ENDIF
    !        ELSE
    !            IF((Id(i,j)**2).GT.1e-18) THEN
    !                print *, 'FATAL ERROR: iM*M !=1'
    !                STOP
    !            ENDIF
    !        ENDIF
    !    ENDDO
    !ENDDO
    !
    CONTINUE
    !
    END SUBROUTINE MatrixInverse3x3
    
    
    
      

SUBROUTINE METRIC_KSS( xc, lapse, gp, gm, shift, Kex, g_cov, g_contr, phi )
#if defined(GRMHD) || defined(GRGPR) 
  USE MainVariables, ONLY: aom, Mbh, P_eps
#endif  
  IMPLICIT NONE 
  !
  REAL, DIMENSION(3), intent(IN) :: xc
  REAL                           :: lapse,gp,gm, phi 
  REAL,dimension(3)              :: shift
  REAL,dimension(3,3)            :: Kex, g_cov, g_contr 

  REAL :: x, y, z, r, z2, r2, aom2, det 
  REAL :: lx, ly, lz, HH, SS, detg, rho, r_hz, sig, theta, a, M    
  REAL :: st, st2, delta, rho2, sigma, zz, V0Z4(54),V070(70) 
  REAL :: transl(3), xGP(3) 
#ifndef GRMHD  
#ifndef GRGPR  
  REAL, PARAMETER :: aom = 0.9, Mbh = 1.0, P_eps = 1e-4    
#endif  
#endif  
  !
  Kex = 0.0 
  phi = 1.0
  !
  ! Rotating black hole in Kerr-Schild spherical coordinates. See Appendix B of Komissarov (2004) MNRAS, 350, 427
  r  = xc(1)
  r2 = r*r
  
  st = SIN(xc(2))
  IF (st < 1.e-6) st = 1.
  st2 = st**2
  
  aom2 = aom**2
  
  delta = r2 - 2.0 * Mbh * r + aom2
  rho2  = r2 + aom2*(1.0 - st2)
  sigma = (r2 + aom2)**2 - aom2*delta*st2
  zz    = 2.0*r/rho2
  
  lapse    = 1.0 / sqrt(1.0 + zz)
  shift(1) = zz/(1.0 + zz)       
  shift(2) = 0.0         
  shift(3) = 0.0  
  
  g_cov( 1, 1:3) = (/ 1.0 + zz,             0.0,        -aom*st2*(1.0 + zz)   /)
  g_cov( 2, 1:3) = (/ 0.0,                  rho2,       0.0                   /)
  g_cov( 3, 1:3) = (/ -aom*st2*(1.0 + zz),  0.0,        (sigma/rho2)*st2      /)
  
  g_contr( 1, 1:3) = (/  lapse**2 + aom2*st2/rho2,   0.0,                       aom/rho2         /)
  g_contr( 2, 1:3) = (/  0.0,                        1.0/rho2,                  0.0              /)
  g_contr( 3, 1:3) = (/  aom/rho2,                   0.0,                       1.0/(rho2*st2)   /)
  
  gp = rho2*st*sqrt(1.0 + zz)
  gm = 1.0/gp  
  !
END SUBROUTINE METRIC_KSS


    
    
SUBROUTINE Cart2Sph_cov(B_Cart,B_sph,x) 
    USE MainVariables, ONLY: aom 
    IMPLICIT NONE
    !input variables
    REAL :: B_Cart(3),B_sph(3),x(3) 
    !local var.
    REAL :: A(3,3),theta,sintheta,costheta,cosphip,sinphip,phi,r,r2,ir2,raom,atanAR,datanAR 
    !!
    !! x=r*cos(theta)*cos(phi)
    !! y=r*cos(theta)*sin(phi)
    !! z=r*sin(theta) 
    !!
    !A(1,1:d) = (/DCOS(r(2))*DCOS(r(3)), - r(1)*DSIN(r(2))*DCOS(r(3)), - r(1)*DCOS(r(2))*DSIN(r(3))          /)
    !A(2,1:d) = (/DCOS(r(2))*DSIN(r(3)), - r(1)*DSIN(r(2))*DSIN(r(3)),   r(1)*DCOS(r(2))*DCOS(r(3))          /)
    !A(3,1:d) = (/DSIN(r(2))           ,   r(1)*DCOS(r(2))           ,   0.0                                 /)
    !
    IF(aom.eq.0.) THEN
        !
        ! x=r*sin(theta)*cos(phi)
        ! y=r*sin(theta)*sin(phi)
        ! z=r*cos(theta) 
        r = x(1)
        theta = x(2)
        phi = x(3)
        !
        sintheta = DSIN(theta)
        costheta = DCOS(theta)
        cosphip = DCOS(phi)
        sinphip = DSIN(phi)
        ! 
        ! wrong. we need the transpose of.
        !A(1,1:d) = (/sintheta*cosphip, + r*costheta*cosphip, - r*sintheta*sinphip          /)
        !A(2,1:d) = (/sintheta*sinphip, + r*costheta*sinphip,   r*sintheta*cosphip          /)
        !A(3,1:d) = (/costheta        , - r*sintheta        ,   0.0                                 /)
        !
        ! 
        A(1:3,1) = (/sintheta*cosphip, + r*costheta*cosphip, - r*sintheta*sinphip          /)
        A(1:3,2) = (/sintheta*sinphip, + r*costheta*sinphip,   r*sintheta*cosphip          /)
        A(1:3,3) = (/costheta        , - r*sintheta        ,   0.0                                 /)
        !
    ELSE
        PRINT *,'Please, check correctness of Cart2Sph_cov with aom'
        STOP
        ! ! Kerr-Schild Cartesian
        ! x=SQRT(r**2+aom**2)*sin(theta)*cos(phi-ATAN2(aom,r))
        ! y=SQRT(r**2+aom**2)*sin(theta)*SIN(phi-ATAN2(aom,r))
        ! z=r*COS(theta)
        !
        ! ! from Kerr-Schild spherical
        r = x(1)
        theta = x(2)
        phi = x(3)
        !
        raom = SQRT(r**2+aom**2)
        atanAR = ATAN2(aom,r)
        datanAR = 1.0/(1.+atanAR**2)
        r2 = r**2
        ir2 = 1./r2
        sintheta = DSIN(theta)
        costheta = DCOS(theta)
        cosphip = DCOS(phi-atanAR)
        sinphip = DSIN(phi-atanAR)
    
        ! wrong. we need the transpose of.
        !A(1,1:d) = (/    0.5/raom*2.0*r*sintheta*cosphip - raom*sintheta*sinphip*(-datanAR)*(-aom*ir2) , +raom*costheta*cosphip , -raom*sintheta*sinphip          /)
        !A(2,1:d) = (/    0.5/raom*2.0*r*sintheta*sinphip + raom*sintheta*cosphip*(-datanAR)*(-aom*ir2) , +raom*costheta*sinphip ,  raom*sintheta*cosphip           /)
        !A(3,1:d) = (/    costheta          ,  - r*sintheta         ,   0.0                                 /)
        !
        A(1:3,1) = (/    0.5/raom*2.0*r*sintheta*cosphip - raom*sintheta*sinphip*(-datanAR)*(-aom*ir2) , +raom*costheta*cosphip , -raom*sintheta*sinphip          /)
        A(1:3,2) = (/    0.5/raom*2.0*r*sintheta*sinphip + raom*sintheta*cosphip*(-datanAR)*(-aom*ir2) , +raom*costheta*sinphip ,  raom*sintheta*cosphip           /)
        A(1:3,3) = (/    costheta          ,  - r*sintheta         ,   0.0                                 /)
        !                                                       ,   raom*sintheta*cosphip                                                          ,   0.0                                 /)
    
    
    
    ENDIF
    !
    B_sph = MATMUL(A,B_Cart)
    !
    continue
    !
    END SUBROUTINE Cart2Sph_cov
    
        
SUBROUTINE Cart2SphMatrix_cov(A,iA,x)
    USE MainVariables, ONLY : d,aom,EQN,CoordTol
    IMPLICIT NONE
    !input variables
    REAL :: A(d,d),iA(d,d),x(d) 
    !local var.
    INTEGER :: i,j
    LOGICAL :: singular
    REAL :: id(3,3),det,x_cart(3),x_curv(3),iA2(3,3)
    REAL :: theta,sintheta,costheta,cosphip,sinphip,phi,r,r2,ir2,raom,atanAR,datanAR,phiscale,thetascale,dtheta,dphi
    !
    ! x=r*sin(theta)*cos(phi)
    ! x=r*sin(theta)*sin(phi)
    ! x=r*cos(theta) 
    !
    IF(aom.eq.0.) THEN
        !
        ! x=r*sin(theta)*cos(phi)
        ! y=r*sin(theta)*sin(phi)
        ! z=r*cos(theta) 
        r = x(1)
        theta = x(2)
        phi = x(3)
        IF(ABS(theta).LT.CoordTol) THEN
            theta = SIGN(MAX(ABS(theta),CoordTol),theta)
        ENDIF
        IF(r.LT.CoordTol) THEN
            r = MAX(r,CoordTol)
        ENDIF
        !IF(ABS(phi).LT.1e-9) THEN
        !    phi = SIGN(MAX(ABS(phi),1e-9),phi)
        !ENDIF
        !IF(ABS(theta-EQN%PI).LT.1e-9) THEN
        !    theta = theta + SIGN(MAX(ABS(theta-EQN%PI),1e-9),theta-EQN%PI)
        !ENDIF
        !IF(ABS(phi-EQN%PI).LT.1e-9) THEN
        !    phi = phi + SIGN(MAX(ABS(phi-EQN%PI),1e-9),phi-EQN%PI)
        !ENDIF
        !IF(ABS(theta-0.5*EQN%PI).LT.1e-9) THEN
        !    theta = theta + SIGN(MAX(ABS(theta-0.5*EQN%PI),1e-9),theta-0.5*EQN%PI)
        !ENDIF
        !IF(ABS(phi-0.5*EQN%PI).LT.1e-9) THEN
        !    phi = phi + SIGN(MAX(ABS(phi-0.5*EQN%PI),1e-9),phi-0.5*EQN%PI)
        !ENDIF
        !!
        !dtheta = SIGN(MAX(ABS(theta),1e-9),theta)
        !dphi = SIGN(MAX(ABS(phi),1e-9),phi)
        !
        sintheta = DSIN(theta)
        costheta = DCOS(theta)
        cosphip = DCOS(phi)
        sinphip = DSIN(phi)
        !  
#ifdef SPH2         
        ! x=r*sin(4*theta)*cos(2*phi)
        ! y=r*sin(4*theta)*sin(2*phi)
        ! z=r*cos(4*theta) 
        A(1:3,1) = (/sintheta*cosphip, + r*4.0*costheta*cosphip, - r*sintheta*2.0*sinphip          /)
        A(1:3,2) = (/sintheta*sinphip, + r*4.0*costheta*sinphip,   r*sintheta*2.0*cosphip          /)
        A(1:3,3) = (/costheta        , - r*4.0*sintheta        ,   0.0                                 /)
#else
        A(1:3,1) = (/sintheta*cosphip, + r*costheta*cosphip, - r*sintheta*sinphip          /)
        A(1:3,2) = (/sintheta*sinphip, + r*costheta*sinphip,   r*sintheta*cosphip          /)
        A(1:3,3) = (/costheta        , - r*sintheta        ,   0.0                                 /)
#endif
        !
    ELSE
        PRINT *,'Please, check correctness of the Cart2SphMatrix_cov jacobian'
        STOP
        ! ! Kerr-Schild Cartesian
        ! x=SQRT(r**2+aom**2)*sin(theta)*cos(phi-ATAN2(aom,r))
        ! y=SQRT(r**2+aom**2)*sin(theta)*SIN(phi-ATAN2(aom,r))
        ! z=r*COS(theta)
        !
        ! ! from Kerr-Schild spherical
        r = x(1)
        theta = x(2)
        phi = x(3)
        !
        raom = SQRT(r**2+aom**2)
        atanAR = ATAN2(aom,r)
        datanAR = 1.0/(1.+atanAR**2)
        r2 = r**2
        ir2 = 1./r2
        sintheta = DSIN(theta)
        costheta = DCOS(theta)
        cosphip = DCOS(phi-atanAR)
        sinphip = DSIN(phi-atanAR)
    
        ! wrong. we need the transpose of.
        !A(1,1:d) = (/    0.5/raom*2.0*r*sintheta*cosphip - raom*sintheta*sinphip*(-datanAR)*(-aom*ir2) , +raom*costheta*cosphip , -raom*sintheta*sinphip          /)
        !A(2,1:d) = (/    0.5/raom*2.0*r*sintheta*sinphip + raom*sintheta*cosphip*(-datanAR)*(-aom*ir2) , +raom*costheta*sinphip ,  raom*sintheta*cosphip           /)
        !A(3,1:d) = (/    costheta          ,  - r*sintheta         ,   0.0                                 /)
        !
        A(1:3,1) = (/    0.5/raom*2.0*r*sintheta*cosphip - raom*sintheta*sinphip*(-datanAR)*(-aom*ir2) , +raom*costheta*cosphip , -raom*sintheta*sinphip          /)
        A(1:3,2) = (/    0.5/raom*2.0*r*sintheta*sinphip + raom*sintheta*cosphip*(-datanAR)*(-aom*ir2) , +raom*costheta*sinphip ,  raom*sintheta*cosphip           /)
        A(1:3,3) = (/    costheta          ,  - r*sintheta         ,   0.0                                 /)
        !
    
    ENDIF
    !
    det = A(1,1)*A(2,2)*A(3,3)-A(1,1)*A(2,3)*A(3,2)-A(2,1)*A(1,2)*A(3,3)+A(2,1)*A(1,3)*A(3,2)+A(3,1)*A(1,2)*A(2,3)-A(3,1)*A(1,3)*A(2,2) !ComputeDet(M,3)
    !
    iA(1,1) =A(2,2)*A(3,3)-A(2,3)*A(3,2)
    iA(1,2) =A(1,3)*A(3,2)-A(1,2)*A(3,3)
    iA(1,3) =A(1,2)*A(2,3)-A(1,3)*A(2,2)
    iA(2,1) =A(2,3)*A(3,1)-A(2,1)*A(3,3)
    iA(2,2) =A(1,1)*A(3,3)-A(1,3)*A(3,1)
    iA(2,3) =A(1,3)*A(2,1)-A(1,1)*A(2,3)
    iA(3,1) =A(2,1)*A(3,2)-A(2,2)*A(3,1)
    iA(3,2) =A(1,2)*A(3,1)-A(1,1)*A(3,2)
    iA(3,3) =A(1,1)*A(2,2)-A(1,2)*A(2,1)
    iA(1:3,1:3) = iA(1:3,1:3)/det
    !
    Id = MATMUL(A(1:3,1:3),iA(1:3,1:3))
    singular = .FALSE.
    IF(ANY(ISNAN(Id))) THEN
        singular = .TRUE.
    ELSE
        DO i=1,3
            DO j=1,3
                IF(i.eq.j) THEN
                    IF((Id(i,j)-1.)**2..GT.1e-14) THEN
                        !print *, 'FATAL ERROR: iM*M !=1'
                        !continue
                        !STOP
                        ! the matrix is singular
                        singular = .TRUE.
                    ENDIF
                ELSE
                    IF((Id(i,j)**2).GT.1e-14) THEN
                        !print *, 'FATAL ERROR: iM*M !=1'
                        !continue
                        !STOP
                        singular = .TRUE.
                    ENDIF
                ENDIF
            ENDDO
        ENDDO
    ENDIF
    !
    !
    IF(singular) THEN
        !A = 0.
        !iA2 = 0.0
        !
        CALL Curved2Cartesian(X_cart,X)
#ifdef SPHERICAL
        CALL Sph2CartMatrix_cov(iA,X_cart)
#elif CYLINDRICAL
        CALL Cyl2CartMatrix_cov(iA,X_cart)
#else
        PRINT *, 'IMPOSSIBLE ERROR in Cart2SphMatrix_cov: singular Jacobian in Cartesian coordinates?'
        STOP
#endif  
        
    Id = MATMUL(A(1:3,1:3),iA(1:3,1:3))
    singular = .FALSE.
    !iA = iA2
    DO i=1,3
        DO j=1,3
            IF(i.eq.j) THEN
                IF((Id(i,j)-1.)**2..GT.1e-18) THEN
                    !print *, 'FATAL ERROR: iM*M !=1'
                    !continue
                    !STOP
                    ! the matrix is singular
                    singular = .TRUE.
                    !
                    continue
                    !
                ENDIF
            ELSE
                IF((Id(i,j)**2).GT.1e-18) THEN
                    !print *, 'FATAL ERROR: iM*M !=1'
                    !continue
                    !STOP
                    singular = .TRUE.
                    !
                    continue
                    !
                ENDIF
            ENDIF
        ENDDO
    ENDDO
    !
    ENDIF
    !
    continue
    !
    END SUBROUTINE Cart2SphMatrix_cov
    
    



  
    
SUBROUTINE Sph2CartMatrix_cov(A,X_cart)
    USE MainVariables, ONLY : d,aom,EQN,CoordTol
    IMPLICIT NONE 
    REAL, INTENT(IN) :: X_cart(3)
    REAL, INTENT(OUT) :: A(3,3)
    REAL :: theta,phi,r,ir,z,y,iy,x,tanphi,y_x,z_yb,z_y,z_x,ix,iyb,aom_r,iz,y_z,x_z,isinphi,icosphi,disinphi,dicosphi,datan
    REAL :: cosphi,sinphi,dphi_dx,dphi_dy,dphi_dz,dtheta_dx,dtheta_dy,dtheta_dz,dr_dx,dr_dy,dr_dz,raom,phiscale,thetascale
    REAL, PARAMETER :: eps = 1e-80
    REAL :: X_curv_tmp(3)
        ! x=r*cos(phi)
        ! y=r*sin(phi)
        ! z=z'
        !
        ! x=r*sin(theta)*cos(phi)
        ! y=r*sin(theta)*sin(phi)
        ! z=r*cos(theta) 
    !
    x = X_cart(1)
    y = X_cart(2)
    z = X_cart(3)
    !
    ix = 1.0/x    ! x/(x**2+eps)
    iy = 1.0/y    !iy = y/(y**2+eps)
    iz = 1.0/z    ! z/(z**2+eps)
    y_x= y*ix
    z_y= z*iy 
    z_x= z*ix 
    !
    y_z= y*iz 
    x_z= x*iz
    ! 
    !
    r = SQRT(SUM(X_cart**2))
    ir = 1/r    !r/(r**2+eps)
    !
    IF(aom.eq.0) THEN
        phi = ATAN2(X_cart(2),X_cart(1))
        sinphi = DSIN(phi)
        cosphi = DCOS(phi)
#ifdef SPH2
        phiscale=0.5
        thetascale=0.25
        !
        !phi = phiscale*ATAN2(X_cart(2),X_cart(1)) 
        !
        dphi_dx = phiscale*1.0/(1.0+y_x**2)*(-1.0)*y_x*ix
        dphi_dy = phiscale*1.0/(1.0+y_x**2)*ix
        dphi_dz = 0.0 
        !
        A(1:3,3) = (/ dphi_dx ,  dphi_dy ,  dphi_dz     /)
        !
        IF(ABS(cosphi).LT.0.5) THEN
            !theta = thetascale*ATAN2( X_cart(2)/sinphi,X_cart(3))
            !
            isinphi=1.0/sinphi
            disinphi = -1.0/(sinphi**2)*cosphi
            datan=1.0/(1.0+(y_z*isinphi)**2)
            dtheta_dx = thetascale*datan*y_z*disinphi*dphi_dx
            dtheta_dy = thetascale*datan*(iz*isinphi + y_z*disinphi*dphi_dy)      
            dtheta_dz = thetascale*datan*y_z*isinphi*(-1.0)*iz  + 0.0  
            !
        ELSE
            ! theta = thetascale*ATAN2(X_cart(1)/cosphi,X_cart(3))
            !
            icosphi=1.0/cosphi
            dicosphi = 1.0/(cosphi**2)*sinphi
            datan=1.0/(1.0+(x_z*icosphi)**2)
            dtheta_dx = thetascale*datan*(iz*icosphi  +  x_z*dicosphi*dphi_dx)
            dtheta_dy = thetascale*datan*x_z*dicosphi*dphi_dy      
            dtheta_dz = thetascale*datan*x_z*icosphi*(-1.0)*iz  + 0.0   
            !
        ENDIF
        !
        A(1:3,2) = (/  dtheta_dx , dtheta_dy  ,   dtheta_dz          /)
        !
        !r=SQRT(SUM(X_cart**2))
        !
        dr_dx = 0.5*ir*2.0*x
        dr_dy = 0.5*ir*2.0*y
        dr_dz = 0.5*ir*2.0*z
        A(1:3,1) = (/dr_dx , dr_dy  , dr_dz /) 
        ! 
#else 
        !phi = ATAN2(X_cart(2),X_cart(1)) 
        !
        dphi_dx = 1.0/(1.0+y_x**2)*(-1.0)*y_x*ix
        dphi_dy = 1.0/(1.0+y_x**2)*ix   ! qui
        dphi_dz = 0.0 
        !
        A(1:3,3) = (/ dphi_dx, dphi_dy, dphi_dz /)
        !
        IF(ABS(cosphi).LT.0.5) THEN
            !theta = thetascale*ATAN2( X_cart(2)/sinphi,X_cart(3))
            !
            isinphi=1.0/sinphi
            disinphi = -1.0/(sinphi**2)*cosphi
            datan=1.0/(1.0+(y_z*isinphi)**2)
            dtheta_dx = datan*y_z*disinphi*dphi_dx
            dtheta_dy = datan*(iz*isinphi + y_z*disinphi*dphi_dy)      
            dtheta_dz = datan*y_z*isinphi*(-1.0)*iz  + 0.0  
            !
        ELSE
            ! theta = thetascale*ATAN2(X_cart(1)/cosphi,X_cart(3))
            !
            icosphi=1.0/cosphi
            dicosphi = 1.0/(cosphi**2)*sinphi
            datan=1.0/(1.0+(x_z*icosphi)**2)
            dtheta_dx = datan*(iz*icosphi  +  x_z*dicosphi*dphi_dx)   ! qui 
            dtheta_dy = datan*x_z*dicosphi*dphi_dy      
            dtheta_dz = datan*x_z*icosphi*(-1.0)*iz  + 0.0      ! qui 
            !
        ENDIF
        !
        A(1:3,2) = (/  dtheta_dx , dtheta_dy  ,   dtheta_dz          /)
        !
        !r=SQRT(SUM(X_cart**2))
        !
        dr_dx = 0.5*ir*2.0*x
        dr_dy = 0.5*ir*2.0*y
        dr_dz = 0.5*ir*2.0*z
        A(1:3,1) = (/dr_dx , dr_dy  , dr_dz /) 
        ! 
#endif
    ELSE
        PRINT *,'Please, check correctness of the Sph2CartMatrix_cov jacobian'
        STOP
        !
        !phi = ATAN2(X_cart(2),X_cart(1))
        !sinphi = DSIN(phi)
        !cosphi = DCOS(phi)
        !!
        !!r=X_curv(1)
        !!X_cart(1)=SQRT(r**2+aom**2)*Dsin(X_curv(2))*Dcos(X_curv(3)-ATAN2(aom,r))
        !!X_cart(2)=SQRT(r**2+aom**2)*Dsin(X_curv(2))*DSIN(X_curv(3)-ATAN2(aom,r))
        !!X_cart(3)=r*DCOS(X_curv(2))
        !! 
        !raom = SQRT(r**2+aom**2)
        !X_cart_tmp(3) = X_cart(3)/r
        !X_cart_tmp(1) = X_cart(1)/raom
        !X_cart_tmp(2) = X_cart(2)/raom
        !!
        !!phi = ATAN2(X_cart(2),X_cart(1)) 
        !!
        !IF(ABS(cosphi).LT.0.5) THEN
        !    theta = ATAN2(X_cart(3)/r,X_cart(2)/raom/sinphi)
        !ELSE
        !    theta = ATAN2(X_cart(3)/r,X_cart(1)/raom/cosphi)
        !ENDIF
        !!
        !phi = ATAN2(X_cart(2),X_cart(1))+ATAN2(aom,r)
        !! 
        !!phi = ATAN2(X_cart_tmp(2),X_cart_tmp(1))+ATAN2(aom,r)
        !!phi2 = ATAN2(X_cart_tmp(2),X_cart_tmp(1))
        !!
        !dphi_dx = 1.0/(1.0+y_x**2)*(-1.0)*y_x*ix +  1.0/(1.0+aom_r**2)*(-1.0)*aom_r*ir*dr_dx
        !dphi_dy = 1.0/(1.0+y_x**2)*ix
        !dphi_dz = 0.0 
        !!
        !A(1:3,3) = (/ dphi_dx ,  dphi_dy ,  dphi_dz     /)
        !!
        !IF(ABS(cosphi).LT.0.5) THEN
        !!theta = ATAN2(X_cart_tmp(3),X_cart_tmp(2)/DSIN(phi2))
        !!
        !dtheta_dx = 1.0/(1.0+z_y**2*sinphi**2)*z_y*cosphi*dphi_dx
        !dtheta_dy = 1.0/(1.0+z_y**2*sinphi**2)*(z_y*sinphi*(-1.0)*iy + z_y*cosphi*dphi_dy)      
        !dtheta_dz = 1.0/(1.0+z_y**2*sinphi**2)*iy*sinphi  + 0.0  
        !!
        !ELSE
        !!theta = ATAN2(X_cart_tmp(3),X_cart_tmp(1)/DCOS(phi2))
        !!
        !dtheta_dx = 1.0/(1.0+z_x**2*cosphi**2)*(z_x*cosphi*(-1.0)*ix  +  z_x*(-sinphi)*dphi_dx)
        !dtheta_dy = 1.0/(1.0+z_x**2*cosphi**2)*z_x*(-sinphi)*dphi_dy      
        !dtheta_dz = 1.0/(1.0+z_x**2*cosphi**2)*ix*cosphi   
        !!
        !ENDIF
        !!
        !A(1:3,2) = (/  dtheta_dx , dtheta_dy  ,   dtheta_dz          /)
        !!
        !!r=SQRT(SUM(X_cart**2))
        !!
        !dr_dx = 0.5*ir*2.0*x
        !dr_dy = 0.5*ir*2.0*y
        !dr_dz = 0.5*ir*2.0*z
        !A(1:3,1) = (/dr_dx , dr_dy  , dr_dz /) 
        !! 
    ENDIF
    !
    !
    continue
    !
END SUBROUTINE Sph2CartMatrix_cov
    
    
      
SUBROUTINE Cyl2CartMatrix_cov(A,X_cart)
    USE MainVariables, ONLY : d,aom,EQN,CoordTol
    IMPLICIT NONE 
    REAL, INTENT(IN) :: X_cart(3)
    REAL, INTENT(OUT) :: A(3,3)
    REAL :: theta,phi,r,ir,z,y,iy,x,tanphi,y_x,z_yb,z_y,z_x,ix,iyb,aom_r
    REAL :: cosphi,sinphi,dphi_dx,dphi_dy,dphi_dz,dtheta_dx,dtheta_dy,dtheta_dz,dr_dx,dr_dy,dr_dz
    REAL, PARAMETER :: eps = 1e-80
    !
    x = X_cart(1)
    y = X_cart(2)
    z = X_cart(3)
    !
    ix = x/(x**2+eps)
    y_x= y*ix
    iy = y/(y**2+eps)
    z_y= z*iy 
    z_x= z*ix
    !
    r = SQRT(SUM(X_cart(1:2)**2))
    ir = r/(r**2+eps)
    !
    ! x=r*cos(phi)
    ! y=r*sin(phi)
    ! z=theta
    !
    phi = ATAN2(X_cart(2),X_cart(1))
    sinphi = DSIN(phi)
    cosphi = DCOS(phi)
    !
    !phi = ATAN2(X_cart(2),X_cart(1)) 
    !
    dphi_dx = 1.0/(1.0+y_x**2)*(-1.0)*y_x*ix
    dphi_dy = 1.0/(1.0+y_x**2)*ix
    dphi_dz = 0.0 
    !
    A(1:3,3) = (/ dphi_dx, dphi_dy, dphi_dz /)
    !
    !theta = z
    !
    dtheta_dx = 0.0
    dtheta_dy = 0.0     
    dtheta_dz = 1.0 
    !
    A(1:3,2) = (/ dtheta_dx, dtheta_dy, dtheta_dz /)
    !
    !r=SQRT(x**2+y**2)
    !
    dr_dx = 0.5*ir*2.0*x
    dr_dy = 0.5*ir*2.0*y
    dr_dz = 0.0
    A(1:3,1) = (/dr_dx , dr_dy  , dr_dz /) 
    !
    continue
    !
END SUBROUTINE Cyl2CartMatrix_cov
    
   




SUBROUTINE Curved2Cartesian(X_cart,X_curv)
    USE MainVariables, ONLY : aom,CoordTol
    IMPLICIT NONE
    REAL, INTENT(IN) :: X_curv(3)
    REAL, INTENT(OUT) :: X_cart(3)
    REAL :: theta,phi,r,z
    REAL :: X_curv_tmp(3)
        ! x=r*cos(phi)
        ! y=r*sin(phi)
        ! z=z'
        !
        ! x=r*sin(theta)*cos(phi)
        ! y=r*sin(theta)*sin(phi)
        ! z=r*cos(theta) 
    
        X_curv_tmp = X_curv
        !
#ifdef SPHERICAL
        IF(X_curv_tmp(1).LT.Coordtol) THEN
            continue
        ENDIF
        !
        X_curv_tmp(1)= MAX(CoordTol, X_curv_tmp(1))     ! avoid negative radii
        !X_curv_tmp(2)= SIGN(CoordTol,X_curv_tmp(2))
        if(abs(x_curv_tmp(2)).lt.coordtol) then
            !x_curv_tmp(2)= 1e-9sign(x_curv_tmp(2))
        X_curv_tmp(2)= SIGN(CoordTol,X_curv_tmp(2))
        endif
        !
        IF(aom.eq.0) THEN
#ifdef SPH2
             X_cart(1)=DSIN(4.0*X_curv_tmp(2))*DCOS(2.0*X_curv_tmp(3))
             X_cart(2)=DSIN(4.0*X_curv_tmp(2))*DSIN(2.0*X_curv_tmp(3))
             X_cart(3)=DCOS(4.0*X_curv_tmp(2)) 
             X_cart(1:3)=X_curv_tmp(1)*X_cart(1:3)
#else
             X_cart(1)=DSIN(X_curv_tmp(2))*DCOS(X_curv_tmp(3))
             X_cart(2)=DSIN(X_curv_tmp(2))*DSIN(X_curv_tmp(3))
             X_cart(3)=DCOS(X_curv_tmp(2)) 
             X_cart(1:3)=X_curv_tmp(1)*X_cart(1:3)
#endif
        ELSE 
             !
             PRINT *,'Please, check correctness of the Cartesian2Curved jacobian'
             STOP
             !
             r=X_curv_tmp(1)
             X_cart(1)=SQRT(r**2+aom**2)*Dsin(X_curv_tmp(2))*Dcos(X_curv_tmp(3)-ATAN2(aom,r))
             X_cart(2)=SQRT(r**2+aom**2)*Dsin(X_curv_tmp(2))*DSIN(X_curv_tmp(3)-ATAN2(aom,r))
             X_cart(3)=r*DCOS(X_curv_tmp(2))
        ENDIF
        !
#elif CYLINDRICAL
        X_curv_tmp(1)= MAX(CoordTol, X_curv_tmp(1))     ! avoid negative radii 
        ! x=r*cos(phi)
        ! y=r*sin(phi)
        ! z=z'
        X_cart(1)=X_curv_tmp(1)*Dcos(X_curv_tmp(3))
        X_cart(2)=X_curv_tmp(1)*Dsin(X_curv_tmp(3))
        X_cart(3)=X_curv_tmp(2) 
        !
#elif ! Cartesian
        !
        X_cart=X_curv_tmp
#endif
    !
    !
    continue
    !
    END SUBROUTINE Curved2Cartesian
    
    
       
SUBROUTINE Cartesian2Curved(X_curv,X_cart)
    USE MainVariables, ONLY : aom,CoordTol
    IMPLICIT NONE
    REAL, INTENT(OUT) :: X_curv(3)
    REAL, INTENT(IN) :: X_cart(3)
    REAL :: theta,phi,r,z,y,x,tanphi
    REAL :: sinphi,cosphi
        ! x=r*cos(phi)
        ! y=r*sin(phi)
        ! z=z'
        !
        ! x=r*sin(theta)*cos(phi)
        ! y=r*sin(theta)*sin(phi)
        ! z=r*cos(theta) 
#ifdef SPHERICAL
        !
        IF(aom.eq.0) THEN
#ifdef SPH2
             !
             phi = ATAN2(X_cart(2),X_cart(1))
             sinphi=DSIN(phi)
             cosphi=DCOS(phi)
             !
             IF(ABS(cosphi).LT.0.5) THEN
                 theta = ATAN2(X_cart(2)/sinphi,X_cart(3))
             ELSE
                 theta = ATAN2(X_cart(1)/cosphi,X_cart(3))
             ENDIF
             !
             phi = 0.5*phi
             ! 
             theta = 0.25*theta
             r=SQRT(SUM(X_cart**2))
             !
             X_curv(1) = r
             X_curv(2) = theta
             X_curv(3) = phi
             ! 
#else
             !
             phi = ATAN2(X_cart(2),X_cart(1))
             sinphi=DSIN(phi)
             cosphi=DCOS(phi)
             ! 
             IF(ABS(cosphi).LT.0.5) THEN
                 theta = ATAN2(X_cart(2)/sinphi,X_cart(3))
             ELSE
                 theta = ATAN2(X_cart(1)/cosphi,X_cart(3))
             ENDIF
             !
             r=SQRT(SUM(X_cart**2))
             !
             X_curv(1) = r
             X_curv(2) = theta
             X_curv(3) = phi
             ! 
#endif
        ELSE 
             !
             PRINT *,'Please, check correctness of the Cartesian2Curved jacobian'
             STOP
             !
             !r=SQRT(SUM(X_cart**2))
             !X_cart_tmp(3) = X_cart(3)/r
             !X_cart_tmp(1) = X_cart(1)/SQRT(r**2+aom**2)
             !X_cart_tmp(2) = X_cart(2)/SQRT(r**2+aom**2)
             !!
             !phi = ATAN2(X_cart_tmp(2),X_cart_tmp(1)) 
             !sinphi=DSIN(phi)
             !cosphi=DCOS(phi)
             !!
             !IF(ABS(cosphi).LT.0.5) THEN
             !    theta = ATAN2(X_cart_tmp(3),X_cart_tmp(2)/sinphi)
             !ELSE
             !    theta = ATAN2(X_cart_tmp(3),X_cart_tmp(1)/cosphi)
             !ENDIF
             !!
             !phi = phi+ATAN2(aom,r) 
             !!
             !X_curv(1) = r
             !X_curv(2) = theta
             !X_curv(3) = phi
             ! 
        ENDIF
        !
#elif CYLINDRICAL
        ! x=r*cos(phi)
        ! y=r*sin(phi)
        ! z=z'
             !
             phi = ATAN2(X_cart(2),X_cart(1))
             !
             r=SQRT(SUM(X_cart**2))
             !
             X_curv(1) = r
             X_curv(2) = phi
             X_curv(3) = X_cart(3)
             ! 
        !
#elif ! Cartesian
        !
        X_curv=X_cart
#endif
    !
    continue
    !
END SUBROUTINE Cartesian2Curved
 


SUBROUTINE Cartesian2Spherical(X_curv,X_cart)
    USE MainVariables, ONLY : aom,CoordTol
    IMPLICIT NONE
    REAL, INTENT(OUT) :: X_curv(3)
    REAL, INTENT(IN) :: X_cart(3)
    REAL :: theta,phi,r,z,y,x,tanphi
    REAL :: sinphi,cosphi
        ! x=r*cos(phi)
        ! y=r*sin(phi)
        ! z=z'
        !
        ! x=r*sin(theta)*cos(phi)
        ! y=r*sin(theta)*sin(phi)
        ! z=r*cos(theta)  
        ! 
        phi = ATAN2(X_cart(2),X_cart(1))
        sinphi=DSIN(phi)
        cosphi=DCOS(phi)
        ! 
        IF(ABS(cosphi).LT.0.5) THEN
            theta = ATAN2(X_cart(2)/sinphi,X_cart(3))
        ELSE
            theta = ATAN2(X_cart(1)/cosphi,X_cart(3))
        ENDIF
        !
        r=SQRT(SUM(X_cart**2))
        !
        IF(ABS(theta).LT.CoordTol) THEN
        theta = SIGN(MAX(ABS(theta),CoordTol),theta)
        ENDIF
        IF(ABS(r).LT.CoordTol) THEN
        r = MAX(r,CoordTol)
        ENDIF
        !
        X_curv(1) = r
        X_curv(2) = theta
        X_curv(3) = phi
        !   
    continue
    !
END SUBROUTINE Cartesian2Spherical
 



 