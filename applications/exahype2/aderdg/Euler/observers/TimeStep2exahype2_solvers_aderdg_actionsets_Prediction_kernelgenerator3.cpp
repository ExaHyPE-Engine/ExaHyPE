#include "TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3.h"


tarch::logging::Log examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::_log( "examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3");





examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3(int treeNumber) {
// @todo Should be overwritten
}


examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::~TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3() {
// @todo Should be overwritten
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::getGridControlEvents() const {
return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::prepareTraversal() {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::unprepareTraversal() {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::beginTraversal(
      ) {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::endTraversal(
      ) {

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::createPersistentFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::destroyPersistentFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::createHangingFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::destroyHangingFace(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::touchFaceFirstTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::touchFaceLastTime(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::createCell(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::destroyCell(
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


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::touchCellFirstTime(
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

  if (not marker.willBeRefined() and repositories::InstanceOfeuler2D.getSolverState()!=euler2D::SolverState::GridConstruction and repositories::InstanceOfeuler2D.getSolverState()==euler2D::SolverState::Prediction) {
  
    const double timeStamp = fineGridCelleuler2DCellLabel.getTimeStamp();
    // needs to declare and define timeStepSize
    
  double timeStepSize = repositories::InstanceOfeuler2D.getAdmissibleTimeStepSize();  
  
    double* luh = fineGridCelleuler2DQ.value;
  
    const int Order = 3;
    const int NumberOfVariables  = 4;
    const int NumberOfParameters = 0;
    
    #if Dimensions==2
    constexpr int spaceBasisSize     = (Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceBasisSize     = (Order+1)*(Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif
    
    constexpr int spaceTimeBasisSize = spaceBasisSize*(Order+1);
    

    constexpr int totalSize = generated::kernels::AderDG::nonlinear::getFusedSTPVISize();  
    
    double memory[totalSize] __attribute__((aligned(16)));
        
    double* lQi           = memory + generated::kernels::AderDG::nonlinear::getlQiShift();
    double* lQhi          = memory + generated::kernels::AderDG::nonlinear::getlQhiShift();

    
    double* lFi           = memory + generated::kernels::AderDG::nonlinear::getlFiShift();
    double* lFhi          = memory + generated::kernels::AderDG::nonlinear::getlFhiShift();
        

    
    double* lSi           = nullptr;
    double* lShi          = nullptr;
    
    
    
    double* gradQ         = nullptr;
    
    
    
    double* PSi           = nullptr;
    double* PSderivatives = nullptr;
    
        
    double lduh[generated::kernels::AderDG::nonlinear::getUpdateSize()];
        
    double lQhbnd[generated::kernels::AderDG::nonlinear::getBndFaceTotalSize()]{0.0};
    double lFhbnd[generated::kernels::AderDG::nonlinear::getBndFluxTotalSize()]{0.0};
    //double lGradQhbnd[generated::kernels::AderDG::nonlinear::getBndGradQTotalSize()]{0.0};
    

     
    
    double rhs[(NumberOfVariables+NumberOfParameters)*spaceTimeBasisSize];

    //nonlinear
    int numberOfIterations = generated::kernels::AderDG::nonlinear::fusedSpaceTimePredictorVolumeIntegral<double, double, double>(
        repositories::InstanceOfeuler2D,
        lduh,
        lQhbnd,
        nullptr, //lGradQhbnd,
        lFhbnd,
        lQi,
        rhs,
        lFi,
        lSi,   // for NCP or Source
        lQhi,
        lFhi,
        lShi,  // for NCP or Source
        gradQ, // for NCP or Source or viscousFlux
        nullptr, //gradQAvg, // for viscousFlux
        luh,
        marker.x(),
        marker.h(),
        timeStamp,
        timeStepSize
      );

     

    generated::kernels::AderDG::nonlinear::solutionUpdate( 
      luh,
      luh, 
      lduh, 
      timeStepSize
    );
      
    /*
    Copy the estimates into the face data containers. Note that peano uses a different ordering of
    the faces that the kernels, so these are reordered somewhat.
    The kernels order by dimension, then side of the face whereas peano orders by
    side, then dimension.
    E.g. in 2d the kernels have the order left, right, lower face, upper face
    whereas peano orders: left, lower, right, upper 
    
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
  
    const int basisElementsByFace  = generated::kernels::AderDG::nonlinear::getBndFaceSize();
    const int fluxElementsByFace   = generated::kernels::AderDG::nonlinear::getBndFluxSize();

    for(int d=0; d<Dimensions; d++){
      //Projected solution estimations
      std::copy_n(&lQhbnd[2*d*basisElementsByFace], basisElementsByFace, fineGridFaceseuler2DQEstimates(d).value+basisElementsByFace);
      std::copy_n(&lQhbnd[(2*d+1)*basisElementsByFace], basisElementsByFace, fineGridFaceseuler2DQEstimates(d+Dimensions).value);
      
      //Projected flux estimations
      std::copy_n(&lFhbnd[2*d*fluxElementsByFace], fluxElementsByFace, fineGridFaceseuler2DQFluxEstimates(d).value+fluxElementsByFace);
      std::copy_n(&lFhbnd[(2*d+1)*fluxElementsByFace], fluxElementsByFace, fineGridFaceseuler2DQFluxEstimates(d+Dimensions).value);            

      //Timestamp information for boundary handling
      fineGridFaceseuler2DFaceLabel(d).setNewTimeStamp(1, timeStamp);
      fineGridFaceseuler2DFaceLabel(d+Dimensions).setNewTimeStamp(0, timeStamp);      
    }
  

  }

}


void examples::exahype2::euler2D::observers::TimeStep2exahype2_solvers_aderdg_actionsets_Prediction_kernelgenerator3::touchCellLastTime(
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


