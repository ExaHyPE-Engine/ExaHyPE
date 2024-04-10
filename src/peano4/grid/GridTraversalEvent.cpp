#include "GridTraversalEvent.h"



#include <sstream>
#include <algorithm>



peano4::grid::GridTraversalEvent::GridTraversalEvent(tarch::la::Vector<Dimensions,double>  __x, tarch::la::Vector<Dimensions,double>  __h, std::bitset<TwoPowerD>  __hasBeenRefined, std::bitset<TwoPowerD>  __willBeRefined, std::bitset<TwoPowerD>  __isVertexLocal, std::bitset<TwoPowerD>  __isParentVertexLocal, std::bitset<TwoPowerD>  __isVertexParentOfSubtree, std::bitset<TwoTimesD>  __isFaceLocal, bool  __isCellLocal, bool  __isParentCellLocal, std::bitset<TwoPowerD>  __isVertexAdjacentToParallelDomainBoundary, std::bitset<TwoTimesD>  __isFaceAdjacentToParallelDomainBoundary, std::bitset<ThreePowerD>  __isAdjacentCellLocal, tarch::la::Vector<TwoPowerD,int>  __vertexDataFrom, tarch::la::Vector<TwoPowerD,int>  __vertexDataTo, tarch::la::Vector<TwoTimesD,int>  __faceDataFrom, tarch::la::Vector<TwoTimesD,int>  __faceDataTo, int  __cellData, tarch::la::Vector<Dimensions,int>  __relativePositionToFather, int  __invokingSpacetree, bool  __invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing){
setX( __x);
setH( __h);
setHasBeenRefined( __hasBeenRefined);
setWillBeRefined( __willBeRefined);
setIsVertexLocal( __isVertexLocal);
setIsParentVertexLocal( __isParentVertexLocal);
setIsVertexParentOfSubtree( __isVertexParentOfSubtree);
setIsFaceLocal( __isFaceLocal);
setIsCellLocal( __isCellLocal);
setIsParentCellLocal( __isParentCellLocal);
setIsVertexAdjacentToParallelDomainBoundary( __isVertexAdjacentToParallelDomainBoundary);
setIsFaceAdjacentToParallelDomainBoundary( __isFaceAdjacentToParallelDomainBoundary);
setIsAdjacentCellLocal( __isAdjacentCellLocal);
setVertexDataFrom( __vertexDataFrom);
setVertexDataTo( __vertexDataTo);
setFaceDataFrom( __faceDataFrom);
setFaceDataTo( __faceDataTo);
setCellData( __cellData);
setRelativePositionToFather( __relativePositionToFather);
setInvokingSpacetree( __invokingSpacetree);
setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing( __invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing);
}



peano4::grid::GridTraversalEvent::GridTraversalEvent( const GridTraversalEvent& copy ) {
  setX( copy.getX() );
  setH( copy.getH() );
  setHasBeenRefined( copy.getHasBeenRefined() );
  setWillBeRefined( copy.getWillBeRefined() );
  setIsVertexLocal( copy.getIsVertexLocal() );
  setIsParentVertexLocal( copy.getIsParentVertexLocal() );
  setIsVertexParentOfSubtree( copy.getIsVertexParentOfSubtree() );
  setIsFaceLocal( copy.getIsFaceLocal() );
  setIsCellLocal( copy.getIsCellLocal() );
  setIsParentCellLocal( copy.getIsParentCellLocal() );
  setIsVertexAdjacentToParallelDomainBoundary( copy.getIsVertexAdjacentToParallelDomainBoundary() );
  setIsFaceAdjacentToParallelDomainBoundary( copy.getIsFaceAdjacentToParallelDomainBoundary() );
  setIsAdjacentCellLocal( copy.getIsAdjacentCellLocal() );
  setVertexDataFrom( copy.getVertexDataFrom() );
  setVertexDataTo( copy.getVertexDataTo() );
  setFaceDataFrom( copy.getFaceDataFrom() );
  setFaceDataTo( copy.getFaceDataTo() );
  setCellData( copy.getCellData() );
  setRelativePositionToFather( copy.getRelativePositionToFather() );
  setInvokingSpacetree( copy.getInvokingSpacetree() );
  setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing( copy.getInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing() );
}





std::string peano4::grid::GridTraversalEvent::toString() const {
  std::ostringstream out;
  out << "(";
  out << "x=" << getX();
  out << ","; 
  out << "h=" << getH();
  out << ","; 
  out << "hasBeenRefined=" << getHasBeenRefined();
  out << ","; 
  out << "willBeRefined=" << getWillBeRefined();
  out << ","; 
  out << "isVertexLocal=" << getIsVertexLocal();
  out << ","; 
  out << "isParentVertexLocal=" << getIsParentVertexLocal();
  out << ","; 
  out << "isVertexParentOfSubtree=" << getIsVertexParentOfSubtree();
  out << ","; 
  out << "isFaceLocal=" << getIsFaceLocal();
  out << ","; 
  out << "isCellLocal=" << _isCellLocal;
  out << ","; 
  out << "isParentCellLocal=" << _isParentCellLocal;
  out << ","; 
  out << "isVertexAdjacentToParallelDomainBoundary=" << getIsVertexAdjacentToParallelDomainBoundary();
  out << ","; 
  out << "isFaceAdjacentToParallelDomainBoundary=" << getIsFaceAdjacentToParallelDomainBoundary();
  out << ","; 
  out << "isAdjacentCellLocal=" << getIsAdjacentCellLocal();
  out << ","; 
  out << "vertexDataFrom=" << _vertexDataFrom;
  out << ","; 
  out << "vertexDataTo=" << _vertexDataTo;
  out << ","; 
  out << "faceDataFrom=" << _faceDataFrom;
  out << ","; 
  out << "faceDataTo=" << _faceDataTo;
  out << ","; 
  out << "cellData=" << _cellData;
  out << ","; 
  out << "relativePositionToFather=" << getRelativePositionToFather();
  out << ","; 
  out << "invokingSpacetree=" << _invokingSpacetree;
  out << ","; 
  out << "invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing=" << _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;
  out << ")";
  return out.str();
}





tarch::la::Vector<Dimensions,double>   peano4::grid::GridTraversalEvent::getX() const {

  tarch::la::Vector<Dimensions,double> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _x[i];
  }
  return result;
      }


void   peano4::grid::GridTraversalEvent::setX(const tarch::la::Vector<Dimensions,double>& value) {

  for( int i=0; i<Dimensions; i++) {
      _x[i] = value(i);
  }
      }


double   peano4::grid::GridTraversalEvent::getX(int index) const {
  return   _x[index];
}


void   peano4::grid::GridTraversalEvent::setX(int index, double value) {
  _x[index] = value;
}


tarch::la::Vector<Dimensions,double>   peano4::grid::GridTraversalEvent::getH() const {

  tarch::la::Vector<Dimensions,double> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _h[i];
  }
  return result;
      }


void   peano4::grid::GridTraversalEvent::setH(const tarch::la::Vector<Dimensions,double>& value) {

  for( int i=0; i<Dimensions; i++) {
      _h[i] = value(i);
  }
      }


double   peano4::grid::GridTraversalEvent::getH(int index) const {
  return   _h[index];
}


void   peano4::grid::GridTraversalEvent::setH(int index, double value) {
  _h[index] = value;
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getHasBeenRefined() const {

  std::bitset<TwoPowerD> result;
  for (int i=0; i<TwoPowerD; i++) result[i] =   _hasBeenRefined[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setHasBeenRefined(const std::bitset<TwoPowerD>&  value) {

  for (int i=0; i<TwoPowerD; i++)   _hasBeenRefined[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getHasBeenRefined(int index) const {
  return   _hasBeenRefined[index];
}


void   peano4::grid::GridTraversalEvent::setHasBeenRefined(int index, bool value) {
  _hasBeenRefined[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipHasBeenRefined(int index) {
  _hasBeenRefined[index] = not   _hasBeenRefined[index];
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getWillBeRefined() const {

  std::bitset<TwoPowerD> result;
  for (int i=0; i<TwoPowerD; i++) result[i] =   _willBeRefined[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setWillBeRefined(const std::bitset<TwoPowerD>&  value) {

  for (int i=0; i<TwoPowerD; i++)   _willBeRefined[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getWillBeRefined(int index) const {
  return   _willBeRefined[index];
}


void   peano4::grid::GridTraversalEvent::setWillBeRefined(int index, bool value) {
  _willBeRefined[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipWillBeRefined(int index) {
  _willBeRefined[index] = not   _willBeRefined[index];
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getIsVertexLocal() const {

  std::bitset<TwoPowerD> result;
  for (int i=0; i<TwoPowerD; i++) result[i] =   _isVertexLocal[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setIsVertexLocal(const std::bitset<TwoPowerD>&  value) {

  for (int i=0; i<TwoPowerD; i++)   _isVertexLocal[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getIsVertexLocal(int index) const {
  return   _isVertexLocal[index];
}


void   peano4::grid::GridTraversalEvent::setIsVertexLocal(int index, bool value) {
  _isVertexLocal[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsVertexLocal(int index) {
  _isVertexLocal[index] = not   _isVertexLocal[index];
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getIsParentVertexLocal() const {

  std::bitset<TwoPowerD> result;
  for (int i=0; i<TwoPowerD; i++) result[i] =   _isParentVertexLocal[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setIsParentVertexLocal(const std::bitset<TwoPowerD>&  value) {

  for (int i=0; i<TwoPowerD; i++)   _isParentVertexLocal[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getIsParentVertexLocal(int index) const {
  return   _isParentVertexLocal[index];
}


void   peano4::grid::GridTraversalEvent::setIsParentVertexLocal(int index, bool value) {
  _isParentVertexLocal[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsParentVertexLocal(int index) {
  _isParentVertexLocal[index] = not   _isParentVertexLocal[index];
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getIsVertexParentOfSubtree() const {

  std::bitset<TwoPowerD> result;
  for (int i=0; i<TwoPowerD; i++) result[i] =   _isVertexParentOfSubtree[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setIsVertexParentOfSubtree(const std::bitset<TwoPowerD>&  value) {

  for (int i=0; i<TwoPowerD; i++)   _isVertexParentOfSubtree[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getIsVertexParentOfSubtree(int index) const {
  return   _isVertexParentOfSubtree[index];
}


void   peano4::grid::GridTraversalEvent::setIsVertexParentOfSubtree(int index, bool value) {
  _isVertexParentOfSubtree[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsVertexParentOfSubtree(int index) {
  _isVertexParentOfSubtree[index] = not   _isVertexParentOfSubtree[index];
}


std::bitset<TwoTimesD>   peano4::grid::GridTraversalEvent::getIsFaceLocal() const {

  std::bitset<TwoTimesD> result;
  for (int i=0; i<TwoTimesD; i++) result[i] =   _isFaceLocal[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setIsFaceLocal(const std::bitset<TwoTimesD>&  value) {

  for (int i=0; i<TwoTimesD; i++)   _isFaceLocal[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getIsFaceLocal(int index) const {
  return   _isFaceLocal[index];
}


void   peano4::grid::GridTraversalEvent::setIsFaceLocal(int index, bool value) {
  _isFaceLocal[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsFaceLocal(int index) {
  _isFaceLocal[index] = not   _isFaceLocal[index];
}


bool   peano4::grid::GridTraversalEvent::getIsCellLocal() const {
  return _isCellLocal;
}


void   peano4::grid::GridTraversalEvent::setIsCellLocal(bool value) {
  _isCellLocal = value;
}


bool   peano4::grid::GridTraversalEvent::getIsParentCellLocal() const {
  return _isParentCellLocal;
}


void   peano4::grid::GridTraversalEvent::setIsParentCellLocal(bool value) {
  _isParentCellLocal = value;
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getIsVertexAdjacentToParallelDomainBoundary() const {

  std::bitset<TwoPowerD> result;
  for (int i=0; i<TwoPowerD; i++) result[i] =   _isVertexAdjacentToParallelDomainBoundary[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setIsVertexAdjacentToParallelDomainBoundary(const std::bitset<TwoPowerD>&  value) {

  for (int i=0; i<TwoPowerD; i++)   _isVertexAdjacentToParallelDomainBoundary[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getIsVertexAdjacentToParallelDomainBoundary(int index) const {
  return   _isVertexAdjacentToParallelDomainBoundary[index];
}


void   peano4::grid::GridTraversalEvent::setIsVertexAdjacentToParallelDomainBoundary(int index, bool value) {
  _isVertexAdjacentToParallelDomainBoundary[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsVertexAdjacentToParallelDomainBoundary(int index) {
  _isVertexAdjacentToParallelDomainBoundary[index] = not   _isVertexAdjacentToParallelDomainBoundary[index];
}


std::bitset<TwoTimesD>   peano4::grid::GridTraversalEvent::getIsFaceAdjacentToParallelDomainBoundary() const {

  std::bitset<TwoTimesD> result;
  for (int i=0; i<TwoTimesD; i++) result[i] =   _isFaceAdjacentToParallelDomainBoundary[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setIsFaceAdjacentToParallelDomainBoundary(const std::bitset<TwoTimesD>&  value) {

  for (int i=0; i<TwoTimesD; i++)   _isFaceAdjacentToParallelDomainBoundary[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getIsFaceAdjacentToParallelDomainBoundary(int index) const {
  return   _isFaceAdjacentToParallelDomainBoundary[index];
}


void   peano4::grid::GridTraversalEvent::setIsFaceAdjacentToParallelDomainBoundary(int index, bool value) {
  _isFaceAdjacentToParallelDomainBoundary[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsFaceAdjacentToParallelDomainBoundary(int index) {
  _isFaceAdjacentToParallelDomainBoundary[index] = not   _isFaceAdjacentToParallelDomainBoundary[index];
}


std::bitset<ThreePowerD>   peano4::grid::GridTraversalEvent::getIsAdjacentCellLocal() const {

  std::bitset<ThreePowerD> result;
  for (int i=0; i<ThreePowerD; i++) result[i] =   _isAdjacentCellLocal[i];
  return result;
}


void   peano4::grid::GridTraversalEvent::setIsAdjacentCellLocal(const std::bitset<ThreePowerD>&  value) {

  for (int i=0; i<ThreePowerD; i++)   _isAdjacentCellLocal[i]=value[i];
}


bool   peano4::grid::GridTraversalEvent::getIsAdjacentCellLocal(int index) const {
  return   _isAdjacentCellLocal[index];
}


void   peano4::grid::GridTraversalEvent::setIsAdjacentCellLocal(int index, bool value) {
  _isAdjacentCellLocal[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsAdjacentCellLocal(int index) {
  _isAdjacentCellLocal[index] = not   _isAdjacentCellLocal[index];
}


tarch::la::Vector<TwoPowerD,int>   peano4::grid::GridTraversalEvent::getVertexDataFrom() const {
  return    _vertexDataFrom;
}


void   peano4::grid::GridTraversalEvent::setVertexDataFrom(const tarch::la::Vector<TwoPowerD,int>& value) {
  _vertexDataFrom = value;
}


int   peano4::grid::GridTraversalEvent::getVertexDataFrom(int index) const {
  return   _vertexDataFrom(index);
}


void   peano4::grid::GridTraversalEvent::setVertexDataFrom(int index, int value) {
  _vertexDataFrom(index) = value;
}


tarch::la::Vector<TwoPowerD,int>   peano4::grid::GridTraversalEvent::getVertexDataTo() const {
  return    _vertexDataTo;
}


void   peano4::grid::GridTraversalEvent::setVertexDataTo(const tarch::la::Vector<TwoPowerD,int>& value) {
  _vertexDataTo = value;
}


int   peano4::grid::GridTraversalEvent::getVertexDataTo(int index) const {
  return   _vertexDataTo(index);
}


void   peano4::grid::GridTraversalEvent::setVertexDataTo(int index, int value) {
  _vertexDataTo(index) = value;
}


tarch::la::Vector<TwoTimesD,int>   peano4::grid::GridTraversalEvent::getFaceDataFrom() const {
  return    _faceDataFrom;
}


void   peano4::grid::GridTraversalEvent::setFaceDataFrom(const tarch::la::Vector<TwoTimesD,int>& value) {
  _faceDataFrom = value;
}


int   peano4::grid::GridTraversalEvent::getFaceDataFrom(int index) const {
  return   _faceDataFrom(index);
}


void   peano4::grid::GridTraversalEvent::setFaceDataFrom(int index, int value) {
  _faceDataFrom(index) = value;
}


tarch::la::Vector<TwoTimesD,int>   peano4::grid::GridTraversalEvent::getFaceDataTo() const {
  return    _faceDataTo;
}


void   peano4::grid::GridTraversalEvent::setFaceDataTo(const tarch::la::Vector<TwoTimesD,int>& value) {
  _faceDataTo = value;
}


int   peano4::grid::GridTraversalEvent::getFaceDataTo(int index) const {
  return   _faceDataTo(index);
}


void   peano4::grid::GridTraversalEvent::setFaceDataTo(int index, int value) {
  _faceDataTo(index) = value;
}


int   peano4::grid::GridTraversalEvent::getCellData() const {
  return _cellData;
}


void   peano4::grid::GridTraversalEvent::setCellData(int value) {
  _cellData = value;
}


tarch::la::Vector<Dimensions,int>   peano4::grid::GridTraversalEvent::getRelativePositionToFather() const {

  tarch::la::Vector<Dimensions,int> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _relativePositionToFather[i];
  }
  return result;
      }


void   peano4::grid::GridTraversalEvent::setRelativePositionToFather(const tarch::la::Vector<Dimensions,int>& value) {

  for( int i=0; i<Dimensions; i++) {
      _relativePositionToFather[i] = value(i);
  }
      }


int   peano4::grid::GridTraversalEvent::getRelativePositionToFather(int index) const {
  return   _relativePositionToFather[index];
}


void   peano4::grid::GridTraversalEvent::setRelativePositionToFather(int index, int value) {
  _relativePositionToFather[index] = value;
}


int   peano4::grid::GridTraversalEvent::getInvokingSpacetree() const {
  return _invokingSpacetree;
}


void   peano4::grid::GridTraversalEvent::setInvokingSpacetree(int value) {
  _invokingSpacetree = value;
}


bool   peano4::grid::GridTraversalEvent::getInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing() const {
  return _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;
}


void   peano4::grid::GridTraversalEvent::setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(bool value) {
  _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype peano4::grid::GridTraversalEvent::Datatype = MPI_DATATYPE_NULL;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridTraversalEvent::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridTraversalEvent::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridTraversalEvent::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridTraversalEvent::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridTraversalEvent::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridTraversalEvent::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridTraversalEvent::freeGlobalCommunciationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridTraversalEvent::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridTraversalEvent::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridTraversalEvent::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


int peano4::grid::GridTraversalEvent::getSenderRank() const {
  return _senderDestinationRank;
}



void peano4::grid::GridTraversalEvent::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  peano4::grid::GridTraversalEvent  instances[2];

  int NumberOfAttributes = 0;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;

  MPI_Datatype* subtypes = new MPI_Datatype[NumberOfAttributes];
  int*          blocklen = new int[NumberOfAttributes];
  MPI_Aint*     disp     = new MPI_Aint[NumberOfAttributes];

  int counter            = 0;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TwoPowerD;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TwoPowerD;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TwoTimesD;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TwoTimesD;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );

  counter = 0;
  MPI_Get_address( &(instances[0]._x.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._h.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._hasBeenRefined), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._willBeRefined), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isVertexLocal), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isParentVertexLocal), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isVertexParentOfSubtree), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isFaceLocal), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isCellLocal), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isParentCellLocal), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isVertexAdjacentToParallelDomainBoundary), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isFaceAdjacentToParallelDomainBoundary), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isAdjacentCellLocal), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._vertexDataFrom.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._vertexDataTo.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._faceDataFrom.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._faceDataTo.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._cellData), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._relativePositionToFather.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._invokingSpacetree), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing), &disp[counter] );
  counter++;

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

  delete[] subtypes;
  delete[] blocklen;
  delete[] disp;

  #else
  // invoke routine once to trigger lazy initialisation
  getForkDatatype();
  getJoinDatatype();
  getBoundaryExchangeDatatype();
  getMultiscaleDataExchangeDatatype();
  getGlobalCommunciationDatatype();
  #endif
}


void peano4::grid::GridTraversalEvent::shutdownDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunciationDatatype();
  #else
  MPI_Datatype type = Datatype;
  MPI_Type_free( &type );
  #endif
}


void peano4::grid::GridTraversalEvent::send(const peano4::grid::GridTraversalEvent& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::grid::GridTraversalEvent::receive(peano4::grid::GridTraversalEvent& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::grid::GridTraversalEvent::send(
  const peano4::grid::GridTraversalEvent& buffer,
  int destination,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  MPI_Request sendRequestHandle;
  int         flag = 0;
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle );
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  }
}


void peano4::grid::GridTraversalEvent::receive(
  peano4::grid::GridTraversalEvent& buffer,
  int source,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  MPI_Status  status;
  MPI_Request receiveRequestHandle;
  int         flag = 0;
  MPI_Irecv( &buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle );
  MPI_Test( &receiveRequestHandle, &flag, &status );
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test( &receiveRequestHandle, &flag, &status );
  }
  buffer._senderDestinationRank = status.MPI_SOURCE;
}
#endif

#ifdef Parallel
void peano4::grid::GridTraversalEvent::sendAndPollDanglingMessages(const peano4::grid::GridTraversalEvent& message, int destination, int tag, MPI_Comm communicator ) {
  peano4::grid::GridTraversalEvent::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridTraversalEvent", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridTraversalEvent", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::grid::GridTraversalEvent::receiveAndPollDanglingMessages(peano4::grid::GridTraversalEvent& message, int source, int tag, MPI_Comm communicator ) {
  peano4::grid::GridTraversalEvent::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridTraversalEvent", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridTraversalEvent", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    