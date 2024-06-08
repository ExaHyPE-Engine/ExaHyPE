#include "{{CLASSNAME}}.h"



tarch::logging::Log  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );


const double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::QuadraturePointsInUnitInterval[] = {
  {{QUADRATURE_POINTS_IN_UNIT_INTERVAL| join(", ")}}
};

const double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::GaussianIntegrationPoints[] = {
  {{GAUSSIAN_INTEGRATION_POINTS| join(", ")}}
};

const double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::GaussianIntegrationWeights[] = {
  {{GAUSSIAN_INTEGRATION_WEIGHTS| join(", ")}}
};

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _localToGlobalMap( ::petsc::LocalToGlobalMap::RankGlobalTreeNumber )  {
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
}


::petsc::LocalToGlobalMap&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLocalToGlobalMap() {
  return _localToGlobalMap;
}


::petsc::LinearEquationSystem&  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLinearEquationSystem() {
  return _linearEquationSystem;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishAssembly() {
}


{% if CELL_CELL_LHS_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > result = {
      {{CELL_CELL_LHS_MATRIX[0]| join(", ")}}
      {% for ROW in CELL_CELL_LHS_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  double scaling = 1.0;
  {% if CELL_CELL_LHS_MATRIX_SCALING >= 0 %}
  for (int i=0; i<{{CELL_CELL_LHS_MATRIX_SCALING}}; i++) {
    scaling *= cellSize(0);
  {% else %}
  for (int i=0; i<(-1)*{{CELL_CELL_LHS_MATRIX_SCALING}}; i++) {
    scaling /= cellSize(0);
  {% endif %}
  }
  return scaling * result;
}
{% endif %}


{% if CELL_CELL_RHS_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRhsMatrix(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > result = {
      {{CELL_CELL_RHS_MATRIX[0]| join(", ")}}
      {% for ROW in CELL_CELL_RHS_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  double scaling = 1.0;
  {% if CELL_CELL_RHS_MATRIX_SCALING >= 0 %}
  for (int i=0; i<{{CELL_CELL_RHS_MATRIX_SCALING}}; i++) {
    scaling *= cellSize(0);
  {% else %}
  for (int i=0; i<(-1)*{{CELL_CELL_RHS_MATRIX_SCALING}}; i++) {
    scaling /= cellSize(0);
  {% endif %}
  }
  return scaling * result;
}
{% endif %}

/**
* This method requires custom scaling in each part of the matrix
* for the DGPoisson experiment. This has been implemented in child 
* class in the benchmarks directory, and it works by calling this
* abstract method to grab the unscaled matrix that was supplied
* by the user, before putting in appropriate scaling and returning.
* 
* If custom behaviour is desired, ensure to pass "None" to the  
* solver class.
*/
{% if CELL_TO_FACE_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FacesPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getProjectionOfCellDataOntoFace(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FacesPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, double > result = {
      {{CELL_TO_FACE_MATRIX[0]| join(", ")}}
      {% for ROW in CELL_TO_FACE_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  double scaling = 1.0;

  {% if CELL_TO_FACE_MATRIX_SCALING is none %}
  // supplied "none" to the scaling for cell to face matrix. putting in custom behaviour
  // in dervied class. reuse this method to grab the matrix itself
  return result;

  {% elif CELL_TO_FACE_MATRIX_SCALING >= 0 %}
  for (int i=0; i<{{CELL_TO_FACE_MATRIX_SCALING}}; i++) {
    scaling *= cellSize(0);
  }
  return scaling * result;
  {% elif CELL_TO_FACE_MATRIX_SCALING <  0 %}
  for (int i=0; i<(-1)*{{CELL_TO_FACE_MATRIX_SCALING}}; i++) {
    scaling /= cellSize(0);
  }
  return scaling * result;
  {% endif %}
}
{% endif %}

/**
* This method requires custom scaling in each part of the matrix
* for the DGPoisson experiment. This has been implemented in child 
* class in the benchmarks directory, and it works by calling this
* abstract method to grab the unscaled matrix that was supplied
* by the user, before putting in appropriate scaling and returning.
* 
* If custom behaviour is desired, ensure to pass "None" to the  
* solver class.
*/
{% if FACE_TO_CELL_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FacesPerCell, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getProjectionOfRiemannSolutionOntoCell(
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::DoFsPerCell, {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FacesPerCell, double > result = {
      {{FACE_TO_CELL_MATRIX[0]| join(", ")}}
      {% for ROW in FACE_TO_CELL_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  double scaling = 1.0;
  {% if FACE_TO_CELL_MATRIX_SCALING is none %}
  // supplied "none" to the scaling for cell to face matrix. putting in custom behaviour
  // in dervied class. reuse this method to grab the matrix itself
  return result;

  {% elif FACE_TO_CELL_MATRIX_SCALING >= 0 %}
  for (int i=0; i<{{FACE_TO_CELL_MATRIX_SCALING}}; i++) {
    scaling *= cellSize(0);
  }
  return scaling * result;
  {% elif FACE_TO_CELL_MATRIX_SCALING <  0 %}
  for (int i=0; i<(-1)*{{FACE_TO_CELL_MATRIX_SCALING}}; i++) {
    scaling /= cellSize(0);
  }
  return scaling * result;
  {% endif %}
}
{% endif %}


{% if FACE_FACE_RIEMANN_PROBLEM_MATRIX!=[] %}
tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns, 2*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns, double > {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRiemannSolver(
  const tarch::la::Vector<Dimensions, double>&  faceCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize
) {
  static tarch::la::Matrix< {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns, 2*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::NodesPerFace*{{NAMESPACE | join("::")}}::{{CLASSNAME}}::FaceUnknowns, double > result = {
      {{FACE_FACE_RIEMANN_PROBLEM_MATRIX[0]| join(", ")}}
      {% for ROW in FACE_FACE_RIEMANN_PROBLEM_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
      {% endfor %}
  };
  return result;
}
{% endif %}



{{NAMESPACE | join("::")}}::vertexdata::{{SOLVER_NAME}}PETScData::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getVertexDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return vertexdata::{{SOLVER_NAME}}PETScData::Type::Outside;
}


{{NAMESPACE | join("::")}}::facedata::{{SOLVER_NAME}}PETScData::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getFaceDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  auto isOnBoundary = [&]( const tarch::la::Vector< Dimensions, double >&  x) -> bool {
    bool isOnBoundary = false;
    for (int d=0; d<Dimensions; d++) {
      isOnBoundary |= tarch::la::smallerEquals( x(d), DomainOffset(d) );
      isOnBoundary |= tarch::la::greaterEquals( x(d), DomainOffset(d)+DomainSize(d) );
    }
    return isOnBoundary;
  };

  return isOnBoundary(x) ? facedata::{{SOLVER_NAME}}PETScData::Type::Boundary : facedata::{{SOLVER_NAME}}PETScData::Type::Interior;
}


{{NAMESPACE | join("::")}}::celldata::{{SOLVER_NAME}}PETScData::Type {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getCellDoFType(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h
) {
  return celldata::{{SOLVER_NAME}}PETScData::Type::Interior;
}

