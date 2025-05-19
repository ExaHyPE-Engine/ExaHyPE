{% include 'DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver.Abstract.generic.template.cpp' %}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  if (_state==State::UpdateCell) {
    clearGlobalResidualAndSolutionUpdate();
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::endMeshSweep() {
  switch (_state) {
    case State::ProjectOntoFaces:
      logDebug( "endMeshSweep()", toString() );
      _state = State::ComputeNumericalFlux;
      break;
    case State::ComputeNumericalFlux:
      logDebug( "endMeshSweep()", toString() );
      _state = State::UpdateCell;
      break;
    case State::UpdateCell:
      logInfo( "endMeshSweep()", toString() );
      _state = State::ProjectOntoFaces;
      break;
    case State::Suspend:
      _state = State::ComputeNumericalFlux;
      break;
    case State::ComputeResidualAndSuspend:
      _state = State::Suspend;
      break;
  }
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateResidual() const {
  return _state == State::UpdateCell
      or _state == State::ComputeNumericalFlux;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateFace() const {
  return _state == State::ComputeNumericalFlux;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoFaces() const {
  return _state == State::ProjectOntoFaces;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoCells() const {
  return _state == State::UpdateCell or _state==State::ComputeResidualAndSuspend;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isSuspended() const{
  return _state == State::Suspend;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspend(bool projectOntoFaces, bool computeAndRestrictResidual) {
  if (projectOntoFaces) {
    _state = State::ProjectOntoFaces;
  }
  else if (computeAndRestrictResidual) {
    _state = State::ComputeResidualAndSuspend;
  }
  else {
    _state = State::Suspend;
  }
}
