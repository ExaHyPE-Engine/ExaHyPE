#include "Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1.h"


tarch::logging::Log examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::_log( "examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1");





examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1(int treeNumber) {
// @todo Should be overwritten
}


examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::~Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::beginTraversal(
      ) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::endTraversal(
      ) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      examples::particles::vertexdata::ParticleSet& fineGridVertexParticleSet,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {

  if ( not marker.hasBeenRefined() and not marker.willBeRefined() ) {
    #if Dimensions==2
    tarch::la::Vector<2,double> domainOffset = { 0.0, 0.0 };
    #else
    tarch::la::Vector<3,double> domainOffset = { 0.0, 0.0,  };
    #endif
    auto newParticles = toolbox::particles::createParticlesAlignedWithGlobalCartesianMesh<globaldata::Particle>(
      0.03125,
      marker.x(),
      marker.h(),
      domainOffset,
      false
    );
    for (auto& particle: newParticles) {
      
 
  /*
   * Initial Conditions for the particular SPH test
   * 
   * 1. Sedov Blast: particles are initialised at rest in regular grid configuration and periodic BC. 
   * Energy is injected to a (or group of) central particle and the blast should propagate
   * isotropically, up to boundary effects.
   * See, e.g., Springel and Hernquist, Mon. Not. R. Astron. Soc. 333, 649–664 (2002).
   *
   */
   
  // Internal cutoff radious used by Peano. Should always be larger than actual SPH cutoff radious (2h).
  particle->setCutOffRadius(0.16);
  
  particle->setDensity ( 1.0 );        
  particle->setMass    ( particle->getDensity()/400/2.56069 );       
//  particle->setMass    ( particle->getDensity()/400/2.56069/14.0509 ); // mesh_size=0.5/60       
  particle->setPressure( 1e-6);
  
  particle->setSmoothingLength( SMOOTH_L_FAC * std::pow(particle->getMass()/particle->getDensity(), 1.0/Dimensions) );
  particle->setSmoothingLengthPrevious( 0.99 * particle->getSmoothingLength() );
 
  // Initialise the blast
  tarch::la::Vector<Dimensions,double> blast_cent(BLAST_CENTER);
  const double dist_blast = tarch::la::norm2( particle->getX() - blast_cent );
  
 // /* Old method */
 // /* Inject energy evenly into central particles */
 // if ( tarch::la::smaller( dist_blast, SPIKE_RADIUS ) ) {
 //   particle->setU( BLAST_ENERGY );
 
   /* New method */
   /* Smooth the explosion with kernel */
   const double h = particle->getSmoothingLength();
   const double R_smooth = 4.0 * h;
   
   // Evaluate kernel
   const double q = dist_blast / h;
   double w, dw_dq;
   kernel_deval(q, w, dw_dq);
 
   // Normalization factors 
   const double W = w / std::pow( h, Dimensions );
 
  if ( tarch::la::smallerEquals( dist_blast, R_smooth ) ) {
    particle->setU( BLAST_ENERGY * W ) ;
  } else {
    particle->setU( 0.0 ); 
  }
  
  /* Set non-zero (background) value in order to get non-zero sound speed across the whole domain */
  if( tarch::la::equals(particle->getU(),0) ){
    particle->setU( particle->getPressure()/( (GAMMA-1) * particle->getDensity() ) ); 
  }
  
  // Evaluate (global) time-step size using u from exploding particle (h_ini is homogeneous)
  const double soundspeed = std::sqrt( GAMMA * particle->getPressure()/particle->getDensity() );
  particle->setSoundSpeed(soundspeed);
  
  /* Initialize remaining fields  */
  
  // Vectors
  for(int d=0; d<Dimensions; d++){
     particle->setV          (d,0);
     particle->setA          (d,0);
     particle->setA_prev_step(d,0);
     particle->setV_prev_step(d,0);
  }
 
  /* For 2D sims, we only get one component */
  particle->setRot_v(0);
  
  /* Initialise the "extended" arrays */
  particle->setx_full(particle->getX());
  particle->setV_full(particle->getV());
  particle->setU_full(particle->getU());

  particle->setUDot  (0.0);        
  particle->setUDot_prev_step(0.0);        
  particle->setHDot  (0.0);        
  particle->setWcount(0.0);        
  particle->setNcount(0  );        
  particle->setOmega (0.0);        
  particle->setRho_dh(0.0);           
  particle->setWcount_dh(0.0);  
  
  particle->setTimeStepSize(GLOBAL_TIME_STEP_SIZE);
  particle->setTime(0.0);
  particle->setHasConverged(false);
  particle->setHasBeenKicked(false);
  particle->setResidual(0.0);
  particle->setIterCount(0);

  // Artificial viscosity scheme
  particle->setDiv_v   (0.0);
//  particle->setV_sig_AV(0.0);
  particle->setV_sig_AV(2 * particle->getSoundSpeed());
  particle->setBalsara (0.0);
 
 

    }
    // just insert them; will be re-assigned then anyway
    fineGridVerticesParticleSet(0).insert( fineGridVerticesParticleSet(0).end(), newParticles.begin(), newParticles.end() );
  }
}


void examples::particles::observers::Init2peano4_toolbox_particles_InsertParticlesAlongCartesianLayoutIntoUnrefinedCells1::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> fineGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& fineGridCellParticleSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<examples::particles::vertexdata::ParticleSet> coarseGridVerticesParticleSet,
      examples::particles::celldata::ParticleSetCellStatistics& coarseGridCellParticleSetCellStatistics) {


}


