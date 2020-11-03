! DIM Initial Data
#ifndef HEADER_INITALDATA
#define HEADER_INITALDATA
!#include "MainVariables.f90"
!#include "expintegrator_type.f90"

RECURSIVE SUBROUTINE InitParameters(STRLEN,PARSETUP) 
	USE MainVariables, ONLY: nVar , nDim, EQN, ICType, NSTOV_rho_atmo, NSTOV_kappa,NSTOV_p_atmo
	USE NSTOV_mod
	IMPLICIT NONE  
	integer          :: STRLEN
	character(len=STRLEN) :: PARSETUP
	real :: igamma

	ICType=trim(parsetup)
	print *, "****************************************************************"
	print *, 'Chosen setup: 	',ICType
	print *, "****************************************************************"
	!stop
	!ICType   = 'CCZ4MinkowskiSrc'
	EQN%Pi    = ACOS(-1.0)
#if defined(CCZ4GRMHD) || defined(GRMHD)
    EQN%DivCleaning_a=1.0
#else
    EQN%DivCleaning_a=0.0
#endif
    !
	select case(ICType)
		case('CCZ4MinkowskiSrc')
			EQN%CCZ4GLMc0   = 1.5   ! 0.1      
			EQN%CCZ4GLMc    = 1.2   ! 2.0    
			EQN%CCZ4GLMd    = 2.0   ! 1.0     
			EQN%CCZ4GLMepsA = 1.0   ! 5. 
			EQN%CCZ4GLMepsP = 1.0   ! 5.  
			EQN%CCZ4GLMepsD = 1.0   ! 0.1 
			!
			EQN%CCZ4itau  = 0.0 
			
			EQN%CCZ4k1  = 0.0  
			EQN%CCZ4k2  = 0.0 
			EQN%CCZ4k3  = 0.0 
			EQN%CCZ4eta = 0.0 
			EQN%CCZ4f   = 0.0 
			EQN%CCZ4g   = 0.0 
			EQN%CCZ4xi  = 0.0 
			EQN%CCZ4e   = 2.0 
			EQN%CCZ4c   = 0.0 
			EQN%CCZ4mu  = 0.0 
			EQN%CCZ4ds  = 1.0 
			EQN%CCZ4sk  = 0.0
			EQN%CCZ4bs   = 0.0      ! set bs=1 if you want to activate the shift convection for beta, b and B (standard CCZ4 formulation). set it to bs=0 to switch off shift convection for those quantities 
			EQN%CCZ4LapseType   = 0 ! harmonic lapse 
			EQN%EinsteinAutoAux = 0 
		case('CCZ4TOV')	
#ifdef RNSTOV
			EQN%gamma   = 2.0

			EQN%CCZ4GLMc0   = 0.1   ! 0.1      
			EQN%CCZ4GLMc    = 0.1   ! 2.0    
			EQN%CCZ4GLMd    = 0.1   ! 1.0     
			EQN%CCZ4GLMepsA = 1.0   ! 5. 
			EQN%CCZ4GLMepsP = 1.0   ! 5.  
			EQN%CCZ4GLMepsD = 1.0   ! 0.1 
			!
			EQN%CCZ4itau  = 0.0 
			
			EQN%CCZ4k1  = 0.0  
			EQN%CCZ4k2  = 0.0 
			EQN%CCZ4k3  = 0.0 
			EQN%CCZ4eta = 0.0 
			EQN%CCZ4f   = 0.0 
			EQN%CCZ4g   = 0.0 
			EQN%CCZ4xi  = 0.0 
			EQN%CCZ4e   = 1.2 
			EQN%CCZ4c   = 0.0 
			EQN%CCZ4mu  = 0.0 
			EQN%CCZ4ds  = 1.0 
			EQN%CCZ4sk  = 0.0
			EQN%CCZ4bs   = 0.0      ! set bs=1 if you want to activate the shift convection for beta, b and B (standard CCZ4 formulation). set it to bs=0 to switch off shift convection for those quantities 
			EQN%CCZ4LapseType   = 0 ! harmonic lapse 
			EQN%EinsteinAutoAux = 0 

			igamma = 1.0/EQN%gamma
			NSTOV_rho_atmo = (NSTOV_p_atmo/NSTOV_kappa)**igamma 
			
			CALL NSTOV_Main
			
#else	
			PRINT *, ' Please compile with -DRNSTOV. '
			STOP 
#endif
	end select
END SUBROUTINE InitParameters

RECURSIVE SUBROUTINE InitialData(xGP, tGp, Q)
	USE, INTRINSIC :: ISO_C_BINDING
	USE MainVariables, ONLY : nVar, nDim, EQN, ICType, nParam, d, NSTOVVar,NSTOVVar_bar, NSTOV_kappa, aom, mbh, nstov_atmo, nstov_t_atm
	USE NSTOV_mod
	IMPLICIT NONE 
	! Argument list 
	REAL, INTENT(IN)               :: xGP(3), tGp        ! 
	REAL, INTENT(OUT)              :: Q(nVar)        ! 
	REAL :: V0(nVar),r
	
	INTEGER :: i,j,k,l,nm,iNewton,Maxnewton
    REAL :: VBase(nVar), ampl(nVar), sigma(d),etaloc
    REAL :: VLL(nVar),VRR(nVar), VZ4(54),V70(70)  
    REAL :: de,u0_test(nVar),v0_tesT(nVar),aomLoc,MbhLoc
    REAL :: du,dv,drho,dTemp,dp,epsilon,xc(d),rbar,xi 
    REAL :: omega, parL(nParam), parR(nParam), igamma,gamma2,gamma1,rho_atm,h,t,p_atm
    REAL :: r1(9), lambda, lambdat, lambdax, mu, rho, cs, cp, ICA, HH, dxH, Kxx,A11
    REAL :: theta, a,  M, zz, r_hz, delta, g_cov(3,3), g_contr(3,3), g_tmp(3,3), Kex(3,3), Aex(3,3) 
    REAL :: traceK, alpha, fa, ggg, fff, k0, sig, AA(3), BB(3,3), BMat(3,3), DD(3,3,3), PP(3)  
    REAL :: rho0, p0, eta, b0, tempaa, tempab, tempac, va2, vax, bv(3), BV_contr(3), vv(3), gm, gp, ddet(3) 
    REAL :: beta(3), betadown(3), dbetadown(3,3), nablabeta(3,3), test(3,3), Gtilde(3)   
    REAL :: Christoffel(3,3,3), dxphi, bbb, dxb, dtb, kyy, kzz, Aref, x0, re0, ms, uu0  
    REAL :: vv_cov(3), shift(3), p, lapse, gammaij(6), ng, up, phi,phi2, dphi(3), rc, vc2, tc, pc, rhoc, lf, vtheta, vphi, vx, vy, vz 
    REAL :: urr, tt, c1, c2, df, vr, ut, f, urc, g_tt, betaru, dtt, detg, psi , xloc(d), minR, b1, b3 
    REAL :: BJ(0:1),DJ(0:1),BY(0:1),DY(0:1) 
    REAL :: IJ(0:1),DIJ(0:1),IY(0:1),DIY(0:1) 
    REAL, PARAMETER :: Pi = ACOS(-1.0) 
    REAL :: xGP_sph(3),xGP_loc(3),A_coord(3,3), detA,iA_coord(3,3),A_coord_contr(3,3),iA_coord_contr(3,3)
    REAL :: pi_greco, xcLoc
    INTEGER :: iErr
    !
    Maxnewton = 50 


	
	select case(ICType)
		case('CCZ4MinkowskiSrc')
 ! 
       V0(:) = 0.0 
       ! 
       V0(1)  = 1.0 
       V0(4)  = 1.0 
       V0(6)  = 1.0 
       V0(17) = 1.0 
       V0(55) = 1.0 
       !
       !V0(54) = 0.2*EXP(-0.5*( xGP(1)**2+xGP(2)**2)/1.0**2 ) 
       !
       r = SQRT( xGP(1)**2 + xGP(2)**2  + xGP(3)**2) 
       !
       V0(60) =     0.001*EXP(-0.5*( (xGP(1)-2.0)**2+xGP(2)**2+xGP(3)**2)/1.0**2 ) +     0.001*EXP(-0.5*( (xGP(1)+2.0)**2+xGP(2)**2+xGP(3)**2)/1.0**2 )
       IF( r>5.0 .AND. r<10. ) THEN 
           V0(61) = -0.2*xGP(2)*( 10.0 - r )/5.0  
           V0(62) = +0.2*xGP(1)*( 10.0 - r )/5.0  
       ELSEIF( r<=5.0 ) THEN
           V0(61) = -0.2*xGP(2)
           V0(62) = +0.2*xGP(1)           
       ELSE 
           V0(61:62) = 0.0  
       ENDIF 
       V0(63) = 0.0 
       V0(64) = 0.5*0.001*EXP(-0.5*( (xGP(1)-2.0)**2+xGP(2)**2+xGP(3)**2)/1.0**2 ) + 0.5*0.001*EXP(-0.5*( (xGP(1)+2.0)**2+xGP(2)**2+xGP(3)**2)/1.0**2 )
       ! 
		case('CCZ4TOV')	
        V0 = 0.0
		

        ! Compute the metric and its derivatives 
        CALL METRIC(  xGP, alpha,  gp, gm, beta, Kex, g_cov, g_contr, phi )
        CALL DMETRIC( xGP, AA, BB, DD, PP )
        test = matmul(g_cov,g_contr)
        detg = g_cov(1,1)*g_cov(2,2)*g_cov(3,3)-g_cov(1,1)*g_cov(2,3)*g_cov(3,2)-g_cov(2,1)*g_cov(1,2)*g_cov(3,3)+g_cov(2,1)*g_cov(1,3)*g_cov(3,2)+g_cov(3,1)*g_cov(1,2)*g_cov(2,3)-g_cov(3,1)*g_cov(1,3)*g_cov(2,2)
        !phi = detg**(-1./6.) 
        AA = AA/alpha   ! convert pure derivatives to aux. variables 
        DD = 0.5*DD     ! convert pure derivatives to aux. variables 
        PP = PP/phi     ! convert pure derivatives to aux. variables  
        betadown = MATMUL(g_cov, beta) 
        ! Derivative of beta down 
        dbetadown = 0.0 
        DO j = 1, 3
         DO i = 1, 3
          DO k = 1, 3
            dbetadown(k,i) = dbetadown(k,i) + 2.0*DD(k,i,j)*beta(j) + g_cov(i,j)*BB(k,j) 
          ENDDO
         ENDDO
        ENDDO 
        ! Christoffel symbols 
        Christoffel = 0.0
        DO k = 1, 3
         DO j = 1, 3
          DO i = 1, 3
              DO l = 1, 3 
                Christoffel(i,j,k) = Christoffel(i,j,k) + g_contr(k,l)*( DD(i,j,l) + DD(j,i,l) - DD(l,i,j) ) 
              ENDDO 
          ENDDO
         ENDDO
        ENDDO 
        ! covariant derivative of the shift beta 
        DO j = 1, 3
         DO i = 1, 3
           nablabeta(i,j) = dbetadown(i,j) 
           DO k = 1, 3
              nablabeta(i,j) = nablabeta(i,j) - Christoffel(i,j,k)*betadown(k) 
           ENDDO
         ENDDO
        ENDDO 
        ! Extrinsic curvature 
        Kex = 0.0
        DO j = 1, 3
         DO i = 1, 3
             Kex(i,j) = 1.0/(2*alpha)*( nablabeta(i,j) + nablabeta(j,i) ) 
         ENDDO
        ENDDO
        ! Trace of K 
        traceK = SUM( Kex*g_contr ) 
        ! The conformal traceless part of the extrinsic curvature
        Aex = phi**2*( Kex - 1./3*traceK*g_cov ) 
        ! The contracted connection coefficients 
        Gtilde = 0.0
        DO i = 1, 3
         DO j = 1, 3
          DO k = 1, 3
           DO l = 1, 3
               Gtilde(i) = Gtilde(i) + 1./phi**2*( g_contr(i,j)*g_contr(k,l)*( 2*DD(l,j,k) + 2*PP(l)*g_cov(j,k) )  ) 
           ENDDO
          ENDDO 
         ENDDO
        ENDDO 
        SELECT CASE(EQN%CCZ4LapseType) 
        CASE(0)  ! harmonic 
           fa = 1.0 
        CASE DEFAULT  ! 1 + log 
           fa = 2.0/alpha
        END SELECT 
        ! K0 to make the PDE for alpha stationary 
        K0 = traceK - 1.0/(alpha*fa)*SUM(beta*AA)   
        !
        ! The metric tensor 
        V0(1) = phi**2*g_cov(1,1)    ! \gamma_11 
        V0(2) = phi**2*g_cov(1,2)    ! \gamma_12
        V0(3) = phi**2*g_cov(1,3)    ! \gamma_13
        V0(4) = phi**2*g_cov(2,2)    ! \gamma_22
        V0(5) = phi**2*g_cov(2,3)    ! \gamma_23
        V0(6) = phi**2*g_cov(3,3)    ! \gamma_33 
        ! The extrinsic curvature         
        V0(7)  = Aex(1,1)  
        V0(8)  = Aex(1,2) 
        V0(9)  = Aex(1,3) 
        V0(10) = Aex(2,2) 
        V0(11) = Aex(2,3) 
        V0(12) = Aex(3,3) 
        ! The cleaning variables Z and Theta 
        V0(13) = 0.0          ! Theta 
        V0(14) = Gtilde(1)    ! G1 
        V0(15) = Gtilde(2)    ! G2 
        V0(16) = Gtilde(3)    ! G3  
        ! The logarithm of the lapse 
        V0(17) = alpha 
        ! The shift 
        V0(18) = beta(1)  
        V0(19) = beta(2) 
        V0(20) = beta(3) 
        !
        ! Auxiliary variables
        ! 
        !! vector A_i = \partial_i \alpha / \alpha 
        !!  
        V0(24) = AA(1)     ! A1 = alpha_1/alpha  
        V0(25) = AA(2)     ! A2 = alpha_2/alpha    
        V0(26) = AA(3)     ! A3 = alpha_3/alpha   
        !
        ! Matrix B_ik = \partial_i \beta_k 
        !
        V0(27) = BB(1,1) 
        V0(28) = BB(2,1)
        V0(29) = BB(3,1)
        V0(30) = BB(1,2)
        V0(31) = BB(2,2)
        V0(32) = BB(3,2)
        V0(33) = BB(1,3)
        V0(34) = BB(2,3)
        V0(35) = BB(3,3) 
        !
        ! tensor D_ijk = 0.5 \partial i \tilde \gamma_jk   
        !
        DO j = 1, 3
         DO i = 1, 3 
          DO k = 1, 3 
            DD(k,i,j) = phi**2*( DD(k,i,j) + PP(k)*g_cov(i,j) ) 
          ENDDO
         ENDDO
        ENDDO 
        !
        V0(36) = DD(1,1,1) 
        V0(37) = DD(1,1,2) 
        V0(38) = DD(1,1,3) 
        V0(39) = DD(1,2,2) 
        V0(40) = DD(1,2,3) 
        V0(41) = DD(1,3,3) 
        V0(42) = DD(2,1,1) 
        V0(43) = DD(2,1,2) 
        V0(44) = DD(2,1,3) 
        V0(45) = DD(2,2,2) 
        V0(46) = DD(2,2,3) 
        V0(47) = DD(2,3,3) 
        V0(48) = DD(3,1,1) 
        V0(49) = DD(3,1,2) 
        V0(50) = DD(3,1,3) 
        V0(51) = DD(3,2,2) 
        V0(52) = DD(3,2,3) 
        V0(53) = DD(3,3,3) 
        ! trace of K 
        V0(54) = traceK 
        ! logarithm of the conformal factor phi 
        V0(55) = phi 
        ! derivative of phi 
        V0(56) = PP(1) 
        V0(57) = PP(2) 
        V0(58) = PP(3) 
        ! The trace of the extrinsic curvature at the initial time 
        V0(59) = K0 
        !
	   if(V0(55)>1.0 .or. V0(55)<0.0) then
			print *, V0
			pause
		end if
		
#ifdef RNSTOV    

        CALL Curved2Cartesian(xGP_loc,xGP)          ! no effects if you are in Cartesian coordiantes
        CALL Cartesian2Spherical(xGP_sph,xGP_loc)   ! here we convert Cartesian to Spherical, independently on the chosen coordiantes
        r=XGP_sph(1)
        ! 
#ifdef SPHERICAL  ! then we are in the original coordinate system        
        CALL NSTOV_x(r,NSTOVVar%qloc)
#elif CYLINDRICAL
        PRINT *, 'CYLINDRICAL COORDINATES NOT TESTED FOR RNSTOV'
#else  
        CALL NSTOV_rbar(r,NSTOVVar%qloc)
#endif
        !
        p = MAX( 1e-13, NSTOVVar%qloc(3) ) 
        igamma = 1.0/EQN%gamma 
        rho=(p/NSTOV_kappa)**igamma 
        VV_cov(1:3) = 0.
        BV_contr(1:3) = 0.

#else
        PRINT *, ' RNSTOV not available. Please compile with -DRNSTOV flag. '
        STOP 
#endif      
       !
#if defined(CCZ4EINSTEIN) || defined(CCZ4GRHD) 
       V0(60:64) = (/ rho, VV_cov(1:3), p /)
	   

	   !pause

#endif      
       !
#if defined(CCZ4GRMHD)   
       V0(60:68) = (/ rho, VV_cov(1:3), p, BV_contr(1:3), 0.0 /)  
#endif		
	CASE DEFAULT
		PRINT *, 'NOT IMPLEMENTED'
		STOP
	END SELECT
	
	CALL PDEPrim2Cons(Q,V0)
	
END SUBROUTINE InitialData

RECURSIVE SUBROUTINE PDElimitervalue(limiter_value,xx,numberOfObservables, observablesMin, observablesMax)
	USE, INTRINSIC :: ISO_C_BINDING
	USE MainVariables, ONLY : nVar, nDim
	IMPLICIT NONE 
	! Argument list 
	REAL, INTENT(IN)               :: xx(3)        ! 
	INTEGER, INTENT(IN)					:: numberOfObservables
	INTEGER, INTENT(OUT)              :: limiter_value        !
	REAL, INTENT(IN)					:: observablesMin(numberOfObservables), observablesMax(numberOfObservables)
	real	:: rr	

	limiter_value=0
	
#ifdef CCZ4GRHD 	
	IF( SQRT(xx(1)**2+xx(2)**2+xx(3)**2)>=7.0 .and. SQRT(xx(1)**2+xx(2)**2+xx(3)**2)<=9.5 ) THEN
		limiter_value = 1
	ELSE
		limiter_value = 0
	ENDIF 
#endif 	

   !limiter_value = 1
END SUBROUTINE PDElimitervalue

#endif
