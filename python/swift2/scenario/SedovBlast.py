
class SedovBlastIC:
    """
    Initialisation snippet for the Sedov Blast problem in different spatial dimensions.
    Returns a string with C++ code.
    """

    def __init__(self):

        self.initialisation_snippet = """

      /*
       * Code inserted via the python initialisation snippet library.
       * Do not modify unless you know what you are doing.
       * Template located in python/swift2/scenario/SedovBlast.py
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

      // Internal search radius used by Peano and with max value R = grid_size/2
      tarch::la::Vector<Dimensions,double> GridSize = marker.h();
      const double GridSize_x = 0.5 * GridSize(0);

      particle->setSearchRadius( 0.999 * GridSize_x);

      /* Constant background density field */
      particle->setDensity ( 1.0 );
      particle->setMass( particle->getDensity() * std::pow((DomainSize(0) / (double) HYDRO_PART_NUMBER) , HYDRO_DIMENSIONS) );

      /* Initial estimate for smoothing length */
      const double eta_fac = particle->getEtaFactor();
      particle->setSmoothingLength( eta_fac * std::pow(particle->getMass()/particle->getDensity(), 1.0 / HYDRO_DIMENSIONS) );

      // Initialise the blast
      const double BLAST_ENERGY = 1.0;
      const double SPIKE_RADIUS = 2.001 / HYDRO_PART_NUMBER;

      tarch::la::Vector<Dimensions,double> blast_center = 0.5 * DomainSize + DomainOffset;

      const double dist_blast = tarch::la::norm2( particle->getX() - blast_center );

      /* Default method */
      /* Inject energy evenly into group of central particles */
      if ( tarch::la::smallerEquals( dist_blast, SPIKE_RADIUS ) ) {
        particle->setU( BLAST_ENERGY / 12.0 / particle->getMass() );
      } else {
        particle->setU( 1e-6 );  // negligible background value
      }

      /* Compute the pressure from the EoS */
      const double pressure = ::swift2::kernels::legacy::eos::gas_pressure_from_internal_energy( particle->getDensity(), particle->getU() );
      particle->setPressure( pressure );

      /* Compute the sound speed */
      const double soundspeed = ::swift2::kernels::legacy::eos::gas_soundspeed_from_internal_energy( particle->getU() );
      particle->setSoundSpeed( soundspeed );

      /* Initialize remaining fields  */

      // Vectors
      for(int d=0; d<Dimensions; d++){
         particle->setV          (d,0);
         particle->setA          (d,0);
      }

      #if Dimensions==3
      for(int d=0; d<Dimensions; d++){
         particle->setRot_v      (d,0);
      }
      #else
      /* For 1D and 2D sims, we only get one component, i.e. scalar */
      particle->setRot_v(0);
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

      #if PeanoDebug > 0

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

      #endif // PeanoDebug > 0


      /* End of inserted block */

    """
