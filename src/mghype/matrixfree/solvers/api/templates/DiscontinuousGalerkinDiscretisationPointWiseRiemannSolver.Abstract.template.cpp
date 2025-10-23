{% include 'DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver.Abstract.generic.template.cpp' %}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString( State state ) {
  switch (state) {
    case State::GridConstruction:
      return "grid-construction";
    case State::InitSolution:
      return "init-solution";
    case State::Solve:
      return "solve";
    case State::Suspend:
      return "suspend";
    case State::ProjectOntoFaces:
      return "project-onto-faces";
    case State::ComputeResidualAndSuspend:
      return "compute-residual-and-suspend";
  }
  return "<undef>";
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::State {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getState() const {
  return _state;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginInitialisationSweep() {
  _state = State::InitSolution;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginGridConstructionSweep() {
  _state = State::GridConstruction;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginPlotSweep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::beginMeshSweep() {
  if (_state==State::InitSolution) {
    _state = State::ProjectOntoFaces;
  }

  if (
    _state==State::Solve
    or
    _state==State::ProjectOntoFaces
  ) {
    clearGlobalResidualAndSolutionUpdate();
  }

  logInfo( "beginMeshSweep()", toString(_state) );
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::endMeshSweep() {
  switch (_state) {
    case State::Solve:
      {
        logInfo( "endMeshSweep()", Solver::toString() );
      }
      break;
    
    /*
    This means we just prolongated from CG -> DG and projected
    the new solution back to the faces. So let's continue to solve
    as normal, remembering that the Riemann problem will be solved first.
    */
    case State::ProjectOntoFaces:
      {
        _state = State::Solve;
      }
      break;
    
    /*
    In general, if we are suspended we should remain suspended until
    the DGCGCoupling takes us out of it, ie the control flow doesn't
    belong to this class. However, we should put in a failsafe so
    that this solver ends up back in the "Solve" state in case of an
    error. 
    */
    case State::Suspend:
      {
        _state = State::ProjectOntoFaces;
      }
      break;

    case State::ComputeResidualAndSuspend:
      {
        _state = State::Suspend; 
      }
      break;

    case State::InitSolution:
      assertionMsg(false, "should never be called for State::InitSolution");
      break;

    case State::GridConstruction:
      assertionMsg(false, "should never be called for State::GridConstruction");
      break;
  }
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateResidual() const {
  return _state == State::Solve
      or _state == State::ProjectOntoFaces;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateFace() const {
  return _state == State::Solve
      or _state == State::ComputeResidualAndSuspend;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoFaces() const {
  return _state == State::Solve
      or _state == State::ProjectOntoFaces;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::projectOntoCells() const {
  return _state == State::Solve;
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
