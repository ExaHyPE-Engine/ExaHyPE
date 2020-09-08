! The Con2Prim and Prim2Con routines for MHD.
! Should be merged with SRHD's.

SUBROUTINE PDEPrim2Cons(Q,V)

  USE Parameters, ONLY: gamma, nVar, nDim
  USE, INTRINSIC :: ISO_C_BINDING
  IMPLICIT NONE
  ! Argument list declaration
  REAL(kind=c_double) :: Q(nVar), V(nVar)
  INTENT(IN)  :: V
  INTENT(OUT) :: Q 
  ! Local variable declaration
  REAL(kind=c_double) :: Prim(nVar), Buf(nVar)
  REAL :: rho,vx,vy,vz,p,bx,by,bz,ex,ey,ez,cs,c0
  REAL :: v2,b2,e2,lf,w,ww,uem,gamma1
  REAL :: lapse, gp, gm, dcs, dc0, eel 
  REAL :: g_contr(3,3), g_cov(3,3)
  REAl :: shift(3), vf(3), vf_cov(3), Ev(3), Bv(3), ExB(3)
  REAL :: A(3,3), devG(3,3), G(3,3), temp(3,3), Id(3,3), detA, eh, S, evv, T, falpha  
  REAL :: alphas, alphal, rhos, rhol, us, vs, ul, vl, es, el
  REAL :: EE(3),BB(3),vv(3),detvEB, vxE(3), vxB(3)  
  INTEGER :: i

  !
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
  gamma1 = gamma/(gamma-1.0)
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

END SUBROUTINE PDEPrim2Cons


SUBROUTINE PDECons2Prim(V,Q,iErr)

  USE Parameters, ONLY: gamma, nVar, nDim
  IMPLICIT NONE
  !--------------------------------------------!
  ! Argument list declaration
  REAL :: Q(nVar), V(nVar)
  INTEGER :: iErr
  INTENT(IN)  :: Q 
  INTENT(OUT) :: V 
  ! Local variable declaration
  REAL        :: iRho
  REAL, PARAMETER :: epsilon = 1e-14
  INTEGER     :: i, iter, indx_nr(2)
  REAL        :: dr, sx, sy, sz, bx, by, bz
  REAL        :: v2, sb, den, vb, zeta, cs    
  REAL        :: gamma1, G1, G12, x1, x2, eps
  REAL        :: rho, vx, vy, vz, p
  REAL        :: lf, lf2, lf3, lf4, gam,e,s2,b2,e2,sb2,w,ww
  REAL        :: k(3), B(3), vel(3)
  REAL        :: p1,q1,dd,phi,temp1, H, k2, kB, T2
  REAL        :: epsilon0, mu0, f, df, drho
  REAL        :: iPi,B28P, dcs, dc0
  REAL        :: dv2, c0, dw
  REAL        :: ri,qi,kappa,z,kappa_max
  REAL        :: ZBRENT_C2P_RHD2, ZBRENT_LF_POLY
  REAL        :: lapse, gp, gm
  REAL        :: g_contr(3,3), g_cov(3,3)
  REAL        :: shift(3), sm(3), sm_cov(3), vf_cov(3), vf(3), Ev(3), Bv(3)
  REAL        :: Qtest(14), dQ(14)
  REAL        :: ExB(3),ExB_up(3), U2e(3), S_up(3)  
  REAL        :: LFsolutions(4), cLF(5), C2, C3 
  COMPLEX     :: LFsolutionsC(4)
  REAL        :: xi(2), alpha_nr(2,2), beta_nr(2), d_nr, S  
  REAL        :: A(3,3), devG(3,3), G(3,3), tempp(3,3), Id(3,3), detA, ehh, evv
  REAL        :: EE(3), vv(3), BB(3), eel, detvEB, vxE(3), vxB(3)  
  REAL        :: vs,vl,us,ul,rhos,rhol,rhoeh,rhoe,alphas,alphal
  REAL        :: xx(9), temp(3) 
  LOGICAL     :: FAILED
  REAL, PARAMETER    :: tol = 1e-8, third=1.0/3.0, p_floor = 1.0e-5, rho_floor = 1.0e-4
  !
  iErr = 0
  !
  ! First option [Del Zanna et al. (2007) A&A, 473, 11-30 (method 3)]
  FAILED = .FALSE.
  gamma1 = gamma/(gamma - 1.0)
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
  Call RTSAFE_C2P_RMHD1(v2,x1,x2,tol,gam,dr,e,s2,b2,sb2,w,FAILED)
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
  !
  ! Second Option [Del Zanna et al. (2003) A&A, 400, 397-413]
  !               [Dumbser et al. (2008) JCP, 227, 8209-8253] 
  !FAILED = .FALSE.
  !g1   = EQN%gamma/(EQN%gamma-1.)
  !epsilon0 = 1.
  !mu0      = 1.
  !!
  !D    = Q(1)
  !k(:) = Q(2:4)
  !E    = Q(5)
  !B(:) = Q(6:8)
  !B2   = B(1)**2 + B(2)**2 + B(3)**2
  !k2   = k(1)**2 + k(2)**2 + k(3)**2
  !kB   = k(1)*B(1) + k(2)*B(2) + k(3)*B(3)
  !!
  !! Initial guess
  !v2   = 0.
  !rho  = 0.
  !p    = 0.
  !vel  = 0.
  !!
  !eps  = 1.e-10
  !!
  !x1   = 0.
  !x2   = 1.-eps
  !v2   = RTSAFE_C2P_RMHD2(x1, x2, tol, g1, D, k2, B2, kB, E, H, FAILED)
  !!
  !IF (FAILED) THEN
  !  iErr = -1
  !  p    = p_floor
  !  rho  = rho_floor
  !  vel  = 0.0
  !ELSE
  !  rho = D*SQRT((1.-v2))
  !  p   = ((1.-v2)*H-rho)/g1
  !  vel = 1./(H+B2)*(k(:)+kB*B(:)/H)
  !ENDIF  
  !!
  !V(1:9) = (/ rho, vel(1), vel(2), vel(3), p, B(1), B(2), B(3), Q(9) /)
  !
  ! Third option [Del Zanna et al. (2007) A&A, 473, 11-30 (method 1)]
  !
  !FAILED = .FALSE.
  !gamma1 = gamma/(gamma - 1.0)
  !gam    = 1.0/gamma1
  !d    = Q(1)
  !sx   = Q(2)
  !sy   = Q(3)
  !sz   = Q(4)
  !e    = Q(5) 
  !bx   = Q(6)
  !by   = Q(7)
  !bz   = Q(8)
  !!
  !! Initial guess (this may be delicate)
  !rho = 0.8
  !vx  = 0.01
  !vy  = 0.01
  !vz  = 0.01
  !p   = 0.8
  !!
  !v2   = vx*vx + vy*vy + vz*vz
  !s2   = sx*sx + sy*sy + sz*sz
  !b2   = bx*bx + by*by + bz*bz
  !sb   = sx*bx + sy*by + sz*bz
  !sb2  = sb**2
  !!
  !w  = (rho + gamma1*p)/(1.0 - v2)
  !xi = (/ v2, w /)
  !eps=1.e-6
  !!
  !DO iter=1,100
  !
  !  call FUNC_C2P_RMHD3(xi,gam,sb2,b2,s2,d,e,eps,alpha_nr,beta_nr)
  !  call ludcmp(alpha_nr,2,2,indx_nr,d_nr)
  !  call lubksb(alpha_nr,2,2,indx_nr,beta_nr)
  !
  !  if (sum(abs(beta_nr/xi))<tol) exit
  !  xi = xi + beta_nr
  !
  !ENDDO
  !
  !v2=max(0.,min(xi(1),1.-eps))
  !w =max(1.e-15,xi(2))
  !
  !den=1./(w+b2)
  !vb=sb/w
  !
  !rho=max(d*sqrt(1.-v2),1.e-15)
  !vx=(sx+vb*bx)*den
  !vy=(sy+vb*by)*den
  !vz=(sz+vb*bz)*den
  !p=max(gam*(w*(1.-v2)-rho),1.e-15)
  !
  !V(1:9) = (/ rho, vx, vy, vz, p, bx, by, bz, Q(9) /)
  !
END SUBROUTINE PDECons2Prim
