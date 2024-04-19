// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractCCZ4SBH_FV.h"

#include "MulticoreOrchestration.h"

#include "tarch/logging/Log.h"

namespace benchmarks {
  namespace exahype2 {
    namespace ccz4 {
      class CCZ4SBH_FV;
#ifdef PureFV
      extern TP::TwoPunctures* twoPunctures;

      void prepareTwoPunctures();
#endif
    } // namespace ccz4
  }   // namespace exahype2
} // namespace benchmarks



class benchmarks::exahype2::ccz4::CCZ4SBH_FV: public benchmarks::exahype2::ccz4::AbstractCCZ4SBH_FV {
private:
  static tarch::logging::Log                _log;
  static tarch::multicore::BooleanSemaphore _semaphore;

  int _numberOfPatches;

public:
  /**
   * Initialise the two punctures object if required
   */
  CCZ4SBH_FV();

  virtual void initialCondition(
    double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    bool                                         gridIsConstructed
  ) override;

  /**
   * Overwrite limiters time step size
   *
   * The FV solver calculates its admissible time step size in finishTimeStep().
   * So you can always overwrite it in startTimeStep(). Due to stability
   * reasons, you should always reduce the admissible time step size but
   * never ever increase it.
   */
  void reduceAdmissibleTimeStepSize(double timeStepSize);

  /**
   * Is octant area overlapping with BH impact area
   *
   * Any unrefined octant overlapping with the impact area should hold a FV
   * solution. Delete logic to isCellOverlappingWithBHImpactArea().
   */
  static bool isCellOverlappingWithBHImpactArea(const peano4::datamanagement::CellMarker& marker);

  static bool areAllFaceConnectedCellsOverlappingWithBHImpactArea(const peano4::datamanagement::CellMarker& marker);

  /**
   * Check two adjacent octants
   *
   * Each face has to adjacent octants. We check if isCellOverlappingWithBHImpactArea()
   * holds for both of them. Delete logic to isCellOverlappingWithBHImpactArea().
   */
  static bool areBothAdjacentCellsOverlappingWithBHImpactArea(const peano4::datamanagement::FaceMarker& marker);

  static bool isOneAdjacentCellOverlappingWithBHImpactArea(const peano4::datamanagement::FaceMarker& marker);

  /**
   * Check if the faceNumberth adjacent face is adjacent to inside cells
   *
   * So first of all, we check if the current cell is overlapping with the BH
   * area. If this is not the case, we can return false immediately.
   * Otherwise, we can move one cell left, right, up, down, front or left and
   * check for this cell as well.
   */
  static bool areBothAdjacentCellsOverlappingWithBHImpactArea(
    const peano4::datamanagement::CellMarker& marker, int faceNumber
  );

  /**
   * Call superclass and after that report/maintain _numberOfPatches.
   */
  virtual void startTimeStep(
    double globalMinTimeStamp, double globalMaxTimeStamp, double globalMinTimeStepSize, double globalMaxTimeStepSize
  ) override;

  /**
   * Call superclass and after that report/maintain _numberOfPatches.
   */
  virtual void finishTimeStep() override;

  void incNumberOfPatches();

private:
  static bool isCellOverlappingWithBHImpactArea(
    const tarch::la::Vector<Dimensions, double>& cellCentre, const tarch::la::Vector<Dimensions, double>& cellH
  );
};
