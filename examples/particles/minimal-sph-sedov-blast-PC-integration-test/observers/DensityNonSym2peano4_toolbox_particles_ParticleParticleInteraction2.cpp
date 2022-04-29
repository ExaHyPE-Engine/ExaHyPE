#include "DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2.h"


tarch::logging::Log examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::_log( "examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2");





examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2(int treeNumber) {
// @todo Should be overwritten
}


examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::~DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::beginTraversal(
      ) {


}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::endTraversal(
      ) {

  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    

  if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
//  and localParticle->getHasConverged()==false 
  ) {  

    // we'll accumulate the data throughout mesh/particle traversal
    localParticle->setDensity (0.0);                      
    localParticle->setWcount  (0.0);                      
    localParticle->setOmega   (0.0);   
    localParticle->setRho_dh  (0.0);   
    localParticle->setWcount_dh(0.0);   
    localParticle->setPressure(0.0);  
    localParticle->setDiv_v   (0.0);   
    localParticle->setRot_v   (0.0);   
    localParticle->setNcount  (0);

  } 
    


  }
}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    

  /* Finalize the calculations from the density loop */

  if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
//  and localParticle->getHasConverged()==false 
  ) {  

    /* Retrieve some properties */
    const double m_a = localParticle->getMass();
    const double h_a = localParticle->getSmoothingLength();
    
    const double h_inv_dim = 1.0/std::pow( h_a, Dimensions );
    const double h_inv_dim_plus_one = h_inv_dim/h_a;

    // Default SPH kernel (cubic spline)
    double W_a = CUBIC_SPLINE_SIGMA ; // kernel form taken at q=0
      
    /* Add the contributions due to the particle itself (i.e. at q=0) */

    localParticle->setDensity  ( localParticle->getDensity()   + m_a * W_a * h_inv_dim );                      
    localParticle->setWcount   ( localParticle->getWcount()    +       W_a * h_inv_dim );                      
    localParticle->setOmega    ( localParticle->getOmega()     + m_a * (-Dimensions * W_a * h_inv_dim_plus_one) );   
    localParticle->setRho_dh   ( localParticle->getRho_dh()    + m_a * (-Dimensions * W_a * h_inv_dim_plus_one) );   
    localParticle->setWcount_dh( localParticle->getWcount_dh() +       (-Dimensions * W_a * h_inv_dim_plus_one) );
    localParticle->setNcount   ( localParticle->getNcount()    + 1 );

    const double rho_inv = 1.0 / localParticle->getDensity();

    /* Finish calculation of the velocity curl components */
    localParticle->setRot_v( localParticle->getRot_v() * h_inv_dim_plus_one * rho_inv);

    /* Finish calculation of the velocity divergence */
    localParticle->setDiv_v( localParticle->getDiv_v() * h_inv_dim_plus_one * rho_inv);
  
  }
  
  

  }
}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellFirstTime(
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
  * Non-symmetric density calculation (as per Swift terminology). 
  * In this, only the Local particle is updated (in the symmetric case, both are updated at the same time).
  *
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

  if (tarch::la::smaller( r, cutoffR ) 
  and tarch::la::greater( r, 0.0     )            /* Self-contribution is calculated elsewhere */  
//  and localParticle->getHasConverged()==false
  ) {  
     
      // Evaluate kernel and its derivative
      double w_a, dw_a_dq;
      kernel_deval(q_a, w_a, dw_a_dq);

      // Normalization factors 
      double W_a, dW_a_dh;
      W_a = w_a / std::pow( h_a, Dimensions );
      dW_a_dh = - ( Dimensions*w_a + q_a*dw_a_dq ) / std::pow( h_a, Dimensions +1 );

      // Increment density of Local particle
      localParticle->setDensity( localParticle-> getDensity() + m_b * W_a ); 

      // Increment number density as well
      localParticle->setWcount ( localParticle-> getWcount () +       W_a );
      
      // Increment exact number of neighbours (for debugging)
      localParticle->setNcount ( localParticle-> getNcount () + 1 ); 

      // Increment drho/dh
      localParticle->setRho_dh   ( localParticle->getRho_dh()    + m_b*dW_a_dh ); 

      // Increment Omega of Local particle
      localParticle->setOmega    ( localParticle->getOmega()     + m_b*dW_a_dh );

      // Increment dWcount/dh
      localParticle->setWcount_dh( localParticle->getWcount_dh() +     dW_a_dh ); 

      assertion3( not std::isnan( localParticle->getOmega() ), localParticle->toString(), localParticle->getOmega(), dW_a_dh );
      
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

    } 
    

    }
  }
}


void examples::particles::observers::DensityNonSym2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellLastTime(
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


