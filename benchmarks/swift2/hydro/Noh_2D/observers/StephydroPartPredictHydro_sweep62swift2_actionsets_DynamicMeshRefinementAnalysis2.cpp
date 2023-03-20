#include "StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2.h"


tarch::logging::Log benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::_log( "benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2");



std::list< peano4::grid::GridControlEvent >  benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::_newGridControlEvents;



benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2(int treeNumber) {
// @todo Should be overwritten
}


benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::~StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::prepareTraversal() {

}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::unprepareTraversal() {

::swift2::commitGridControlEvents( _newGridControlEvents );
_newGridControlEvents.clear();
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::beginTraversal(
      ) {

  _localGridControlEvents.clear();

}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::endTraversal(
      ) {

  static tarch::multicore::BooleanSemaphore semaphore;
  tarch::multicore::Lock lock(semaphore);

  _newGridControlEvents.insert( _newGridControlEvents.end(), _localGridControlEvents.begin(), _localGridControlEvents.end() );
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::createPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::destroyPersistentVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::createHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::destroyHangingVertex(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::touchVertexFirstTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::touchVertexLastTime(
      const peano4::datamanagement::VertexMarker& marker,
      benchmarks::swift2::noh::vertexdata::hydroPartSet& fineGridVertexhydroPartSet,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  fineGridCellhydroPartSetCellStatistics.setNumberOfParticles( std::numeric_limits<int>::max() );
  fineGridCellhydroPartSetCellStatistics.setParentOfRefinedCell( true );
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {


}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  fineGridCellhydroPartSetCellStatistics.setNumberOfParticles(0);
  fineGridCellhydroPartSetCellStatistics.setParentOfRefinedCell( false );
}


void benchmarks::swift2::noh::observers::StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> fineGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& fineGridCellhydroPartSetCellStatistics,
      peano4::datamanagement::VertexEnumerator<benchmarks::swift2::noh::vertexdata::hydroPartSet> coarseGridVerticeshydroPartSet,
      benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& coarseGridCellhydroPartSetCellStatistics) {

  /*
  
   Local particles
   ===============
   
   Determine local number of particles associated with the adjacent
   vertices.
  
   */
  int count = 0;
  for (int i=0; i<TwoPowerD; i++) {
    count += fineGridVerticeshydroPartSet(i).size();
  }
  
  
  /*
  
   Determine local counter
   =======================
   
   We might already have received data from finer cells (if we are 
   in a refined cell), so we add the data. We might also be in a 
   newly generated cell, where adding new particles might cause an
   overflow. So I protect the accumulation.
  
   */
  if ( fineGridCellhydroPartSetCellStatistics.getNumberOfParticles() < std::numeric_limits<int>::max() ) {
    fineGridCellhydroPartSetCellStatistics.setNumberOfParticles(
      fineGridCellhydroPartSetCellStatistics.getNumberOfParticles()
      +
      count
    );
  }
  
  /*
  
   Restrict the particle count to the next coarser level
   =====================================================
  
   */
  coarseGridCellhydroPartSetCellStatistics.setNumberOfParticles(
    coarseGridCellhydroPartSetCellStatistics.getNumberOfParticles() 
    +
    fineGridCellhydroPartSetCellStatistics.getNumberOfParticles()
  );

  if ( marker.willBeRefined() ) {
    coarseGridCellhydroPartSetCellStatistics.setParentOfRefinedCell( true );
  }
  
  /*
    
   Erase
   =====
   
   If we want to erase a cell, we actually make the region that is to be 
   erased slightly bigger. While we can pick this blow-up factor arbitrarily
   (up to a factor of three might make sense in some cases), but if we make 
   is 2.0, then an erased cell covers half of its neighbour. If we have a 
   cell which hosts a particle, this cell will then maybe not coarsen, but 
   all of its neighbours do, so the cell is eliminated. This triggers a 
   mesh oscillation. So the factor should either be smaller than 2.0, or we
   have to veto that an area with particles is overwritten. Indeed we go 
   down the latter route and mark cells that we may not erase with a refine
   event which describes the mesh status quo.
         
   
   This is a tricky part. Our idea is that we mark only examine refined cells
   one level above the finest mesh. These are cells which will be refined and
   have been refined, and also carry only few particles such that we could
   safely erase their children. While we only examine refined cells, we do not
   examine refined cells which have refined children. To find this out, we
   look at the flag getParentOfRefinedCell().
   
  */
  if (
    not marker.willBeRefined()
    and
    fineGridCellhydroPartSetCellStatistics.getNumberOfParticles() > 20
    and
    marker.h()(0)/3.0>0.001
  ) {
    const double ScaleMarker = 0.8;
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
    newEntry.setOffset( marker.getOffset() + marker.h() * (1.0-ScaleMarker) / 2.0 );
    newEntry.setWidth( marker.h() * ScaleMarker );
    newEntry.setH( marker.h()/3.0 * 1.1 );
    _localGridControlEvents.push_back(newEntry);
    logDebug( "touchCellLastTime(...)", "add " << newEntry.toString() );
  }

  if (
    marker.willBeRefined() 
    and 
    marker.hasBeenRefined()
    and
    fineGridCellhydroPartSetCellStatistics.getNumberOfParticles() <= 0
    and
    not fineGridCellhydroPartSetCellStatistics.getParentOfRefinedCell()
    and
    marker.h()(0)<0.1 // we will remove the suboctants
  ) {
    const double ScaleMarker = 3.1;
    peano4::grid::GridControlEvent newEntry;
    newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Erase );
    newEntry.setOffset( marker.getOffset() + marker.h() * (1.0-ScaleMarker) / 2.0 );
    newEntry.setWidth( marker.h() * ScaleMarker );
    newEntry.setH( marker.h() * ScaleMarker );
    _localGridControlEvents.push_back(newEntry);
    logDebug( "touchCellLastTime(...)", "add " << newEntry.toString() );
  }
}


