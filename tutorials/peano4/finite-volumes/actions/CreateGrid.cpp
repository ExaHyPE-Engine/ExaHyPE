#include "CreateGrid.h"
#include "Constants.h"
#include "peano4/utils/Loop.h"


tarch::logging::Log examples::finitevolumes::actions::CreateGrid::_log( "examples::finitevolumes::actions::CreateGrid");


examples::finitevolumes::actions::CreateGrid::CreateGrid(int treeNumber) {
// @todo Please implement
}


examples::finitevolumes::actions::CreateGrid::~CreateGrid() {
// @todo Please implement
}


std::vector< peano4::grid::GridControlEvent > examples::finitevolumes::actions::CreateGrid::getGridControlEvents() {
  std::vector< peano4::grid::GridControlEvent > result;

  result.push_back(peano4::grid::GridControlEvent(
    peano4::grid::GridControlEvent::RefinementControl::Refine,
    tarch::la::Vector<Dimensions,double>(0.0),
    tarch::la::Vector<Dimensions,double>(1.0),
    tarch::la::Vector<Dimensions,double>(0.3)
//    tarch::la::Vector<Dimensions,double>(0.1)
  ));

  return result;
}


void examples::finitevolumes::actions::CreateGrid::beginTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "beginTraversal()" );
// @todo Please implement
// logTraceOut( "beginTraversal()" );
}


void examples::finitevolumes::actions::CreateGrid::endTraversal(
      ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "endTraversal()" );
// @todo Please implement
// logTraceOut( "endTraversal()" );
}


void examples::finitevolumes::actions::CreateGrid::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::finitevolumes::facedata::Q& fineGridFaceQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {
  #if Dimensions==2
  int NumberOfEntries = PatchSize * 2;
  #else
  int NumberOfEntries = PatchSize * PatchSize * 2;
  #endif
  for (int i=0; i<NumberOfEntries; i++) {
    for (int unknowns=0; unknowns<NumberOfUnknownsPerCell; unknowns++) {
      fineGridFaceQ.value[i*NumberOfUnknownsPerCell+unknowns] = 0.0;
    }
  }
}


void examples::finitevolumes::actions::CreateGrid::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::finitevolumes::facedata::Q& fineGridFaceQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "destroyPersistentFace()" );
// @todo Please implement
// logTraceOut( "destroyPersistentFace()" );
}


void examples::finitevolumes::actions::CreateGrid::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::finitevolumes::facedata::Q& fineGridFaceQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "createHangingFace()" );
// @todo Please implement
// logTraceOut( "createHangingFace()" );
}


void examples::finitevolumes::actions::CreateGrid::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::finitevolumes::facedata::Q& fineGridFaceQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "destroyHangingFace()" );
// @todo Please implement
// logTraceOut( "destroyHangingFace()" );
}


void examples::finitevolumes::actions::CreateGrid::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::finitevolumes::facedata::Q& fineGridFaceQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchFaceFirstTime()" );
// @todo Please implement
// logTraceOut( "touchFaceFirstTime()" );
}


void examples::finitevolumes::actions::CreateGrid::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::finitevolumes::facedata::Q& fineGridFaceQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchFaceLastTime()" );
// @todo Please implement
// logTraceOut( "touchFaceLastTime()" );
}


void examples::finitevolumes::actions::CreateGrid::createCell(
  const peano4::datamanagement::CellMarker& marker,
  peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> fineGridFacesQ,
  examples::finitevolumes::celldata::Q& fineGridCellQ,
  peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
  examples::finitevolumes::celldata::Q& coarseGridCellQ
) {
  const double cellWidth    = marker.h()(0);
  const double subcellWidth = cellWidth / PatchSize;
  dfor(k,PatchSize) {
    const tarch::la::Vector<Dimensions,double> subcellCentre =
      marker.x() - 0.5*cellWidth + k.convertScalar<double>() * subcellWidth + 0.5 * subcellWidth;
    int dofIndex = peano4::utils::dLinearised(k,PatchSize) * NumberOfUnknownsPerCell;

    fineGridCellQ.value[dofIndex+0] = 1.0;  // rho
    fineGridCellQ.value[dofIndex+1] = 0;    // velocities
    fineGridCellQ.value[dofIndex+2] = 0;
    fineGridCellQ.value[dofIndex+3] = 0;

    if (
      tarch::la::norm2( subcellCentre-tarch::la::Vector<Dimensions,double>(0.5) ) < 0.03
    ) {
      fineGridCellQ.value[dofIndex+4] = 1.0; // inner energy
    }
    else {
      fineGridCellQ.value[dofIndex+4] = 0.0;
    }
  }
}


void examples::finitevolumes::actions::CreateGrid::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> fineGridFacesQ,
      examples::finitevolumes::celldata::Q& fineGridCellQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "destroyCell()" );
// @todo Please implement
// logTraceOut( "destroyCell()" );
}


void examples::finitevolumes::actions::CreateGrid::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> fineGridFacesQ,
      examples::finitevolumes::celldata::Q& fineGridCellQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchCellFirstTime()" );
// @todo Please implement
// logTraceOut( "touchCellFirstTime()" );
}


void examples::finitevolumes::actions::CreateGrid::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> fineGridFacesQ,
      examples::finitevolumes::celldata::Q& fineGridCellQ,
      peano4::datamanagement::FaceEnumerator<examples::finitevolumes::facedata::Q> coarseGridFacesQ,
      examples::finitevolumes::celldata::Q& coarseGridCellQ) {

// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( "touchCellLastTime()" );
// @todo Please implement
// logTraceOut( "touchCellLastTime()" );
}


