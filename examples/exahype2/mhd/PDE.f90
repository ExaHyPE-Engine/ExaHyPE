!!!
!!! A Special Relativistic ideal Magnetohydrodynamics ExaHyPE Kernel.
!!! Based on files provided V(7) Olindo, adopted V(7) Sven.
!!! Based on srhd3dfortran/PDE.f90.
!!!

! To avoid needless copying, these preprocessor rules allow
! neat naming conventions

SUBROUTINE PDEEigenvalues(Lambda,Q,nv) 
  USE Parameters, ONLY : nVar, nDim , gamma
  USE, INTRINSIC :: ISO_C_BINDING 
  IMPLICIT NONE 
  ! Argument list  
  REAL, INTENT(IN)  :: Q(nVar), nv(nDim)  
  REAL, INTENT(OUT) :: Lambda(nVar)  
  ! Local variables  
  INTEGER :: iErr
  REAL :: cs2, cs, c0, v2, w, gamma1, vn, den, u, c
  REAL :: ex, ey, ez, b2, e2, a2, ca2, vf1, vf2, vel(nDim)
  REAL :: V(nVar)
  
  ! MUST be 1 as Divergence Cleaning factor is 1.
  ! they travel with speed 1
  Lambda = 1
  RETURN

END SUBROUTINE PDEEigenvalues
 


SUBROUTINE PDEFlux(F,Q,normal) 
  USE Parameters, ONLY : nVar, nDim, gamma, DivCleaning_a
  USE, INTRINSIC :: ISO_C_BINDING 
  IMPLICIT NONE 
  ! Argument list  
  REAL, INTENT(IN)  :: Q(nVar)
  INTEGER, INTENT(IN) :: normal
  REAL, INTENT(OUT) :: F(nVar)
  ! Local variables  
  REAL :: v2,lf,w,ww,wwx,wwy,wwz,gamma1
  REAL :: ex,ey,ez,b2,e2,uem
  REAL :: V(nVar)
  INTEGER :: iErr
  
  CALL PDECons2Prim(V,Q,iErr)
  gamma1 = gamma/(gamma-1.0)

  ex     = - (V(3)*V(8) - V(4)*V(7))
  ey     = - (V(4)*V(6) - V(2)*V(8))
  ez     = - (V(2)*V(7) - V(3)*V(6))

  v2     = V(2)**2 + V(3)**2 + V(4)**2
  b2     = V(6)**2 + V(7)**2 + V(8)**2
  e2     = ex**2 + ey**2 + ez**2
  lf     = 1.0/sqrt(1.0 - v2)
  w      = V(1) + gamma1*V(5)
  ww     = w*lf**2
  uem    = 0.5*(b2 + e2)
  wwx    = ww*V(2)
  wwy    = ww*V(3)
  wwz    = ww*V(4)

  !
  F = 0.0
  IF (normal.eq.0) THEN
  F(1) = V(2)*V(1)*lf
  F(2) = wwx*V(2) - V(6)*V(6) - ex*ex + V(5) + uem
  F(3) = wwx*V(3) - V(6)*V(7) - ex*ey
  F(4) = wwx*V(4) - V(6)*V(8) - ex*ez 
  F(5) = wwx + (ey*V(8) - ez*V(7)) 
  F(6) = V(9)
  F(7) = -ez
  F(8) = ey  
  F(9) = DivCleaning_a**2*V(6)
  return
  ENDIF
  !
  IF (normal==1) THEN
  F(1) = V(3)*V(1)*lf
  F(2) = wwy*V(2) - V(7)*V(6) - ey*ex 
  F(3) = wwy*V(3) - V(7)*V(7) - ey*ey + V(5) + uem
  F(4) = wwy*V(4) - V(7)*V(8) - ey*ez 
  F(5) = wwy + (ez*V(6) - ex*V(8)) 
  F(6) = ez 
  F(7) = V(9) 
  F(8) = -ex   
  F(9) = DivCleaning_a**2*V(7)
  return
  ENDIF
  !
  IF ( normal==2 ) THEN
    F(1) = V(4)*V(1)*lf
    F(2) = wwz*V(2) - V(8)*V(6) - ez*ex 
    F(3) = wwz*V(3) - V(8)*V(7) - ez*ey 
    F(4) = wwz*V(4) - V(8)*V(8) - ez*ez + V(5) + uem
    F(5) = wwz + (ex*V(7) - ey*V(6)) 
    F(6) = -ey  
    F(7) = ex   
    F(8) = V(9)   
    F(9) = DivCleaning_a**2*V(8)
    return
  ENDIF
  !
END SUBROUTINE PDEFlux 
 
SUBROUTINE PDESource(S,Q) 
  USE Parameters, ONLY : nVar, nDim, gamma, DivCleaning_a
  USE, INTRINSIC :: ISO_C_BINDING 
  IMPLICIT NONE 
  ! Argument list  
  REAL, INTENT(IN)  :: Q(nVar) 
  REAL, INTENT(OUT) :: S(nVar)
  ! Local variables  
  
  S = 0.0

  S(9) = - DivCleaning_a * Q(9)

END SUBROUTINE PDESource
