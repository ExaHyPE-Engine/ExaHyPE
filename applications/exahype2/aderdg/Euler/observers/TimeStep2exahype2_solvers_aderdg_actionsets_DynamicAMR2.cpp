#include "TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2.h"


tarch::logging::Log examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::_log( "examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2");





examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2(int treeNumber) {
// @todo Should be overwritten
}


examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::~TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::prepareTraversal() {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::unprepareTraversal() {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::beginTraversal(
      ) {


}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::endTraversal(
      ) {


}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::createPersistentFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::destroyPersistentFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


  if ( repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridInitialisation ) {
  
    const int Order = 3;
    const int NumberOfVariables = 4;
    const int NumberOfParameters = 0;
    const int strideQ = NumberOfVariables+NumberOfParameters;
    
    #if Dimensions==2
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif

    const int basisElementsPerFace  = spaceFaceSize*strideQ;
    const int fluxElementsPerFace   = spaceFaceSize*NumberOfVariables;

    if(!marker.isInteriorFaceWithinPatch()){

      std::fill_n(
        fineGridFaceeuler2DQEstimates.value,
        2*basisElementsPerFace,
        0.0
      );

      //left half of face
      kernels::aderdg::generic::c::singleLevelFaceUnknownsProlongation<
        euler2D, //SolverType,
        double, //pStoreType
        strideQ, //numberOfVariables
        Order+1>(
          fineGridFaceeuler2DQEstimates.value, //double* lQhbndFine
          coarseGridFaceseuler2DQEstimates(marker.getSelectedFaceNumber()).value, //const double* lQhbndCoarse
          marker.getRelativePositionWithinFatherFace() //const tarch::la::Vector<Dimensions-1, int>& subfaceIndex
      );

      //right half of face
      kernels::aderdg::generic::c::singleLevelFaceUnknownsProlongation<
        euler2D, //SolverType
        double, //pStoreType        
        strideQ, //numberOfVariables
        Order+1>(
          fineGridFaceeuler2DQEstimates.value+basisElementsPerFace, //double* lQhbndFine
          coarseGridFaceseuler2DQEstimates(marker.getSelectedFaceNumber()).value+basisElementsPerFace, //const double* lQhbndCoarse
          marker.getRelativePositionWithinFatherFace() //const tarch::la::Vector<Dimensions-1, int>& subfaceIndex
      );

      std::fill_n(
        fineGridFaceeuler2DQFluxEstimates.value,
        2*fluxElementsPerFace,
        0.0
      );

      //left half of face
      kernels::aderdg::generic::c::singleLevelFaceUnknownsProlongation<
        euler2D, //SolverType
        double, //pStoreType        
        NumberOfVariables, //numberOfVariables
        Order+1>(
          fineGridFaceeuler2DQFluxEstimates.value, //double* lQhbndFine
          coarseGridFaceseuler2DQFluxEstimates(marker.getSelectedFaceNumber()).value, //const double* lQhbndCoarse
          marker.getRelativePositionWithinFatherFace() //const tarch::la::Vector<Dimensions-1, int>& subfaceIndex
      );

      //right half of face
      kernels::aderdg::generic::c::singleLevelFaceUnknownsProlongation<
        euler2D, //SolverType
        double, //pStoreType        
        NumberOfVariables, //numberOfVariables
        Order+1>(
          fineGridFaceeuler2DQFluxEstimates.value+fluxElementsPerFace, //double* lQhbndFine
          coarseGridFaceseuler2DQFluxEstimates(marker.getSelectedFaceNumber()).value+fluxElementsPerFace, //const double* lQhbndCoarse
          marker.getRelativePositionWithinFatherFace() //const tarch::la::Vector<Dimensions-1, int>& subfaceIndex
      );

   }

  }

    
}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


  if(true and repositories::InstanceOfeuler2D.getSolverState()==euler2D::SolverState::Prediction){

    const int Order = 3;
    const int NumberOfVariables = 4;
    const int NumberOfParameters = 0;
    const int strideQ = NumberOfVariables+NumberOfParameters;
    
    #if Dimensions==2
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif

    const int basisElementsPerFace  = spaceFaceSize*strideQ;
    const int fluxElementsPerFace   = spaceFaceSize*NumberOfVariables;

    int facePosition = marker.getSelectedFaceNumber()<Dimensions ? 1 : 0;

    kernels::aderdg::generic::c::singleLevelFaceUnknownsRestriction<
      euler2D, //SolverType
      double, //pStoreType      
      strideQ,      //numberOfVariables
      Order+1>(
        coarseGridFaceseuler2DQEstimates(marker.getSelectedFaceNumber()).value+facePosition*basisElementsPerFace, //const double* lQhbndCoarse
        fineGridFaceeuler2DQEstimates.value+facePosition*basisElementsPerFace, //double* lQhbndFine          
        marker.getRelativePositionWithinFatherFace() //const tarch::la::Vector<Dimensions-1, int>& subfaceIndex
    );

    kernels::aderdg::generic::c::singleLevelFaceUnknownsRestriction<
      euler2D, //SolverType
      double, //pStoreType      
      NumberOfVariables,      //numberOfVariables
      Order+1>(
        coarseGridFaceseuler2DQFluxEstimates(marker.getSelectedFaceNumber()).value+facePosition*fluxElementsPerFace, //const double* lQhbndCoarse
        fineGridFaceeuler2DQFluxEstimates.value+facePosition*fluxElementsPerFace, //double* lQhbndFine          
        marker.getRelativePositionWithinFatherFace() //const tarch::la::Vector<Dimensions-1, int>& subfaceIndex
    );

  }

    
}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::euler2D::facedata::euler2DFaceLabel& fineGridFaceeuler2DFaceLabel,
      examples::exahype2::euler2D::facedata::euler2DQEstimates& fineGridFaceeuler2DQEstimates,
      examples::exahype2::euler2D::facedata::euler2DQFluxEstimates& fineGridFaceeuler2DQFluxEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DFaceLabel> coarseGridFaceseuler2DFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQEstimates> coarseGridFaceseuler2DQEstimates,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::euler2D::facedata::euler2DQFluxEstimates> coarseGridFaceseuler2DQFluxEstimates,
      examples::exahype2::euler2D::celldata::euler2DCellLabel& coarseGridCelleuler2DCellLabel,
      examples::exahype2::euler2D::celldata::euler2DQ& coarseGridCelleuler2DQ) {


    if ( not marker.willBeRefined()and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction  and repositories::InstanceOfeuler2D.getSolverState()==euler2D::SolverState::Prediction ) {

      const int Order = 3;
      const int NumberOfVariables = 4;
      const int NumberOfParameters = 0;
      const int strideQ = NumberOfVariables+NumberOfParameters;
      
      #if Dimensions==2
      constexpr int spaceFaceSize      = (Order+1);
      #else
      constexpr int spaceFaceSize      = (Order+1)*(Order+1);
      #endif

      const int basisElementsPerFace = spaceFaceSize*strideQ;
      const int fluxElementsPerFace = spaceFaceSize*NumberOfVariables;

      std::fill_n(fineGridFaceeuler2DQEstimates.value, 2*basisElementsPerFace, 0.0);
      std::fill_n(fineGridFaceeuler2DQFluxEstimates.value, 2*fluxElementsPerFace, 0.0);      

    }

    
}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::touchFaceLastTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::createCell(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::destroyCell(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::touchCellFirstTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_DynamicAMR2::touchCellLastTime(
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


