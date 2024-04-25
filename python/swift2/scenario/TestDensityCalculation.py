class TestDensityCalculation:
    """
    Initialisation snippet for testing the calculation of the SPH density.
    Returns a string with C++ code.
    """

    def __init__(self):
        self.initialisation_snippet = """

    /*
     * Most of the initialised variables are dummy (i.e. not used)
     */

    // Internal search radious used by Peano and with max value R = grid_size/2

    // Internal search radius used by Peano and with max value R = grid_size/2
    tarch::la::Vector<Dimensions,double> GridSize = marker.h();
    const double GridSize_x = GridSize(0);

    particle->setSearchRadius( 0.9 * GridSize_x / 2.0);

    particle->setDensity ( 1.0 );
    particle->setMass( particle->getDensity() / std::pow(HYDRO_PART_NUMBER, HYDRO_DIMENSIONS) );

    particle->setU( 1e-1 );
    particle->setUDot( 0 );
    particle->setU_full( 1e-1 );
    particle->setPressure( 1e-1 );
    particle->setSoundSpeed( 1e-1 );

    /* Compute initial smoothing length */
    const double eta_fac = particle->getEtaFactor();
    particle->setSmoothingLength( eta_fac * std::pow(particle->getMass()/particle->getDensity(), 1.0 / HYDRO_DIMENSIONS) );
    particle->setSmoothingLengthPrevious( particle->getSmoothingLength() );

    /* Initialize remaining fields  */

    // Vectors
    for(int d=0; d<Dimensions; d++){
       particle->setV          (d,0);
       particle->setV_full     (d,0);
       particle->setA          (d,0);
    }

    particle->setRot_v(0);
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
    particle->setCellHasUpdatedParticle(false);
    particle->setResidual(0.0);
    particle->setIterCount(0);

    // Artificial viscosity scheme
    particle->setDiv_v   (0.0);
    particle->setV_sig_AV(2.0 * particle->getSoundSpeed());
    particle->setBalsara (0.0);

    particle->setPartid(1);

    /* End of inserted block */

    """
