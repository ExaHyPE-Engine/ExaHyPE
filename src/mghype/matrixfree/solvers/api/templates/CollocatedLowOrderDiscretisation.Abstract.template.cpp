#include "{{CLASSNAME}}.h"
tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {}


{% if LOCAL_ASSEMBLY_MATRIX!=[] %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalAssemblyMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > result = {
      {{LOCAL_ASSEMBLY_MATRIX[0]| join(", ")}}
      {% for ROW in LOCAL_ASSEMBLY_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  // @todo add in scaling

  return result;
}
{% endif %}

{% if MASS_MATRIX!=[] %}
tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMassMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{VERTEX_CARDINALITY}}*TwoPowerD, {{VERTEX_CARDINALITY}}*TwoPowerD, double > result = {
      {{MASS_MATRIX[0]| join(", ")}}
      {% for ROW in MASS_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };

  // @todo add in scaling

  return result;
}
{% endif %}


{{NAMESPACE | join("::")}}::vertexdata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getVertexDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double > & x ) -> bool{
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  return isOnBoundary(x) ? vertexdata::{{SOLVER_NAME}}::Type::Boundary : vertexdata::{{SOLVER_NAME}}::Type::Interior;
}

{{NAMESPACE | join("::")}}::celldata::{{SOLVER_NAME}}::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return celldata::{{SOLVER_NAME}}::Type::Interior;
}

