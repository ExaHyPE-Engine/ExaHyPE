def algebraic_source():
  return """
//This term is not present in the Zenodo repository at https://zenodo.org/records/6386282
//however it is present in Leo's "phd" tag at https://gitlab.lrz.de/ExaHyPE-Seismic/ExaSeis/-/blob/leo/phd/Applications/DynamicRupture/PMLSlipWeakening/PMLSlipWeakening.cpp?ref_type=tags#L468
// We are working on the assumption that it should be in there.

  S[9]  = -Q[0];
  S[10] = -Q[1];
  S[11] = -Q[2];
"""


def riemann_solver():
  return """
  context->riemannSolver(
    FL, FR,
    QL, QR,
    t, dt,
    h,
    direction,
    isBoundaryFace,
    faceIndex,
    1 //surface
  );
"""

def userIncludes():
  return """
#define _CUSTOM_COORDINATES
#define ASAGI_NOMPI
#define _TOP 1

#include "../ExaSeis_core/Curvilinear/ContextDynamicRupture.h"
#include "../ExaSeis_core/Numerics/riemannsolverPMLDynamicRupture.h"

#include "peano4/datamanagement/CellMarker.h"
"""

def abstractUserDefinitions():
  return """
ContextDynamicRupture<{{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}},
  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Order + 1,
  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NumberOfUnknowns,
  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NumberOfAuxiliaryVariables,
  {{SOLUTION_STORAGE_PRECISION}}>* {{NAMESPACE | join("::")}}::{{CLASSNAME}}::context;

tarch::la::Vector<DIMENSIONS,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::invertProjection(const tarch::la::Vector<DIMENSIONS,double> coordinates){

  if(peano4::datamanagement::CellMarker::isContained(
    coordinates, DomainOffset+0.5*DomainSize, DomainSize, 0.
  )){
    double fixedCoords[3];
    fixedCoords[toolbox::curvi::Coordinate::X] = coordinates[0];
    fixedCoords[toolbox::curvi::Coordinate::Y] = coordinates[1];
    fixedCoords[toolbox::curvi::Coordinate::Z] = coordinates[2];

    tarch::la::Vector<DIMENSIONS,double> result;
    toolbox::curvi::Interface* curviInterface = context->getInterface();
    result[0] = curviInterface->invertProjection(toolbox::curvi::Coordinate::X, fixedCoords);
    result[1] = curviInterface->invertProjection(toolbox::curvi::Coordinate::Y, fixedCoords);
    result[2] = curviInterface->invertProjection(toolbox::curvi::Coordinate::Z, fixedCoords);

    return result;
  }
  return coordinates;
}
"""

def abstractDeclarations():
  return """
public:
  double QuadraturePoints1d[Order+1];

  tarch::la::Vector<DIMENSIONS,double> invertProjection(const tarch::la::Vector<DIMENSIONS,double> coordinates);
  
protected:
  static ContextDynamicRupture<VariableShortcutsElasticSolver, Order + 1, NumberOfUnknowns, NumberOfAuxiliaryVariables, {{SOLUTION_STORAGE_PRECISION}}>* context;

  static constexpr int     pml_cell_width    = 1;
  static constexpr double  pml_alpha_const   = 1.5;
  static constexpr double  pml_alpha_scalar  = 0.0;
  static constexpr double  pml_rel_error     = 0.001;
  static constexpr int     pml_power         = 1;
"""

def init_grid_step_implementation(scenario_string):
  return """
  std::copy_n(
    kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    (Order+1),
    QuadraturePoints1d    
  );

  std::string topography_string = \"""" + scenario_string + """.yaml";

  //setting coarsestMeshLevel and maxAdaptiveMeshLevel
  double maxDSize = std::numeric_limits<double>::min();
  double minDSize = std::numeric_limits<double>::max();
  tarch::la::Vector<DIMENSIONS, double> sizes = DomainSize;
  
  for(int i=0; i<DIMENSIONS; i++){
    if(sizes[i]>maxDSize){
      maxDSize = sizes[i];
    }
    if(sizes[i]<minDSize){
      minDSize = sizes[i];
    }    
  }
  
  int depth = 0;
  
  //finding coarsest possible Mesh level
  while(maxDSize>MaxAdmissibleCellH){
    depth += 1;
    maxDSize /= 3.0;
  }
  
  double realCoarsestMeshSize = maxDSize;
  double coarsestMeshLevel = depth;  
  
  depth = 0;
  while(minDSize>MinAdmissibleCellH){
    depth += 1;
    minDSize /= 3.0;
  }

  logTraceOutWith1Argument("Freesurface set at ", std::to_string(_TOP * 2));

  context = new ContextDynamicRupture<VariableShortcutsElasticSolver, Order + 1, NumberOfUnknowns, NumberOfAuxiliaryVariables, {{SOLUTION_STORAGE_PRECISION}}>(
    topography_string,
    coarsestMeshLevel,
    realCoarsestMeshSize,
    depth,
    DomainOffset, DomainSize,
    kernels::ElasticSolver::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    kernels::ElasticSolver::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudx
  );

  std::cout << "PML is using " << pml_cell_width << " elements" << std::endl;
  std::cout << "Freesurface set at " << _TOP * 2 << std::endl;
"""

def finish_time_step_implementation(scenario_string):
  return """
  if (_solverState==SolverState::GridInitialisation) {
    std::string filename_rupture_model  = \"""" + scenario_string + """_fault.yaml";
    context->initRuptureModel(filename_rupture_model);
  }
"""
