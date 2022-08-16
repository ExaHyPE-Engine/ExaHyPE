# @TODO: add dependence on SPH scheme (only minimal SPH ATM)
# @TODO: add dependence on input parameters, e.g. grid size, HYDRO_DIMENSIONS...

class SodIC:
  """
  Initialisation snippet for the Sod problem in different spatial dimensions.
  Returns a string with C++ code.
  """

  def __init__(self, dimensions):

    if( dimensions==1 ):
      self.initialisation_snippet = """

      /* Code inserted via the python initialisation snippet library.
       * Do not modify unless you know what you are doing.
       */

      /*
       * Initial Conditions: Sod shockwave.
       * Particles are deposited in two regions with different densities.
       * @TODO: implement particles positions to achieve this.
       */
     
      // Internal cutoff radious used by Peano and with max value R = grid_size/2
      /* For grid_size = 1/3 */
      particle->setCutOffRadius(1.0 / 3.0 / 2.0);
      
      /* Constant density field */
      particle->setDensity ( 1.0 );        
      particle->setMass( particle->getDensity() / std::pow(HYDRO_PART_NUMBER, HYDRO_DIMENSIONS) );

      /* Set neglibible internal energy */
      particle->setU( 1e-6 ); 

      /* Initial estimate for smoothing length */
      particle->setSmoothingLength( SMOOTH_L_FAC * std::pow(particle->getMass()/particle->getDensity(), 1.0/HYDRO_DIMENSIONS ) );
      particle->setSmoothingLengthPrevious( particle->getSmoothingLength() );
     
      /* Compute the pressure from the EoS */
      const double pressure = gas_pressure_from_internal_energy( particle->getDensity(), particle->getU() );
      particle->setPressure( pressure );
      
      /* Set sound speed */
      const double soundspeed = gas_soundspeed_from_internal_energy( particle->getU() );
      particle->setSoundSpeed(soundspeed);
      
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
      particle->setHasBeenKicked(true);
      particle->setResidual(0.0);
      particle->setIterCount(0);
      particle->setIsBoundaryParticle(false);

      // Artificial viscosity scheme
      particle->setDiv_v   (0.0);
      particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
      particle->setBalsara (0.0);

      
      /* Calculate particle displacements.
       * Can be combined with the guard option.
       */
      
      particle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );    

      if(
        particle->getMoveState()!=globaldata::Particle::MoveState::Moved
    //    and
    //    particle->getParallelState()==globaldata::Particle::ParallelState::Local
        )
      {

        // Displace particles to generate underdense region (Sod) 
        const double part_spacing = (1.0 / HYDRO_PART_NUMBER);

        // New inter-particle spacing (after displacing)
        const double dx_new = 2.0 * part_spacing;
      
        // Set origin from which to displace away from
      //  const double offset = round( HYDRO_PART_NUMBER / 2 ) * part_spacing; // 0.503937 for 127 parts
        const double offset = 0.0;  // first particle on the left of the box.
        
        // Relative position of the particle
        const double pos = particle->getX()(0) - offset;
        
        // Get index relative to origin of displacement
        const int index = round( pos / part_spacing );

        // Calculate displacement
        const double disp = index * dx_new;
        const double final_pos = offset + disp;
      
        // Set particle id
        particle->setPartid( index );
      
        if( final_pos > offset and index < HYDRO_PART_NUMBER)
          {
      

          const double disp_eff = disp - pos;
          if(disp_eff > 1./3.){
            // Flag OK particle
            particle->setHasBeenKicked(false);

            // Verbose
            logInfo( "Init()", " index = " << index );
            logInfo( "Init()", " Warning: displacement too large, disp = " << disp );
            logInfo( "Init()", " Warning: disp_eff = " << disp_eff );
    //        particle->setX(0, 1.0);


          } else {
              
            // Verbose
            logInfo( "Init()", " Displacing particle: " << particle->toString() );
            logInfo( "Init()", " index = " << index );
            logInfo( "Init()", " offset = " << offset );
            logInfo( "Init()", " disp_eff = " << disp_eff );
            logInfo( "Init()", " disp = " << disp );
        
            // Apply displacement
            particle->setX(0, final_pos);

            // Re-calculate density
            particle->setDensity( particle->getDensity() * part_spacing / dx_new);
      
            // Verbose
            logInfo( "Init()", " Final x-position = " << particle->getX()(0) );
          }

        }

        // Important: flag that particle has been moved 
        particle->setMoveState( globaldata::Particle::MoveState::Moved );

      }

      /* End of inserted block */

      """
    else:
      print('dimensions not supported!')
