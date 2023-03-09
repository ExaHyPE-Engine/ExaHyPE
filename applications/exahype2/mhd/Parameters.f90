! MHD Parameters: Riemann problem, Rotor problem, Blast wave
  
  MODULE Parameters 
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

    INTEGER, PARAMETER             :: nDim = 2                            ! The number of space dimensions that we actually want to simulate  
    INTEGER, PARAMETER             :: nVar = 9                            ! The number of variables of the PDE system  
    
    
    ! And even more parameters
    
    ! 4/3 used in ADERDG3D-SRMHD-Z4 by MD, 01.Nov2016
    REAL, PARAMETER                :: gamma = 4.0/3.0
      
    ! Divergence cleaning.
    ! Vasco: 0.5
    ! Michael D: Dont use it!

    ! 1.0 used in ADERDG3D-SRMHD-Z4 by MD, 01.Nov2016
    REAL, PARAMETER :: DivCleaning_a = 1.0  ! Vasco: 0.5, Michael D: No.

  END MODULE Parameters  
