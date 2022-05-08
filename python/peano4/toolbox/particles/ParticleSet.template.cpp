#include "{{CLASSNAME}}.h"
#include "globaldata/{{PARTICLE_TYPE}}.h"


tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


#if PeanoDebug>=1
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getDebugH() const {
  return _debugH;
}
#endif


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(const {{CLASSNAME}}& neighbour, const peano4::datamanagement::VertexMarker& marker) {
  // @todo Hier muss man noch nachdenken:
  // - hier ist alles shallow, weil wir eben doch denken, dass die echten Kopien schon gemacht worden sind

  // @todo Muessen wir evtl. Redundanzen bereinigen? Ich vermute schon, weil wenn ein Partikel
  //. eine Domaene verlaesst, dann nehme n erst mal alle anderen an, dass es ihnen gehoert fuer
  //  eine Iterationen und setzen dementsprechend das Flag net um
  for (auto p: neighbour) {
    if (
      p->getParallelState()==DoFType::ParallelState::Local
      and
      p->getNewParallelState()==DoFType::NewParallelState::Local
    ) {
      logDebug( "clone(...)", "assume incoming particle " << p->toString() << " is local on other rank and will be virtual here" );
      p->setParallelState( DoFType::ParallelState::Virtual );
      this->push_back(p);
    }
    else if (
      p->getParallelState()==DoFType::ParallelState::Virtual
      and
      p->getNewParallelState()==DoFType::NewParallelState::Local
    ) {
      assertion1( p->getMoveState()==DoFType::MoveState::New, p->toString() );
      logDebug( "clone(...)", "incoming particle " << p->toString() << " has been virtual but neighbour decided it is local there just brefore sending us copy. This can happen throughout initialisation. Will be virtual here" );
      p->setParallelState( DoFType::ParallelState::Virtual );
      this->push_back(p);
    }
    else if (
      p->getParallelState()==DoFType::ParallelState::Local
      and
      p->getNewParallelState()==DoFType::NewParallelState::Virtual
    ) {
      logDebug( "clone(...)", "incoming particle " << p->toString() << " is now marked as virtual on remote tree, i.e. has left its domain. Assume it is local here, though we might revise this decision (as particle has actually gone somewhere else)" );
      p->setParallelState( DoFType::ParallelState::Local );
      this->push_back(p);
    }
    else if (
      p->getParallelState()==DoFType::ParallelState::Virtual
      and
      p->getNewParallelState()==DoFType::NewParallelState::Virtual
    ) {
      logDebug( "clone(...)", "incoming particle " << p->toString() << " had been virtual on other rank and still is virtual. Might reside on global boundary. Ignore here" );
    }
    else {
      assertion1(false,p->toString());
    }
  }
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream msg;
  msg << "(size=" << size();
  #if PeanoDebug>=1
  msg << ",x=" << _debugX
      << ",h=" << _debugH;
  #endif
  msg << ")";
  return msg.str();
}


#ifdef Parallel
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initDatatype() {
  DoFType::initDatatype();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::shutdownDatatype() {
  DoFType::shutdownDatatype();
}
#endif


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinCutOffRadius() const {
  double result = std::numeric_limits<double>::max();
  for (auto& p: *this) {
    result = std::min( result, p->getCutOffRadius() );
  }
  return result;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone( const {{CLASSNAME}}& otherSet ) {
  assertion1( empty(), toString() );
  for (auto p: otherSet) {
    DoFType* newParticle =  new DoFType(*p);
    this->push_back( newParticle );
  }
  _debugX = otherSet._debugX;
  _debugH = otherSet._debugH;
}

