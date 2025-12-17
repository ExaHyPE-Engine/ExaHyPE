// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarks-main.h"

#include "AbstractFVSolver.h"
#include "Constants.h"
#include "DataRepository.h"
#include "EnclaveDataRepository.h"
#include "FVSolver.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/fv/BoundaryConditions.h"
#include "exahype2/fv/PatchUtils.h"
#include "kernels/FVSolver/HandleBoundaryFused.h"
#include "kernels/FVSolver/ProjectPatchFused.h"
#include "kernels/FVSolver/ReconstructPatch.h"
#include "kernels/FVSolver/ReconstructPatchFused.h"
#include "kernels/FVSolver/RiemannSolverFused.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/peano4.h"
#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"
#include "tarch/logging/Log.h"
#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "toolbox/blockstructured/Interpolation.h"
#include "toolbox/blockstructured/Projection.h"
#include "toolbox/blockstructured/Restriction.h"

using namespace benchmarks::exahype2::kernelbenchmarks;

namespace {
  tarch::logging::Log        _log("");
  tarch::timing::Measurement timeStepMeasurement;
  double                     printNextTimeStep                = 0.0;
  bool                       haveReceivedNonCriticalAssertion = false;

  std::vector<peano4::datamanagement::CellMarker> _cells;
  const tarch::la::Vector<DIMENSIONS, double>     CellSize = DomainSize / static_cast<double>(GridLength);

  template <typename T>
  std::string toScientificNotationString(T value, int precision = 4, bool considerSign = false, double threshold = 1e6) {
    std::ostringstream out;
    double             v = static_cast<double>(value);

    if (std::abs(v) < threshold) {
      if (considerSign) {
        out << std::showpos;
      }
      out << value;
      return out.str();
    }

    out << std::scientific << std::setprecision(precision);
    if (considerSign) {
      out << std::showpos;
    }
    out << v;
    return out.str();
  }
} // namespace

void printFinalStatistics(
  const tarch::timing::Measurement& measurement,
  const tarch::timing::Measurement& measurementBoundary,
  const tarch::timing::Measurement& measurementReconstruct,
  const tarch::timing::Measurement& measurementRiemann,
  const tarch::timing::Measurement& measurementProjection
) {
  if (measurementBoundary.getNumberOfMeasurements() > 0) {
    logInfo(
      "printFinalStatistics()",
      "Boundary handling:                       " << measurementBoundary.getAccumulatedValue(
      ) << "s\t" << measurementBoundary.toString()
    );
  }
  if (measurementReconstruct.getNumberOfMeasurements() > 0) {
    logInfo(
      "printFinalStatistics()",
      "Reconstruction:                          " << measurementReconstruct.getAccumulatedValue(
      ) << "s\t" << measurementReconstruct.toString()
    );
  }
  if (measurementRiemann.getNumberOfMeasurements() > 0) {
    logInfo(
      "printFinalStatistics()",
      "Riemann solver:                          " << measurementRiemann.getAccumulatedValue(
      ) << "s\t" << measurementRiemann.toString()
    );
  }
  if (measurementProjection.getNumberOfMeasurements() > 0) {
    logInfo(
      "printFinalStatistics()",
      "Projection:                              " << measurementProjection.getAccumulatedValue(
      ) << "s\t" << measurementProjection.toString()
    );
  }

  if (measurement.getNumberOfMeasurements() > 0) {
    logInfo(
      "printFinalStatistics()",
      "Time stepping:                           " << measurement.getAccumulatedValue(
      ) << "s\t" << measurement.toString()
    );

    const auto globalNumberOfPatches = _cells.size();
    const auto totalNumberOfDoFs     = globalNumberOfPatches
                                   * std::pow(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS)
                                   * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables);

    logInfo(
      "printFinalStatistics()",
      "Average patch updates per second:  " << globalNumberOfPatches / measurement.getValue()
    );
    logInfo(
      "printFinalStatistics()",
      "Average DoF updates per second:    " << totalNumberOfDoFs / measurement.getValue()
    );
  }
}

void plotGrid() {
  repositories::startPlottingStep();
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "solutions/solution-FVSolver-" << counter;

  auto writer = tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    DIMENSIONS,
    snapshotFileName.str(),
    "solutions/solution-FVSolver",
    tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::IndexFileMode::AppendNewData,
    0.5 * (repositories::getMinTimeStamp() + repositories::getMaxTimeStamp())
  );

  auto dataWriter = writer.createCellDataWriter(
    "FVSolverQ",
    AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
    AbstractFVSolver::NumberOfUnknowns,
    PlotDescription,
    "",
    nullptr
  );
  dataWriter->setPrecision(PlotterPrecision);

  for (auto& marker : _cells) {
    if (not marker.hasBeenRefined()) {
      const int patchIndex = writer.plotPatch(marker.getX() - marker.getH() * 0.5, marker.getH());
      int       cellIndex  = dataWriter->getFirstCellWithinPatch(patchIndex);
      int       currentDoF = 0;
      double*   QOut       = DataRepository::getInstance().getCellQ(marker.getX(), marker.getH());
      dfor(k, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch) {
        double* data = QOut + currentDoF;

        dataWriter->plotCell(cellIndex, data);
        cellIndex++;
        currentDoF += AbstractFVSolver::NumberOfUnknowns;
      }
    }
  }
  dataWriter->close();
  writer.writeToFile();
  delete dataWriter;
  repositories::finishPlottingStep();
}

void interpolateBetweenAMRBorder() {
  for (auto& marker : _cells) {
    if (marker.getH()[0] < CellSize[0]) { // Check if cell is in AMR region
      auto parentSize   = marker.getH() * 3.0;
      auto parentCenter = marker.getInvokingParentCellsCentre();

      for (int f = 0; f < TWO_TIMES_D; f++) {
        int normal = f % DIMENSIONS;
        if ((f >= DIMENSIONS and marker.getRelativePositionWithinFatherCell()[normal] == 2)
            or (f < DIMENSIONS and marker.getRelativePositionWithinFatherCell()[normal] == 0)) {
          peano4::datamanagement::FaceMarker faceMarker{};
          faceMarker.relativePositionWithinFatherCell = marker.getRelativePositionWithinFatherCell();
          faceMarker.selectedFace                     = f;
          double* fineGridFace   = DataRepository::getInstance().getFaceQNew(marker.getX(), marker.getH(), f);
          double* coarseGridFace = DataRepository::getInstance().getFaceQNew(parentCenter, parentSize, f);
          toolbox::blockstructured::restrictInnerHalfOfHaloLayer_AoS_inject(
            faceMarker,
            AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
            1, // Halo size
            AbstractFVSolver::NumberOfUnknowns,
            fineGridFace,
            coarseGridFace,
            false
          );
        }
      }
    }
  }
  for (auto& marker : _cells) {
    if (marker.getH()[0] < CellSize[0]) { // Check if cell is in AMR region
      auto parentCenter = marker.getInvokingParentCellsCentre();
      auto parentSize   = marker.getH() * 3.0;

      for (int f = 0; f < TWO_TIMES_D; f++) {
        int normal = f % DIMENSIONS;
        if ((f >= DIMENSIONS and marker.getRelativePositionWithinFatherCell()[normal] == 2)
            or (f < DIMENSIONS and marker.getRelativePositionWithinFatherCell()[normal] == 0)) {
          peano4::datamanagement::FaceMarker faceMarker{};
          faceMarker.relativePositionWithinFatherCell = marker.getRelativePositionWithinFatherCell();
          faceMarker.selectedFace                     = f;
          double* fineGridFace   = DataRepository::getInstance().getFaceQNew(marker.getX(), marker.getH(), f);
          double* coarseGridFace = DataRepository::getInstance().getFaceQNew(parentCenter, parentSize, f);
          toolbox::blockstructured::interpolateHaloLayer_AoS_piecewise_constant(
            faceMarker,
            AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
            1, // Halo size
            AbstractFVSolver::NumberOfUnknowns,
            coarseGridFace,
            fineGridFace
          );
        }
      }
    }
  }
}

void applyInitialConditions() {
  for (auto& marker : _cells) {
    if (not marker.hasBeenRefined()) {
      double* QOut = DataRepository::getInstance().getCellQ(marker.getX(), marker.getH());
      // Initialise cell data with initial conditions.
      dfor(volume, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch) {
        int linearIndex = peano4::utils::dLinearised(volume, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch)
                          * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables);
        FVSolver::initialCondition(
          QOut + linearIndex,
          ::exahype2::fv::getVolumeCentre(
            marker.getX(),
            marker.getH(),
            AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
            volume
          ),
          ::exahype2::fv::getVolumeSize(marker.getH(), AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch),
          true
        );
      }
    }
  }
}

peano4::datamanagement::CellMarker createMarkerFromIndex(const tarch ::la ::Vector<DIMENSIONS, int>& index) {
  tarch::la::Vector<DIMENSIONS, double> cellIndex        = tarch::la::convertScalar<double>(index);
  tarch::la::Vector<DIMENSIONS, double> cellPositionBase = DomainOffset + CellSize / 2.0;
  tarch::la::Vector<DIMENSIONS, double>
    cellPosition = cellPositionBase + tarch::la::multiplyComponents(cellIndex, CellSize);
  peano4::datamanagement::CellMarker marker;
  marker.h      = CellSize;
  marker.centre = cellPosition;
  return marker;
}

std::vector<peano4::datamanagement::CellMarker> createRefinedMarkersFromIndex(
  const tarch ::la ::Vector<DIMENSIONS, int>& index
) {
  tarch::la::Vector<DIMENSIONS, double> cellIndex              = tarch::la::convertScalar<double>(index);
  tarch::la::Vector<DIMENSIONS, double> deeperCellIndex        = cellIndex * 3.0;
  tarch::la::Vector<DIMENSIONS, double> deeperCellSize         = CellSize / 3.0;
  tarch::la::Vector<DIMENSIONS, double> cellPositionBase       = DomainOffset + CellSize / 2.0;
  tarch::la::Vector<DIMENSIONS, double> deepercellPositionBase = DomainOffset + deeperCellSize / 2.0;

  std::vector<peano4::datamanagement::CellMarker> result;

  tarch::la::Vector<DIMENSIONS, double>
    cellPosition = cellPositionBase + tarch::la::multiplyComponents(cellIndex, CellSize);
  peano4::datamanagement::CellMarker marker;
  marker.centre = cellPosition;
  marker.h      = CellSize;
  marker.flags |= peano4::datamanagement::CellMarker::HasBeenRefined;
  result.push_back(marker);

  dfor(offset, 3) {
    tarch::la::Vector<DIMENSIONS, double> deepCellOffset = tarch::la::convertScalar<double>(offset);
    tarch::la::Vector<DIMENSIONS, double> localCellIndex = deepCellOffset + deeperCellIndex;
    tarch::la::Vector<DIMENSIONS, double>
      deeperCellPosition = deepercellPositionBase + tarch::la::multiplyComponents(localCellIndex, deeperCellSize);
    peano4::datamanagement::CellMarker marker;
    marker.centre                           = deeperCellPosition;
    marker.h                                = deeperCellSize;
    marker.relativePositionWithinFatherCell = offset;
    result.push_back(marker);
  }
  return result;
}

void createGrid() {
  repositories::startGridConstructionStep();
  _cells.reserve(std::pow(GridLength, DIMENSIONS));
  tarch::la::Vector<DIMENSIONS, double> DomainCenter = DomainOffset + DomainSize / 2.0;
  dfor(volume, GridLength) {
    peano4::datamanagement::CellMarker marker = createMarkerFromIndex(volume);
    if (UseAMR and tarch::la::norm2(marker.getX() - DomainCenter) < 0.1) {
      auto markers = createRefinedMarkersFromIndex(volume);
      _cells.insert(std::end(_cells), std::begin(markers), std::end(markers));
    } else {
      _cells.push_back(marker);
    }
  }
  repositories::finishGridConstructionStep();
}

void initGrid() {
  repositories::startGridInitialisationStep();
  applyInitialConditions();
  repositories::finishGridInitialisationStep();
}

int main(int argc, char** argv) {
  int returnCode = EXIT_SUCCESS;

  peano4::init(&argc, &argv, DomainOffset, DomainSize);

  repositories::loadBalancer.enable(false);
  repositories::initLogFilters();
  repositories::startSimulation();

  createGrid();
  initGrid();

  EnclaveDataRepository& enclaveGrid = EnclaveDataRepository::getInstance();
  enclaveGrid.startCollection();
  for (auto& marker : _cells) {
    if (not marker.hasBeenRefined()) {
      enclaveGrid.bookmarkCell(marker.getX(), marker.getH());
    }
  }

  enclaveGrid.buildFromBookmarks();

  benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::projectPatchFused(
    enclaveGrid.getQ(),
    enclaveGrid.getFacePool(),
    enclaveGrid.getFaceIndices(),
    enclaveGrid.getNumberOfEnclaveCells(),
    0
  );

  tarch::timing::Measurement measurement(1.0);
  tarch::timing::Measurement measurementBoundary(1.0);
  tarch::timing::Measurement measurementReconstruct(1.0);
  tarch::timing::Measurement measurementRiemann(1.0);
  tarch::timing::Measurement measurementProjection(1.0);
  tarch::timing::Watch       timeStepWatch("::", "main()", false);
  double                     timeStamp            = 0.0;
  double                     nextMinPlotTimeStamp = FirstPlotTimeStamp;
  double                     nextMaxPlotTimeStamp = FirstPlotTimeStamp;
  bool                       continueToSolve      = true;
  int                        count                = 0;

  while (continueToSolve) {
    if (Samples > 0 and count++ >= Samples) {
      continueToSolve = false;
    }
    if (repositories::getMinTimeStamp() >= nextMinPlotTimeStamp
        or repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp) {
      if (repositories::getMinTimeStamp() >= nextMinPlotTimeStamp) {
        nextMinPlotTimeStamp += TimeInBetweenPlots;
      }
      if (repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp) {
        nextMaxPlotTimeStamp += TimeInBetweenPlots;
      }

      if constexpr (ShouldPlot) {
        enclaveGrid.syncCellsDeviceToHost();
        plotGrid();
      }
    }
    if (UseAMR) {
      enclaveGrid.syncFacesDeviceToHost();
      interpolateBetweenAMRBorder();
      enclaveGrid.syncFacesHostToDevice();
      // Hack to make static AMR work in the mini-app. The full-app uses face updates that get interpolated between
      // coarse and fine faces. In the mini-app we do interpolation a bit different because we only use one face type to
      // keep things simple. We therefore first restrict the fine grid faces into the coarse grid faces, we do this for
      // all faces that have been refined. We then interpolate these coarse grid faces onto the fine grid faces. This is
      // needed at the AMR boundary, but not at the internal faces of the refined region. I have currently implemented
      // no way to distinguish between internal faces in the AMR region and those at the boundary. The interpolation
      // step therefore introduces a loss of precision at the internal faces of the AMR region. We can circumvent this
      // by reprojecting the cell data onto the faces for all cells. This is less than ideal and should be improved.
      benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::projectPatchFused(
        enclaveGrid.getQ(),
        enclaveGrid.getFacePool(),
        enclaveGrid.getFaceIndices(),
        enclaveGrid.getNumberOfEnclaveCells(),
        0
      );
    }

    timeStepWatch.start();
    repositories::startTimeStep();

    double dt = repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();

    tarch::timing::Watch watch("", "", false);
    watch.start();

    tarch::timing::Watch watchBoundary("", "", false);
    watchBoundary.start();
    benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::handleBoundaryFused(
      repositories::instanceOfFVSolver,
      enclaveGrid.getFacePool(),
      enclaveGrid.getBoundaryFaceIndices(),
      enclaveGrid.getBoundaryFaceNumbers(),
      enclaveGrid.getBoundaryCellIndices(),
      enclaveGrid.getCellCentres(),
      enclaveGrid.getCellSizes(),
      timeStamp,
      dt,
      enclaveGrid.getNumberOfBoundaryFaces(),
      0
    );
    watchBoundary.stop();

    tarch::timing::Watch watchReconstruct("", "", false);
    watchReconstruct.start();
    benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::reconstructPatchFused(
      enclaveGrid.getQ(),
      enclaveGrid.getFacePool(),
      enclaveGrid.getFaceIndices(),
      enclaveGrid.getQReconstructed(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchReconstruct.stop();

    tarch::timing::Watch watchRiemann("", "", false);
    watchRiemann.start();
    const auto maxEigenvalue = benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::riemannSolverFused(
      repositories::instanceOfFVSolver,
      enclaveGrid.getQReconstructed(),
      enclaveGrid.getQ(),
      timeStamp,
      dt,
      enclaveGrid.getCellCentres()[0].data(),
      enclaveGrid.getCellSizes()[0].data(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchRiemann.stop();

    tarch::timing::Watch watchProjection("", "", false);
    watchProjection.start();
    benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::projectPatchFused(
      enclaveGrid.getQ(),
      enclaveGrid.getFacePool(),
      enclaveGrid.getFaceIndices(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchProjection.stop();
    watch.stop();

    measurement.setValue(watch.getCalendarTime());
    measurementBoundary.setValue(watchBoundary.getCalendarTime());
    measurementReconstruct.setValue(watchReconstruct.getCalendarTime());
    measurementRiemann.setValue(watchRiemann.getCalendarTime());
    measurementProjection.setValue(watchProjection.getCalendarTime());

    repositories::instanceOfFVSolver.setMaxEigenvalue(maxEigenvalue);
    const auto newTimeStepSize = repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();
    repositories::instanceOfFVSolver
      .update(newTimeStepSize, timeStamp + dt, enclaveGrid.getMinVolumeH(), enclaveGrid.getMaxVolumeH());

    timeStamp += repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();
    repositories::finishTimeStep();

    timeStepWatch.stop();
    timeStepMeasurement.setValue(timeStepWatch.getCalendarTime());

    if (timeStepMeasurement.getAccumulatedValue() >= printNextTimeStep) {
      const auto totalPersistentMemory = DataRepository::getInstance().getMemoryUsageInBytes();

      const auto globalNumberOfPatches = _cells.size();
      const auto totalNumberOfDoFs
        = globalNumberOfPatches * std::pow(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS)
          * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables);

      std::ostringstream msg;
      msg
        << "#Patches=" << toScientificNotationString(globalNumberOfPatches) << ", #DoFs="
        << toScientificNotationString(totalNumberOfDoFs) << ", #Memory=" << std::fixed << std::setprecision(2)
        << tarch::convertMemorySize(totalPersistentMemory, tarch::chooseMemoryUsageFormat(totalPersistentMemory)) << " "
        << tarch::toString(tarch::chooseMemoryUsageFormat(totalPersistentMemory));
      logInfo("main()", msg.str());

      if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
        const auto dt               = measurement.getMostRecentValue();
        const auto patchesPerSecond = globalNumberOfPatches / dt;
        const auto dofsPerSecond    = totalNumberOfDoFs / dt;

        logInfo(
          "main()",
          "Patch updates per second: "
            << toScientificNotationString(patchesPerSecond)
            << ", DoF updates per second: " << toScientificNotationString(dofsPerSecond)
        );
      }

      repositories::printTimeStep(false);
      printNextTimeStep += repositories::PrintTimeStepIntervalInSeconds;
    }

    if (repositories::getMinTimeStamp() >= MinTerminalTime and repositories::getMaxTimeStamp() >= MaxTerminalTime) {
      continueToSolve = false;
    }

    if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNonCriticalAssertion) {
      continueToSolve                  = false;
      haveReceivedNonCriticalAssertion = true;
      logError("main()", "Noncritical assertion has been triggered in code. Dump final state and terminate.");
    }
  }

  // Plot last timestep
  if constexpr (ShouldPlot) {
    enclaveGrid.syncCellsDeviceToHost();
    plotGrid();
  }
  repositories::finishSimulation();

  if (haveReceivedNonCriticalAssertion) {
    logWarning("main()", "Terminated gracefully after noncritical assertion");
    returnCode = EXIT_FAILURE;
  } else {
    // Print final timestep information
    repositories::printTimeStep(false);
    logInfo("main()", "Terminated successfully");
    returnCode = EXIT_SUCCESS;
  }

  printFinalStatistics(
    measurement,
    measurementBoundary,
    measurementReconstruct,
    measurementRiemann,
    measurementProjection
  );

  enclaveGrid.clear();

  peano4::shutdown();

  return returnCode;
}
