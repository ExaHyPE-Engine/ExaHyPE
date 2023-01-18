#include "{{CLASSNAME}}.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


std::bitset<Dimensions> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return std::numeric_limits<double>::max();
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return 0.0;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return 0.0;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  return std::numeric_limits<double>::max();
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxMeshSize() const {
  return std::numeric_limits<double>::max();
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinMeshSize() const {
  return 0.0;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return true;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::computeNewMarkerState(
  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}&                                                cellMarker,
  const peano4::datamanagement::FaceEnumerator<benchmarks::exahype2::euler::sphericalaccretionupscaling::facedata::{{MARKER}}>&  faceMarker
) {
  bool cellHoldsBnotA              = false;
  bool neighbourHoldsBnotA         = false;
  bool cellHoldsBdeterminesA       = false;
  bool neighbourHoldsBdeterminesA  = false;
  bool cellHoldsAdeterminesB       = false;
  bool neighbourHoldsAdeterminesB  = false;

  cellHoldsBnotA        = cellMarker.getValue() > MaxThreshold;
  cellHoldsBdeterminesA = cellMarker.getValue() > 0.5*MaxThreshold+0.5*MinThreshold;
  cellHoldsAdeterminesB = cellMarker.getValue() > MinThreshold;

  for (int d=0; d<Dimensions; d++) {
    double leftNeighbour  = 2.0 * (faceMarker(d).getOldAverage()            - 0.5 * cellMarker.getValue());
    double rightNeighbour = 2.0 * (faceMarker(d+Dimensions).getOldAverage() - 0.5 * cellMarker.getValue());

    neighbourHoldsBnotA |= leftNeighbour  > MaxThreshold;
    neighbourHoldsBnotA |= rightNeighbour > MaxThreshold;

    neighbourHoldsBdeterminesA |= leftNeighbour  > 0.5*MaxThreshold+0.5*MinThreshold;
    neighbourHoldsBdeterminesA |= rightNeighbour > 0.5*MaxThreshold+0.5*MinThreshold;

    neighbourHoldsAdeterminesB |= leftNeighbour  > MinThreshold;
    neighbourHoldsAdeterminesB |= rightNeighbour > MinThreshold;
  }

  // "lift" current state
  cellHoldsBdeterminesA |= neighbourHoldsBnotA;
  cellHoldsAdeterminesB |= neighbourHoldsBdeterminesA;

  benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker newMarker =
    benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AnotB;
  if (cellHoldsBnotA) {
    newMarker = benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BnotA;
  }
  else if (cellHoldsBdeterminesA) {
    newMarker = benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BdeterminesA;
  }
  else if (cellHoldsAdeterminesB) {
    newMarker = benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AdeterminesB;
  }

  const bool AhadBeenStoredBefore =
    cellMarker.getMarker() == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AnotB
    or
    cellMarker.getMarker() == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AdeterminesB;

  const bool BhadBeenStoredBefore =
    cellMarker.getMarker() == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BnotA
    or
    cellMarker.getMarker() == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BdeterminesA;

  if (
    newMarker == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AnotB
    or
    newMarker == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AdeterminesB
    or
    newMarker == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BdeterminesA
  ) {
    cellMarker.setTransitionA(
      AhadBeenStoredBefore
        ? benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::TransitionA::LoadStore
        : benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::TransitionA::Store
    );
  }
  else {
    cellMarker.setTransitionA(
      AhadBeenStoredBefore
        ? benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::TransitionA::Load
        : benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::TransitionA::None
    );
  }

  if (
    newMarker == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BnotA
    or
    newMarker == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AdeterminesB
    or
    newMarker == benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BdeterminesA
  ) {
    cellMarker.setTransitionB(
      BhadBeenStoredBefore
        ? benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::TransitionB::LoadStore
        : benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::TransitionB::Store
    );
  }
  else {
    cellMarker.setTransitionB(
      BhadBeenStoredBefore
        ? benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::TransitionB::Load
        : benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::TransitionB::None
    );
  }

  cellMarker.setMarker( newMarker );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::constrainValue( benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}& marker ) {
  const double Tolerance = 0.01;

  switch (marker.getMarker()) {
    case benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BnotA:
      marker.setValue( std::min(marker.getValue(), MaxValue) );
//      marker.setValue( std::max(marker.getValue(), MaxThreshold * (1.0+Tolerance) ) );
      break;
    case benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::BdeterminesA:
//      marker.setValue( std::min(marker.getValue(), MaxThreshold * (1.0-Tolerance) ) );
//      marker.setValue( std::max(marker.getValue(), (0.5*MinThreshold+0.5*MaxThreshold)*(1.0+Tolerance) ) );
      break;
    case benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AdeterminesB:
//      marker.setValue( std::min(marker.getValue(), (0.5*MinThreshold+0.5*MaxThreshold)*(1.0-Tolerance) ) );
//      marker.setValue( std::min(marker.getValue(), MinThreshold * (1.0+Tolerance) ) );
      break;
    case benchmarks::exahype2::euler::sphericalaccretionupscaling::celldata::{{MARKER}}::Marker::AnotB:
//      marker.setValue( std::min(marker.getValue(), MinThreshold * (1.0-Tolerance) ) );
      marker.setValue( std::max(marker.getValue(), MinValue) );
      break;
  }
}




