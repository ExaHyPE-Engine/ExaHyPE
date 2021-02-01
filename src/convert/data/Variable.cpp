#include "Variable.h"
#include "tarch/Assertions.h"
#include "tarch/la/ScalarOperations.h"


convert::data::Variable::Variable(std::string name_, int dofsPerAxis_, int unknowns_, PeanoDataType type_, int dimensions_):
  name(name_),
  dofsPerAxis(dofsPerAxis_),
  unknowns(unknowns_),
  type(type_),
  dimensions(dimensions_) {
  assertion1( dofsPerAxis>=1, dofsPerAxis );
  assertion1( unknowns>=1, unknowns );
}


bool convert::data::Variable::operator<( const convert::data::Variable& rhs ) const {
  return name<rhs.name;
}


bool convert::data::Variable::operator==( const convert::data::Variable& rhs ) const {
  return name==rhs.name;
}


int convert::data::Variable::getTotalNumberOfQuantitiesPerPatch() const {
  return unknowns * getTotalNumberOfDofsPerPatch();
}


int convert::data::Variable::getTotalNumberOfDofsPerPatch() const {
  return tarch::la::aPowI(dimensions,dofsPerAxis);
}


int convert::data::Variable::getVerticesPerAxisInCartesianMesh() const {
  if (type==PeanoDataType::Cell_Values) {
	return dofsPerAxis+1;
  }
  else {
	return dofsPerAxis;
  }
}


int convert::data::Variable::getTotalNumberOfVerticesInCartesianMesh() const {
  return tarch::la::aPowI(dimensions,getVerticesPerAxisInCartesianMesh());
}


int convert::data::Variable::getTotalNumberOfCellsInCartesianMesh() const {
  return tarch::la::aPowI(dimensions,getVerticesPerAxisInCartesianMesh()-1);
}


std::string convert::data::Variable::toString() const {
  std::ostringstream msg;

  msg << "(name:" << name
      << ",dofs-per-axis:" << dofsPerAxis
      << ",unknowns:" << unknowns
      << ",type:" << (type==convert::data::PeanoDataType::Cell_Values ? "cell-values" : "vertex-values" )
      << ",dim:" << dimensions
      << ")";

  return msg.str();
}
