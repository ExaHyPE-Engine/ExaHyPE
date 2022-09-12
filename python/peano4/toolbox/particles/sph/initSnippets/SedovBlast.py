# @TODO: add dependence on SPH scheme (only minimal SPH ATM)
# @TODO: add dependence on input parameters, e.g. grid size, HYDRO_DIMENSIONS...
# @TODO: merge 1D and 2D based on HYDRO_DIMENSIONS input parameter.
class SedovBlastIC:
  """
  Initialisation snippet for the Sedov Blast problem in different spatial dimensions.
  Returns a string with C++ code.
  """
  def __init__(self, dimensions):

    if( dimensions==2 ):
      self.initialisation_snippet = """

      /* Code inserted via the python initialisation snippet library.
       * Do not modify unless you know what you are doing.
       */
 
      /*
       * Initial Conditions: Sedov Blast.
       * 
       * Particles are initialised at rest in regular grid configuration and periodic BC. 
       * Energy is injected to a (or group of) central particle and the blast should propagate
       * isotropically, up to boundary effects.
       * See, e.g., Springel and Hernquist, Mon. Not. R. Astron. Soc. 333, 649–664 (2002).
       *
       */

      // Internal cutoff radius used by Peano and with max value R = grid_size/2
      tarch::la::Vector<Dimensions,double> GridSize = marker.h();
      const double GridSize_x = GridSize(0); 

      particle->setCutOffRadius( GridSize_x / 2.0);
       
      /* Constant density field */
      particle->setDensity ( 1.0 );        
      particle->setMass( particle->getDensity() / std::pow(HYDRO_PART_NUMBER, HYDRO_DIMENSIONS) );

      /* Initial estimate for smoothing length */
      particle->setSmoothingLength( SMOOTH_L_FAC * std::pow(particle->getMass()/particle->getDensity(), 1.0 / HYDRO_DIMENSIONS) );
      particle->setSmoothingLengthPrevious( particle->getSmoothingLength() );
     
      // Initialise the blast
      tarch::la::Vector<Dimensions,double> blast_cent(BLAST_CENTER);
      const double dist_blast = tarch::la::norm2( particle->getX() - blast_cent );
      
       /* Old method */
       /* Inject energy evenly into central particles */
       if ( tarch::la::smaller( dist_blast, SPIKE_RADIUS ) ) {
         particle->setU( BLAST_ENERGY / 12.0 / particle->getMass() );
       } else {
         particle->setU( 1e-6 );  // negligible background value
       }

//       /* New method */
//       /* Smooth the explosion with kernel */
//       const double h = particle->getSmoothingLength();
//       const double R_smooth = 4.0 * h;
//       
//       // Evaluate kernel
//       const double q = dist_blast / h;
//       double w, dw_dq;
//       kernel_deval(q, w, dw_dq);
//     
//       // Normalization factors 
//       const double W = w / std::pow( h, Dimensions );
//     
//      if ( tarch::la::smallerEquals( dist_blast, R_smooth ) ) {
//        particle->setU( BLAST_ENERGY * W ) ;
//      } else {
//        particle->setU( 1e-6 );  // negligible background value
//      }

      /* Compute the pressure from the EoS */
      const double pressure = gas_pressure_from_internal_energy( particle->getDensity(), particle->getU() );
      particle->setPressure( pressure );
      
      /* Compute the sound speed */
      const double soundspeed = gas_soundspeed_from_internal_energy( particle->getU() );
      particle->setSoundSpeed( soundspeed );
      
      /* Initialize remaining fields  */
      
      // Vectors
      for(int d=0; d<Dimensions; d++){
         particle->setV          (d,0);
         particle->setA          (d,0);
         particle->setA_prev_step(d,0);
      }
     
      /* For 2D sims, we only get one component */
      particle->setRot_v(0);
      
      /* Initialise the "extended" arrays */
      particle->setV_full(particle->getV());
      particle->setU_full(particle->getU());

      particle->setUDot  (0.0);        
      particle->setUDot_prev_step(0.0);        
      particle->setHDot  (0.0);        
      particle->setWcount(0.0);        
      particle->setNcount(0  );        
      particle->setOmega (1.0);  // set to 1 to avoid problem with boundary particles.
      particle->setF     (0.0);        
      particle->setRho_dh(0.0);           
      particle->setWcount_dh(0.0);  

      particle->setHLeft (HYDRO_h_MIN);        
      particle->setHRight(HYDRO_h_MAX);        
      
      particle->setTimeStepSize(GLOBAL_TIME_STEP_SIZE);
      particle->setHasConverged(false);
      particle->setHasNoNeighbours(false);
      particle->setHasBeenKicked(false);
      particle->setResidual(0.0);
      particle->setIterCount(0);
      particle->setIsBoundaryParticle(false);

      // Artificial viscosity scheme
      particle->setDiv_v   (0.0);
      particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
      particle->setBalsara (0.0);
     
    """
    elif( dimensions==1 ):
      self.initialisation_snippet = """

      /*
       * Initial Conditions: Sedov Blast.
       * 
       * Particles are initialised at rest in regular grid configuration and periodic BC. 
       * Energy is injected to a (or group of) central particle and the blast should propagate
       * isotropically, up to boundary effects.
       * See, e.g., Springel and Hernquist, Mon. Not. R. Astron. Soc. 333, 649–664 (2002).
       *
       */

      // Internal cutoff radious used by Peano and with max value R = grid_size/2
      /* For grid_size = 1/3 */
      particle->setCutOffRadius(1.0 / 3.0 / 2.0);
      
      /* Constant density field */
      particle->setDensity ( 1.0 );        
      particle->setMass( particle->getDensity() / std::pow(HYDRO_PART_NUMBER, HYDRO_DIMENSIONS) );

      /* Initial estimate for smoothing length */
      particle->setSmoothingLength( SMOOTH_L_FAC * std::pow(particle->getMass()/particle->getDensity(), 1.0/HYDRO_DIMENSIONS ) );
      particle->setSmoothingLengthPrevious( particle->getSmoothingLength() );
     
      // Initialise the blast
      const double spike_radius = 2.0 * (1.0 / HYDRO_PART_NUMBER);
      tarch::la::Vector<Dimensions,double> blast_cent(BLAST_CENTER);
      const double dist_blast = tarch::la::norm2( particle->getX() - blast_cent );
      
       /* Old method */
       /* Inject energy evenly into central particles */
       if ( tarch::la::smaller( dist_blast, spike_radius ) ) {
         particle->setU( BLAST_ENERGY / 4.0 / particle->getMass() );
       } else {
         particle->setU( 1e-6 );  // negligible background value
       }

//       /* New method */
//       /* Smooth the explosion with kernel */
//       const double h = particle->getSmoothingLength();
//       const double R_smooth = 4.0 * h;
//       
//       // Evaluate kernel
//       const double q = dist_blast / h;
//       double w, dw_dq;
//       kernel_deval(q, w, dw_dq);
//     
//       // Normalization factors 
//       const double W = w / std::pow( h, Dimensions );
//     
//      if ( tarch::la::smallerEquals( dist_blast, R_smooth ) ) {
//        particle->setU( BLAST_ENERGY * W ) ;
//      } else {
//        particle->setU( 1e-6 );  // negligible background value
//      }

      /* Compute the pressure from the EoS */
      const double pressure = gas_pressure_from_internal_energy( particle->getDensity(), particle->getU() );
      particle->setPressure( pressure );
      
      /* Compute the sound speed */
      const double soundspeed = gas_soundspeed_from_internal_energy( particle->getU() );
      particle->setSoundSpeed( soundspeed );
      
      /* Initialize remaining fields  */
      
      // Vectors
      for(int d=0; d<Dimensions; d++){
         particle->setV          (d,0);
         particle->setA          (d,0);
         particle->setA_prev_step(d,0);
      }
     
      /* For 2D sims, we only get one component */
      particle->setRot_v(0);
      
      /* Initialise the "extended" arrays */
      particle->setV_full(particle->getV());
      particle->setU_full(particle->getU());

      particle->setUDot  (0.0);        
      particle->setUDot_prev_step(0.0);        
      particle->setHDot  (0.0);        
      particle->setWcount(0.0);        
      particle->setNcount(0  );        
      particle->setOmega (1.0);  // set to 1 to avoid problem with boundary particles.
      particle->setF     (0.0);        
      particle->setRho_dh(0.0);           
      particle->setWcount_dh(0.0);  

      particle->setHLeft (HYDRO_h_MIN);        
      particle->setHRight(HYDRO_h_MAX);        
      
      particle->setTimeStepSize(GLOBAL_TIME_STEP_SIZE);
      particle->setHasConverged(false);
      particle->setHasNoNeighbours(false);
      particle->setHasBeenKicked(false);
      particle->setResidual(0.0);
      particle->setIterCount(0);
      particle->setIsBoundaryParticle(false);

      // Artificial viscosity scheme
      particle->setDiv_v   (0.0);
      particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
      particle->setBalsara (0.0);
     
      /* End of inserted block */

      """
    else:
      print('dimensions not supported!')

