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
	REAL(8) :: igamma

	ICType=trim(parsetup)
	print *, "****************************************************************"
	print *, 'Chosen setup: 	',ICType
	print *, "****************************************************************"
	!stop
        !ICType   = 'MGCCZ4MinkowskiSrc'
	EQN%Pi    = ACOS(-1.0)
#if defined(MGCCZ4GRMHD) || defined(GRMHD)
    EQN%DivCleaning_a=1.0D0
#else
    EQN%DivCleaning_a=0.0D0
#endif
    !
	select case(ICType)
        case('GaugeWave')
            EQN%MGCCZ4GLMc0   = 1.5D0   ! 0.1      
			EQN%MGCCZ4GLMc    = 1.2D0   ! 2.0    
			EQN%MGCCZ4GLMd    = 2.0D0   ! 1.0     
			EQN%MGCCZ4GLMepsA = 1.0D0   ! 5. 
			EQN%MGCCZ4GLMepsP = 1.0D0   ! 5.  
			EQN%MGCCZ4GLMepsD = 1.0D0   ! 0.1 
			!
			EQN%MGCCZ4itau  = 1.0D0 
			
			EQN%MGCCZ4k1  = 0.0D0  !modified according to the version in ExaHyPE 1
			EQN%MGCCZ4k2  = 0.0D0 
			EQN%MGCCZ4k3  = 0.0D0 
			EQN%MGCCZ4eta = 0.0D0 
			EQN%MGCCZ4f   = 0.0D0 
			EQN%MGCCZ4g   = 0.0D0 
			EQN%MGCCZ4xi  = 0.0D0 
			EQN%MGCCZ4e   = 1.0D0 
			EQN%MGCCZ4c   = 1.0D0 
			EQN%MGCCZ4mu  = 0.2D0 
			EQN%MGCCZ4ds  = 1.0D0 
			EQN%MGCCZ4sk  = 0.0D0
			EQN%MGCCZ4bs   = 0.0D0      ! set bs=1 if you want to activate the shift convection for beta, b and B (standard MGCCZ4 formulation). set it to bs=0 to switch off shift convection for those quantities 
			EQN%MGCCZ4LapseType   = 0 ! harmonic lapse 
			EQN%EinsteinAutoAux = 0D0 

		case('MGCCZ4MinkowskiSrc')
			EQN%MGCCZ4GLMc0   = 1.5   ! 0.1      
			EQN%MGCCZ4GLMc    = 1.2   ! 2.0    
			EQN%MGCCZ4GLMd    = 2.0   ! 1.0     
			EQN%MGCCZ4GLMepsA = 1.0   ! 5. 
			EQN%MGCCZ4GLMepsP = 1.0   ! 5.  
			EQN%MGCCZ4GLMepsD = 1.0   ! 0.1 
			!
			EQN%MGCCZ4itau  = 0.0 
			
			EQN%MGCCZ4k1  = 0.0  
			EQN%MGCCZ4k2  = 0.0 
			EQN%MGCCZ4k3  = 0.0 
			EQN%MGCCZ4eta = 0.0 
			EQN%MGCCZ4f   = 0.0 
			EQN%MGCCZ4g   = 0.0 
			EQN%MGCCZ4xi  = 0.0 
			EQN%MGCCZ4e   = 2.0 
			EQN%MGCCZ4c   = 0.0 
			EQN%MGCCZ4mu  = 0.0 
			EQN%MGCCZ4ds  = 1.0 
			EQN%MGCCZ4sk  = 0.0
			EQN%MGCCZ4bs   = 0.0      ! set bs=1 if you want to activate the shift convection for beta, b and B (standard MGCCZ4 formulation). set it to bs=0 to switch off shift convection for those quantities 
			EQN%MGCCZ4LapseType   = 0 ! harmonic lapse 
			EQN%EinsteinAutoAux = 0 
		case('MGCCZ4TOV')	
#ifdef RNSTOV
			EQN%gamma   = 2.0

			EQN%MGCCZ4GLMc0   = 0.1   ! 0.1      
			EQN%MGCCZ4GLMc    = 0.1   ! 2.0    
			EQN%MGCCZ4GLMd    = 0.1   ! 1.0     
			EQN%MGCCZ4GLMepsA = 1.0   ! 5. 
			EQN%MGCCZ4GLMepsP = 1.0   ! 5.  
			EQN%MGCCZ4GLMepsD = 1.0   ! 0.1 
			!
			EQN%MGCCZ4itau  = 0.0 
			
			EQN%MGCCZ4k1  = 0.0  
			EQN%MGCCZ4k2  = 0.0 
			EQN%MGCCZ4k3  = 0.0 
			EQN%MGCCZ4eta = 0.0 
			EQN%MGCCZ4f   = 0.0 
			EQN%MGCCZ4g   = 0.0 
			EQN%MGCCZ4xi  = 0.0 
			EQN%MGCCZ4e   = 1.2 
			EQN%MGCCZ4c   = 0.0 
			EQN%MGCCZ4mu  = 0.0 
			EQN%MGCCZ4ds  = 1.0 
			EQN%MGCCZ4sk  = 0.0
			EQN%MGCCZ4bs   = 0.0      ! set bs=1 if you want to activate the shift convection for beta, b and B (standard MGCCZ4 formulation). set it to bs=0 to switch off shift convection for those quantities 
			EQN%MGCCZ4LapseType   = 0 ! harmonic lapse 
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
