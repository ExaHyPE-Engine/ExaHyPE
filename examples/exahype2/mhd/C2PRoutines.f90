! Con2Prim helper routines, a file by Olindo Zanotti.

! Note that we have these routines now alternatively also as C++
! available.
! Using this multiline comment you can choose which implementation
! you want to use:
SUBROUTINE RTSAFE_C2P_RMHD1(v2,X1,X2,XACC,gam,d,e,s2,b2,sb2,w,FAILED)
USE parameters, ONLY : nVar ! DebugFile,,DebugPrint
  IMPLICIT NONE
  INTEGER, PARAMETER    :: MAXIT=400
  INTEGER               :: J,i
  REAL                  :: v2 
  REAL                  :: X1,X2,XACC,gam,d,e,s2,b2,sb2,w
  REAL                  :: FL,FH,DF,XH,XL,SWAP,DXOLD,DX,F,TEMP,Xtmp,Vtmp(7),Ftmp,dFtmp,dXtmp
  LOGICAL               :: FAILED
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
  CALL FUNC_C2P_RMHD1(X1,FL,DF,gam,d,e,s2,b2,sb2,w)
  IF(FL.EQ.0.) THEN
     v2=X1
     RETURN
  ENDIF
  CALL FUNC_C2P_RMHD1(X2,FH,DF,gam,d,e,s2,b2,sb2,w)
  IF(FH.EQ.0.) THEN
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
     IF(((v2-XH)*DF-F)*((v2-XL)*DF-F).GE.0. &
          .OR. ABS(2.*F).GT.ABS(DXOLD*DF) ) THEN
        DXOLD=DX
        DX=0.5*(XH-XL)
        v2=XL+DX
        IF(XL.EQ.v2) THEN
            continue
            RETURN
        ENDIF
     ELSE
        DXOLD=DX
        DX=F/DF
        TEMP=v2
        v2=v2-DX
        IF(TEMP.EQ.v2) THEN
            continue
            RETURN
        ENDIF
     ENDIF
     IF(ABS(DX).LT.XACC) THEN
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
END SUBROUTINE 

PURE SUBROUTINE FUNC_C2P_RMHD1(x,f,df,gam,d,e,s2,b2,sb2,w)
  !
  ! This is the CONS2PRIM strategy adopted by Del Zanna et al. (2007) A&A, 473, 11-30
  ! and it corresponds to their choice 3 in Section 3.2
  !
IMPLICIT NONE
  REAL, PARAMETER :: third=1./3.
  INTEGER    :: iter
  REAL       :: x,f,df,v2,rho,c0,c2,c3,dw,dc2,dc3,dlogw,wb,vb2
  REAL       :: gam,d,e,s2,b2,sb2,w
  REAL, PARAMETER :: tolerance = 1e-10
  INTENT(IN) :: x,gam,d,e,s2,b2,sb2
  INTENT(OUT):: f,df,w
  
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
  
  if ( abs ( c0) < 1.0d-20) then
     w = -c2 / c3
  else
     w = max ( - c2 / c3, ( -c0 / c3)**third)
     do iter = 1,100
        dw = -((c3*w + c2)*w**2 + c0)/((3*c3*w + 2*c2)*w)
        if (abs(dw/w).LT.tolerance) THEN
                exit
        ENDIF
        !
        w = w + dw
     end do
  endif
  
  dc3   = gam
  dc2   = 0.5 * ( b2 - gam * rho / (1.0 - v2))
  dlogw = -( dc3 * w + dc2 ) / ( 3.0 * c3 * w + 2.0 * c2)
  wb    = w + b2
  vb2   = sb2 / w**2
  f     = wb**2 * v2 - ( 2.0 * w + b2) * vb2 - s2
  df    = wb * ( wb + 2.0 * dlogw * ( w * v2 + vb2))
  !
END SUBROUTINE FUNC_C2P_RMHD1

