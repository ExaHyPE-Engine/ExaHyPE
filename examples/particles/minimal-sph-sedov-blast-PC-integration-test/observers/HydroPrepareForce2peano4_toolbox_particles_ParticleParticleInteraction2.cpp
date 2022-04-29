#include "HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2.h"


tarch::logging::Log examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::_log( "examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2");





examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2(int treeNumber) {
// @todo Should be overwritten
}


examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::~HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::beginTraversal(
      ) {


}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::endTraversal(
      ) {

  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    
 
  if(localParticle->getParallelState()==globaldata::Particle::ParallelState::Local
//  and localParticle->getHasConverged()==false 
  ) {  

    /* Retrieve basic properties of local and active particles */
    const double rho_a = localParticle->getDensity();
    const double m_a = localParticle->getMass();  
    const double h_a = localParticle->getSmoothingLength();

    const double h_inv = 1.0/h_a;
  
    /* Compute the norm of the curl */
    /* 2D sims */
    const double curl_v = std::sqrt( localParticle->getRot_v() * localParticle->getRot_v() );
  
    /* Compute the norm of div v */
    const double abs_div_v = tarch::la::abs( localParticle->getDiv_v() );
    
    /* Compute the sound speed */
    const double soundspeed = gas_soundspeed_from_internal_energy( localParticle->getU() );

    // Construct final expression of Omega
    /* Swift Expression for equivalent to Omega term ("f", see also Springel and Hernquist) */
   
    const double dh_drho_a = - h_a / rho_a / Dimensions;
    const double common_factor = h_a / Dimensions / localParticle->getWcount();
    
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
    
    /* Grad-h-term (Omega) */
    /* SWIFT */
    localParticle->setOmega( grad_h_term );  
    localParticle->setPressure(P_a);
    localParticle->setSoundSpeed(soundspeed);
    localParticle->setBalsara(balsara);

  }


  }
}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  auto& localParticles = fineGridVertexParticleSet;
  for (auto& localParticle: localParticles ) {
    

  /* We do not excecute any specific action here */


  }
}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellFirstTime(
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
      
  
  /* We do not excecute any specific action here */  
  

    }
  }
}


void examples::particles::observers::HydroPrepareForce2peano4_toolbox_particles_ParticleParticleInteraction2::touchCellLastTime(
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


