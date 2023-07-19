#include "CCZ4SBH_FV.h"

#include "CCZ4SBH_FD4.h"
#include "exahype2/RefinementControl.h"
#include "repositories/SolverRepository.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "tarch/multicore/Lock.h"

tarch::multicore::BooleanSemaphore benchmarks::exahype2::ccz4::CCZ4SBH_FV::_semaphore;
tarch::logging::Log benchmarks::exahype2::ccz4::CCZ4SBH_FV::_log("benchmarks::exahype2::ccz4::CCZ4SBH_FV");

benchmarks::exahype2::ccz4::CCZ4SBH_FV::CCZ4SBH_FV() { prepareTwoPunctures(); }

void benchmarks::exahype2::ccz4::CCZ4SBH_FV::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructred);

  applications::exahype2::ccz4::ApplyTwoPunctures(Q, volumeCentre, 0, twoPunctures, not gridIsConstructred);

  logTraceOut("initialCondition(...)");
}

void benchmarks::exahype2::ccz4::CCZ4SBH_FV::reduceAdmissibleTimeStepSize(double timeStepSize) {
  assertion(timeStepSize <= getAdmissibleTimeStepSize());
  _admissibleTimeStepSize = timeStepSize;
}

bool benchmarks::exahype2::ccz4::CCZ4SBH_FV::isCellOverlappingWithBHImpactArea(
  const peano4::datamanagement::CellMarker& marker
) {
  return isCellOverlappingWithBHImpactArea(marker.x(), marker.h());
}

bool benchmarks::exahype2::ccz4::CCZ4SBH_FV::areAllFaceConnectedCellsOverlappingWithBHImpactArea(
  const peano4::datamanagement::CellMarker& marker
) {
  if (isCellOverlappingWithBHImpactArea(marker)) {
    bool result = true;
    for (int d = 0; d < Dimensions; d++) {
      tarch::la::Vector<Dimensions, double> neighbourCellCentre = marker.x();

      neighbourCellCentre(d) = marker.x()(d) - marker.h()(d);
      result &= isCellOverlappingWithBHImpactArea(neighbourCellCentre, marker.h());

      neighbourCellCentre(d) = marker.x()(d) + marker.h()(d);
      result &= isCellOverlappingWithBHImpactArea(neighbourCellCentre, marker.h());
    }
    return result;
  } else {
    return false;
  }
}

bool benchmarks::exahype2::ccz4::CCZ4SBH_FV::areBothAdjacentCellsOverlappingWithBHImpactArea(
  const peano4::datamanagement::CellMarker& marker, int faceNumber
) {
  if (isCellOverlappingWithBHImpactArea(marker)) {
    const int                             normal             = faceNumber % Dimensions;
    tarch::la::Vector<Dimensions, double> adjacentCellCentre = marker.x();
    adjacentCellCentre(normal) += faceNumber >= Dimensions ? marker.h()(normal) : -marker.h()(normal);
    return isCellOverlappingWithBHImpactArea(adjacentCellCentre, marker.h());
  } else {
    return false;
  }
}

bool benchmarks::exahype2::ccz4::CCZ4SBH_FV::areBothAdjacentCellsOverlappingWithBHImpactArea(
  const peano4::datamanagement::FaceMarker& marker
) {
  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  tarch::la::Vector<Dimensions, double> leftCellCentre  = marker.x();
  tarch::la::Vector<Dimensions, double> rightCellCentre = marker.x();

  leftCellCentre(normal)  -= 0.5 * marker.h()(normal);
  rightCellCentre(normal) += 0.5 * marker.h()(normal);

  return isCellOverlappingWithBHImpactArea(leftCellCentre, marker.h())
     and isCellOverlappingWithBHImpactArea(rightCellCentre, marker.h());
}

bool benchmarks::exahype2::ccz4::CCZ4SBH_FV::isOneAdjacentCellOverlappingWithBHImpactArea(
  const peano4::datamanagement::FaceMarker& marker
) {
  const int normal = marker.getSelectedFaceNumber() % Dimensions;

  tarch::la::Vector<Dimensions, double> leftCellCentre  = marker.x();
  tarch::la::Vector<Dimensions, double> rightCellCentre = marker.x();

  leftCellCentre(normal)  -= 0.5 * marker.h()(normal);
  rightCellCentre(normal) += 0.5 * marker.h()(normal);

  return isCellOverlappingWithBHImpactArea(leftCellCentre, marker.h())
      or isCellOverlappingWithBHImpactArea(rightCellCentre, marker.h());
}

bool benchmarks::exahype2::ccz4::CCZ4SBH_FV::isCellOverlappingWithBHImpactArea(
  const tarch::la::Vector<Dimensions, double>& cellCentre, const tarch::la::Vector<Dimensions, double>& cellH
) {
  return tarch::la::norm2(cellCentre) - tarch::la::norm2(0.5 * cellH) < BlackHoleFVRegion;
}

void benchmarks::exahype2::ccz4::CCZ4SBH_FV::startTimeStep(
  double globalMinTimeStamp, double globalMaxTimeStamp, double globalMinTimeStepSize, double globalMaxTimeStepSize
) {
  AbstractCCZ4SBH_FV::startTimeStep(
    globalMinTimeStamp, globalMaxTimeStamp, globalMinTimeStepSize, globalMaxTimeStepSize
  );

  _numberOfPatches = 0;
}

void benchmarks::exahype2::ccz4::CCZ4SBH_FV::finishTimeStep() {
  AbstractCCZ4SBH_FV::finishTimeStep();

  logInfo("finishTimeStep()", "number of patches on this rank=" << _numberOfPatches);
}

void benchmarks::exahype2::ccz4::CCZ4SBH_FV::incNumberOfPatches() {
  tarch::multicore::Lock lock(_semaphore);
  _numberOfPatches++;
}
