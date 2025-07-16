#include "{{CLASSNAME}}.h"

#include "tarch/accelerator/accelerator.h"

#include <algorithm>


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
  _dataSmartPointer( other._dataSmartPointer ) {
  value = other.value==nullptr ? nullptr : _dataSmartPointer.get();
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}({{CLASSNAME}}&& other) noexcept:
  _dataSmartPointer(std::move(other._dataSmartPointer)) {

  value = other.value;
  other.value = nullptr;  // Leave source in a valid state
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}& {{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=({{CLASSNAME}}&& other) noexcept {
  if (this != &other) {
    _dataSmartPointer = std::move(other._dataSmartPointer);
    value = other.value;
    other.value = nullptr;  // Leave source in a valid state
  }
  return *this;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::clone(const {{CLASSNAME}}& other) {
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
  _dataSmartPointer = other._dataSmartPointer;
  value = _dataSmartPointer.get();
  
  return *this;
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString() const {
  std::ostringstream result;
  result << "(";
  if ( value==nullptr ) {
    result << "no-data";
  }
  else {
    result << "smart-pointer";
  }
  result << ")";
  return result.str();
}


{% if DATA_ASSOCIATION == 1 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] peano4::grid::TraversalObserver::SendReceiveContext context,
  [[maybe_unused]] const {{CLASSNAME}}& neighbour,
  [[maybe_unused]] const peano4::datamanagement::VertexMarker& marker,
  [[maybe_unused]] int spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  [[maybe_unused]] const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  [[maybe_unused]] const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag(
  [[maybe_unused]] const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 2 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] peano4::grid::TraversalObserver::SendReceiveContext context,
  [[maybe_unused]] const {{CLASSNAME}}& neighbour,
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker,
  [[maybe_unused]] int spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag(
  [[maybe_unused]] const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 3 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(
  [[maybe_unused]] peano4::grid::TraversalObserver::SendReceiveContext context,
  [[maybe_unused]] const {{CLASSNAME}}& neighbour,
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker,
  [[maybe_unused]] int spacetreeId
) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) const {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


::peano4::grid::LoadStoreComputeFlag {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadStoreComputeFlag(
  [[maybe_unused]] const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[1]}}& {{arg[2]}} {% endfor %}
) {
  return {{LOAD_STORE_COMPUTE_FLAG}};
}
{% endif -%}

tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Cardinality, {{FLOAT_TYPE}} > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::get() const {
  tarch::la::Vector< Cardinality, {{FLOAT_TYPE}} > output(0.0);
  if (value != nullptr) std::copy_n( value, Cardinality, output.data() );
  return output;
}

{{FLOAT_TYPE}} {{NAMESPACE | join("::")}}::{{CLASSNAME}}::get(int index) const {
  return _dataSmartPointer[index];
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::set( const tarch::la::Vector< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Cardinality, {{FLOAT_TYPE}} >& v ) {
  if (value != nullptr) std::copy_n( v.data(), Cardinality, value );
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::set( int index, {{FLOAT_TYPE}} value ) {
  _dataSmartPointer[index] = value;
}

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


MPI_Datatype  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getGlobalCommunicationDatatype() {
  return Datatype;
}

#endif
