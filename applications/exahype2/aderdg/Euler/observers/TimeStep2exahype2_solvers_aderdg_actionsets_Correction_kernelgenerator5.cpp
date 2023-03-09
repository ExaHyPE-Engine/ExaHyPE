#include "TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5.h"


tarch::logging::Log examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::_log( "examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5");





examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5(int treeNumber) {
// @todo Should be overwritten
}


examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::~TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::prepareTraversal() {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::unprepareTraversal() {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::beginTraversal(
      ) {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::endTraversal(
      ) {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::createPersistentFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::destroyPersistentFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::createHangingFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::destroyHangingFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::touchFaceFirstTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::touchFaceLastTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::createCell(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::destroyCell(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::touchCellFirstTime(
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

  if (not marker.willBeRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction and repositories::InstanceOfeuler2D.getSolverState()==euler2D::SolverState::Correction) {
  
    const double timeStamp = fineGridCelleuler2DCellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    
  double timeStepSize = repositories::InstanceOfeuler2D.getAdmissibleTimeStepSize();
    
    
    const int Order = 3;
    const int NumberOfVariables = 4;
    const int NumberOfParameters = 0;
    
    #if Dimensions==2
    constexpr int spaceBasisSize     = (Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceBasisSize     = (Order+1)*(Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif
    
    double* luh = fineGridCelleuler2DQ.value;
    double lduh[generated::kernels::AderDG::nonlinear::getUpdateSize()]{0.0};

    const int basisElementsByFace = generated::kernels::AderDG::nonlinear::getBndFaceSize();
    const int fluxElementsByFace  = generated::kernels::AderDG::nonlinear::getBndFluxSize();
    
    double FL[fluxElementsByFace];
    double FR[fluxElementsByFace];
    
#if Dimensions==2
    double cellCentre[Dimensions] = {marker.x()[0], marker.x()[1]};
    double cellSize[Dimensions]   = {marker.h()[0], marker.h()[1]};
#else
    double cellCentre[Dimensions] = {marker.x()[0], marker.x()[1], marker.x()[2]};  
    double cellSize[Dimensions]   = {marker.h()[0], marker.h()[1], marker.h()[2]};
#endif
    
    /*
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
    
    for(int d=0; d<Dimensions; d++){
    
      //negative face
      double* QL = fineGridFaceseuler2DQEstimates(d).value;
      double* QR = fineGridFaceseuler2DQEstimates(d).value+basisElementsByFace;

      /*
      **  Data from the face is copied into a buffer because operations are directly performed on this
      **  by the kernels and we want the faces to remain unmodified for the neighbour.
      **  The solution elements are only accessed but not used as buffers and therefore no copying is needed.
      */
      std::copy_n(fineGridFaceseuler2DQFluxEstimates(d).value, fluxElementsByFace, FL);
      std::copy_n(fineGridFaceseuler2DQFluxEstimates(d).value +fluxElementsByFace, fluxElementsByFace, FR);
      
      const double inverseDxDirection = 1/marker.h()[d];      
      tarch::la::Vector<Dimensions,double> faceCentre = marker.x();
      faceCentre[d] -= 0.5*marker.h()[d];

      generated::kernels::AderDG::nonlinear::riemannSolver<double, double>(
        repositories::InstanceOfeuler2D,
        FL,
        FR,
        QL,
        QR,
        timeStamp,
        timeStepSize,
        faceCentre,
        marker.h(),
        d //direction
      );

      generated::kernels::AderDG::nonlinear::faceIntegral<double>(
        lduh,
        FR,
        d, //direction
        0, //orientation
        inverseDxDirection
      );


      //positive face
      QL = fineGridFaceseuler2DQEstimates(d+Dimensions).value;
      QR = fineGridFaceseuler2DQEstimates(d+Dimensions).value+basisElementsByFace;
      std::copy_n(fineGridFaceseuler2DQFluxEstimates(d+Dimensions).value, fluxElementsByFace, FL);
      std::copy_n(fineGridFaceseuler2DQFluxEstimates(d+Dimensions).value +fluxElementsByFace, fluxElementsByFace, FR);

      faceCentre[d] += marker.h()[d];
      
      generated::kernels::AderDG::nonlinear::riemannSolver<double, double>(
        repositories::InstanceOfeuler2D,
        FL,
        FR,
        QL,
        QR,
        timeStamp,
        timeStepSize,
        faceCentre,
        marker.h(),
        d //direction
      );

      generated::kernels::AderDG::nonlinear::faceIntegral<double>(
        lduh,
        FL,
        d, //direction
        1, //orientation
        inverseDxDirection
      );

      
    }// for d    
    

    generated::kernels::AderDG::nonlinear::solutionUpdate( 
      luh,
      luh, 
      lduh, 
      timeStepSize
    );


    
  
    double maxEigenvalue = kernels::aderdg::generic::c::maxScaledEigenvalue<
      euler2D
      >(
        repositories::InstanceOfeuler2D,
        luh,
        marker.x(),
        marker.h(),
        timeStamp,
        timeStepSize
      );

    repositories::InstanceOfeuler2D.setMaxEigenvalue( maxEigenvalue );  
    
    // This is a value set for stats reasons. We'll ignore it later as we 
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    //const double newTimeStepSize = 0.0;
    const double newTimeStepSize =  repositories::InstanceOfeuler2D.getMinCellSize(false)>0.0 ?
      repositories::InstanceOfeuler2D.getAdmissibleTimeStepSize() * marker.h()(0) / repositories::InstanceOfeuler2D.getMinCellSize(false) :
      0.0;

  
    fineGridCelleuler2DCellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCelleuler2DCellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCelleuler2DCellLabel.setHasUpdated(true);
      
    repositories::InstanceOfeuler2D.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
   
  }
}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Correction_kernelgenerator5::touchCellLastTime(
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


