# @TODO: add dependence on SPH scheme (only minimal SPH ATM)
# @TODO: add dependence on input parameters, e.g. grid size, HYDRO_DIMENSIONS...
# @TODO: add dependence for the background pressure degree of freedom (Mach)


class GreshoChanVortexIC:
    """
    Initialisation snippet for the Gresho-Chan vortex in different spatial dimensions.
    Returns a string with C++ code.
    """

    def __init__(self, dimensions):
        if dimensions == 2:
            self.initialisation_snippet = """

      /* Code inserted via the python initialisation snippet library.
       * Do not modify unless you know what you are doing.
       */

      /*
       * Initial Conditions: Gresho-Chan vortex.
       * We give the particles values for the azhimutal velocity and
       * pressure in a spherically-symmetric way.
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

      // Box centre
      tarch::la::Vector<Dimensions,double> x_0(0.5);

      // Distance of particle to centre
      tarch::la::Vector<Dimensions,double> pos = particle->getX();
      tarch::la::Vector<Dimensions,double> dist = pos - x_0;
      double r = tarch::la::norm2(dist);

      // avoid velocity divergences at r=0
      r += 1e-10;

      /* Initial azhimutal velocity */
      double v_phi;

      if( r >= 0 && r < 0.2) {
        v_phi = 5.0 * r;
      } else if( r >= 0.2 && r < 0.4){
        v_phi = 2.0 - 5.0 * r;
      } else {
        v_phi = 0;
      }

      // Set velocity profile
      tarch::la::Vector<Dimensions,double> v_ini;

      // Get x and y particle coordinates
      const double pos_x = pos[0];
      const double pos_y = pos[1];

      v_ini[0] = - v_phi * (pos_y - 0.5) / r;
      v_ini[1] =   v_phi * (pos_x - 0.5) / r;

      particle->setV( v_ini );

      /* Initial pressure profile */
      double pressure;

      if( r >= 0 && r < 0.2) {
        pressure = 5.0 + 12.5 * r * r;
      } else if( r >= 0.2 && r < 0.4){
        pressure = 9.0 + 12.5 * r * r - 20.0 * r + 4.0 * log( 5.0 * r );
      } else {
        pressure = 3.0 + 4.0 * log( 2.0 );
      }

      // Set pressure profile
      particle->setPressure( pressure );

      /* Set U from the EoS */
      double u_ini = ::swift2::legacy::eos::gas_internal_energy_from_pressure( particle->getDensity(), particle->getPressure() );
      particle->setU( u_ini );

      /* Set sound speed */
      const double soundspeed = ::swift2::legacy::eos::gas_soundspeed_from_internal_energy( particle->getU() );
      particle->setSoundSpeed(soundspeed);

      /*
       * Initialize remaining fields
       */

      // Vectors
      for(int d=0; d<Dimensions; d++){
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
      particle->setF     (0.0);
      particle->setRho_dh(0.0);
      particle->setWcount_dh(0.0);

      particle->setHLeft (particle->getSmlMin());
      particle->setHRight(particle->getSmlMax());

      particle->setTimeStepSize(GLOBAL_TIME_STEP_SIZE);
      particle->setHasConverged(false);
      particle->setHasNoNeighbours(false);
      particle->setResidual(0.0);
      particle->setIterCount(0);

      // Artificial viscosity scheme
      particle->setDiv_v   (0.0);
      particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
      particle->setBalsara (0.0);

      /* End of inserted block */

      """
        else:
            print("dimensions not supported!")
