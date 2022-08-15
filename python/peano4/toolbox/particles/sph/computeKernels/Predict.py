# @TODO: add dependencies on input parameters (time-step size, etc)

# -----------------------------------------------------------------------------
# Hydro Predict step kernels
# -----------------------------------------------------------------------------


class LeapfrogPredict:
  """
  Kernels for the Predict step that is used in the leapfrog scheme in SWIFT.
  Apparently, this is particularly relevant for multi-dt (not yet implemented)
  """

  def __init__(self):
    self.touch_particle_first_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* We do not excecute any specific action here */
  
    /* End of inserted block */

    """
    self.particle_particle_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* We do not excecute any specific action here */
    
    /* End of inserted block */

    """
    self.touch_particle_last_time_compute_kernel = """

    /* Code inserted via the python snippet library.
     * Do not modify unless you know what you are doing.
     */

    /* Predict hydro terms (SWIFT) 
     * Relevant for inactive particles when using local-timestepping */
    
    const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
    
    const double dt_kick_hydro = 0.5 * GLOBAL_TIME_STEP_SIZE;
    const double dt_drift = GLOBAL_TIME_STEP_SIZE;
    const double dt_therm = dt_kick_hydro;
  //  const double dt_therm = dt_drift;
    
    if( 
      localParticle->getParallelState() == globaldata::Particle::ParallelState::Local 
      and 
      not localParticle->getIsBoundaryParticle() )
      {
    
        /* Predict velocities for hydro terms */
        localParticle->setV( localParticle->getV() + localParticle->getA() * dt_kick_hydro );

        /* 
         * Predict additional particle fields forward in time after the Drift step
         */
        
        /* Predict the internal energy */
        localParticle->setU( localParticle->getU() + localParticle->getUDot() * dt_therm );

        /* Predict smoothing length */
        const double h_inv = 1.0 / localParticle->getSmoothingLength();
        const double w1 = localParticle->getHDot() * h_inv * dt_drift;
        localParticle->setSmoothingLength( localParticle->getSmoothingLength() * std::exp(w1) );
        
        /* SWIFT custom exponential functions -- @TODO implement */
  //      
  //    if (fabsf(w1) < 0.2f) {
  //      p->h *= approx_expf(w1); /* 4th order expansion of exp(w) */
  //    } else {
  //      p->h *= expf(w1);
  //    }
  //

        /* Predict density */
        const double w2 = - HYDRO_DIMENSIONS * w1;
        localParticle->setDensity( localParticle->getDensity() * std::exp(w2) );

  //
  //    if (fabsf(w2) < 0.2f) {
  //      p->rho *= approx_expf(w2); /* 4th order expansion of exp(w) */
  //    } else {
  //      p->rho *= expf(w2);
  //    }
  //

        /* Check against entropy floor */
  //      const float floor_A = entropy_floor(p, cosmo, floor_props);
  //      const float floor_u = gas_internal_energy_from_entropy(p->rho, floor_A);

        /* Check against absolute minimum */
  //      const float min_u =
  //          hydro_props->minimal_internal_energy / cosmo->a_factor_internal_energy;

  //      p->u = max(p->u, floor_u);
  //      p->u = max(p->u, min_u);

        localParticle->setU( std::max( localParticle->getU(), 0.0) );

        /* Compute the new pressure */
        const double pressure = 
          gas_pressure_from_internal_energy( localParticle->getDensity(), localParticle->getU() );

        /* Compute the new sound speed */
  //      const float soundspeed = gas_soundspeed_from_pressure(p->rho, pressure);

        const double soundspeed = gas_soundspeed_from_internal_energy( localParticle->getU() );

        localParticle->setPressure(pressure);
        localParticle->setSoundSpeed(soundspeed);
        localParticle->setV_sig_AV( std::max(localParticle->getV_sig_AV(), 2.0 * soundspeed) );
      
    }

    /* End of inserted block */

    """


