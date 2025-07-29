// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarks-main.h"

#include "AbstractFVSolver.h"
#include "Constants.h"
#include "DataRepository.h"
#include "FVSolver.h"
#include "celldata/FVSolverQ.h"
#include "celldata/FVSolverQReconstructed.h"
#include "exahype2/CellData.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/fv/BoundaryConditions.h"
#include "exahype2/fv/PatchUtils.h"
#include "facedata/FVSolverQNew.h"
#include "kernels/FVSolver/ReconstructPatch.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/peano4.h"
#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"
#include "tarch/logging/Log.h"
#include "tarch/plotter/griddata/blockstructured/PeanoTextPatchFileWriter.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "tasks/FVSolverEnclaveTask.h"
#include "toolbox/blockstructured/Projection.h"

using namespace benchmarks::exahype2::kernelbenchmarks;

namespace {
  tarch::logging::Log        _log("");
  tarch::timing::Measurement timeStepMeasurement;
  double                     printNextTimeStep = 0.0;
  bool                       haveReceivedNonCriticalAssertion = false;

  std::vector<peano4::datamanagement::CellMarker> _cells;
  const tarch::la::Vector<Dimensions, double>     CellSize = DomainSize / static_cast<double>(GridLength);
} // namespace

void reportRuntime(const tarch::timing::Measurement& measurement) {
  const std::string context    = "reportRuntime()";
  constexpr int     LabelWidth = 25; // Set a fixed width for all labels

  if (measurement.getNumberOfMeasurements() == 0) {
    logInfo(context, "Timing Measurement Report: No data recorded.");
    return;
  }

  logInfo(context, "---------- Timing Measurement Report ----------");
  double avg             = measurement.getValue();
  double std_dev         = measurement.getStandardDeviation();
  double min_val         = measurement.min();
  double max_val         = measurement.max();
  int    min_idx         = measurement.getMinMeasurement();
  int    max_idx         = measurement.getMaxMeasurement();
  double min_dev_percent = (avg == 0) ? 0.0 : (min_val / avg - 1.0) * 100.0;
  double max_dev_percent = (avg == 0) ? 0.0 : (max_val / avg - 1.0) * 100.0;

  std::stringstream msg;

  msg.str("");
  msg << std::left << std::setw(LabelWidth) << "Average Value:" << std::fixed << std::setprecision(8) << avg << "s";
  logInfo(context, msg.str());

  msg.str("");
  msg << std::left << std::setw(LabelWidth) << "Average Cell Updates:" << std::fixed << std::setprecision(0)
      << _cells.size() / avg;
  logInfo(context, msg.str());

  msg.str("");
  msg << std::left << std::setw(LabelWidth) << "Standard Deviation:" << std::scientific << std::setprecision(8)
      << std_dev;
  logInfo(context, msg.str());

  msg.str("");
  msg << std::left << std::setw(LabelWidth) << "# Measurements:" << measurement.getNumberOfMeasurements();
  logInfo(context, msg.str());

  msg.str("");
  msg
    << std::left << std::setw(LabelWidth) << "Min Value:" << std::fixed << std::setprecision(8) << min_val << "s (at #"
    << min_idx << ") "
    << "[" << std::fixed << std::setprecision(2) << min_dev_percent << "%]";
  logInfo(context, msg.str());

  msg.str("");
  msg
    << std::left << std::setw(LabelWidth) << "Max Value:" << std::fixed << std::setprecision(8) << max_val << "s (at #"
    << max_idx << ") " << std::showpos // Show the '+' sign
    << "[" << std::fixed << std::setprecision(2) << max_dev_percent << "%]";
  logInfo(context, msg.str());

  msg.str("");
  msg << std::left << std::setw(LabelWidth) << "Most Recent Value:" << std::fixed << std::setprecision(8)
      << measurement.getMostRecentValue() << "s";
  logInfo(context, msg.str());

  msg.str("");
  msg << std::left << std::setw(LabelWidth) << "Accumulated Time:" << std::fixed << std::setprecision(8)
      << measurement.getAccumulatedValue() << "s";
  logInfo(context, msg.str());

  logInfo(context, "-----------------------------------------------");
}

void plotGrid() {
  repositories::startPlottingStep();
  static int counter = -1;
  counter++;

  std::ostringstream snapshotFileName;
  snapshotFileName << "solutions/solution-FVSolver-" << counter;

  auto writer = tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter(
    Dimensions,
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
    const int patchIndex = writer.plotPatch(marker.x() - marker.h() * 0.5, marker.h());
    int       cellIndex  = dataWriter->getFirstCellWithinPatch(patchIndex);
    int       currentDoF = 0;
    double*   QOut       = DataRepository::getInstance().getCellQ(marker.x(), marker.h());
    dfor(k, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch) {
      double* data = QOut + currentDoF;

      dataWriter->plotCell(cellIndex, data);
      cellIndex++;
      currentDoF += AbstractFVSolver::NumberOfUnknowns;
    }
  }
  dataWriter->close();
  writer.writeToFile();
  delete dataWriter;
  repositories::finishPlottingStep();
}

void invokeFVSolverBoundaryConditions(
  const double* NOALIAS                        Qinside,
  double* NOALIAS                              Qoutside,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  repositories::instanceOfFVSolver.boundaryConditions(Qinside, Qoutside, x, h, t, normal);
}

void applyBoundaryConditionsToAxis(int axis, double t, double dt) {
  const double ratio = DomainSize[0] / CellSize[0];
  const int    depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
  dfore(volume, GridLength, axis, 0) {
    tarch::la::Vector<Dimensions, double> faceOffset = CellSize / 2.0;
    // --- Low Boundary ---
    tarch::la::Vector<Dimensions, double>
      faceCenterLow = tarch::la::multiplyComponents(tarch::la::convertScalar<double>(volume), CellSize) + faceOffset
                      + DomainOffset;
    int faceIndexLow    = DataRepository::Indexing::getFaceIndex(faceCenterLow, CellSize, axis);
    faceCenterLow[axis] = DomainOffset[axis];

    // --- High Boundary ---
    tarch::la::Vector<Dimensions, double> faceCenterHigh = faceCenterLow;
    faceCenterHigh[axis] = DomainSize[axis] - faceOffset[axis] + DomainOffset[axis]; // high cell Centre
    int faceIndexHigh    = DataRepository::Indexing::getFaceIndex(faceCenterHigh, CellSize, axis);
    faceCenterHigh[axis] = DomainSize[axis] + DomainOffset[axis]; // High face centre

    // Apply BC to low face
    double* lowFacePtr = DataRepository::getInstance().getFaceQNew(faceIndexLow, axis);
    ::exahype2::fv::applyBoundaryConditions<double>(
      invokeFVSolverBoundaryConditions,
      faceCenterLow,
      CellSize,
      t,
      dt,
      AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
      1,
      AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables,
      axis,
      lowFacePtr
    );

    // Apply BC to high face
    double* highFacePtr = DataRepository::getInstance().getFaceQNew(faceIndexHigh, axis + Dimensions);
    ::exahype2::fv::applyBoundaryConditions<double>(
      invokeFVSolverBoundaryConditions,
      faceCenterHigh,
      CellSize,
      t,
      dt,
      AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
      1,
      AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables,
      axis + Dimensions,
      highFacePtr
    );
  }
}

void applyBoundaryConditions() {
  const double t  = 0.0;
  const double dt = 0.0;

  for (int axis = 0; axis < Dimensions; ++axis) {
    applyBoundaryConditionsToAxis(axis, t, dt);
  }
}

void projectPatchOntoFaces() {
  for (auto& marker : _cells) {
    double* QOut = DataRepository::getInstance().getCellQ(marker.x(), marker.h());
    double* faces[TwoTimesD];
    for (int i = 0; i < TwoTimesD; i++) {
      faces[i] = DataRepository::getInstance().getFaceQNew(marker.x(), marker.h(), i);
    }
    toolbox::blockstructured::projectPatchSolutionOntoFaces(
      AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch,
      1,
      AbstractFVSolver::NumberOfUnknowns,
      AbstractFVSolver::NumberOfAuxiliaryVariables,
      QOut,
      faces
    );
  }
}

void applyInitialConditions() {
  for (auto& marker : _cells) {
    double* QOut = DataRepository::getInstance().getCellQ(marker.x(), marker.h());
    // Initialise cell data with initial conditions.
    dfor(volume, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch) {
      int linearIndex = peano4::utils::dLinearised(volume, AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch)
                        * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables);
      FVSolver::initialCondition(
        QOut + linearIndex,
        ::exahype2::fv::
          getVolumeCentre(marker.x(), marker.h(), AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch, volume),
        ::exahype2::fv::getVolumeSize(marker.h(), AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch),
        true
      );
    }
  }
}

peano4::datamanagement::CellMarker createMarkerFromIndex(const tarch ::la ::Vector<Dimensions, int>& index) {
  tarch::la::Vector<Dimensions, double> cellIndex        = tarch::la::convertScalar<double>(index);
  tarch::la::Vector<Dimensions, double> cellPositionBase = DomainOffset + CellSize / 2.0;
  tarch::la::Vector<Dimensions, double>
                                   cellPosition = cellPositionBase + tarch::la::multiplyComponents(cellIndex, CellSize);
  peano4::grid::GridTraversalEvent event;
  event.setX(cellPosition);
  event.setH(CellSize);
  return peano4::datamanagement::CellMarker(event);
}

void createGrid() {
  repositories::startGridConstructionStep();
  _cells.reserve(std::pow(GridLength, Dimensions));
  dfor(volume, GridLength) { _cells.push_back(createMarkerFromIndex(volume)); }
  repositories::finishGridConstructionStep();
}

void initGrid() {
  repositories::startGridInitialisationStep();
  applyInitialConditions();
  projectPatchOntoFaces();
  repositories::finishGridInitialisationStep();
}

int main(int argc, char** argv) {
  int returnCode = EXIT_SUCCESS;

  peano4::init(
    &argc,
    &argv,
    benchmarks::exahype2::kernelbenchmarks::DomainOffset,
    benchmarks::exahype2::kernelbenchmarks::DomainSize
  );

  repositories::initLogFilters();
  repositories::startSimulation();

  createGrid();
  logInfo("main()", "Created grid with " << _cells.size() << " cells");
  initGrid();
  logInfo("main()", "Initialised grid");

  DataRepository& grid = DataRepository::getInstance();

  ::exahype2::CellData<double, double> patchData(_cells.size(), tarch::MemoryLocation::Device);
  int                                  faceCardinality      = grid.getFaceCardinality();
  double*                              facesFlattened       = new double[_cells.size() * TwoTimesD * faceCardinality];
  double*                              facesFlattenedDevice = tarch::allocateMemory<double>(
    _cells.size() * TwoTimesD * faceCardinality,
    tarch::MemoryLocation::Device
  );

  double* QInFlattened = tarch::allocateMemory<double>(
    _cells.size() * grid.getCellQReconstructedCardinality(),
    tarch::MemoryLocation::Device
  );

  double* QOutFlattened = tarch::allocateMemory<double>(
    _cells.size() * grid.getCellQCardinality(),
    tarch::MemoryLocation::Device
  );
  double* QOutFlattenedHost = grid.getCellQ(0);

  tarch::la::Vector<Dimensions, double>* cellCentresHost = new tarch::la::Vector<Dimensions, double>[_cells.size()];
  tarch::la::Vector<Dimensions, double>* cellSizeHost    = new tarch::la::Vector<Dimensions, double>[_cells.size()];

  for (int patchIndex = 0; patchIndex < _cells.size(); patchIndex++) {
    cellCentresHost[patchIndex] = _cells[patchIndex].x();
    cellSizeHost[patchIndex]    = _cells[patchIndex].h();
  }
  tarch::copyTo<tarch::la::Vector<Dimensions, double>, tarch::la::Vector<Dimensions, double>>(
    patchData.cellCentre,
    cellCentresHost,
    _cells.size(),
    tarch::MemoryLocation::Device
  );
  tarch::copyTo<tarch::la::Vector<Dimensions, double>, tarch::la::Vector<Dimensions, double>>(
    patchData.cellSize,
    cellSizeHost,
    _cells.size(),
    tarch::MemoryLocation::Device
  );

  patchData.facesFlattened = facesFlattenedDevice;
  patchData.QOutFlattened  = QOutFlattened;
  patchData.QInFlattened   = QInFlattened;

  tarch::timing::Measurement measurement(1.0);
  tarch::timing::Watch       timeStepWatch("::", "main()", false);
  double                     timeStamp            = 0.0;
  double                     nextMinPlotTimeStamp = FirstPlotTimeStamp;
  double                     nextMaxPlotTimeStamp = FirstPlotTimeStamp;
  bool                       continueToSolve      = true;

  while (continueToSolve) {
    if (repositories::getMinTimeStamp() >= nextMinPlotTimeStamp
        or repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp) {
      if (repositories::getMinTimeStamp() >= nextMinPlotTimeStamp) {
        nextMinPlotTimeStamp += TimeInBetweenPlots;
      }
      if (repositories::getMaxTimeStamp() >= nextMaxPlotTimeStamp) {
        nextMaxPlotTimeStamp += TimeInBetweenPlots;
      }

      if constexpr (ShouldPlot) {
        plotGrid();
      }
    }

    timeStepWatch.start();

    applyBoundaryConditions();

    for (int patchIndex = 0; patchIndex < _cells.size(); patchIndex++) {
      for (int f = 0; f < TwoTimesD; ++f) {
        std::copy_n(
          grid.getFaceQNew(_cells[patchIndex].x(), _cells[patchIndex].h(), f),
          faceCardinality,
          &facesFlattened
            [patchIndex * TwoTimesD * faceCardinality // Skip whole patches
             + f * faceCardinality                    // Move to the f-th face inside the patch
        ]
        );
      }
    }
    tarch::copyTo<double, double>(
      QOutFlattened,
      QOutFlattenedHost,
      _cells.size() * grid.getCellQCardinality(),
      tarch::MemoryLocation::Device
    );
    tarch::copyTo<double, double>(
      facesFlattenedDevice,
      facesFlattened,
      _cells.size() * TwoTimesD * faceCardinality,
      tarch::MemoryLocation::Device
    );

    patchData.t  = timeStamp;
    patchData.dt = repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();
    repositories::startTimeStep();

    tarch::timing::Watch watch("", "", false);
    benchmarks::exahype2::kernelbenchmarks::tasks::FVSolverEnclaveTask::fuse(patchData);
    watch.stop();
    measurement.setValue(watch.getCalendarTime());

    timeStamp += repositories::instanceOfFVSolver.getAdmissibleTimeStepSize();
    repositories::finishTimeStep();

    tarch::copyFrom<double, double>(
      QOutFlattened,
      QOutFlattenedHost,
      _cells.size() * grid.getCellQCardinality(),
      tarch::MemoryLocation::Device
    );

    projectPatchOntoFaces();

    timeStepWatch.stop();
    timeStepMeasurement.setValue(timeStepWatch.getCalendarTime());

    if (timeStepMeasurement.getAccumulatedValue() >= printNextTimeStep) {
      repositories::printTimeStep(false);
      if (timeStepMeasurement.getNumberOfMeasurements() > 0) {
        logInfo(
          "main()",
          "Average cell updates per second: "
          << _cells.size() / measurement.getValue()
        );
      }
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

  reportRuntime(measurement);

  delete[] facesFlattened;
  delete[] cellCentresHost;
  delete[] cellSizeHost;
  tarch::freeMemory(facesFlattenedDevice, tarch::MemoryLocation::Device);
  tarch::freeMemory(QOutFlattened, tarch::MemoryLocation::Device);
  tarch::freeMemory(QInFlattened, tarch::MemoryLocation::Device);

  peano4::shutdown();

  return returnCode;
}
