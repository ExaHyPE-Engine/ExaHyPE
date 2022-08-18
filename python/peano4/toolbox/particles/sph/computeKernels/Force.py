# @TODO: add other SPH schemes besides the minimal SPH one (i.e. SPHENIX)

# -----------------------------------------------------------------------------
# Force loop compute kernels 
# -----------------------------------------------------------------------------

class Force:
  """
  Compute kernels for the SPH Force calculation. Non-symmetric case, i.e. only one
  particle (the `local` particle) is updated.
  """

  def __init__(self, hydroScheme='minimal'):
    self.hydroScheme = hydroScheme
    if hydroScheme == 'minimal' :
      self.touch_particle_first_time_compute_kernel = """

      /* Code inserted via the python snippet library.
       * Do not modify unless you know what you are doing.
       */

      /* Done by "hydro_reset_acceleration" in SWIFT */
      
      if( 
        localParticle->getParallelState() == globaldata::Particle::ParallelState::Local
        and 
        not localParticle->getIsBoundaryParticle() ) 
        {

        /* Reset the acceleration*/
        localParticle->setA   (0.0);   
        
        /* Reset the time derivatives */
        localParticle->setUDot(0.0);   
        localParticle->setHDot(0.0);   
        localParticle->setV_sig_AV(2.0 * localParticle->getSoundSpeed());

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
       * @brief Non-symmetric force interaction between two particles (as per Swift terminology). 
       * In this, only the Local particle is updated (in the symmetric case, both are updated at the same time).
       * Time derivative of the internal energy, du/dt, is also updated here.
       * Subscripts a and b represent indices of Local and Active particles, respectively:
       *
       * h_a: Smoothing length of Local particle
       * h_b: Smoothign length of Active particle
       * dx : Vector separating the two particles (x_loc - x_act)
       * r  : Distance between the two particles  (|| dx ||)
       *
       */
      
      
      /* Distance between the two particles */
      tarch::la::Vector<Dimensions,double> dx = localParticle->getX() - activeParticle->getX();
      const double r = tarch::la::norm2(dx);
      const double r_inv = r ? 1.0 / r : 0.0;

      // Retrieve basic properties of local and active particles
      const double rho_a = localParticle ->getDensity();
      const double rho_b = activeParticle->getDensity();
      
      const double P_a = localParticle ->getPressure();
      const double P_b = activeParticle->getPressure(); 
      
      const double m_a = localParticle ->getMass();  
      const double m_b = activeParticle->getMass();  

      // Smoothing lengths
      const double h_a = localParticle ->getSmoothingLength();
      const double h_b = activeParticle->getSmoothingLength();

      // Dimensionless distance to evaluate kernel
      const double q_a = r / h_a;
      const double q_b = r / h_b;
      
      // Get actual interaction radius
      const double cutoffR = KERNEL_SUPPORT_RAD*h_a;
     
      if( 
        tarch::la::smaller( r,cutoffR ) 
        and
        tarch::la::greater( r,0.0 ) 
        and 
        not localParticle->getIsBoundaryParticle() ) 
        {

        // Evaluate kernel and its derivative for particle a
        double w_a, dw_a_dq;
        kernel_deval(q_a, w_a, dw_a_dq);

        // Evaluate kernel and its derivative for particle b
        double w_b, dw_b_dq;
        kernel_deval(q_b, w_b, dw_b_dq);

        // Normalization factors 

/* 
        // PHANTOM CODE version
        double wa_dr = dw_a_dq / std::pow( h_a, HYDRO_DIMENSIONS + 1 );
        double wb_dr = dw_b_dq / std::pow( h_b, HYDRO_DIMENSIONS + 1 );
*/

        /* SWIFT KERNEL */
        double wa, wb;
        double wa_dr, wb_dr;
        double dw_dqa, dw_dqb;

        kernel_deval_SWIFT(q_a, wa, dw_dqa);
        kernel_deval_SWIFT(q_b, wb, dw_dqb);
        wa_dr = dw_dqa / std::pow( h_a, HYDRO_DIMENSIONS + 1 );
        wb_dr = dw_dqb / std::pow( h_b, HYDRO_DIMENSIONS + 1 );

        /* Variable smoothing length term */

/*
        // PHANTOM CODE version
        const double f_ab = 1.0 / localParticle->getOmega() ;
        const double f_ba = 1.0 / activeParticle->getOmega() ;
*/
        
        /* SWIFT "f" */
        const double f_ab = 1.0 - localParticle ->getF()/m_b;
        const double f_ba = 1.0 - activeParticle->getF()/m_a;

        /* Compute gradient terms */
        const double P_over_rho2_a = P_a / (rho_a * rho_a) * f_ab;
        const double P_over_rho2_b = P_b / (rho_b * rho_b) * f_ba;

        /* Compute dv dot r. */
        double dvdr = tarch::la::dot( (localParticle->getV() - activeParticle->getV()), dx );

        /* Are the particles moving towards each others ? */
        const double omega_ab = std::min(dvdr, 0.0);
        const double mu_ab = r_inv * omega_ab; /* This is 0 or negative */
      
        /* Compute sound speeds and signal velocity */
        const double v_sig = localParticle->getSoundSpeed() + activeParticle->getSoundSpeed() -
                             BETA_AV * mu_ab;
      
        /* Balsara term */
        const double balsara_a = localParticle ->getBalsara();
        const double balsara_b = activeParticle->getBalsara();
      
        /* Construct the full viscosity term */
        const double rho_ab = 0.5 * (rho_a + rho_b);
        const double visc = -0.25 * v_sig * mu_ab * (balsara_a + balsara_b) / rho_ab;

        /* Convolve with the kernel */
        const double visc_acc_term = 0.5 * visc * (wa_dr * f_ab + wb_dr * f_ba) * r_inv;

        /* SPH acceleration term */
        const double sph_acc_term = (P_over_rho2_a * wa_dr + P_over_rho2_b * wb_dr) * r_inv;
        
        /* Assemble the acceleration */
        const double acc = sph_acc_term + visc_acc_term;
              
        /* Use the force Luke ! */
        localParticle->setA( localParticle->getA() - m_b * acc * dx );
        assertion8( not tarch::la::isEntryNan( localParticle->getA() ), localParticle->toString(), sph_acc_term, visc_acc_term, f_ab, f_ba,
                    activeParticle->getOmega(), activeParticle->getF(), activeParticle->getIsBoundaryParticle() );
                
        /* Get the time derivative for u. */
        const double sph_du_term_a = P_over_rho2_a * dvdr * r_inv * wa_dr;

        /* Viscosity term */
        const double visc_du_term = 0.5 * visc_acc_term * dvdr;

        /* Assemble the energy equation term */
        const double du_dt_a = sph_du_term_a + visc_du_term;

        /* Internal energy time derivative */
        localParticle->setUDot(localParticle->getUDot() + m_b * du_dt_a);

        /* Get the time derivative for h. */
        localParticle->setHDot(localParticle->getHDot() - m_b * dvdr * r_inv/rho_b * wa_dr * f_ab);
        assertion6( not std::isnan( localParticle->getHDot() ), localParticle->toString(), dvdr, r_inv, wa_dr, f_ab, f_ba );

        /* Update the signal velocity */
        localParticle->setV_sig_AV( std::max(localParticle->getV_sig_AV(), v_sig) );
      }
      
      /* End of inserted block */

        """
      self.touch_particle_last_time_compute_kernel = """

      /* Code inserted via the python snippet library.
       * Do not modify unless you know what you are doing.
       */

      /* Done by "hydro_end_force" does in SWIFT */

      if( 
        localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
        and 
        not localParticle->getIsBoundaryParticle() ) 
        {

        localParticle->setHDot( 
          localParticle->getHDot() * localParticle->getSmoothingLength() / HYDRO_DIMENSIONS );
      
      }

      /* End of inserted block */

      """
    else:
      print('Hydro Scheme not supported for this kernel!')


