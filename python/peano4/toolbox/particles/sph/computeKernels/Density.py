# @TODO: add other SPH schemes besides the minimal SPH one.

# ------------------------------------------------------------------------------
# Calculation of Density and Newton-Raphson for variable smoothing length h
# ------------------------------------------------------------------------------

class ComputeDensityAndUpdateSmoothingLength:
  """
  Compute kernels for the SPH density calculation.
  Non-symmetric case, i.e. only one particle (the `local` particle) is updated.
  """

  def __init__(self, hydroScheme='minimal'):
    self.hydroScheme = hydroScheme
    if hydroScheme == 'minimal' :
      self.touch_particle_first_time_compute_kernel = """

      /* Code inserted via the python snippet library.
       * Do not modify unless you know what you are doing.
       */

      /* This is equivalent to the hydro_init_part function in SWIFT */

      /* @brief Prepares a particle for the density calculation  
       *
       * Zeroes all the relevant arrays in perparation for the sums taking place
       * in the loop over neighbours (local-active set)
       *
       */

      if( 
        localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
        // and not localParticle->getHasConverged()
        and not localParticle->getIsBoundaryParticle() )
        {  

        // we'll accumulate the data throughout mesh/particle traversal
        localParticle->setDensity (0.0);                      
        localParticle->setWcount  (0.0);                      
        localParticle->setRho_dh  (0.0);   
        localParticle->setWcount_dh(0.0);   
        localParticle->setDiv_v   (0.0);   
        localParticle->setRot_v   (0.0);   

        /* Not needed in SWIFT */
        localParticle->setNcount  (0);
        localParticle->setOmega   (0.0);   
      } 
    
      /* End of inserted block */

      """
      #
      # @todo Tobias: It is not clear if this is clever. If we have only particle-particle 
      # interaction kernels, then we can't have an inner while loop per particle. So teh 
      # corsett might not be beneficial performance-wisely.
      #
      self.particle_particle_compute_kernel = """

        /* Code inserted via the python snippet library.
         * Do not modify unless you know what you are doing.
         */

        /*
         * @brief Non-symmetric density calculation (as per Swift terminology). 
         * In this, only the local particle is updated (in the symmetric case, both 
         * are updated at the same time). This compute kernel is inserted by Peano's
         * Python API into the nested loop over active and local particles. So we can
         * assume that those two terms do exist.
         *
         * The present kernel is a sole particle-particle interaction kernel.
         *
         * Here we calculate the density, Omega and drho/dt sums.
         * The latter can be used in the Newton-Raphson iteration.
         *
         * Subscripts a and b represent indices of Local and Active particles, respectively:
         *
         * h_a: Smoothing length of Local particle
         * h_b: Smoothign length of Active particle
         * dx : Vector separating the two particles (x_loc - x_act)
         * r  : Distance between the two particles  (|| dx ||)
         *
         */


        tarch::la::Vector<Dimensions,double> dx = localParticle->getX() - activeParticle->getX();
        const double r = tarch::la::norm2(dx);

        // Velocity terms for viscosity
        tarch::la::Vector<Dimensions,double> dv_ab;
        tarch::la::Vector<Dimensions,double> curlvr(0.0);

        // Retrieve basic properties of local and active particles
        const double rho_b = activeParticle->getDensity();

        const double m_a = localParticle ->getMass();  
        const double m_b = activeParticle->getMass();  

        const double h_a = localParticle->getSmoothingLength();

        // Normalized distance to evaluate kernel
        const double q_a = r / h_a;
        
        // Get actual interaction radius
        const double cutoffR = KERNEL_SUPPORT_RAD*h_a;

        if(
          tarch::la::smaller( r, cutoffR ) 
          and 
          tarch::la::greater( r, 0.0 )         /* Self-contribution is calculated elsewhere */  
      //  and not localParticle->getHasConverged()
          and 
          not localParticle->getIsBoundaryParticle() ) 
          {  

          // Evaluate kernel and its derivative

/*
          // PHANTOM code version
          double w_a, dw_a_dq;
          kernel_deval(q_a, w_a, dw_a_dq);

          // Normalization factors 
          double W_a, dW_a_dh;
          W_a = w_a / std::pow( h_a, HYDRO_DIMENSIONS );
          dW_a_dh = - ( HYDRO_DIMENSIONS * w_a + q_a * dw_a_dq ) / std::pow( h_a, HYDRO_DIMENSIONS +1 );
*/

          /* SWIFT KERNEL */
          double W_a, dW_a_dh;
          double w, dw_dh;
          kernel_deval_SWIFT(q_a, w, dw_dh);
          W_a = w / std::pow( h_a, HYDRO_DIMENSIONS );
          dW_a_dh = - ( HYDRO_DIMENSIONS * w + q_a * dw_dh ) / std::pow( h_a, HYDRO_DIMENSIONS + 1 );

          /* Increment density */
          localParticle->setDensity( localParticle-> getDensity() + m_b * W_a ); 

          /* Increment fractional number density */
          localParticle->setWcount ( localParticle-> getWcount () +       W_a );
          
          /* Debug: Increment number of detected neighbours */
          localParticle->setNcount ( localParticle-> getNcount () + 1 ); 

          /* Increment drho/dh */
          localParticle->setRho_dh   ( localParticle->getRho_dh()    + m_b*dW_a_dh ); 

          /* Increment Omega */
          /* Notice that this is the same as drho_dh in SWIFT */
          localParticle->setOmega    ( localParticle->getOmega()     + m_b*dW_a_dh );

          /* Increment dWcount/dh */
          localParticle->setWcount_dh( localParticle->getWcount_dh() +     dW_a_dh ); 

          /* Now we need to compute the div terms */
          const double r_inv = r ? 1.0 / r : 0.0;     
          const double fac_a = m_a * dw_a_dq * r_inv;

          /* Compute dv dot r */
          dv_ab = localParticle->getV() - activeParticle->getV();
          const double dvdr = tarch::la::dot( dv_ab , dx );
          localParticle->setDiv_v( localParticle->getDiv_v() - fac_a * dvdr );
          
          /* Compute dv cross r */
          /* 2D Term only has 1 component (z) */
          const double curlvr2D = dv_ab[0]*dx[1] - dv_ab[1]*dx[0];
          localParticle->setRot_v( localParticle->getRot_v() + fac_a * curlvr2D );

          assertion3( not std::isnan( localParticle->getOmega() ), localParticle->toString(), localParticle->getOmega(), dW_a_dh );
        } 

        /* End of inserted block */

        """
      self.touch_particle_last_time_compute_kernel = """

      /* This is equivalent to the hydro_end_density function in SWIFT */

      /* @brief Finishes the calculations from the density loop
       *
       * Multiplies the denstiy and number of neighbours by the appropiate
       * constants and add the self-contribution term. Additional quantities
       * such as the velocity gradients will also get the final terms added to
       * them here.
       *
       * Also adds/multiplies the cosmological terms if need be. @TODO: add.
       *
       */

      if(
        localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
    //  and not localParticle->getHasConverged()
        and 
        not localParticle->getIsBoundaryParticle() )
        {

        /* Retrieve some properties */
        const double m_a = localParticle->getMass();
        const double h_a = localParticle->getSmoothingLength();

        /* Some smoothing length multiples */
        const double h_inv = 1.0 / h_a;
        const double h_inv_dim = std::pow( h_inv, HYDRO_DIMENSIONS );
        const double h_inv_dim_plus_one = h_inv_dim * h_inv;


        /* SWIFT KERNEL */
        double W_a, w, dw_dh;
        kernel_deval_SWIFT(0, w, dw_dh);
        W_a = w;

        /* Final operation. Add the self-contributions (i.e. at q=0) */
        localParticle->setDensity  ( localParticle->getDensity()   + m_a * W_a * h_inv_dim );
        localParticle->setWcount   ( localParticle->getWcount()    +       W_a * h_inv_dim );
        localParticle->setRho_dh   ( localParticle->getRho_dh()    + m_a * (-HYDRO_DIMENSIONS * W_a * h_inv_dim_plus_one) );
        localParticle->setWcount_dh( localParticle->getWcount_dh() +       (-HYDRO_DIMENSIONS * W_a * h_inv_dim_plus_one) );

        /* Not needed in SWIFT */
        localParticle->setNcount   ( localParticle->getNcount()    + 1 );
        localParticle->setOmega    ( localParticle->getOmega()     + m_a * (-HYDRO_DIMENSIONS * W_a * h_inv_dim_plus_one) );

        const double rho_inv = 1.0 / localParticle->getDensity();
        // @TODO: implement cosmological factors
        const double a_inv2 = 1.0;

        /* Finish calculation of the velocity curl components */
        localParticle->setRot_v( localParticle->getRot_v() * h_inv_dim_plus_one * a_inv2 * rho_inv);

        /* Finish calculation of the velocity divergence */
        localParticle->setDiv_v( localParticle->getDiv_v() * h_inv_dim_plus_one * a_inv2 * rho_inv);
        
        
        //
        // =====================
        // prepare forces
        // =====================
        //
        {
          /* Retrieve basic properties of local and active particles */
          const double rho_a = localParticle->getDensity();
          const double m_a = localParticle->getMass();  
          const double h_a = localParticle->getSmoothingLength();
     
          const double h_inv = 1.0 / h_a;
        
          /* Compute the norm of the curl */
          double curl_v = 0;

          if (HYDRO_DIMENSIONS == 2){  /* in 2D sims Rot_v is a scalar */
            curl_v = std::sqrt( localParticle->getRot_v() * localParticle->getRot_v() );
          } else {
            curl_v = tarch::la::norm2( localParticle->getRot_v() );
          }
        
          /* Compute the norm of div v */
          const double abs_div_v = tarch::la::abs( localParticle->getDiv_v() );
          
          /* Compute the sound speed */
          const double soundspeed = gas_soundspeed_from_internal_energy( localParticle->getU() );
        
          /* Construct the final expression of the grad-h-term.
           * Swift use "f" (see also Springel and Hernquist). 
           * PHANTOM use Omega, which is proportional to ~ 1/f. (Price et al 2018) */
         
          const double dh_drho_a = - h_a / rho_a / HYDRO_DIMENSIONS;
          const double common_factor = h_a / HYDRO_DIMENSIONS / localParticle->getWcount();
          
          double grad_h_term;
          const double grad_W_term = common_factor * localParticle->getWcount_dh();
          
          // Avoid problematic particles
          if(grad_W_term < -0.9999){
               /* if we get here, we either had very small neighbour contributions
                 (which should be treated as a no neighbour case in the ghost) or
                 a very weird particle distribution (e.g. particles sitting on
                 top of each other). Either way, we cannot use the normal
                 expression, since that would lead to overflow or excessive round
                 off and cause excessively high accelerations in the force loop */
            grad_h_term = 0;

            /* Verbose */
            logInfo( "touchVertexLastTime()", "Setting grad_h_term=0 for part:  " << localParticle->toString() );

          } else {

            grad_h_term = common_factor * localParticle->getRho_dh()/(1.0 + grad_W_term);

          }
        
          /* Compute the Balsara Switch */
          const double balsara = ALPHA_AV * abs_div_v /
                                 (abs_div_v + curl_v + 0.0001 * soundspeed * h_inv);
                                     
          assertion6( std::isfinite(balsara), localParticle->toString(), balsara, abs_div_v, 
                                              curl_v, soundspeed, ALPHA_AV );
        
          /* Update variables */
          const double P_a = gas_pressure_from_internal_energy( rho_a, localParticle->getU() );
          
          /* SWIFT */ 
          localParticle->setF( grad_h_term );      
          
          /* Price: finish calculation of Omega */
          localParticle->setOmega( 1.0 - dh_drho_a * localParticle->getOmega() ); 
          /* Re-set if gone ill */
          if( tarch::la::smallerEquals( localParticle->getOmega(), 0.0) ){
            localParticle->setOmega(1.0);
          }

          localParticle->setPressure(P_a);
          localParticle->setSoundSpeed(soundspeed);
          localParticle->setBalsara(balsara);

        } /* End hydro_prepare_force */
        
        
        //
        // =============================
        // Newton's Apples
        // =============================
        //
        {
          const double timeStepSize = GLOBAL_TIME_STEP_SIZE;
          const double hydro_eta_dim = std::pow(SMOOTH_L_FAC, HYDRO_DIMENSIONS);
          const double eps = H_TOLERANCE; 

          /* h value at the start of iteration procedure 
           * Here we use the last update on previous time-step */
          const double h_0 = localParticle->getSmoothingLengthPrevious();

          /* Set initial guess for NR */
//          const double h_init = h_0;
          // Improved initial guess
          const double h_init = h_0 + localParticle->getHDot() * timeStepSize;

          /* Get some useful values */
          double h_old = localParticle->getSmoothingLength();

          if( localParticle->getIterCount() == 0 ){    /* use initial guess h_init */
            h_old = h_init;
          }

          const double h_old_dim = std::pow( h_old, HYDRO_DIMENSIONS );
          const double h_old_dim_minus_one = std::pow( h_old, HYDRO_DIMENSIONS - 1 );
          
          double h_new;

          /* Check that smoothing length from previous iteration is non-zero */
          assertion3( h_old > tarch::la::NUMERICAL_ZERO_DIFFERENCE, localParticle->toString(), h_init, h_0 );
        
          if( localParticle->getWcount() < 1e-5 ) { /* No neighbours case */

            /* Flag that there were no neighbours */
            localParticle->setHasNoNeighbours(true);

            /* Verbose */
            logInfo( "touchVertexLastTime()", "no neighbour case for particle " << localParticle->toString() );

            /* Double h and try again */
            h_new = 2.0 * h_old;

          } else {

            /* Compute one step of the Newton-Raphson scheme */
            
            /* SWIFT's calcualtion based on Wcount */

            const double n_sum = localParticle->getWcount() * h_old_dim;
            const double n_target = hydro_eta_dim;
            const double f = n_sum - n_target;
            const double f_prime = localParticle->getWcount_dh() * h_old_dim +
                                      HYDRO_DIMENSIONS * localParticle->getWcount() * h_old_dim_minus_one;

            
            /* PHANTOM code version */

/*
            const double one_over_dh_drho_a = - HYDRO_DIMENSIONS * localParticle->getDensity() / h_old;
            const double rho_sum = localParticle->getDensity();
            const double rho_target = m_a * std::pow( SMOOTH_L_FAC / h_old, HYDRO_DIMENSIONS);
            const double f = rho_sum - rho_target; // Price et al 2018 sign
            const double f_prime = - one_over_dh_drho_a * localParticle->getOmega(); // Opposite sign of Price et al 2018
*/

            // Debug
            assertion6( f_prime !=0, localParticle->toString(), f_prime, h_new, h_old, HYDRO_h_MIN, HYDRO_h_MAX );       

            /* Skip if h is already h_max and we don't have enough neighbours */
            /* Same if we are below h_min */
            /* Same if we have hit the Peano cutoff radius 
             */

            tarch::la::Vector<Dimensions,double> GridSize = marker.h();
            const double GridSizeHalf = 0.5 * GridSize(0); 
            const double CutoffParticleH = KERNEL_SUPPORT_RAD * localParticle->getSmoothingLength();
     
            if ( ((localParticle->getSmoothingLength() >= HYDRO_h_MAX) && (f < 0.0)) ||
                 ((localParticle->getSmoothingLength() <= HYDRO_h_MIN) && (f > 0.0)) ||
                   tarch::la::greaterEquals(CutoffParticleH, GridSizeHalf) ){

              /* We have a particle whose smoothing length is already set (wants
               * to be larger but has already hit the maximum OR wants to be
               * smaller but has already reached the minimum). So, just tidy up
               * as if the smoothing length had converged correctly  */

              /* Verbose about this particle */
              logInfo( "touchVertexLastTime()", " This particle is unable to shrink/grow anymore:  " << localParticle->toString() );

              /* Re-set the value from previous time-step */
              h_new = h_0;
            }

            /* Normal case: 
             * We are able to use Newton-Raphson to get a better value of h */

            /* Avoid floating point exception from f_prime = 0 */
            h_new = h_old - f / ( f_prime + tarch::la::NUMERICAL_ZERO_DIFFERENCE );
              
            /* Safety check: truncate to the range [ h_old/2 , 2h_old ]. */
            h_new = std::min( h_new, 2.0 * h_old );
            h_new = std::max( h_new, 0.5 * h_old );

          }   /* end of if( localParticle->getWcount() < 1e-5 ) else block */

          /* By now we have got h_new for either of the yes/no neighbour cases */

          /* Check convergence */
          const double residual = std::abs(h_new - h_old) / h_0;
          localParticle->setResidual ( residual );

          if(residual > eps){ /* this particle has not converged */
      
            /* Ok, correct then */

            localParticle->setSmoothingLength( h_new );

            /* Increase the iteration counter */
            localParticle->setIterCount( localParticle->getIterCount() + 1 );
      
          } else {
            
            /* No need to update */

            /* However, if this is iter 0, we still update */
            if (localParticle->getIterCount() == 0) {
              localParticle->setSmoothingLength( h_new );
            }

            /* Flag that particle has already converged */
            localParticle->setHasConverged( true );

          }

        } /* End of Newton-Raphson */

      } /* End of localParticle->getParallelState()==globaldata::Particle::ParallelState::Local ... */

      /* End of inserted block */

      """
    else:
      print('Hydro Scheme not supported for this kernel!')


