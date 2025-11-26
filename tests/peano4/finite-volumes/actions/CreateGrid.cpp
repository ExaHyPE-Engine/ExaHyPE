// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CreateGrid.h"

#include "Constants.h"
#include "peano4/utils/Loop.h"

tarch::logging::Log tests::peano4::finitevolumes::actions::CreateGrid::_log(
  "tests::peano4::finitevolumes::actions::CreateGrid"
);

tests::peano4::finitevolumes::actions::CreateGrid::CreateGrid(int treeNumber) {}

tests::peano4::finitevolumes::actions::CreateGrid::~CreateGrid() {}

std::vector<::peano4::grid::GridControlEvent> tests::peano4::finitevolumes::actions::CreateGrid::getGridControlEvents(
) const {
  std::vector<::peano4::grid::GridControlEvent> controlEvents;
  ::peano4::grid::GridControlEvent              newEvent;
  newEvent.setRefinementControl(::peano4::grid::GridControlEvent::RefinementControl::Refine);
#if DIMENSIONS == 2
  newEvent.offset = {0.0, 0.0};
  newEvent.width  = {1.0, 1.0};
  newEvent.h      = {MinH, MinH};
#else
  newEvent.offset = {0.0, 0.0, 0.0};
  newEvent.width  = {1.0, 1.0, 1.0};
  newEvent.h      = {MinH, MinH, MinH};
#endif
  controlEvents.push_back(std::move(newEvent));
  return controlEvents;
}

void tests::peano4::finitevolumes::actions::CreateGrid::prepareTraversal() {}

void tests::peano4::finitevolumes::actions::CreateGrid::unprepareTraversal() {}

void tests::peano4::finitevolumes::actions::CreateGrid::beginTraversal() {}

void tests::peano4::finitevolumes::actions::CreateGrid::endTraversal() {}

void tests::peano4::finitevolumes::actions::CreateGrid::createPersistentFace(
  const ::peano4::datamanagement::FaceMarker&                                         marker,
  tests::peano4::finitevolumes::facedata::Q&                                          fineGridFaceQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  ::tests::peano4::finitevolumes::celldata::Q&                                        coarseGridCellQ
) {
#if DIMENSIONS == 2
  int NumberOfEntries = PatchSize * 2;
#else
  int NumberOfEntries = PatchSize * PatchSize * 2;
#endif
  for (int i = 0; i < NumberOfEntries; i++) {
    for (int unknowns = 0; unknowns < NumberOfUnknownsPerCell; unknowns++) {
      fineGridFaceQ.data()[i * NumberOfUnknownsPerCell + unknowns] = 0.0;
    }
  }
}

void tests::peano4::finitevolumes::actions::CreateGrid::destroyPersistentFace(
  const ::peano4::datamanagement::FaceMarker&                                         marker,
  tests::peano4::finitevolumes::facedata::Q&                                          fineGridFaceQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {}

void tests::peano4::finitevolumes::actions::CreateGrid::createHangingFace(
  const ::peano4::datamanagement::FaceMarker&                                         marker,
  tests::peano4::finitevolumes::facedata::Q&                                          fineGridFaceQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {}

void tests::peano4::finitevolumes::actions::CreateGrid::destroyHangingFace(
  const ::peano4::datamanagement::FaceMarker&                                         marker,
  tests::peano4::finitevolumes::facedata::Q&                                          fineGridFaceQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {}

void tests::peano4::finitevolumes::actions::CreateGrid::touchFaceFirstTime(
  const ::peano4::datamanagement::FaceMarker&                                         marker,
  tests::peano4::finitevolumes::facedata::Q&                                          fineGridFaceQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {}

void tests::peano4::finitevolumes::actions::CreateGrid::touchFaceLastTime(
  const ::peano4::datamanagement::FaceMarker&                                         marker,
  tests::peano4::finitevolumes::facedata::Q&                                          fineGridFaceQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {}

void tests::peano4::finitevolumes::actions::CreateGrid::createCell(
  const ::peano4::datamanagement::CellMarker&                                         marker,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> fineGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          fineGridCellQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {
  const double cellWidth    = marker.getH()(0);
  const double subcellWidth = cellWidth / PatchSize;
  dfor(k, PatchSize) {
    const tarch::la::Vector<DIMENSIONS, double>
      subcellCentre = marker.getX() - 0.5 * cellWidth + tarch::la::convertScalar<double>(k) * subcellWidth
                      + 0.5 * subcellWidth;
    int dofIndex = ::peano4::utils::dLinearised(k, PatchSize) * NumberOfUnknownsPerCell;

    fineGridCellQ.data()[dofIndex + 0] = 1.0; // rho
    fineGridCellQ.data()[dofIndex + 1] = 0;   // velocities
    fineGridCellQ.data()[dofIndex + 2] = 0;
    fineGridCellQ.data()[dofIndex + 3] = 0;

    if (tarch::la::norm2(subcellCentre - tarch::la::Vector<DIMENSIONS, double>(0.5)) < 0.03) {
      fineGridCellQ.data()[dofIndex + 4] = 1.0; // inner energy
    } else {
      fineGridCellQ.data()[dofIndex + 4] = 0.0;
    }
  }
}

void tests::peano4::finitevolumes::actions::CreateGrid::destroyCell(
  const ::peano4::datamanagement::CellMarker&                                         marker,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> fineGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          fineGridCellQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {}

void tests::peano4::finitevolumes::actions::CreateGrid::touchCellFirstTime(
  const ::peano4::datamanagement::CellMarker&                                         marker,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> fineGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          fineGridCellQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {}

void tests::peano4::finitevolumes::actions::CreateGrid::touchCellLastTime(
  const ::peano4::datamanagement::CellMarker&                                         marker,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> fineGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          fineGridCellQ,
  ::peano4::datamanagement::FaceEnumerator<tests::peano4::finitevolumes::facedata::Q> coarseGridFacesQ,
  tests::peano4::finitevolumes::celldata::Q&                                          coarseGridCellQ
) {}
