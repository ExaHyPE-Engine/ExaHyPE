# @TODO: add dependence on SPH scheme (only minimal SPH ATM)
# @TODO: merge 1D and 2D based on HYDRO_DIMENSIONS input parameter.
class SedovBlastIC:
    """
    Initialisation snippet for the Sedov Blast problem in different spatial dimensions.
    Returns a string with C++ code.
    """

    def __init__(self):
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

      // Internal search radius used by Peano and with max value R = grid_size/2
      tarch::la::Vector<Dimensions,double> GridSize = marker.h();
      const double GridSize_x = GridSize(0);

      particle->setSearchRadius( 0.9 * GridSize_x / 2.0);

      /* Constant density field */
      particle->setDensity ( 1.0 );
      particle->setMass( particle->getDensity() / std::pow(HYDRO_PART_NUMBER, HYDRO_DIMENSIONS) );

      /* Initial estimate for smoothing length */
      const double eta_fac = particle->getEtaFactor();
      particle->setSmoothingLength( eta_fac * std::pow(particle->getMass()/particle->getDensity(), 1.0 / HYDRO_DIMENSIONS) );
      particle->setSmoothingLengthPrevious( particle->getSmoothingLength() );

      // Initialise the blast
      const double BLAST_CENTER = 0.5;
      const double BLAST_ENERGY = 1.0;
      const double SPIKE_RADIUS = 2.0 * (1.0 / HYDRO_PART_NUMBER);

      tarch::la::Vector<Dimensions,double> blast_cent(BLAST_CENTER);
      const double dist_blast = tarch::la::norm2( particle->getX() - blast_cent );

       /* Default method */
       /* Inject energy evenly into group of central particles */
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
      particle->setNcount(0  );
      particle->setF     (0.0);
      particle->setRho_dh(0.0);
      particle->setWcount_dh(0.0);

      particle->setHLeft (particle->getSmlMin());
      particle->setHRight(particle->getSmlMax());

      particle->setHasConverged(false);
      particle->setHasNoNeighbours(false);
      particle->setResidual(0.0);
      particle->setIterCount(0);

      // Artificial viscosity scheme
      particle->setDiv_v   (0.0);
      particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
      particle->setBalsara (0.0);

      /* Verbose */
      logDebug( "Init()", "dist = " << dist );
      logDebug( "Init()", "Particle: " << particle->toString() );

      /* End of inserted block */

    """
