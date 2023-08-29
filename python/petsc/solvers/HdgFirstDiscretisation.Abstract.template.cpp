#include "{{CLASSNAME}}.h"



tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _cellLocalToGlobalMap(   ::petsc::LocalToGlobalMap::RankGlobalTreeNumber ),
  _vertexLocalToGlobalMap( ::petsc::LocalToGlobalMap::RankGlobalTreeNumber ),
  _faceLocalToGlobalMap(   ::petsc::LocalToGlobalMap::RankGlobalTreeNumber )  {

}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {

}


::petsc::LocalToGlobalMap&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceLocalToGlobalMap() {
  return _faceLocalToGlobalMap;
}

::petsc::LocalToGlobalMap&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellLocalToGlobalMap() {
  return _cellLocalToGlobalMap;
}

::petsc::LocalToGlobalMap&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getVertexLocalToGlobalMap() {
  return _vertexLocalToGlobalMap;
}
