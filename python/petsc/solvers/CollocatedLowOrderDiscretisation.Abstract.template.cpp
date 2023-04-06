#include "{{CLASSNAME}}.h"



tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _localToGlobalMap( ::petsc::LocalToGlobalMap::RankGlobalTreeNumber )  {

}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {

}


::petsc::LocalToGlobalMap&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalToGlobalMap() {
  return _localToGlobalMap;
}
