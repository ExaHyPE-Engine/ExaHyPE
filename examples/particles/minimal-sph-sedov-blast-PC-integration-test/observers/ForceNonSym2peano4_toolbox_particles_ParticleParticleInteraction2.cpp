#include "ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2.h"


tarch::logging::Log examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::_log( "examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2");





examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2(int treeNumber) {
// @todo Should be overwritten
}


examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::~ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::beginTraversal(
      ) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::endTraversal(
      ) {

  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    

  /* Done by hydro_reset_acceleration in Swift */
  
  if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

    localParticle->setA   (0.0);   
    localParticle->setUDot(0.0);   
  
  }


  }
}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    

   /* Here we do what "hydro_end_force" does in SWIFT */

   if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local){

     localParticle->setHDot( 
       localParticle->getHDot() * localParticle->getSmoothingLength() / Dimensions);
  
   }


  }
}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  if ( true ) {
    std::list< globaldata::Particle* >  localParticles;
    _numberOfActiveParticlesAdded.push_back(0);
    for (int i=0; i<TwoPowerD; i++) {
      for (auto& p: fineGridVerticesParticleSet(i) ) {
        _activeParticles.push_front( p );
        _numberOfActiveParticlesAdded.back()++;
      }
      for (auto& p: fineGridVerticesParticleSet(i) ) {
        bool append = marker.isContained( p->getX() );
        if (append) {
          localParticles.push_front( p );
        }
      }
    }

    std::list< globaldata::Particle* >&  activeParticles = _activeParticles;
    for (auto& localParticle: localParticles )
    for (auto& activeParticle: activeParticles ) {
      
  
/*
 * Non-symmetric force interaction between two particles (as per Swift terminology). 
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
 
  if ( tarch::la::smaller( r,cutoffR ) ) {
    if ( tarch::la::greater( r,0.0 ) ) {

      // Evaluate kernel and its derivative for particle a
      double w_a, dw_a_dq;
      kernel_deval(q_a, w_a, dw_a_dq);

      // Evaluate kernel and its derivative for particle b
      double w_b, dw_b_dq;
      kernel_deval(q_b, w_b, dw_b_dq);

      // Normalization factors 
      double wa_dr = dw_a_dq / std::pow( h_a, Dimensions + 1 );
      double wb_dr = dw_b_dq / std::pow( h_b, Dimensions + 1 );

      /* Variable smoothing length term */
      const double f_ab = 1.0 - localParticle ->getOmega()/m_b;
      const double f_ba = 1.0 - activeParticle->getOmega()/m_a;

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

      /* Update the signal velocity */
      localParticle->setV_sig_AV( std::max(localParticle->getV_sig_AV(), v_sig) );
    }
  }
  

    }
  }
}


void examples::particles::observers::ForceNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  for (int i=0; i<_numberOfActiveParticlesAdded.back(); i++) {
    _activeParticles.pop_back();
  }
  _numberOfActiveParticlesAdded.pop_back();
}


