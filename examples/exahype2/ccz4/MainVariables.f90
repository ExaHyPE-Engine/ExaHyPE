#ifndef HEADER_MAINVARIABLES
#define HEADER_MAINVARIABLES

! DIM Parameters
  
  MODULE MainVariables 
    IMPLICIT NONE  
    PUBLIC  

    ! This typesDef.f90 is a relict still from the old Fortran interface in Exahype.
    ! However, the following two variables are needed in the Fortran code. They
    ! should provided by the glue code generator in an appropriate way.

    ! If you modify the SRHD.exahype, please do the following mapping by hand:
    !
    ! solver ADER-DG SRHDSolver / unknowns   ->  goes to ->  nVar
    ! computational-domain / dimension       ->  goes to ->  nDim
    !

    ! Here, we obtain DIMENSIONS from the C++ preprocessor
#if defined(Dim3)
    INTEGER, PARAMETER             	:: nDim = 3                   ! The number of space dimensions
#elif defined(Dim2)
    INTEGER, PARAMETER             	:: nDim = 2                   ! The number of space dimensions
#endif
	INTEGER, PARAMETER             	:: nAux = 0
    INTEGER, PARAMETER             	:: nVar = 96                           ! The number of variables of the PDE system 
    INTEGER, PARAMETER 				:: nLin = 0
	INTEGER, PARAMETER				:: nParam=0
	INTEGER, PARAMETER				:: d=3
    !CHARACTER(LEN=20), PARAMETER	:: ICType='NLOPRUPTURE'
	CHARACTER(LEN=20)				:: ICType
  TYPE tEquations 
      REAL(8)    :: gamma, Pi, c0, g = 9.81, friction = 1.0     
      REAL(8)    :: CCZ4k1, CCZ4k2, CCZ4k3, CCZ4eta, CCZ4itau, CCZ4f, CCZ4g, CCZ4xi, CCZ4e, CCZ4c, CCZ4mu, CCZ4ds, CCZ4sk, CCZ4bs  
      REAL(8)    :: CCZ4GLMc0 = 0.5, CCZ4GLMc = 0.75, CCZ4GLMd = 0.75, CCZ4GLMepsD = 1e-2, CCZ4GLMepsA = 1e-2, CCZ4GLMepsP = 1e-2, cs, alpha, beta, lambda, cv, rho0, p0, tau1, tau2, mu, kappa ,tau 
      INTEGER :: CCZ4LapseType, EinsteinAutoAux = 0, ReferenceDepth = 1.0    
      REAL(8)    :: DivCleaning_a = 1.0 
  END TYPE tEquations 
	
	TYPE(tEquations) :: EQN
	
   !Variables for NSTOV module 
#ifdef SPHERICAL  
    INTEGER, PARAMETER :: NSTOV_nODE = 3
#else
    INTEGER, PARAMETER :: NSTOV_nODE = 4
#endif
    INTEGER, PARAMETER :: NSTOV_nODE_p = 3, NSTOV_ATMO = 0 ! 1 => Landau atmo
    REAL(8), PARAMETER :: NSTOV_rho_c = 1.28e-3
    REAL(8), PARAMETER :: NSTOV_kappa = 100
    REAL(8), PARAMETER    :: p_floor = 1.0e-16, rho_floor = 1.0e-10
    REAL(8) :: NSTOV_rho_atmo != 1e-10
    REAL(8), PARAMETER :: NSTOV_p_atmo = 1e-15 , NSTOV_t_atm=1.0
    !
	REAL(8)            ::  Mbh = 1.0, aom = 0.0 
	REAL(8), PARAMETER    :: P_eps = 1e-4
	integer, parameter :: MYRANK=0 	
    
    !
    TYPE tNSTOVVar
        INTEGER :: Computed
        REAL(8) :: Int
        !INTEGER, PARAMETER :: nODE = 3
        REAL(8) :: Mass, radius,r1,dr1,r2,dr2,rW,drW,rUni,C,p_R ,rho_R,lapse_C
        INTEGER :: n_B,iradius,ir1,ir2,irW 
        REAL(8), DIMENSION (:,:),     ALLOCATABLE :: q,dq
        REAL(8), DIMENSION (:),     ALLOCATABLE :: r,dr
        REAL(8), DIMENSION (:),     ALLOCATABLE :: qloc
    END TYPE tNSTOVVar
    TYPE(tNSTOVVar) :: NSTOVVar,NSTOVVar_bar,NSTOVVar_barNew
    !
    REAL(8), PARAMETER :: CoordTol =1e-11	
	
	
	
  ! 3-point Gaussian quadrature 
  REAL, PARAMETER     :: sGP3(3) = (/ 0.5-sqrt(15.)/10.,0.5,0.5+sqrt(15.)/10. /) 
  REAL, PARAMETER     :: wGP3(3) = (/ 5./18., 8./18., 5./18. /) 
  INTEGER, PARAMETER  :: nGP3 = 3 

  END MODULE MainVariables  
#endif
