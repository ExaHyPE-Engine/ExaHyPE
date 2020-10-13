#include "{{CLASSNAME}}.h"


{{INCLUDES}}


{% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::{{CLASSNAME}}() {
}


{% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::{{CLASSNAME}}(const {{CLASSNAME}}& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy(other.value, other.value+{{CARDINALITY_2D}},value);
  #else
  std::copy(other.value, other.value+{{CARDINALITY_3D}},value);
  #endif
}


{% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}& {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::operator=(const {{CLASSNAME}}& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif
  
  #if Dimensions==2
  std::copy(other.value, other.value+{{CARDINALITY_2D}},value);
  #else
  std::copy(other.value, other.value+{{CARDINALITY_3D}},value);
  #endif
  return *this;
}


std::string {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::toString() const {
  std::string result = std::string("()");
  return result;
}


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


{% if DATA_ASSOCIATION == 1 -%}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::merge(const {{CLASSNAME}}& neighbour, const peano4::datamanagement::VertexMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::send(const peano4::datamanagement::VertexMarker& marker) {
  return {{SEND_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::storePersistently(const peano4::datamanagement::VertexMarker& marker) {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return {{LOAD_PERSISTENT_CONDITION}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 2 -%}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::merge(const {{CLASSNAME}}& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::send(const peano4::datamanagement::FaceMarker& marker) {
  return {{SEND_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::FaceMarker& marker) {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::storePersistently(const peano4::datamanagement::FaceMarker& marker) {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::FaceMarker& marker) {
  return {{LOAD_PERSISTENT_CONDITION}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 3 -%}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::merge(const {{CLASSNAME}}& neighbour, const peano4::datamanagement::CellMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::send(const peano4::datamanagement::CellMarker& marker) {
  return {{SEND_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::CellMarker& marker) {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::storePersistently(const peano4::datamanagement::CellMarker& marker) {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::CellMarker& marker) {
  return {{LOAD_PERSISTENT_CONDITION}};
}
{% endif -%}
  

  
#ifdef Parallel
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::initDatatype() {
  {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}  instances[2];
 
  #if PeanoDebug>=1
    MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };

    #if Dimensions==2
    int blocklen[] = { Dimensions, Dimensions, {{CARDINALITY_2D}} };
    #else
    int blocklen[] = { Dimensions, Dimensions, {{CARDINALITY_3D}} };
    #endif

    const int NumberOfAttributes = 3;
  #else   
    MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
    #if Dimensions==2
    int blocklen[] = { {{CARDINALITY_2D}} };
    #else
    int blocklen[] = { {{CARDINALITY_3D}} };
    #endif
  
    const int NumberOfAttributes = 1;  
  #endif

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  #if PeanoDebug>=1
  MPI_Get_address( &(instances[0]._debugX.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._debugH.data()[0]), &disp[currentAddress] );
  currentAddress++;
  #endif
  MPI_Get_address( &(instances[0].value), &disp[currentAddress] );
  currentAddress++;

  MPI_Aint offset = disp[0] - baseFirstInstance;
  MPI_Aint extent = baseSecondInstance - baseFirstInstance - offset;
  for (int i=NumberOfAttributes-1; i>=0; i--) {
    disp[i] = disp[i] - disp[0];
  }

  int errorCode = 0; 
  MPI_Datatype tmpType; 
  errorCode += MPI_Type_create_struct( NumberOfAttributes, blocklen, disp, subtypes, &tmpType );
  errorCode += MPI_Type_create_resized( tmpType, offset, extent, &Datatype );
  errorCode += MPI_Type_commit( &Datatype );
  errorCode += MPI_Type_free( &tmpType );
  if (errorCode) std::cerr << "error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::Datatype;
#endif
