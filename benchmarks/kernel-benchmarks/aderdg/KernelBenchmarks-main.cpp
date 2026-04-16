// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarks-main.h"

#include "AbstractAderSolver.h"
#include "AderSolver.h"
#include "Constants.h"
#include "DataRepository.h"
#include "EnclaveDataRepository.h"
#include "EnclaveTasks.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/dg/DGUtils.h"
#include "kernels/AderSolver/AMRRoutines.h"
#include "peano4/Globals.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/peano4.h"
#include "peano4/utils/Loop.h"
#include "tarch/logging/Log.h"
#include "tarch/plotter/AMRPlotter.h"
#include "tarch/plotter/HDF5FileWriter.h"
#include "tarch/plotter/XDMFTimeSeriesWriter.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "toolbox/blockstructured/Interpolation.h"
#include "toolbox/blockstructured/Projection.h"
#include "toolbox/blockstructured/Restriction.h"

#ifdef UseAMR
#error "At the moment the kernel benchmark does not support AMR"
#endif

using namespace benchmarks::exahype2::kernelbenchmarks;

namespace {
  tarch::logging::Log        _log("");
  tarch::timing::Measurement timeStepMeasurement;
  double                     logNextTimeStep                  = 0.0;
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

void logFinalStatistics(
  const tarch::timing::Measurement& measurement,
  const tarch::timing::Measurement& measurementBoundary,
  const tarch::timing::Measurement& measurementPredictor,
  const tarch::timing::Measurement& measurementRiemann,
  const tarch::timing::Measurement& measurementCorrector
) {
  if (measurementBoundary.getNumberOfMeasurements() > 0) {
    logInfo(
      "logFinalStatistics()",
      "Boundary handling:                       "
        << measurementBoundary.getAccumulatedValue() << "s\t" << measurementBoundary.toString()
    );
  }
  if (measurementPredictor.getNumberOfMeasurements() > 0) {
    logInfo(
      "logFinalStatistics()",
      "Predictor:                          "
        << measurementPredictor.getAccumulatedValue() << "s\t" << measurementPredictor.toString()
    );
  }
  if (measurementRiemann.getNumberOfMeasurements() > 0) {
    logInfo(
      "logFinalStatistics()",
      "Riemann solver:                          "
        << measurementRiemann.getAccumulatedValue() << "s\t" << measurementRiemann.toString()
    );
  }
  if (measurementCorrector.getNumberOfMeasurements() > 0) {
    logInfo(
      "logFinalStatistics()",
      "Corrector:                              "
        << measurementCorrector.getAccumulatedValue() << "s\t" << measurementCorrector.toString()
    );
  }

  if (measurement.getNumberOfMeasurements() > 0) {
    logInfo(
      "logFinalStatistics()",
      "Time stepping:                           "
        << measurement.getAccumulatedValue() << "s\t" << measurement.toString()
    );

    const auto globalNumberOfPatches = _cells.size();
    const auto totalNumberOfDoFs
      = globalNumberOfPatches * std::pow(AbstractAderSolver::Order + 1, DIMENSIONS)
        * (AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables);

    logInfo(
      "logFinalStatistics()",
      "Average patch updates per second:  " << globalNumberOfPatches / measurement.getValue()
    );
    logInfo(
      "logFinalStatistics()",
      "Average DoF updates per second:    " << totalNumberOfDoFs / measurement.getValue()
    );
  }
}

void plotGrid() {
  repositories::startPlottingStep();

  const std::string outputDirectory  = "solutions";
  const std::string solutionFileName = "AderSolver";

  const double t = 0.5 * (repositories::getMinTimeStamp() + repositories::getMaxTimeStamp());

  static tarch::plotter::XDMFTimeSeriesWriter timeSeriesWriter;
  tarch::plotter::HDF5FileWriter              fileWriter(outputDirectory, 1);
  auto                                        _cellPlotter   = fileWriter.createCellPlotter(0);
  auto                                        _vertexPlotter = fileWriter.createVertexPlotter(0);

  auto _vertexDataPlotter = fileWriter.createVertexDataPlotter(0, "AderSolverQ", AbstractAderSolver::NumberOfUnknowns);

  for (auto& marker : _cells) {
    if (not marker.hasBeenRefined()) {
      const auto* data = DataRepository::getInstance().getCellQ(marker.getX(), marker.getH());

      constexpr int PointsPerAxis = AbstractAderSolver::Order + 1;
      constexpr int TotalPoints   = (DIMENSIONS == 2) ? PointsPerAxis * PointsPerAxis
                                                      : PointsPerAxis * PointsPerAxis * PointsPerAxis;

      int vertexIndices[TotalPoints];

      int counter         = 0;
      int linearisedIndex = 0;
      dfor(index, AbstractAderSolver::Order + 1) {
        vertexIndices[counter] = _vertexPlotter->plotVertex(
          ::exahype2::dg::getQuadraturePoint(
            marker.getX(),
            marker.getH(),
            index,
            repositories::instanceOfAderSolver.Order + 1,
            kernels::AderSolver::Quadrature<SolverStoragePrecision>::nodes
          )
        );

        double values[AbstractAderSolver::NumberOfUnknowns];
        for (int u = 0; u < AbstractAderSolver::NumberOfUnknowns; u++) {
          values[u] = data[linearisedIndex + u];
        }
        _vertexDataPlotter->plotVertexData(vertexIndices[counter], values, AbstractAderSolver::NumberOfUnknowns);

        counter++;
        linearisedIndex += AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables;
      }

#if DIMENSIONS == 3
      for (int k = 0; k < PointsPerAxis - 1; k++) {
#endif
        for (int j = 0; j < PointsPerAxis - 1; j++) {
          for (int i = 0; i < PointsPerAxis - 1; i++) {
            int idx = i + j * PointsPerAxis
#if DIMENSIONS == 3
                      + k * PointsPerAxis * PointsPerAxis
#endif
              ;

            int s_x = 1;
            int s_y = PointsPerAxis;

#if DIMENSIONS == 2
            int v[4];
            v[0]          = vertexIndices[idx];
            v[1]          = vertexIndices[idx + s_x];
            v[2]          = vertexIndices[idx + s_x + s_y];
            v[3]          = vertexIndices[idx + s_y];
            int cellIndex = _cellPlotter->plotQuadrangle(v);
#elif DIMENSIONS == 3
          int s_z = PointsPerAxis * PointsPerAxis;
          int v[8];
          v[0]          = vertexIndices[idx];
          v[1]          = vertexIndices[idx + s_x];
          v[2]          = vertexIndices[idx + s_x + s_y];
          v[3]          = vertexIndices[idx + s_y];
          v[4]          = vertexIndices[idx + s_z];
          v[5]          = vertexIndices[idx + s_x + s_z];
          v[6]          = vertexIndices[idx + s_x + s_y + s_z];
          v[7]          = vertexIndices[idx + s_y + s_z];
          int cellIndex = _cellPlotter->plotHexahedron(v);
#endif
          }
        }
#if DIMENSIONS == 3
      }
#endif
    }
  }

  timeSeriesWriter.writeSnapshotFile(solutionFileName, t, fileWriter);
  timeSeriesWriter.writeTimeSeriesMetaFile(outputDirectory, solutionFileName);
  repositories::finishPlottingStep();
}


void interpolateBetweenAMRBorder() {

#if DIMENSIONS == 2
  constexpr int SpaceFaceSize = AbstractAderSolver::Order + 1;
#else
  constexpr int SpaceFaceSize = (AbstractAderSolver::Order + 1) * (AbstractAderSolver::Order + 1);
#endif

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

          int facePosition = faceMarker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;
          if (repositories::instanceOfAderSolver.isFirstGridSweepOfTimeStep()) {
            auto coarseGridFaceQFlux = DataRepository::getInstance().getFaceQFluxEstimates(parentCenter, parentSize, f);
            std::fill_n(
              coarseGridFaceQFlux + (1 - facePosition) * (SpaceFaceSize * AbstractAderSolver::NumberOfUnknowns),
              (SpaceFaceSize * AbstractAderSolver::NumberOfUnknowns),
              0.0
            );
          }
          if (repositories::instanceOfAderSolver.getSolverState()
              == AderSolver::SolverState::PredictionOnHangingCells) {
            auto fineGridFaceQFlux = DataRepository::getInstance().getFaceQFluxEstimates(
              marker.getX(),
              marker.getH(),
              f
            );
            auto coarseGridFaceQFlux = DataRepository::getInstance().getFaceQFluxEstimates(parentCenter, parentSize, f);
            // Fluxes
            kernels::AderSolver::faceFluxRestriction(
              coarseGridFaceQFlux + (1 - facePosition) * (SpaceFaceSize * AbstractAderSolver::NumberOfUnknowns),
              fineGridFaceQFlux + facePosition * (SpaceFaceSize * AbstractAderSolver::NumberOfUnknowns),
              faceMarker.getRelativePositionWithinFatherFace()
            );
          }
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
          auto fineGridFaceQ     = DataRepository::getInstance().getFaceQEstimates(marker.getX(), marker.getH(), f);
          auto coarseGridFaceQ   = DataRepository::getInstance().getFaceQEstimates(parentCenter, parentSize, f);
          auto fineGridFaceQFlux = DataRepository::getInstance().getFaceQFluxEstimates(marker.getX(), marker.getH(), f);
          auto coarseGridFaceQFlux = DataRepository::getInstance().getFaceQFluxEstimates(parentCenter, parentSize, f);

          int facePosition = faceMarker.getSelectedFaceNumber() < DIMENSIONS ? 0 : SpaceFaceSize;

          if (repositories::instanceOfAderSolver.getSolverState()
              == AderSolver::SolverState::PredictionOnHangingCells) {
            kernels::AderSolver::faceUnknownsProlongation(
              fineGridFaceQ
                + facePosition
                    * (AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables),
              fineGridFaceQFlux + facePosition * AbstractAderSolver::NumberOfUnknowns,
              coarseGridFaceQ
                + facePosition
                    * (AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables),
              coarseGridFaceQFlux + facePosition * AbstractAderSolver::NumberOfUnknowns,
              faceMarker.getRelativePositionWithinFatherFace()
            );
          }
        }
      }
    }
  }
}

void applyInitialConditions() {
  for (auto& marker : _cells) {
    if (not marker.hasBeenRefined()) {
      auto* QOut = DataRepository::getInstance().getCellQ(marker.getX(), marker.getH());
      // Initialise cell data with initial conditions.
      int linearisedIndex = 0;
      dfor(index, AbstractAderSolver::Order + 1) {
        repositories::instanceOfAderSolver.initialCondition(
          QOut + linearisedIndex,
          ::exahype2::dg::getQuadraturePoint(
            marker.getX(),
            marker.getH(),
            index,
            repositories::instanceOfAderSolver.Order + 1,
            kernels::AderSolver::Quadrature<SolverStoragePrecision>::nodes
          ),
          marker.getH()
        );
        linearisedIndex += (AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables);
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

  tarch::timing::Measurement measurement(1.0);
  tarch::timing::Measurement measurementBoundary(1.0);
  tarch::timing::Measurement measurementPredictor(1.0);
  tarch::timing::Measurement measurementRiemann(1.0);
  tarch::timing::Measurement measurementCorrector(1.0);
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
    if ((repositories::getMinTimeStamp() >= nextMinPlotTimeStamp
         or repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp)
        and repositories::mayPlot()) {
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

      // TODO
      //  benchmarks::exahype2::kernelbenchmarks::kernels::FVSolver::projectPatchFused(
      //    enclaveGrid.getQ(),
      //    enclaveGrid.getFacePool(),
      //    enclaveGrid.getFaceIndices(),
      //    enclaveGrid.getNumberOfEnclaveCells(),
      //    0
      //  );
    }

    timeStepWatch.start();
    repositories::startTimeStep();

    double dt = repositories::instanceOfAderSolver.getAdmissibleTimeStepSize();

    tarch::timing::Watch watch("", "", false);
    watch.start();

    tarch::timing::Watch watchPredictor("", "", false);
    watchPredictor.start();
    benchmarks::exahype2::kernelbenchmarks::predictorFused(
      repositories::instanceOfAderSolver,
      enclaveGrid.getQ(),
      enclaveGrid.getQOld(),
      enclaveGrid.getFaceEstimatesPool(),
      enclaveGrid.getFaceFluxesPool(),
      enclaveGrid.getFaceIndices(),
      timeStamp,
      dt,
      enclaveGrid.getCellCentres()[0].data(),
      enclaveGrid.getCellSizes()[0].data(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchPredictor.stop();

    tarch::timing::Watch watchBoundary("", "", false);
    watchBoundary.start();
    benchmarks::exahype2::kernelbenchmarks::handleBoundaryFused(
      repositories::instanceOfAderSolver,
      enclaveGrid.getFaceEstimatesPool(),
      enclaveGrid.getFaceFluxesPool(),
      enclaveGrid.getBoundaryFaceIndices(),
      enclaveGrid.getBoundaryFaceNumbers(),
      enclaveGrid.getBoundaryCellIndices(),
      enclaveGrid.getCellCentres(),
      enclaveGrid.getCellSizes(),
      timeStamp,
      dt,
      enclaveGrid.getNumberOfBoundaryFaces(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchBoundary.stop();

    tarch::timing::Watch watchRiemann("", "", false);
    watchRiemann.start();
    riemannFused(
      repositories::instanceOfAderSolver,
      enclaveGrid.getFaceEstimatesPool(),
      enclaveGrid.getFaceFluxesPool(),
      enclaveGrid.getUniqueFaceIndices(),
      enclaveGrid.getFaceNumbers(),
      enclaveGrid.getFaceCellIndices(),
      timeStamp,
      dt,
      enclaveGrid.getCellCentres()[0].data(),
      enclaveGrid.getCellSizes()[0].data(),
      enclaveGrid.getNumberOfFaces(),
      0
    );
    watchRiemann.stop();

    tarch::timing::Watch watchCorrector("", "", false);
    watchCorrector.start();
    benchmarks::exahype2::kernelbenchmarks::correctorFused(
      repositories::instanceOfAderSolver,
      enclaveGrid.getQ(),
      enclaveGrid.getFaceFluxesPool(),
      enclaveGrid.getFaceIndices(),
      timeStamp,
      dt,
      enclaveGrid.getCellCentres()[0].data(),
      enclaveGrid.getCellSizes()[0].data(),
      enclaveGrid.getNumberOfEnclaveCells(),
      0
    );
    watchCorrector.stop();
    watch.stop();

    measurement.setValue(watch.getCalendarTime());
    measurementBoundary.setValue(watchBoundary.getCalendarTime());
    measurementPredictor.setValue(watchPredictor.getCalendarTime());
    measurementRiemann.setValue(watchRiemann.getCalendarTime());
    measurementCorrector.setValue(watchCorrector.getCalendarTime());

    const auto newTimeStepSize = repositories::instanceOfAderSolver.getAdmissibleTimeStepSize();
    repositories::instanceOfAderSolver
      .update(newTimeStepSize, timeStamp + dt, enclaveGrid.getMinVolumeH(), enclaveGrid.getMaxVolumeH());

    timeStamp += repositories::instanceOfAderSolver.getAdmissibleTimeStepSize();
    repositories::finishTimeStep();

    timeStepWatch.stop();
    timeStepMeasurement.setValue(timeStepWatch.getCalendarTime());

    if (timeStepMeasurement.getAccumulatedValue() >= logNextTimeStep) {
      const auto totalPersistentMemory = DataRepository::getInstance().getMemoryUsageInBytes();

      const auto globalNumberOfPatches = _cells.size();
      const auto totalNumberOfDoFs
        = globalNumberOfPatches * std::pow(AbstractAderSolver::Order + 1, DIMENSIONS)
          * (AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables);

      std::ostringstream msg;
      msg
        << "Patches=" << toScientificNotationString(globalNumberOfPatches) << ", DoFs="
        << toScientificNotationString(totalNumberOfDoFs) << ", Memory=" << std::fixed << std::setprecision(2)
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

      repositories::logTimeStep();
      logNextTimeStep += repositories::LogTimeStepIntervalInSeconds;
    }

    if (repositories::getMinTimeStamp() >= MinTerminalTime and repositories::getMaxTimeStamp() >= MaxTerminalTime) {
      continueToSolve = false;
    }

    if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNonCriticalAssertion) {
      continueToSolve                  = false;
      haveReceivedNonCriticalAssertion = true;
      logError("main()", "Noncritical assertion has been triggered in code. Dump final state and terminate.");
    }

  } // continueToSolve

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
    // Log final timestep information
    repositories::logTimeStep();
    logInfo("main()", "Terminated successfully");
    returnCode = EXIT_SUCCESS;
  }

  logFinalStatistics(measurement, measurementBoundary, measurementPredictor, measurementRiemann, measurementCorrector);

  enclaveGrid.clear();
  peano4::shutdown();
  return returnCode;
}
