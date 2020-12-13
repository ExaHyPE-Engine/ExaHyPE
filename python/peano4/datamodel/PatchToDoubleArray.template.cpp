#include "{{CLASSNAME}}.h"


{{INCLUDES}}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(const {{CLASSNAME}}& other) {
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


{{NAMESPACE | join("::")}}::{{CLASSNAME}}& {{NAMESPACE | join("::")}}::{{CLASSNAME}}::operator=(const {{CLASSNAME}}& other) {
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
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(const {{CLASSNAME}}& neighbour, const peano4::datamanagement::VertexMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(const peano4::datamanagement::VertexMarker& marker) {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(const peano4::datamanagement::VertexMarker& marker) {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return {{LOAD_PERSISTENT_CONDITION}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 2 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(const {{CLASSNAME}}& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(const peano4::datamanagement::FaceMarker& marker) {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::FaceMarker& marker) {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(const peano4::datamanagement::FaceMarker& marker) {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::FaceMarker& marker) {
  return {{LOAD_PERSISTENT_CONDITION}};
}
{% endif -%}


{% if DATA_ASSOCIATION == 3 -%}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::merge(const {{CLASSNAME}}& neighbour, const peano4::datamanagement::CellMarker& marker) {
  {{MERGE_METHOD_DEFINITION}}
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::send(const peano4::datamanagement::CellMarker& marker) {
  return {{SEND_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::receiveAndMerge(const peano4::datamanagement::CellMarker& marker) {
  return {{RECEIVE_AND_MERGE_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::storePersistently(const peano4::datamanagement::CellMarker& marker) {
  return {{STORE_PERSISTENT_CONDITION}};
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::loadPersistently(const peano4::datamanagement::CellMarker& marker) {
  return {{LOAD_PERSISTENT_CONDITION}};
}
{% endif -%}
  

  
#ifdef Parallel
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initDatatype() {
  {{NAMESPACE | join("::")}}::{{CLASSNAME}}  instances[2];
 
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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}
  

MPI_Datatype   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Datatype;
#endif
