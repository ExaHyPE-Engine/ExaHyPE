def initial(pointwise_initial_conditions):
  return """
  std::fill_n(Q, (NumberOfAuxiliaryVariables + NumberOfUnknowns) * (Order + 1) * (Order + 1) * (Order + 1), 0);

  context->initUnknownsPatch(Q, x, h, 0.0, 0.0,
  [&](
      const {{SOLUTION_STORAGE_PRECISION}}* const x,
      const tarch::la::Vector<DIMENSIONS,double>& center,
      const double t,
      const double dt, 
      {{SOLUTION_STORAGE_PRECISION}}* Q
    ) -> void { """ + pointwise_initial_conditions + """
    }
  );
"""

def flux():
  return """
  auto sigma_xx = Q[Shortcuts::sigma + 0];
  auto sigma_yy = Q[Shortcuts::sigma + 1];
  auto sigma_zz = Q[Shortcuts::sigma + 2];
  auto sigma_xy = Q[Shortcuts::sigma + 3];
  auto sigma_xz = Q[Shortcuts::sigma + 4];
  auto sigma_yz = Q[Shortcuts::sigma + 5];

  auto jacobian = Q[Shortcuts::jacobian];

  std::fill_n(F, NumberOfUnknowns, 0.);

  switch (normal) {
    case 0: {
      auto q_x = Q[Shortcuts::metric_derivative + 0];
      auto q_y = Q[Shortcuts::metric_derivative + 1];
      auto q_z = Q[Shortcuts::metric_derivative + 2];
      F[0]       = -jacobian * (q_x * sigma_xx + q_y * sigma_xy + q_z * sigma_xz);
      F[1]       = -jacobian * (q_x * sigma_xy + q_y * sigma_yy + q_z * sigma_yz);
      F[2]       = -jacobian * (q_x * sigma_xz + q_y * sigma_yz + q_z * sigma_zz);
    } break;
    case 1: {
      auto r_x = Q[Shortcuts::metric_derivative + 3];
      auto r_y = Q[Shortcuts::metric_derivative + 4];
      auto r_z = Q[Shortcuts::metric_derivative + 5];
      F[0]       = -jacobian * (r_x * sigma_xx + r_y * sigma_xy + r_z * sigma_xz);
      F[1]       = -jacobian * (r_x * sigma_xy + r_y * sigma_yy + r_z * sigma_yz);
      F[2]       = -jacobian * (r_x * sigma_xz + r_y * sigma_yz + r_z * sigma_zz);
    } break;
    case 2: {
      auto s_x = Q[Shortcuts::metric_derivative + 6];
      auto s_y = Q[Shortcuts::metric_derivative + 7];
      auto s_z = Q[Shortcuts::metric_derivative + 8];
      F[0]       = -jacobian * (s_x * sigma_xx + s_y * sigma_xy + s_z * sigma_xz);
      F[1]       = -jacobian * (s_x * sigma_xy + s_y * sigma_yy + s_z * sigma_yz);
      F[2]       = -jacobian * (s_x * sigma_xz + s_y * sigma_yz + s_z * sigma_zz);
    }
  }
"""

def ncp():
  return """
  auto        rho       = Q[Shortcuts::rho];
  auto        cp        = Q[Shortcuts::cp];
  auto        cs        = Q[Shortcuts::cs];
  auto        jacobian  = Q[Shortcuts::jacobian];
  auto        mu        = rho * cs * cs;
  auto        lambda    = rho * cp * cp - 2 * mu;
  auto        rho_inv   = 1.0 / (rho * jacobian);

  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0.0);

  switch (normal) {
    case 0: {
      auto u_q                              = deltaQ[Shortcuts::v + 0];
      auto v_q                              = deltaQ[Shortcuts::v + 1];
      auto w_q                              = deltaQ[Shortcuts::v + 2];
      auto q_x                              = Q[Shortcuts::metric_derivative + 0];
      auto q_y                              = Q[Shortcuts::metric_derivative + 1];
      auto q_z                              = Q[Shortcuts::metric_derivative + 2];
      BTimesDeltaQ[Shortcuts::sigma + 0]                     = -q_x*u_q;
      BTimesDeltaQ[Shortcuts::sigma + 1]                     = -q_y*v_q;
      BTimesDeltaQ[Shortcuts::sigma + 2]                     = -q_z*w_q;
      BTimesDeltaQ[Shortcuts::sigma + 3]                     = -(q_y*u_q+q_x*v_q); //sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4]                     = -(q_z*u_q+q_x*w_q); //sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5]                     = -(q_z*v_q+q_y*w_q); //sigma_yz
    } break;
    case 1: {
      auto u_r                              = deltaQ[Shortcuts::v + 0];
      auto v_r                              = deltaQ[Shortcuts::v + 1];
      auto w_r                              = deltaQ[Shortcuts::v + 2];
      auto r_x                              = Q[Shortcuts::metric_derivative + 3];
      auto r_y                              = Q[Shortcuts::metric_derivative + 4];
      auto r_z                              = Q[Shortcuts::metric_derivative + 5];
      BTimesDeltaQ[Shortcuts::sigma + 0]                     = -r_x*u_r;
      BTimesDeltaQ[Shortcuts::sigma + 1]                     = -r_y*v_r;
      BTimesDeltaQ[Shortcuts::sigma + 2]                     = -r_z*w_r;
      BTimesDeltaQ[Shortcuts::sigma + 3]                     = -(r_y*u_r+r_x*v_r); //sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4]                     = -(r_z*u_r+r_x*w_r); //sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5]                     = -(r_z*v_r+r_y*w_r); //sigma_yz
    } break;
    case 2: {
      auto u_s                              = deltaQ[Shortcuts::v + 0];
      auto v_s                              = deltaQ[Shortcuts::v + 1];
      auto w_s                              = deltaQ[Shortcuts::v + 2];
      auto s_x                              = Q[Shortcuts::metric_derivative + 6];
      auto s_y                              = Q[Shortcuts::metric_derivative + 7];
      auto s_z                              = Q[Shortcuts::metric_derivative + 8];
      BTimesDeltaQ[Shortcuts::sigma + 0]                     = -s_x*u_s;
      BTimesDeltaQ[Shortcuts::sigma + 1]                     = -s_y*v_s;
      BTimesDeltaQ[Shortcuts::sigma + 2]                     = -s_z*w_s;
      BTimesDeltaQ[Shortcuts::sigma + 3]                     = -(s_y*u_s+s_x*v_s); //sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4]                     = -(s_z*u_s+s_x*w_s); //sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5]                     = -(s_z*v_s+s_y*w_s); //sigma_yz
    }
  }
"""

def algebraic_source():
  return """
  std::fill_n(S, NumberOfAuxiliaryVariables, 0);

  S[9]  = -Q[0]; 
  S[10] = -Q[1];
  S[11] = -Q[2];
"""

def multiplyMaterialParameterMatrix():
  return """
  auto rho = Q[Shortcuts::rho];
  auto cp  = Q[Shortcuts::cp];
  auto cs  = Q[Shortcuts::cs];
  auto jacobian = Q[Shortcuts::jacobian];

  auto mu      = rho * cs * cs;
  auto lambda  = rho * cp * cp - 2 * mu;
  auto rho_inv=1.0/(rho*jacobian);

  // Rhs uses the same formula regardless of dimension, hence no switch necessary

  rhs[Shortcuts::v + 0] = rho_inv * rhs[Shortcuts::v + 0];
  rhs[Shortcuts::v + 1] = rho_inv * rhs[Shortcuts::v + 1];
  rhs[Shortcuts::v + 2] = rho_inv * rhs[Shortcuts::v + 2];

  double lam_temp = lambda * (rhs[3] + rhs[4] + rhs[5]);

  rhs[3]=(2*mu) * rhs[3] +lam_temp;
  rhs[4]=(2*mu) * rhs[4] +lam_temp;
  rhs[5]=(2*mu) * rhs[5] +lam_temp;
  
  rhs[6]= mu*rhs[6];
  rhs[7]= mu*rhs[7];
  rhs[8]= mu*rhs[8];
  
  rhs[9]=  rhs[9];
  rhs[10]= rhs[10];
  rhs[11]= rhs[11];


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
#include "../ExaSeis_core/Numerics/riemannsolverDynamicRupture.h"

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
  static ContextDynamicRupture<VariableShortcuts{{SOLVER_NAME}}, Order + 1, NumberOfUnknowns, NumberOfAuxiliaryVariables, {{SOLUTION_STORAGE_PRECISION}}>* context;
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

  context = new ContextDynamicRupture<VariableShortcuts{{SOLVER_NAME}}, Order + 1, NumberOfUnknowns, NumberOfAuxiliaryVariables, {{SOLUTION_STORAGE_PRECISION}}>(
    topography_string,
    // filename_rupture_model,
    coarsestMeshLevel,
    realCoarsestMeshSize,
    depth,
    DomainOffset, DomainSize,
    kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    kernels::{{SOLVER_NAME}}::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudx
  );

  std::cout << "Freesurface set at " << _TOP * 2 << std::endl;
"""

def abstractDestructor():
  return """
  delete context;
"""

def finish_time_step_implementation(scenario_string):
  return """
  if (_solverState==SolverState::GridInitialisation) {
    std::string filename_rupture_model  = \"""" + scenario_string + """_fault.yaml";
    context->initRuptureModel(filename_rupture_model);
  }
"""
