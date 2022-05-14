#include "MyMapping.h"
#include "FaceData.h"
#include "CellData.h"


tarch::logging::Log   examples::integerdiffusionthroughfaces::MyMapping::_log( "examples::integerdiffusionthroughfaces::MyMapping" );



void examples::integerdiffusionthroughfaces::MyMapping::beginTraversal() {
}


void examples::integerdiffusionthroughfaces::MyMapping::endTraversal() {
}


void examples::integerdiffusionthroughfaces::MyMapping::createPersistentFace(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  logTraceIn( "createPersistentFace(...)" );
  data.value = 0;
  logTraceOut( "createPersistentFace(...)" );
}


void examples::integerdiffusionthroughfaces::MyMapping::createHangingFace(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  logTraceIn( "createHangingFace(...)" );
  data.value = 0;
  logTraceOut( "createHangingFace(...)" );
}


void examples::integerdiffusionthroughfaces::MyMapping::destroyPersistentFace(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
}


void examples::integerdiffusionthroughfaces::MyMapping::destroyHangingFace(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
}


void examples::integerdiffusionthroughfaces::MyMapping::touchFaceFirstTime(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  logTraceInWith4Arguments( "touchFaceFirstTime(...)", x, h, normal, data.toString() );
  data.oldValue = data.value;
  data.value    = 0;
  logTraceOut( "touchFaceFirstTime(...)" );
}


void examples::integerdiffusionthroughfaces::MyMapping::touchFaceLastTime(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,double>&  normal,
  FaceData&                                    data
) {
  logTraceIn( "touchFaceLastTime(...)" );
  logTraceOut( "touchFaceLastTime(...)" );
}


void examples::integerdiffusionthroughfaces::MyMapping::createCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
) {
  logTraceIn( "createCell(...)" );
  data.value = 0;
  logTraceOut( "createCell(...)" );
}


void examples::integerdiffusionthroughfaces::MyMapping::destroyCell(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
) {
}


const int MarkerFineGridCell   = 5;
const int MarkerCoarseGridReplication           = MarkerFineGridCell+1;
const int MarkerCoarseGridWithoutData           = MarkerFineGridCell+2;
const int MarkerFineGridCellWhichCanBeDeleted   = MarkerFineGridCell-1;


void examples::integerdiffusionthroughfaces::MyMapping::touchCellFirstTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
) {
  logTraceInWith1Argument( "touchCellFirstTime(...)", data.toString() );

  assertion(false);

/*
  // Integer diffusion - preamble
  data.oldValue = data.value;
  if (data.value==MarkerCoarseGridReplication) {
	int maxOfNeighbours = 0;
    for (int i=0; i<TwoTimesD; i++) {
      maxOfNeighbours = std::max( maxOfNeighbours, faces(i).oldValue-data.value );
    }
    if (maxOfNeighbours>=MarkerCoarseGridReplication) {
      data.oldValue = MarkerCoarseGridWithoutData;
    }
  }

  // Part written by user to identify working grid
  bool stimulus =
    center(0) > 0.4 and center (0) <0.6 and
    center(1) > 0.4 and center (1) <0.6;

  if (
    (stimulus and not marker.isRefined )
	or
    (not stimulus and h(0)<1.0/3.0 and h(0)>=1.0/3.0/3.0)
	or
    (not stimulus and not marker.isRefined and h(0)>=1.0/3.0)
  ) {
    data.value = MarkerFineGridCell;
  }

  // Integer diffusion - epilogue
  if (coarseData.oldValue>=MarkerCoarseGridReplication and data.value<MarkerFineGridCell) {
    data.value = MarkerFineGridCellWhichCanBeDeleted;
  }
  else if (data.value < MarkerFineGridCell) {
    data.value = std::max(data.oldValue-1,0);
    for (int i=0; i<TwoTimesD; i++) {
      data.value = std::max( data.value, faces(i).oldValue-data.oldValue-1 );
    }
  }
*/

  logTraceOutWith1Argument( "touchCellFirstTime(...)", data.toString() );
}


void examples::integerdiffusionthroughfaces::MyMapping::touchCellLastTime(
  const tarch::la::Vector<Dimensions,double>&  center,
  const tarch::la::Vector<Dimensions,double>&  h,
  CellData&                                    data,
  Faces&                                       faces,
  CellData&                                    coarseData,
  Faces&                                       coarseFaces
) {
  logTraceInWith1Argument( "touchCellLastTime(...)", data.toString() );

  for (int i=0; i<TwoTimesD; i++) {
    faces(i).value += data.value;
  }

  if (
    data.value==MarkerFineGridCell
	or
    data.value==MarkerCoarseGridReplication
	or
    data.value==MarkerFineGridCellWhichCanBeDeleted
  ) {
    coarseData.value = MarkerCoarseGridReplication;
    logDebug( "touchCellLastTime(...)", "set coarse cell marker to " << coarseData.toString() );
  }

  logTraceOutWith1Argument( "touchCellLastTime(...)", data.toString() );
}

