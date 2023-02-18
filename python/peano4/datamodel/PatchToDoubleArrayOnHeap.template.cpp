#include "{{CLASSNAME}}.h"

#include "tarch/accelerator/accelerator.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _dataSmartPointer(
    static_cast< {{FLOAT_TYPE}}* >( allocateMemory(Cardinality, tarch::MemoryLocation::Heap) )
  ) {
  value = &( _dataSmartPointer[0] );
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(const {{CLASSNAME}}& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone(const {{CLASSNAME}}& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  #if Dimensions==2
  std::copy_n( other.value, {{CARDINALITY_2D}}, value );
  #else
  std::copy_n( other.value, {{CARDINALITY_3D}}, value );
  #endif
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}& {{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=(const {{CLASSNAME}}& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = &( _dataSmartPointer[0] );
  
  return *this;
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream result;
  result << "(";
  #if PeanoDebug>=1
  result << "x=" << _debugX;
  result << ",";
  result << "h=" << _debugH;
  #endif
  result << ")";
  return result.str();
}


#if PeanoDebug>=1

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setDebugX( const tarch::la::Vector<Dimensions,double>& data ) {
  _debugX = data;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setDebugH( const tarch::la::Vector<Dimensions,double>& data ) {
  // At the moment, we only use square/cubic elements
  assertion2( data(0)>0.0, data, toString() );
  assertion2( data(0)==data(1), data, toString() );
  #if Dimensions==3
  assertion2( data(1)==data(2), data, toString() );
  #endif
  _debugH = data;
}


tarch::la::Vector<Dimensions,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getDebugX() const {
  return _debugX;
}


tarch::la::Vector<Dimensions,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getDebugH() const {
  return _debugH;
}
#endif


{% if DATA_ASSOCIATION == 1 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const peano4::datamanagement::VertexMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(
  const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return {{LOAD_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(
  const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::compute(
  const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{COMPUTE_CONDITION}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 2 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadPersistently(
  const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) {
  return {{LOAD_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(
  const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::compute(
  const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{COMPUTE_CONDITION}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 3 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const peano4::datamanagement::CellMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadPersistently(
  const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) {
  return {{LOAD_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(
  const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::compute(
  const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{COMPUTE_CONDITION}};
}
{% endif -%}
  

  
#ifdef Parallel
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initDatatype() {
  int errorCode = MPI_Type_vector(
    1,           // number of blocks (count)
    Cardinality, // number of entries per block (blocklength),
    0,           // no stride as we work with a continuous datatype,
    {{MPI_FLOAT_TYPE}},
    &Datatype
  );
  
  errorCode += MPI_Type_commit( &Datatype );

  if (errorCode) std::cerr << "error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Datatype;


MPI_Datatype  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getForkDatatype() {
  return Datatype;
}


MPI_Datatype  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getJoinDatatype() {
  return Datatype;
}


MPI_Datatype  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getBoundaryExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


MPI_Datatype  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getGlobalCommunciationDatatype() {
  return Datatype;
}

#endif
