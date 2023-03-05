#include "InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3.h"


tarch::logging::Log examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::_log( "examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3");





examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3(int treeNumber) {
// @todo Should be overwritten
}


examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::~InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::getGridControlEvents() const {

    return repositories::refinementControl.getGridControlEvents();
}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::prepareTraversal() {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::unprepareTraversal() {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::beginTraversal(
      ) {

  _localRefinementControl.clear();

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::endTraversal(
      ) {

  repositories::refinementControl.merge( _localRefinementControl );

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> fineGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> fineGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> fineGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& fineGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& fineGridCelleuler2DQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> fineGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> fineGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> fineGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& fineGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& fineGridCelleuler2DQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> fineGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> fineGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> fineGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& fineGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& fineGridCelleuler2DQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {

}


void examples::exahype2::euler2D::observers::InitGrid2exahype2_solvers_aderdg_actionsets_AdaptivityCriterion3::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> fineGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> fineGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> fineGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& fineGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& fineGridCelleuler2DQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {
  
  logTraceInWith2Arguments( "touchCellFirstTime(...)", marker.willBeRefined(), marker.hasBeenRefined() );

  ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();

  if ( 
    not marker.willBeRefined() 
    and
    tarch::la::equals(repositories::InstanceOfeuler2D.getMinTimeStamp(),0.0)  
    and
    tarch::la::greater( tarch::la::max( marker.h() ), repositories::InstanceOfeuler2D.MaxAdmissibleCellH) 
  ) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } 
  else if ( 
    not marker.willBeRefined() and not marker.hasBeenRefined() and not marker.willBeRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction
    and
    not marker.willBeRefined() 
    and
    tarch::la::greater( tarch::la::max( marker.h() ), repositories::InstanceOfeuler2D.MaxAdmissibleCellH) 
  ) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } 
  else if ( not marker.willBeRefined() and tarch::la::equals(repositories::InstanceOfeuler2D.getMinTimeStamp(),0.0)  ) { 
    refinementCriterion = repositories::InstanceOfeuler2D.refinementCriterion(
      fineGridCelleuler2DQ.value,
      marker.x(),
      marker.h(),
      repositories::InstanceOfeuler2D.getMinTimeStamp()
    );
     
    if (refinementCriterion==::exahype2::RefinementCommand::Refine and tarch::la::max( marker.h() ) < repositories::InstanceOfeuler2D.MinAdmissibleCellH ) {
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } 
    else if (refinementCriterion==::exahype2::RefinementCommand::Erase ) {
      logDebug( "touchCellFirstTime(...)", "drop coarsen instructions, as we are in initial grid setup phase" );
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } 
  }
  else if ( 
    not marker.willBeRefined() and not marker.hasBeenRefined() and not marker.willBeRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction
    and
    fineGridCelleuler2DCellLabel.getHasUpdated()
  ) { 
    refinementCriterion = repositories::InstanceOfeuler2D.refinementCriterion(
      fineGridCelleuler2DQ.value,
      marker.x(),
      marker.h(),
      repositories::InstanceOfeuler2D.getMinTimeStamp()
    );
     
    if (refinementCriterion==::exahype2::RefinementCommand::Refine and tarch::la::max( marker.h() ) < repositories::InstanceOfeuler2D.MinAdmissibleCellH ) {
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } 
    else if (refinementCriterion==::exahype2::RefinementCommand::Erase and 3.0* tarch::la::max( marker.h() ) > repositories::InstanceOfeuler2D.MaxAdmissibleCellH ) {
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } 
  }
  else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
    
  _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion, 2 );
  logTraceOutWith1Argument( "touchCellFirstTime(...)", toString(refinementCriterion) );
  
}


