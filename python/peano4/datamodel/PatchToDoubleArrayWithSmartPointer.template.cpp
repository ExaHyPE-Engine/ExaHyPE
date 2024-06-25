#include "{{CLASSNAME}}.h"

#include "tarch/accelerator/accelerator.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _dataSmartPointer(
    tarch::allocateMemory< {{FLOAT_TYPE}} >( {{MPI_ELEMENTS_PER_FLOAT}}*Cardinality, tarch::MemoryLocation::Heap ),
    [] (auto p) {
      tarch::freeMemory( p, tarch::MemoryLocation::Heap );
    }
  ) {
  value = _dataSmartPointer.get();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(ObjectConstruction):
  _dataSmartPointer(
    nullptr,
    [] (auto p) {
      tarch::freeMemory( p, tarch::MemoryLocation::Heap );
    }
  ) {
  value = nullptr;
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(const {{CLASSNAME}}& other):
  #if PeanoDebug>=1
  _debugX( other._debugX ),
  _debugH( other._debugH ),
  #endif
  _dataSmartPointer( other._dataSmartPointer ) {
  value = other.value==nullptr ? nullptr : _dataSmartPointer.get();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone(const {{CLASSNAME}}& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  assertion2( value==nullptr, toString(), other.toString() );

  if (other.value!=nullptr) {
    _dataSmartPointer = std::shared_ptr< {{FLOAT_TYPE}}[] >(
      tarch::allocateMemory< {{FLOAT_TYPE}} >( {{MPI_ELEMENTS_PER_FLOAT}}*Cardinality, tarch::MemoryLocation::Heap ),
      [] (auto p) {
        tarch::freeMemory( p, tarch::MemoryLocation::Heap );
      }
    );
    value = _dataSmartPointer.get();

    #if Dimensions==2
    std::copy_n( other.value, {{CARDINALITY_2D}}, value );
    #else
    std::copy_n( other.value, {{CARDINALITY_3D}}, value );
    #endif
  }
  else {
    value = nullptr;
  }
}


std::shared_ptr< {{FLOAT_TYPE}}[] > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSmartPointer() const {
  return _dataSmartPointer;
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}& {{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=(const {{CLASSNAME}}& other) {
  #if PeanoDebug>=1
  _debugX = other._debugX;
  _debugH = other._debugH;
  #endif

  _dataSmartPointer = other._dataSmartPointer;
  value = _dataSmartPointer.get();
  
  return *this;
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream result;
  result << "(";
  #if PeanoDebug>=1
  result << "x=" << _debugX;
  result << ",";
  result << "h=" << _debugH;
  result << ",";
  #endif
  if ( value==nullptr ) {
    result << "no-data";
  }
  else {
    result << "smart-pointer";
  }
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
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const peano4::datamanagement::VertexMarker& marker, int spacetreeId) {
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


::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag(
  const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 2 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const peano4::datamanagement::FaceMarker& marker, int spacetreeId) {
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


::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag(
  const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 3 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const peano4::datamanagement::CellMarker& marker, int spacetreeId) {
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


::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag(
  const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif -%}


#ifdef Parallel
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initDatatype() {
  int errorCode = MPI_Type_vector(
    1,           // number of blocks (count)
    {{MPI_ELEMENTS_PER_FLOAT}}*Cardinality, // number of entries per block (blocklength),
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
