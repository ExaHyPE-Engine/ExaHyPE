! DIM Initial Data
#ifndef HEADER_INITALDATA
#define HEADER_INITALDATA
!#include "MainVariables.f90"
!#include "expintegrator_type.f90"

RECURSIVE SUBROUTINE InitParameters(STRLEN,PARSETUP) 
	USE MainVariables, ONLY: nVar , nDim, EQN, ICType, NSTOV_rho_atmo, NSTOV_kappa,NSTOV_p_atmo
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
        case('GaugeWave')
            EQN%CCZ4GLMc0   = 1.5   ! 0.1      
			EQN%CCZ4GLMc    = 1.2   ! 2.0    
			EQN%CCZ4GLMd    = 2.0   ! 1.0     
			EQN%CCZ4GLMepsA = 1.0   ! 5. 
			EQN%CCZ4GLMepsP = 1.0   ! 5.  
			EQN%CCZ4GLMepsD = 1.0   ! 0.1 
			!
			EQN%CCZ4itau  = 1.0 
			
			EQN%CCZ4k1  = 0.0  !modified according to the version in ExaHyPE 1
			EQN%CCZ4k2  = 0.0 
			EQN%CCZ4k3  = 0.0 
			EQN%CCZ4eta = 0.0 
			EQN%CCZ4f   = 0.0 
			EQN%CCZ4g   = 0.0 
			EQN%CCZ4xi  = 0.0 
			EQN%CCZ4e   = 1.0 
			EQN%CCZ4c   = 1.0 
			EQN%CCZ4mu  = 0.2 
			EQN%CCZ4ds  = 1.0 
			EQN%CCZ4sk  = 0.0
			EQN%CCZ4bs   = 0.0      ! set bs=1 if you want to activate the shift convection for beta, b and B (standard CCZ4 formulation). set it to bs=0 to switch off shift convection for those quantities 
			EQN%CCZ4LapseType   = 0 ! harmonic lapse 
			EQN%EinsteinAutoAux = 0 

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

#endif
