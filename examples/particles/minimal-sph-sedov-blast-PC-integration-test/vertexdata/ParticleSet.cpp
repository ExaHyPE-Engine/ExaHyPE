#include "ParticleSet.h"
#include "globaldata/Particle.h"


#if PeanoDebug>=1
void examples::particles::vertexdata::ParticleSet::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void examples::particles::vertexdata::ParticleSet::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> examples::particles::vertexdata::ParticleSet::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> examples::particles::vertexdata::ParticleSet::getDebugH() const {
  return _debugH;
}
#endif


void examples::particles::vertexdata::ParticleSet::merge(const ParticleSet& neighbour, const peano4::datamanagement::VertexMarker& marker) {
}


bool examples::particles::vertexdata::ParticleSet::send(const peano4::datamanagement::VertexMarker& marker) {
  return false;
}


bool examples::particles::vertexdata::ParticleSet::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return false;
}


bool examples::particles::vertexdata::ParticleSet::storePersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool examples::particles::vertexdata::ParticleSet::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


std::string examples::particles::vertexdata::ParticleSet::toString() const {
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
void examples::particles::vertexdata::ParticleSet::initDatatype() {
  DoFType::initDatatype();
}


void examples::particles::vertexdata::ParticleSet::shutdownDatatype() {
  DoFType::shutdownDatatype();
}
#endif


double examples::particles::vertexdata::ParticleSet::getMinCutOffRadius() const {
  double result = std::numeric_limits<double>::max();
  for (auto& p: *this) {
    result = std::min( result, p->getCutOffRadius() );
  }
  return result;
}