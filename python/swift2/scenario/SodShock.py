# @TODO: add dependence on SPH scheme (only minimal SPH ATM)
# @TODO: add dependence on input parameters, e.g. grid size, HYDRO_DIMENSIONS...


class SodShockIC:
    """
    Initialisation snippet for the Sod shock problem in different spatial dimensions.
    This problem should have been initialized via a HDF5 file which set up
    the relevant variables.
    Here we initialize the rest of arrays which are mostly trivial, but
    also fine-tune the boundary particles.

    Returns a string with C++ code.
    """

    def __init__(self, dimensions):
        if dimensions == 1:
            self.initialisation_snippet = """

      /* Code inserted via the python initialisation snippet library.
       * Do not modify unless you know what you are doing.
       */

      /*
       * Initial Conditions: Sod shockwave.
       * This problem should have been initialized via a HDF5 file which set up
       * the relevant variables.
       * Here we initialize the rest of arrays which are mostly trivial.
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

      particle->setU( 1e-1 );
      particle->setPressure( 1e-1 );
      particle->setSoundSpeed( 1e-1 );

      /* Initialize remaining fields  */

      // Vectors
      for(int d=0; d<Dimensions; d++){
         particle->setV          (d,0);
         particle->setA          (d,0);
//         particle->setA_prev_step(d,0);
      }

      /* For 2D sims, we only get one component */
      particle->setRot_v(0);

      /* Initialise the "extended" arrays */
      particle->setV_full( particle->getV() );
      particle->setU_full( particle->getU() );

      particle->setUDot  (0.0);
//      particle->setUDot_prev_step(0.0);
      particle->setHDot  (0.0);
      particle->setWcount(0.0);
      particle->setNcount(0  );
      particle->setF     (0.0);
      particle->setRho_dh(0.0);
      particle->setWcount_dh(0.0);

      particle->setHLeft (particle->getSmlMin());
      particle->setHRight(particle->getSmlMax());

//      particle->setTimeStepSize(GLOBAL_TIME_STEP_SIZE);
      particle->setTime(0);
      particle->setHasConverged(false);
      particle->setHasNoNeighbours(false);
      particle->setResidual(0.0);
      particle->setIterCount(0);

      // Artificial viscosity scheme
      particle->setDiv_v   (0.0);
      particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
      particle->setBalsara (0.0);

      particle->setPartid(1);

      /* End of inserted block */

      """
        else:
            print("dimensions not supported!")
