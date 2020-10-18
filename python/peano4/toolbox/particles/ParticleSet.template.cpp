#include "{{CLASSNAME}}.h"
#include "globaldata/{{PARTICLE_TYPE}}.h"


#if PeanoDebug>=1
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getDebugH() const {
  return _debugH;
}
#endif


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::merge(const {{CLASSNAME}}& neighbour, const peano4::datamanagement::VertexMarker& marker) {
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::send(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::storePersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


std::string {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::toString() const {
  std::ostringstream msg;
  msg << "(#" << size();
  #if PeanoDebug>=1
  msg << ",x=" << _debugX
      << ",h=" << _debugH;
  #endif
  msg << ")";
  return msg.str();
}


#ifdef Parallel
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::initDatatype() {
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::shutdownDatatype() {
}


MPI_Datatype   {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::Datatype;
#endif
