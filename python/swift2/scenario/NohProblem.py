# @TODO: add dependence on SPH scheme (only minimal SPH ATM)
# @TODO: add dependence on input parameters, e.g. grid size ...


class NohProblemIC:
    """
    Initialisation snippet for the Noh problem in different spatial dimensions.
    Returns a string with C++ code.
    """

    def __init__(self, dimensions):
        if dimensions == 2:
            self.initialisation_snippet = """

      /* Code inserted via the python initialisation snippet library.
       * Do not modify unless you know what you are doing.
       */

      /*
       * Initial Conditions: the Noh problem.
       * We give particles an initial velocity towards the centre.
       * Particles will then develop a shock that propagates outwards.
       *
       */

      // Internal search radius used by Peano and with max value R = grid_size/2
      tarch::la::Vector<Dimensions,double> GridSize = marker.h();
      const double GridSize_x = GridSize(0);

      particle->setSearchRadius( 0.9 * GridSize_x / 2.0);

      /* Constant density field */
      particle->setDensity ( 1.0 );
      particle->setMass( particle->getDensity() / std::pow(HYDRO_PART_NUMBER, HYDRO_DIMENSIONS) );

      const double eta_fac = particle->getEtaFactor();
      /* Initial estimate for smoothing length */
      particle->setSmoothingLength( eta_fac * std::pow(particle->getMass() / particle->getDensity(), 1.0 / HYDRO_DIMENSIONS) );

      /* Set neglibible internal energy */
      particle->setU( 1e-6 );

      /* Compute the pressure from the EoS */
      const double pressure = ::swift2::kernels::legacy::eos::gas_pressure_from_internal_energy( particle->getDensity(), particle->getU() );
      particle->setPressure( pressure );

      /* Compute the sound speed */
      const double soundspeed = ::swift2::kernels::legacy::eos::gas_soundspeed_from_internal_energy( particle->getU() );
      particle->setSoundSpeed(soundspeed);

      /*
       * Set initial velocity for the spherical collapse
       */

      // Box centre
      tarch::la::Vector<Dimensions,double> x_0(0.5);

      // Distance of particle to centre
      tarch::la::Vector<Dimensions,double> dist = particle->getX() - x_0;
      double norm_dist = tarch::la::norm2(dist);

      // Cap min value of distance
      if ( tarch::la::equals(norm_dist, 0.0) ){
        norm_dist = 1e-10;
      }

      // Set velocity
      tarch::la::Vector<Dimensions,double> v_ini = -1.0 * dist / norm_dist;

      particle->setV( v_ini );

      /* Initialize remaining fields  */

      // Vectors
      for(int d=0; d<Dimensions; d++){
         particle->setA          (d,0);
      }

      /* For 2D sims, we only get one component */
      particle->setRot_v(0);

      /* Initialise the "extended" arrays */
      particle->setV_full(particle->getV());
      particle->setU_full(particle->getU());

      particle->setUDot  (0.0);
      particle->setHDot  (0.0);
      particle->setWcount(0.0);
      particle->setF     (0.0);
      particle->setRho_dh(0.0);
      particle->setWcount_dh(0.0);

      particle->setHasNoNeighbours(false);
      particle->setSmoothingLengthIterCount(0);

      // Artificial viscosity scheme
      particle->setDiv_v   (0.0);
      particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
      particle->setBalsara (0.0);

      particle->setCellHasUpdatedParticle(false);
      particle->setPartid(1);

      /* Verbose */
      logDebug( "Init()", "dist = " << dist );
      logDebug( "Init()", "v_ini = " << v_ini );
      logDebug( "Init()", "Particle: " << particle->toString() );

      /* End of inserted block */

    """
        else:
            print("dimensions not supported!")
