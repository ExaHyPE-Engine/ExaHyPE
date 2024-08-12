// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CCZ4SBH_FV.h"

#ifdef PureFV
#else
#include "CCZ4SBH_FD4.h"
#endif

#include "exahype2/RefinementControl.h"
#include "repositories/SolverRepository.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "tarch/multicore/Lock.h"

tarch::multicore::BooleanSemaphore benchmarks::exahype2::ccz4::CCZ4SBH_FV::_semaphore;
tarch::logging::Log benchmarks::exahype2::ccz4::CCZ4SBH_FV::_log("benchmarks::exahype2::ccz4::CCZ4SBH_FV");

#ifdef PureFV
TP::TwoPunctures* benchmarks::exahype2::ccz4::twoPunctures = nullptr;

void benchmarks::exahype2::ccz4::prepareTwoPunctures() {
  if (twoPunctures == nullptr) {
    twoPunctures = new TP::TwoPunctures();

    // first we set the parameter. TODO:find a way to read parameter from python script
    // int swi=0;//0--single black hole, 1--BBH hoc, 2--BBH rotation, 3--GW150914

    twoPunctures->par_b             = 1.0;
    twoPunctures->center_offset[0]  = -1.0;
    twoPunctures->center_offset[1]  = 0.0;
    twoPunctures->center_offset[2]  = 0.0;
    twoPunctures->target_M_plus     = 1.0; // adm mass
    twoPunctures->par_P_plus[0]     = 0.0;
    twoPunctures->par_P_plus[1]     = 0.0;
    twoPunctures->par_P_plus[2]     = 0.0; // linear momentum
    twoPunctures->par_S_plus[0]     = 0.0;
    twoPunctures->par_S_plus[1]     = 0.0;
    twoPunctures->par_S_plus[2]     = 0.0; // spin
    twoPunctures->target_M_minus    = 0.0; // adm mass
    twoPunctures->par_P_minus[0]    = 0.0;
    twoPunctures->par_P_minus[1]    = 0.0;
    twoPunctures->par_P_minus[2]    = 0.0; // linear momentum
    twoPunctures->par_S_minus[0]    = 0.0;
    twoPunctures->par_S_minus[1]    = 0.0;
    twoPunctures->par_S_minus[2]    = 0.0;          // spin
    twoPunctures->grid_setup_method = "evaluation"; // evaluation or Taylor expansion
    twoPunctures->TP_epsilon        = 1e-6;

    twoPunctures->PrintParameters();

    // then solve the equation
    twoPunctures->Run();
  }
}
#endif

benchmarks::exahype2::ccz4::CCZ4SBH_FV::CCZ4SBH_FV() { prepareTwoPunctures(); }

void benchmarks::exahype2::ccz4::CCZ4SBH_FV::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructed
) {
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructed);

  applications::exahype2::ccz4::ApplyTwoPunctures(Q, volumeCentre, 0, twoPunctures, not gridIsConstructed);

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
