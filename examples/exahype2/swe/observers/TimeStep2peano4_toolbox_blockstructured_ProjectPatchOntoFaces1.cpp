#include "TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1.h"


tarch::logging::Log examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::_log( "examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1");





examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1(int treeNumber) {
}


examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::~TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1() {
}


std::vector< peano4::grid::GridControlEvent > examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::beginTraversal(
      ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::endTraversal(
      ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::createPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::destroyPersistentFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::createHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::destroyHangingFace(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::touchFaceFirstTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::touchFaceLastTime(
      const peano4::datamanagement::FaceMarker& marker,
      examples::exahype2::swe::facedata::SWEQ& fineGridFaceSWEQ,
      examples::exahype2::swe::facedata::SWEQNew& fineGridFaceSWEQNew,
      examples::exahype2::swe::facedata::Label& fineGridFaceLabel,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::createCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> fineGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> fineGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> fineGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& fineGridCellSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::destroyCell(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> fineGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> fineGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> fineGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& fineGridCellSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::touchCellFirstTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> fineGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> fineGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> fineGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& fineGridCellSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {


}


void examples::exahype2::swe::observers::TimeStep2peano4_toolbox_blockstructured_ProjectPatchOntoFaces1::touchCellLastTime(
      const peano4::datamanagement::CellMarker& marker,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> fineGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> fineGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> fineGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& fineGridCellSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQ> coarseGridFacesSWEQ,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::SWEQNew> coarseGridFacesSWEQNew,
      peano4::datamanagement::FaceEnumerator<examples::exahype2::swe::facedata::Label> coarseGridFacesLabel,
      examples::exahype2::swe::celldata::SWEQ& coarseGridCellSWEQ) {

  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= 1.0*2;
      }
      else {
        base *= 11;
      }
    }
    return result;
  };
  
  if ( not marker.isRefined() ) {
    logTraceIn( "touchCellLastTime(...)" );
  
    for(int d=0; d<Dimensions; d++) {
      /**
       * d-loop over all dimensions except d. The vector k's entry d is set
       * to 0. We start with the left/bottom face, i.e. the one closer to the 
       * coordinate system's origin.
       */
      dfore(k,11,d,0) {
        for (int i=0; i<1.0; i++) {
          tarch::la::Vector<Dimensions,int> patchCell   = k;
          tarch::la::Vector<Dimensions,int> overlapCell = k;
          patchCell(d)   = i;
          overlapCell(d) = i+1.0;
          
          int patchCellSerialised   = peano4::utils::dLinearised(patchCell,11);
          int overlapCellSerialised = serialisePatchIndex(overlapCell,d);
          for (int j=0; j<4; j++) {
            fineGridFacesSWEQNew(d).value[overlapCellSerialised*4+j] = 
              fineGridCellSWEQ.value[patchCellSerialised*4+j];
          }
  
          patchCell(d)   = i+11-1.0;
          overlapCell(d) = i;
          
          patchCellSerialised   = peano4::utils::dLinearised(patchCell,11);
          overlapCellSerialised = serialisePatchIndex(overlapCell,d);
          for (int j=0; j<4; j++) {
            fineGridFacesSWEQNew(d+Dimensions).value[overlapCellSerialised*4+j] = 
              fineGridCellSWEQ.value[patchCellSerialised*4+j];
          }
        }
      }
    }
    logTraceOut( "touchCellLastTime(...)" );
  }

}


