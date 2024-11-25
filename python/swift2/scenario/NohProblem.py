# @TODO: add dependence on SPH scheme (only minimal SPH ATM)
# @TODO: add dependence on input parameters, e.g. grid size ...


class NohProblemIC:
    """
    Initialisation snippet for the Noh problem in different spatial dimensions.
    Returns a string with C++ code.
    """

    def __init__(self, dimensions):
        if dimensions == 2 or dimensions == 3:
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
      const double GridSize_x = 0.5 * GridSize(0);

      particle->setSearchRadius( 0.999 * GridSize_x );

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
      tarch::la::Vector<Dimensions,double> x_0 = DomainOffset + 0.5 * DomainSize;

      // Distance of particle to centre
      tarch::la::Vector<Dimensions,double> dist = particle->getX() - x_0;
      double norm_dist = tarch::la::norm2(dist);

      // Cap min value of distance
      norm_dist = std::max(norm_dist, 1e-10);

      // Set velocity
      tarch::la::Vector<Dimensions,double> v_ini = -1.0 * dist / norm_dist;

      particle->setV( v_ini );

      /* Initialize remaining fields  */

      // Vectors
      for(int d=0; d<Dimensions; d++){
         particle->setA(d,0.);
      }

      #if Dimensions < 3
      /* For 2D sims, we only get one component */
      particle->setRot_v(0.);
      #else
      for(int d=0; d<Dimensions; d++){
         particle->setRot_v(d, 0.);
      }
      #endif


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

      // Set a reproducible particle ID assuming we're distributing particles on a regular grid
      // int max - 1
      int MAX_PART_NUMBER = 2147483646;
      if (HYDRO_DIMENSIONS==2) {
        // ~ sqrt int max - 1
        MAX_PART_NUMBER = 46339;
      }
      else if (HYDRO_DIMENSIONS==3) {
        // ~ cbrt int max - 1
        MAX_PART_NUMBER = 1289;
      }

      double xp = particle->getX(0);
      double oneOverDx = MAX_PART_NUMBER / DomainSize[0];
      int i = (int) ((xp - DomainOffset[0]) * oneOverDx);
      int partid = i;

      // Assuming here we're running 1D experiments on a 2D grid,
      // as we always do, then we don't need compile time macro guards.
      if (HYDRO_DIMENSIONS > 1) {
        double yp = particle->getX(1);
        double oneOverDy = MAX_PART_NUMBER / DomainSize[1];
        int j = (int) ((yp - DomainOffset[1]) * oneOverDy);
        partid += j * MAX_PART_NUMBER;
      }

      // For 3D, we need the guard.
      #if Dimensions > 2
      if (HYDRO_DIMENSIONS > 2){
        double zp = particle->getX(2);
        double oneOverDz = MAX_PART_NUMBER / DomainSize[2];
        int k = (int) ((zp - DomainOffset[2]) * oneOverDz);
        partid += k * MAX_PART_NUMBER * MAX_PART_NUMBER;
      }
      #endif

      particle->setPartid(partid);

      logDebug( "Init()", "dist = " << dist );
      logDebug( "Init()", "v_ini = " << v_ini );
      logDebug( "Init()", "Particle: " << particle->toString() );

      #if PeanoDebug > 0
      // TODO MLADEN: REMOVE THIS ONCE YOU'RE SATISFIED.

      int max_reasonable;
      if (HYDRO_DIMENSIONS == 1)
        max_reasonable = MAX_PART_NUMBER + 1;
      else if (HYDRO_DIMENSIONS == 2)
        max_reasonable = (MAX_PART_NUMBER + 1) * (MAX_PART_NUMBER + 1);
      else
        max_reasonable = (MAX_PART_NUMBER + 1) * (MAX_PART_NUMBER + 1) * (MAX_PART_NUMBER + 1);

      assertion2(partid <= max_reasonable, partid, max_reasonable);
      assertion1(partid >= 0, partid);

      #endif

      /* End of inserted block */

    """
        else:
            print("dimensions not supported!")
