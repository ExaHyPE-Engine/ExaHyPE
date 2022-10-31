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

    if( dimensions==1 ):
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

      /* Initialize remaining fields  */
      
      // Vectors
      for(int d=0; d<Dimensions; d++){
         particle->setA          (d,0);
         particle->setA_prev_step(d,0);
      }
     
      /* For 2D sims, we only get one component */
      particle->setRot_v(0);
      
      /* Initialise the "extended" arrays */
      particle->setV_full( particle->getV() );
      particle->setU_full( particle->getU() );

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

      

      /* End of inserted block */

      """
    else:
      print('dimensions not supported!')
