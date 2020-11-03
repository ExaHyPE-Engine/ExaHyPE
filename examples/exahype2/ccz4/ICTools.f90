
    
 MODULE NSTOV_modVar
    IMPLICIT NONE 
#if defined(RNSTOV) 
    REAL, DIMENSION (:), POINTER :: t,void1,void2,void3
    REAL, DIMENSION (:,:), POINTER :: y
    REAL, DIMENSION (:,:,:), POINTER :: yDG
    !
#endif     
 END MODULE NSTOV_modVar
    
        
        
    
MODULE NSTOV_mod
    IMPLICIT NONE
#if defined(RNSTOV)
#ifdef PARALLEL
    INCLUDE 'mpif.h'
#endif
    PRIVATE
    ! TOV star parameters
    
    ! ODE paramters
    REAL :: Int
    REAL              :: epsilon                    ! precision of real variables 
    REAL, PARAMETER :: tend_B = 200     ! Set the final radius here: sufficiently large   
    REAL, PARAMETER :: dt_B =0.002      ! Define the size of the time step 
    REAL, PARAMETER :: t0_B = 0.0       ! Initial time
    INTEGER, PARAMETER :: M_loc=5          ! polynomial order
    INTEGER, PARAMETER :: mDOF_loc = M_loc +1          ! polynomial order
    INTEGER, PARAMETER :: mGP_loc  = M_loc +1          ! polynomial order
    !
    REAL, ALLOCATABLE :: Tmatrix(:,:),iTmatrix(:,:),RTmatrix(:,:) !,iRTmatrix(:,:) 
    REAL, ALLOCATABLE :: Mt(:,:),iMt(:,:) !,iMtMe(:,:)
    REAL, ALLOCATABLE :: Mt1(:,:),Mt0(:,:),MtV(:,:),Met(:,:),iMet(:,:),iMtMet(:,:)
    !   
    REAL, ALLOCATABLE :: tin(:), tiGP(:),wGPt(:), wM(:)              ! nodes for constructing the basis functions and associated Gaussian weights 
    REAL, ALLOCATABLE :: phiGPt(:,:), phiGPt_xi(:,:)  ! Basis functions computed in Gaussian points 
    REAL, ALLOCATABLE :: phipt(:), phimt(:)  ! Basis functions computed in Gaussian points 
    REAL, ALLOCATABLE :: phipt_xi(:), phimt_xi(:)  ! Basis functions computed in Gaussian points 
    ! 
    REAL, ALLOCATABLE :: ddt(:,:)
    ! ESTRAPOLATION OPERATOR
    REAL, ALLOCATABLE :: Evol(:,:),iMetEv(:,:),phiGPt_evol(:,:),phiGPt_evol_xi(:,:)
    !
    INTERFACE NSTOV_Main 
        MODULE PROCEDURE NSTOV_Main
    END INTERFACE 
    INTERFACE NSTOV_x
        MODULE PROCEDURE NSTOV_x
    END INTERFACE 
    !
    INTERFACE NSTOV_rbar
        MODULE PROCEDURE NSTOV_rbar
    END INTERFACE 

    !
    INTERFACE BaseFunc1D_t
        MODULE PROCEDURE BaseFunc1D_t
    END INTERFACE 
    ! 
    PUBLIC :: NSTOV_Main,NSTOV_x,NSTOV_rbar !,DGTOV
    !
    CONTAINS
     
SUBROUTINE NSTOV_x(r,NSTOV_q) 
    USE MainVariables, ONLY : EQN,myrank,NSTOVVar, NSTOV_nODE, NSTOV_rho_c, NSTOV_rho_atmo, NSTOV_kappa, NSTOV_p_atmo, NSTOV_nODE_p
    IMPLICIT NONE
    ! input argument 
    INTEGER,PARAMETER :: nVar=NSTOV_nODE 
    REAL :: Q(nVar),x,y1(nVar),y2(nVar),y3(nVar),yP(nVar) 
    REAL :: x1,x2,x3
    REAL, ALLOCATABLE :: myX(:)
    ! local variables 
    REAL :: den1,den2,den3,rUni
    REAL :: P(nVar,nVar)
        real :: r,NSTOV_q(NSTOV_nODE) 
        INTENT(OUT) :: NSTOV_q
        ! local variables
        integer :: i,j,jm,jp,jj(1),i1,i2,i3
        real :: xi,dxi,NSTOV_qtmp(NSTOV_nODE)
        !
        IF(NSTOVVar%Computed.NE.12345.AND.myrank.eq.0) THEN
            print *,'|'
            print *,'| ERROR in NSTOV_x: NSTOV equation has not been integrated yet.'
            print *,'|'
            STOP
        ENDIF
        !
        NSTOV_q = 0.
        !
        !ALLOCATE(myX(2))
        !!
        !myX = ABS(NSTOVVar%r(:)-r)
        !! 
        !jj=MINLOC(myX)
        !j=jj(1)
        !! 
        !jm = MAX(1,j-1)
        !jp = MIN(NSTOVVar%n_B,j+1)
        !!
        !x1=NSTOVVar%r(jm)
        !x2=NSTOVVar%r(j)
        !x3=NSTOVVar%r(jp)
        !y1(1:nVar)=NSTOVVar%q(1:nVar,jm)
        !y2(1:nVar)=NSTOVVar%q(1:nVar,j)
        !y3(1:nVar)= NSTOVVar%q(1:nVar,jp) 
        !!
        !NSTOV_qtmp(1:nVar) = (r-x2)*(r-x3)/(x1-x2)/(x1-x3)*y1 + (r-x1)*(r-x3)/(x2-x1)/(x2-x3)*y2 + (r-x1)*(r-x2)/(x3-x1)/(x3-x2)*y3 
        !!
        !NSTOV_q = NSTOV_qtmp
        !DEALLOCATE(myX)
        !!
        !RETURN 
        !
        i1 = FLOOR(r/dt_B)
        i1 = MAX(i1,1)
        i1 = MIN(i1,NSTOVVar%n_B-1)
        DO while(r.GT.NSTOVVar%r(i1+1)+1e-13.OR.r.LT.NSTOVVar%r(i1)-1e-13)
            IF(r.LT.NSTOVVar%r(i1)-1e-13) THEN
                i1=i1-1
            ELSEIF(r.GT.NSTOVVar%r(i1+1)+1e-13) THEN
                i1=i1+1
            ELSE
                IF(myrank.eq.0) THEN
                    print *,'|'
                    print *,'| ERROR in NSTOV_x: '
                    print *,'|             r,i       :',NSTOVVar%r(i1),NSTOVVar%r(i1+1) 
                    print *,'|         eta(i),eta(i+1):',NSTOVVar%r(i1),NSTOVVar%r(i1+1)                  
                    print *,'|'
                    STOP
                ENDIF
            ENDIF
        ENDDO
        ! 
        x1=(r-NSTOVVar%r(i1))/NSTOVVar%dr(i1)
        ! 
        IF(x1.LE.0.5) THEN
            j=i1
        ELSE
            j=i1+1
        ENDIF
        !
        jm=j-1
        jp=j+1
        !
        x1=NSTOVVar%r(jm)
        x2=NSTOVVar%r(j)
        x3=NSTOVVar%r(jp)
        y1(1:nVar)=NSTOVVar%q(1:nVar,jm)
        y2(1:nVar)=NSTOVVar%q(1:nVar,j)
        y3(1:nVar)= NSTOVVar%q(1:nVar,jp)
        !
        IF(ABS(x1-x2).LT.1e-14) x2=x2+1e-14 
        IF(ABS(x3-x2).LT.1e-14) x3=x3+1e-14 
        !
        ! quadratic interpolation
        !den1 = x1**2-x2*x1-x3*x1+x2*x3
        !den2 = x2*x1-x3*x1-x2**2+x2*x3
        !den3 = x2*x1-x3*x1-x2*x3+x3**2
        !P(1:nVar,1) = 1.0/den1*y1(1:nVar)-1.0/den2*y2(1:nVar)+1.0/den3*y3(1:nVar)
        !P(1:nVar,2) = -(x2+x3)/den1*y1(1:nVar)+(x1+x3)/den2*y2(1:nVar)-(x1+x2)/den3*y3(1:nVar)
        !P(1:nVar,3) = x2*x3/den1*y1(1:nVar)-x3*x1/den2*y2(1:nVar)+x2*x1/den3*y3(1:nVar)
        !!
        !NSTOV_qtmp(1:nVar) = P(1:nVar,1)*r**2 + P(1:nVar,2)*r + P(1:nVar,3)
        !NSTOV_q=NSTOV_qtmp
        ! 
        ! Lagrange interpolating polynomial
        NSTOV_q(1:nVar) = (r-x2)*(r-x3)/(x1-x2)/(x1-x3)*y1 + (r-x1)*(r-x3)/(x2-x1)/(x2-x3)*y2 + (r-x1)*(r-x2)/(x3-x1)/(x3-x2)*y3 
        !
        continue
        !
    !
END SUBROUTINE NSTOV_x    
    


RECURSIVE SUBROUTINE NSTOV_rbar(r,NSTOV_q) 
    USE MainVariables, ONLY : EQN,myrank,NSTOVVar,NSTOVVar_barNew,NSTOVVar_bar,  NSTOV_nODE, NSTOV_rho_c, NSTOV_rho_atmo, NSTOV_kappa, NSTOV_p_atmo, NSTOV_nODE_p
    IMPLICIT NONE
    ! input argument 
    INTEGER,PARAMETER :: nVar=NSTOV_nODE 
    REAL :: Q(nVar),x,y1(nVar),y2(nVar),y3(nVar),yP(nVar),dt
    REAL :: x1,x2,x3
    REAL, ALLOCATABLE :: myX(:)
    ! local variables 
    REAL :: den1,den2,den3,rUni
    REAL :: P(nVar,nVar)
        real :: r,NSTOV_q(NSTOV_nODE) 
        INTENT(OUT) :: NSTOV_q
        ! local variables
        integer :: i,j,jm,jp,jj(1),i1,i2,i3
        real :: xi,dxi,NSTOV_qtmp(NSTOV_nODE)
        !
        IF(NSTOVVar%Computed.NE.12345.AND.myrank.eq.0) THEN
            print *,'|'
            print *,'| ERROR in NSTOV_rbar: NSTOV equation has not been integrated yet.'
            print *,'|'
            STOP
        ENDIF
        !
        NSTOV_q = 0.
        ! 
        dt = NSTOVVar_barNew%dr(1)
        i1 = FLOOR(r/dt)
        i1 = MAX(i1,1)
        i1 = MIN(i1,NSTOVVar%n_B-1)
        DO while(r.GT.NSTOVVar_barNew%r(i1+1)+1e-13.OR.r.LT.NSTOVVar_barNew%r(i1)-1e-13)
            IF(r.LT.NSTOVVar_barNew%r(i1)-1e-13) THEN
                i1=i1-1
            ELSEIF(r.GT.NSTOVVar_barNew%r(i1+1)+1e-13) THEN
                i1=i1+1
            ELSE
                IF(myrank.eq.0) THEN
                    print *,'|'
                    print *,'| ERROR in NSTOV_x: '
                    print *,'|             r,i       :',NSTOVVar_barNew%r(i1),NSTOVVar_barNew%r(i1+1) 
                    print *,'|         eta(i),eta(i+1):',NSTOVVar_barNew%r(i1),NSTOVVar_barNew%r(i1+1)                  
                    print *,'|'
                    STOP
                ENDIF
            ENDIF
        ENDDO
        ! 
        x1=(r-NSTOVVar_barNew%r(i1))/NSTOVVar_barNew%dr(i1)
        ! 
        IF(x1.LE.0.5) THEN
            j=i1
        ELSE
            j=i1+1
        ENDIF
        !
        jm=j-1
        jp=j+1
        !
        x1=NSTOVVar_barNew%r(jm)
        x2=NSTOVVar_barNew%r(j)
        x3=NSTOVVar_barNew%r(jp)
        y1(1:nVar)=NSTOVVar_barNew%q(1:nVar,jm)
        y2(1:nVar)=NSTOVVar_barNew%q(1:nVar,j)
        y3(1:nVar)= NSTOVVar_barNew%q(1:nVar,jp)
        !
        IF(ABS(x1-x2).LT.1e-14) x2=x2+1e-14 
        IF(ABS(x3-x2).LT.1e-14) x3=x3+1e-14 
        ! 
        ! Lagrange interpolating polynomial
        NSTOV_q(1:nVar) = (r-x2)*(r-x3)/(x1-x2)/(x1-x3)*y1 + (r-x1)*(r-x3)/(x2-x1)/(x2-x3)*y2 + (r-x1)*(r-x2)/(x3-x1)/(x3-x2)*y3 
        !
        continue
        !
    !
END SUBROUTINE NSTOV_rbar    
    
SUBROUTINE NSTOV_Main  
    USE MainVariables, ONLY : EQN,myrank,NSTOVVar, NSTOV_nODE, NSTOV_rho_c, NSTOV_rho_atmo, NSTOV_kappa, NSTOV_p_atmo, NSTOV_nODE_p,NSTOVVar_bar,NSTOVVar_barNew
        USE NSTOV_modVar
        IMPLICIT NONE
        !
        NSTOVVar%n_B =ceiling(tend_B/dt_B) +1   ! il "+1" è la condizione iniziale al t=0
        !NSTOVVar%n_B = NSTOVVar%n_B
        ALLOCATE (NSTOVVar%r(0:NSTOVVar%n_B+1),NSTOVVar%dr(0:NSTOVVar%n_B-1),NSTOVVar%q(NSTOV_nODE,0:NSTOVVar%n_B))
        ALLOCATE (t(0:NSTOVVar%n_B),void1(0:NSTOVVar%n_B),void2(0:NSTOVVar%n_B),void3(0:NSTOVVar%n_B),y(NSTOV_nODE,0:NSTOVVar%n_B))
        ALLOCATE(NSTOVVar%qloc(NSTOV_nODE))
        !
        ALLOCATE(yDG(mDOF_loc,NSTOV_nODE,0:NSTOVVar%n_B))
        !
        CALL NSTOV_solution(NSTOVVar%r,NSTOVVar%dr,NSTOVVar%q,1)
        !
#ifndef SPHERICAL
        NSTOVVar_bar%n_B=NSTOVVar%n_B
        ALLOCATE (NSTOVVar_bar%r(0:NSTOVVar_bar%n_B+1),NSTOVVar_bar%dr(0:NSTOVVar_bar%n_B-1),NSTOVVar_bar%q(NSTOV_nODE,0:NSTOVVar_bar%n_B+1))
        !ALLOCATE (t(0:NSTOVVar_bar%n_B),void1(0:NSTOVVar_bar%n_B),void2(0:NSTOVVar_bar%n_B),void3(0:NSTOVVar_bar%n_B),y(NSTOV_nODE,0:NSTOVVar_bar%n_B))
        ALLOCATE(NSTOVVar_bar%qloc(NSTOV_nODE))
        !
        NSTOVVar_barNew%n_B=NSTOVVar%n_B
        ALLOCATE (NSTOVVar_barNew%r(0:NSTOVVar_barNew%n_B+1),NSTOVVar_barNew%dr(0:NSTOVVar_barNew%n_B-1),NSTOVVar_barNew%q(NSTOV_nODE,0:NSTOVVar_barNew%n_B+1))
        !ALLOCATE (t(0:NSTOVVar_barNew%n_B),void1(0:NSTOVVar_barNew%n_B),void2(0:NSTOVVar_barNew%n_B),void3(0:NSTOVVar_barNew%n_B),y(NSTOV_nODE,0:NSTOVVar_barNew%n_B))
        ALLOCATE(NSTOVVar_barNew%qloc(NSTOV_nODE))
        !
        CALL NSTOV_solution_rbar
        !
#endif
        NSTOVVar%Computed = 12345
        !
END SUBROUTINE NSTOV_Main
    
SUBROUTINE NSTOV_solution(r,dr,q,printer) 
    USE MainVariables, ONLY : EQN,myrank,NSTOVVar, NSTOV_nODE, NSTOV_rho_c, NSTOV_rho_atmo, NSTOV_kappa, NSTOV_p_atmo, NSTOV_nODE_p
    IMPLICIT NONE
    REAL, DIMENSION (0:NSTOVVar%n_B) :: r
    REAL, DIMENSION (NSTOV_nODE,0:NSTOVVar%n_B) :: q
    REAL, DIMENSION (0:NSTOVVar%n_B-1) :: dr
    !REAL :: Int
    INTEGER :: i,printer
    !
    CALL NSTOV(r,q)
    !
    DO i=1,NSTOVVar%n_B-1
        dr(i)=r(i+1)-r(i)
    END DO
    r(0) = r(1)-1e-12
    dr(0) =  r(1)-r(0)
    q(1:NSTOV_nODE,0) = q(1:NSTOV_nODE,1) 
    !
    IF(printer.eq.1.AND.myrank.eq.0) THEN
        OPEN (1,file='NSTOV_q.dat')
        OPEN (3,file='NSTOV_dr.dat')
        100 format (1000E18.10)
#ifndef SPHERICAL 
            WRITE(1,*)    'VARIABLES = "r" "q1" "q2" "q3" "q4" ' 
#else
            WRITE(1,*)    'VARIABLES = "r" "q1" "q2" "q3" ' 
#endif 
            WRITE(3,*)    'VARIABLES = "i" "dr" '
            WRITE(1,*)     'ZONE T= NSTOV_q ' 
            WRITE(3,*)    'ZONE T= NSTOV_dr '
  
        DO i=1,NSTOVVar%n_B-1
            WRITE(1,100) r(i), q(1:NSTOV_nODE,i) 
            WRITE(3,100) real(i), dr(i)
        END DO
        i=NSTOVVar%n_B
            WRITE(1,100) r(i), q(1:NSTOV_nODE,i) 
        CLOSE(1)
        CLOSE(3)
    END IF
    !
    continue
    !
END SUBROUTINE NSTOV_solution


#ifndef SPHERICAL

SUBROUTINE NSTOV_solution_rbar
    USE MainVariables, ONLY : EQN,myrank,NSTOVVar,NSTOVVar_bar,NSTOVVar_barNew, NSTOV_nODE, NSTOV_rho_c, NSTOV_rho_atmo, NSTOV_kappa, NSTOV_p_atmo, NSTOV_nODE_p  
    IMPLICIT NONE
    ! local variables 
    INTEGER :: n,iloc,jj(1),jm,jp,j,i
    REAL :: r(1),rbar(1),C,Radius,Mass,dt,x1,x2,x3,absDR(NSTOVVar%n_B),rloc
    REAL, DIMENSION(NSTOV_nODE) :: y1,y2,y3
    LOGICAL, PARAMETER :: printer = .TRUE.
    !
    Radius = NSTOVVar%r(NSTOVVar%iradius)
    Mass = NSTOVVar%q(1,NSTOVVar%iradius)
    !
    rloc=SQRT(Radius**2-2.0*Mass*Radius)
    C = 0.5*(rloc+Radius-Mass)/Radius/EXP(NSTOVVar%q(4,NSTOVVar%iradius))
    NSTOVVar%C = C
    !
    DO n=1,NSTOVVar%n_B
        !
        r(1)=NSTOVVar%r(n)
        rbar(1) = r(1)*C*exp(NSTOVVar%q(4,n))
        !
        NSTOVVar_bar%q(:,n) = NSTOVVar%q(:,n)
        NSTOVVar_bar%r(n) = rbar(1)
        !
        If(n.GT.1) NSTOVVar_bar%dr(n-1)=NSTOVVar_bar%r(n)-NSTOVVar_bar%r(n-1)
        !CALL NSTOV_x(r,NSTOVVar%qloc)
        ! 
    END DO
    !
    NSTOVVar_bar%q(:,0)  = NSTOVVar_bar%q(:,1) 
    NSTOVVar_bar%r(0)  = -1e-12
    NSTOVVar_bar%q(:,NSTOVVar%n_B+1)  = NSTOVVar_bar%q(:,NSTOVVar%n_B)
    NSTOVVar_bar%r(NSTOVVar%n_B+1)  = NSTOVVar_bar%r(NSTOVVar%n_B)+1e-12
    !
    !
    dt = dt_B*NSTOVVar_bar%r(NSTOVVar%n_B)/NSTOVVar%r(NSTOVVar%n_B)
    !
    jj=1
    !
    DO n=1,NSTOVVar%n_B
        !
        NSTOVVar_barNew%dr(n-1)=dt 
        r(1)=NSTOVVar_barNew%dr(n-1)*REAL(n-1)
        NSTOVVar_barNew%r(n)=r(1) 
        !
        absDR = ABS(NSTOVVar_bar%r(1:)-r(1))
        ! 
        jj=jj(1)-1 + MINLOC(absDR(jj(1):NSTOVVar%n_B))
        j=jj(1)
        !
        jm = j-1 !MAX(1,j-1)
        jp = j+1 !MIN(NSTOVVar%n_B,j+1)
        !
        !DO  WHILE(ABS(NSTOVVar_bar%r(jm)-NSTOVVar_bar%r(j)).LT.1e-14) 
        !    jm=jm-1
        !END DO
        !!
        !DO  WHILE(ABS(NSTOVVar_bar%r(jp)-NSTOVVar_bar%r(j)).LT.1e-14) 
        !    jp=jp+1
        !END DO
        ! 
        IF(ABS(NSTOVVar_bar%r(jp)-r(1)).LT.ABS(NSTOVVar_bar%r(j)-r(1))) THEN
            continue
        ENDIF
        
        IF(ABS(NSTOVVar_bar%r(jm)-r(1)).LT.ABS(NSTOVVar_bar%r(j)-r(1))) THEN
            continue
        ENDIF
        
        x1=NSTOVVar_bar%r(jm)
        x2=NSTOVVar_bar%r(j)
        x3=NSTOVVar_bar%r(jp)
        y1(1:NSTOV_nODE)=NSTOVVar_bar%q(1:NSTOV_nODE,jm)
        y2(1:NSTOV_nODE)=NSTOVVar_bar%q(1:NSTOV_nODE,j)
        y3(1:NSTOV_nODE)= NSTOVVar_bar%q(1:NSTOV_nODE,jp)
        !
        IF(ABS(x1-x2).LT.1e-14) x2=x2+1e-14 
        IF(ABS(x3-x2).LT.1e-14) x3=x3+1e-14 
        ! 
        ! Lagrange interpolating polynomial
        NSTOVVar_barNew%qloc = (r(1)-x2)*(r(1)-x3)/(x1-x2)/(x1-x3)*y1 + (r(1)-x1)*(r(1)-x3)/(x2-x1)/(x2-x3)*y2 + (r(1)-x1)*(r(1)-x2)/(x3-x1)/(x3-x2)*y3
        NSTOVVar_barNew%q(1:NSTOV_nODE,n) = NSTOVVar_barNew%qloc(1:NSTOV_nODE)
        ! 
    END DO
    !
    NSTOVVar_barNew%q(:,0)  = NSTOVVar_barNew%q(:,1)  
    NSTOVVar_barNew%r(0)  = -1e-12
    NSTOVVar_barNew%q(:,NSTOVVar%n_B+1)  = NSTOVVar_barNew%q(:,NSTOVVar%n_B)
    NSTOVVar_barNew%r(NSTOVVar%n_B+1)  = NSTOVVar_barNew%r(NSTOVVar%n_B)+1e-12
    !
    !
    NSTOVVar_barNew%Mass = Mass 
    NSTOVVar_bar%Mass = Mass 
    NSTOVVar_bar%radius = NSTOVVar_bar%r(NSTOVVar%iradius) 
    NSTOVVar_barNew%radius = NSTOVVar_bar%r(NSTOVVar%iradius) 
    continue
    !
    IF(printer.AND.myrank.eq.0) THEN
        OPEN (1,file='NSTOV_q_Newrbar.dat')
        OPEN (2,file='NSTOV_q_rbar.dat')
        OPEN (3,file='NSTOV_dr_Newrbar.dat')
        OPEN (4,file='NSTOV_dr_rbar.dat')
100     format (1000E18.10)
#ifndef SPHERICAL
            WRITE(1,*)    'VARIABLES = "r" "q1" "q2" "q3" "q4" ' 
            WRITE(2,*)    'VARIABLES = "r" "q1" "q2" "q3" "q4"  '
#else
            WRITE(1,*)    'VARIABLES = "r" "q1" "q2" "q3" ' 
            WRITE(2,*)    'VARIABLES = "r" "q1" "q2" "q3" '
#endif 
            WRITE(3,*)    'VARIABLES = "i" "dr" '
            WRITE(4,*)    'VARIABLES = "i" "dr" '
            WRITE(1,*)     'ZONE T= NSTOV_q_Newrbar ' 
            WRITE(2,*)     'ZONE T= NSTOV_q_rbar ' 
            WRITE(3,*)    'ZONE T= NSTOV_dr_Newrbar '
            WRITE(4,*)    'ZONE T= NSTOV_dr_rbar '
  
        DO i=1,NSTOVVar%n_B-1
            WRITE(1,100) NSTOVVar_barNew%r(i), NSTOVVar_barNew%q(1:NSTOV_nODE,i) 
            WRITE(2,100) NSTOVVar_bar%r(i), NSTOVVar_bar%q(1:NSTOV_nODE,i) 
            WRITE(3,100) real(i), NSTOVVar_barNew%dr(i)
            WRITE(4,100) real(i), NSTOVVar_bar%dr(i) 
        END DO
        i=NSTOVVar%n_B
            WRITE(1,100) NSTOVVar_barNew%r(i), NSTOVVar_barNew%q(1:NSTOV_nODE,i)
            WRITE(2,100) NSTOVVar_bar%r(i), NSTOVVar_bar%q(1:NSTOV_nODE,i) 
        CLOSE(1)
        CLOSE(3)
        CLOSE(2)
        CLOSE(4)
    END IF
    !
    continue
    !
END SUBROUTINE NSTOV_solution_rbar

#endif

SUBROUTINE NSTOV(eta,q) 
    USE MainVariables, ONLY : EQN,myrank,NSTOVVar, NSTOV_nODE, NSTOV_rho_c, NSTOV_rho_atmo, NSTOV_kappa, NSTOV_p_atmo, NSTOV_nODE_p  
    USE NSTOV_modVar, ONLY : t,y 
    IMPLICIT NONE
    ! input arguments 
    REAL, DIMENSION (0:NSTOVVar%n_B) :: eta 
    REAL, DIMENSION (NSTOV_nODE,0:NSTOVVar%n_B) :: q 
    REAL :: g,q3,igamma
    ! local variables
    REAL :: f2,tol,dg,df2,BCq2,time,dt,tloc
    REAL, DIMENSION (NSTOV_nODE) :: y0,k1,k2,k3,k4,tmp1
    INTEGER :: NMAX,n,n_final,nloc,ii,iRefinet2
    LOGICAL :: Refinet,Refinet2,Passed,Passed2,Passed3
    y0(1:NSTOV_nODE) = 0.0 
    y0(2) = 0.0                                        ! P(0) = P(rhoc) polytropic EOS    !q3 
    y0(NSTOV_nODE_p) = NSTOV_kappa*NSTOV_rho_c**EQN%gamma   ! P(0) = P(rhoc) polytropic EOS    !q3 
    !    t0_B = 0;           ! Initial time
    !    tEnd_B = 20;        ! Set the final time here (sufficiently large) 
    !    dt_B = 0.1;         ! Define the size of the time step 
    BCq2 = 0.0           ! Boundary condition at t = tEnd_B for P  y(2,NSTOVVar%n_B)=BCq2
    NMAX =  NSTOVVar%n_B 
    time = t0_B
    ! Start the algorithm here 
    y(:,1) = y0(:)
    t(1) = t0_B
    dt = dt_B
    Refinet = .TRUE.
    Refinet2 = .TRUE.
    Passed = .FALSE.
    Passed2 = .FALSE.
    Passed3 = .TRUE.
    NSTOVVar%rW = 1e14
    NSTOVVar%drW = 1e14
    NSTOVVar%r2 = 1e14
    NSTOVVar%dr2 = 1e14
    iRefinet2=0
    n_final = 10*NMAX
    nloc=0
    DO n=1,NMAX !-1
        nloc=nloc+1 
        !
        IF(time+dt.GE.tEnd_B-1e-7)   THEN        
            dt = tEnd_B - time        ! Adjust last time step
        END IF                            
        IF (time.GE.tEnd_B)  EXIT       ! Exit if tEnd_B has been reached 
        !
        tmp1 = y(:,nloc)
        tloc = time
        CALL NSTOV_ODESys( tmp1(:),tloc,k1(:))   ! First auxiliary variable 
        tmp1 = y(:,nloc) + 0.5*dt*k1(:)
        tloc = time + dt*0.5
        !IF(Passed) THEN
            tmp1(3) = MAX(tmp1(3),0.0)            
        !    continue
        !ENDIF
        CALL NSTOV_ODESys( tmp1(:),tloc,k2(:))   ! Second auxiliary variable  
        tmp1 = y(:,nloc) + 0.5*dt*k2(:)
        tloc = time + dt*0.5
        !IF(Passed) THEN
            tmp1(3) = MAX(tmp1(3),0.0)            
        !    continue
        !ENDIF
        CALL NSTOV_ODESys( tmp1(:),tloc,k3(:))   ! Third auxiliary variable  
        tmp1 = y(:,nloc) + dt*k3(:)
        tloc = time + dt 
        !IF(Passed) THEN
            tmp1(3) = MAX(tmp1(3),0.0)            
        !    continue
        !ENDIF
        CALL NSTOV_ODESys( tmp1(:),tloc,k4(:))   ! Fourth auxiliary variable  
        !
        y(:,nloc+1) = y(:,nloc) + dt/6.0*( k1(:) + 2*k2(:) + 2*k3(:) + k4(:)) 
        !
		!print *, 'RK', nloc,y(:,nloc+1)
		!pause
        !IF(Passed) THEN
            y(3,nloc+1) = MAX(y(3,nloc+1),0.0)            
        !    continue
        !ENDIF
        ! 
        ! 
        IF( y(NSTOV_nODE_p,nloc+1).LE.1e-16 .AND. .NOT. Passed ) THEN  
                !throw away this time step 
                NSTOVVar%iradius = nloc
                NSTOVVar%radius = t(nloc)
                NSTOVVar%Mass = y(1,nloc)
                Passed=.TRUE. 
                !nloc=nloc-1 
                !cycle
        ENDIF
        ! 
        !
        time = time + dt                ! Update physical time
        t(nloc+1) = time                   ! Save time in a vector for plotting 
        !
    END DO

    !n_final = MIN(nloc,NSTOVVar%n_B)
    y(1,nloc+1:NSTOVVar%n_B) =  y(1,nloc) 
    y(2,nloc+1:NSTOVVar%n_B) =  y(2,nloc) 
#ifndef SPHERICAL   
    y(4,nloc+1:NSTOVVar%n_B) =  y(4,nloc) 
#endif    
    y(NSTOV_nODE_p,NSTOVVar%iradius+1:NSTOVVar%n_B) = y(NSTOV_nODE_p,NSTOVVar%iradius)
    !
    DO n=1,NSTOVVar%n_B
        q(1:NSTOV_nODE,n)  = y(1:NSTOV_nODE,n)  
        eta(n)= t(n)
    END DO
    !
    ! match Schwarzschild metric and fix integration constants.
    DO n=1,NSTOVVar%n_B
        q(2,n)  = y(2,n)- y(2,NSTOVVar%iradius)+0.5*LOG(1-2.0*y(1,NSTOVVar%iradius)/NSTOVVar%radius)
    END DO
    !
    q(:,0)  = q(:,1) 
    !
    NSTOVVar%p_R = y(NSTOV_nODE_p,NSTOVVar%iradius)
    igamma = 1.0/EQN%gamma 
    NSTOVVar%rho_R=(NSTOVVar%p_R/NSTOV_kappa)**igamma 
    NSTOVVar%lapse_C= EXP(y(2,1))
    !
END SUBROUTINE NSTOV
 

SUBROUTINE NSTOV_ODESys(y,t,f)  
    USE MainVariables, ONLY : EQN,myrank,NSTOVVar, NSTOV_nODE, NSTOV_rho_c, NSTOV_rho_atmo, NSTOV_kappa, NSTOV_p_atmo, NSTOV_nODE_p
    IMPLICIT NONE
    REAL :: q1,q2,q3,q4,t,rho,igamma,gamma1,p,den,rhoh,t2,m2_r
    REAL, DIMENSION (NSTOV_nODE) :: f,y
    !
    q1 = y(1)       ! m
    q2 = y(2)       ! phi
    q3 = MAX(y(3),NSTOV_p_atmo)       ! p
    !
    p=q3
    igamma = 1.0/EQN%gamma 
    rho=(p/NSTOV_kappa)**igamma 
    gamma1 = EQN%gamma/(EQN%gamma-1.0)
    rhoh = rho + gamma1*p
    !
    IF(ABS(t).LT.1e-12) THEN
        f(:) =0.
        RETURN
    ENDIF
    !
    t2=t**2
    f(1) = 4.0*EQN%PI*(rhoh-p)*t2     
    f(2) = 4.0*EQN%PI*p*t2*t+q1
    den =t*(t -2.0*q1)
    f(2) = f(2)/den  
    f(3) = -rhoh*f(2)  
    !
#ifndef SPHERICAL    
    q4 = y(4)               ! p
    m2_r = SQRT(1.0-2.0*q1/t)
    f(4) = (1.0-m2_r)/m2_r/t             ! p
#endif    
    !
END SUBROUTINE NSTOV_ODESys
 


SUBROUTINE NSTOV_ODESys_DG(yDG,t,f)  
    USE MainVariables, ONLY : EQN,myrank,NSTOVVar, NSTOV_nODE, NSTOV_rho_c, NSTOV_rho_atmo, NSTOV_kappa, NSTOV_p_atmo, NSTOV_nODE_p
    IMPLICIT NONE
    REAL, DIMENSION(mDOF_loc) :: q1,q2,q3,q4,t,rho,igamma,gamma1,p,den,rhoh,t2,m2_r
    REAL, DIMENSION (mDOF_loc,NSTOV_nODE) :: f,yDG
    !
    q1 = yDG(:,1)       ! m
    q2 = yDG(:,2)       ! phi
    q3 = MAX(yDG(:,3),NSTOV_p_atmo)       ! p
    !
    p=q3
    igamma = 1.0/EQN%gamma 
    rho(:)=(p(:)/NSTOV_kappa)**igamma 
    gamma1 = EQN%gamma/(EQN%gamma-1.0)
    rhoh(:) = rho(:) + gamma1*p(:)
    !
    IF(ANY(ABS(t).LT.1e-12)) THEN
        f=0.
        RETURN
    ENDIF
    !
    t2=t**2
    f(:,1) = 4.0*EQN%PI*(rhoh-p)*t2     
    f(:,2) = 4.0*EQN%PI*p*t2*t+q1
    den(:) =t(:)*(t(:) -2.0*q1(:))
    f(:,2) = f(:,2)/den(:)  
    f(:,3) = -rhoh(:)*f(:,2)  
    !
#ifndef SPHERICAL    
    q4(:) = yDG(:,4)               ! p
    m2_r(:) = SQRT(1.0-2.0*q1(:)/t(:))
    f(:,4) = (1.0-m2_r(:))/m2_r(:)/t(:)             ! p
#endif    
    !
END SUBROUTINE NSTOV_ODESys_DG
 



SUBROUTINE BaseFunc1D_t(phi,phi_xi,xi,N)    !compute the vector of the basis at a given point x=xi
   !USE MainVariables, ONLY : tin
   IMPLICIT NONE
   ! Argument list 
   REAL        :: phi(N+1), phi_xi(N+1), xi  
   INTEGER     :: N
   ! Local variables 
   INTEGER     :: i,j,m
   REAL        :: tmp   
   INTENT(IN)  :: xi
   INTENT(OUT) :: phi, phi_xi 
   ! 
   ! Initialize variables 
   phi      = 1. 
   phi_xi   = 0. 
   ! Lagrange polynomial and its derivative 
   DO m = 1,N+1
      DO j = 1,N+1
         IF(j.EQ.m) CYCLE 
         phi(m) = phi(m)*(xi-tin(j))/(tin(m)-tin(j))    ! phi_m(x) is zero in every x=tin(j), but tin(m)
      ENDDO                                             ! note: the integration is done along more points xi, nGP!=N+1=nDOF
      DO i = 1,N+1
         IF(i.EQ.m) CYCLE
         tmp = 1. 
         DO j = 1,N+1
            IF(j.EQ.i) CYCLE 
            IF(j.EQ.m) CYCLE 
            tmp = tmp*(xi-tin(j))/(tin(m)-tin(j))    
         ENDDO 
         phi_xi(m) = phi_xi(m) + tmp/(tin(m)-tin(i)) 
      ENDDO 
   ENDDO 
   !
END SUBROUTINE BaseFunc1D_t
                        
!    
!
!SUBROUTINE ComputeUzero_unst !(Qlocx,Qlocy,Qlocz)
!    USE MainVariables !, ONLY : nEdge,vn_old,,nFace,nDOF,mDOF,nElem,RTmatrix,Qxe_old,Qye_old,Qze_old,FQxe,FQye,FQze
!    USE Edges_Tools
!    USE TmpVariables
!    IMPLICIT NONE
!    ! input variables
!    LOGICAL TEST_UNST
!    !local variables
!    integer :: nn,i,k,j,lll,rrr,ki,s,ii,jj,kk,tt,dimloc
!#ifdef DIM3D    
!    REAL, DIMENSION (nDOF,nDOF,nDOF,mDOF,nDIM) :: v_tmp
!#else
!    REAL, DIMENSION (nDOF,nDOF,1,mDOF,nDIM)    :: v_tmp
!#endif
!    !INTENT(OUT) :: Qlocx,Qlocy,Qlocz
!    !
!    DO j=1,nEdge
!#ifdef DIM3D
!        DO kk=1,nDOF
!#else
!        DO kk=1,1
!#endif
!            DO jj=1,nDOF
!                DO ii=1,nDOF
!                    ! for every Picard iteration: Fun restart from vn_old!!!!!
!                    ! RTmatrix is : MATMUL(iMt,Mt0)
!                    Fun(ii,jj,kk,:,j) = MATMUL(RTmatrix,vn_old(ii,jj,kk,:,j))
!                ENDDO
!            ENDDO
!        ENDDO
!        ENDDO
!        !Fun=vn_old
!    CONTINUE
!    !
!    
!    
!!    DO nn=1,nElem
!!#ifdef DIM3D
!!        DO kk=1,nDOF
!!#else
!!        DO kk=1,1
!!#endif
!!            DO jj=1,nDOF
!!                DO ii=1,nDOF
!!                    FQxe(ii,jj,kk,:,nn) = MATMUL(RTmatrix,Qxe_old(ii,jj,kk,:,nn))
!!                    FQye(ii,jj,kk,:,nn) = MATMUL(RTmatrix,Qxe_old(ii,jj,kk,:,nn))
!!                    FQze(ii,jj,kk,:,nn) = MATMUL(RTmatrix,Qxe_old(ii,jj,kk,:,nn))
!!                    !Qlocx(ii,jj,kk,:,nn) = MATMUL(RTmatrix,Qxe_old(ii,jj,kk,:,nn))
!!                    !Qlocy(ii,jj,kk,:,nn) = MATMUL(RTmatrix,Qxe_old(ii,jj,kk,:,nn))
!!                    !Qlocz(ii,jj,kk,:,nn) = MATMUL(RTmatrix,Qxe_old(ii,jj,kk,:,nn)) 
!!                ENDDO
!!            ENDDO
!!        ENDDO
!!    ENDDO
!    !
!    END SUBROUTINE ComputeUzero_unst
!    
!    

#endif

END MODULE NSTOV_mod
    
    