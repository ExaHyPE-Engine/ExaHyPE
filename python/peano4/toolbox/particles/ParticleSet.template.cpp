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
  for (auto p: neighbour) {
    if (
      p->getParallelState()==DoFType::ParallelState::Local
    ) {
      logDebug( "merge(...)", "assume incoming particle " << p->toString() << " is local on other rank and will be virtual here" );
      p->setParallelState( DoFType::ParallelState::Virtual );
      this->push_back(p);
    }
    else if (
      p->getParallelState()==DoFType::ParallelState::Virtual
    ) {
      logDebug( "merge(...)", "incoming particle " << p->toString() << " is virtual on neighbour, but it was not deleted there. Assume it has just switched from local to virtual as it has moved and thus is now local here" );
      p->setParallelState( DoFType::ParallelState::Local );
      this->push_back(p);
    }
    else {
      assertion1(false,p->toString());
    }
  }

  eliminateSpatialDuplicates( true );

  logDebug( "merge(...)", "merged incoming set of " << neighbour.size() << " particle(s) into local set of vertex " << marker.x() << " which now hosts " << this->size() << " particle(s):" );
  for (auto& p: *this) {
    logDebug( "merge(...)", " - " << p->toString() );
  }
}


int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::eliminateSpatialDuplicates(bool examineOnlyVirtualParticles) {
  int numberOfEliminations = 0;
  auto p = this->begin();
  while (p!=this->end()) {
    bool foundDuplicate = false;

    if (
      examineOnlyVirtualParticles
      and
      (*p)->getParallelState()==DoFType::ParallelState::Virtual
    ) {
      auto pp = this->begin();
      while (
        pp!=this->end()
        and
        not foundDuplicate
      ) {
        if (
          pp!=p
          and
          tarch::la::equals( (*p)->getX(), (*pp)->getX() )
        ) {
          foundDuplicate = true;
          pp = this->end();
        }
        else {
          pp++;
        }
      }
    }
    else if ( not examineOnlyVirtualParticles ) {
      auto pp = p;
      pp++;
      while (
        pp!=this->end()
        and
        not foundDuplicate
      ) {
        if (
          not examineOnlyVirtualParticles
          and
          tarch::la::equals( (*p)->getX(), (*pp)->getX() )
        ) {
          foundDuplicate = true;
          pp = this->end();
        }
        else {
          pp++;
        }
      }
    }

    if (foundDuplicate) {
      logDebug( "eliminateSpatialDuplicates()", "eliminate particle pp=" << (*p)->toString() << " as it is a duplicate" );
      delete *p;
      p = erase(p);
      numberOfEliminations++;
    }
    else {
      p++;
    }
  }
  return numberOfEliminations;
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
  #if PeanoDebug>=1
  _debugX = otherSet._debugX;
  _debugH = otherSet._debugH;
  #endif
}

