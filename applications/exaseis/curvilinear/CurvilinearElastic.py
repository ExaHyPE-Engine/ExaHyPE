

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


def boundary():
  return """
  std::copy_n(Qinside, NumberOfUnknowns + NumberOfAuxiliaryVariables, Qoutside);
"""

def eigenvalue():
  return """
  auto cp  = Q[Shortcuts::cp];
  auto cs  = Q[Shortcuts::cs];
  auto q_x = Q[Shortcuts::metric_derivative + 0];
  auto q_y = Q[Shortcuts::metric_derivative + 1];
  auto q_z = Q[Shortcuts::metric_derivative + 2];
  auto r_x = Q[Shortcuts::metric_derivative + 3];
  auto r_y = Q[Shortcuts::metric_derivative + 4];
  auto r_z = Q[Shortcuts::metric_derivative + 5];
  auto s_x = Q[Shortcuts::metric_derivative + 6];
  auto s_y = Q[Shortcuts::metric_derivative + 7];
  auto s_z = Q[Shortcuts::metric_derivative + 8];

  double lambda[9] = {0.};

  lambda[0] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cp;
  lambda[1] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs;
  lambda[2] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs;

  lambda[3] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cp;
  lambda[4] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs;
  lambda[5] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs;

  lambda[6] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cp;
  lambda[7] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs;
  lambda[8] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs;

  return *std::max_element(lambda, lambda + 9);
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

  switch (normal) {
    case 0: {
      auto q_x = Q[Shortcuts::metric_derivative + 0];
      auto q_y = Q[Shortcuts::metric_derivative + 1];
      auto q_z = Q[Shortcuts::metric_derivative + 2];
      F[0]       = -jacobian * (q_x * sigma_xx + q_y * sigma_xy + q_z * sigma_xz);
      F[1]       = -jacobian * (q_x * sigma_xy + q_y * sigma_yy + q_z * sigma_yz);
      F[2]       = -jacobian * (q_x * sigma_xz + q_y * sigma_yz + q_z * sigma_zz);
      F[3]       = 0.0;
      F[4]       = 0.0;
      F[5]       = 0.0;
      F[6]       = 0.0;
      F[7]       = 0.0;
      F[8]       = 0.0;
    } break;
    case 1: {
      auto r_x = Q[Shortcuts::metric_derivative + 3];
      auto r_y = Q[Shortcuts::metric_derivative + 4];
      auto r_z = Q[Shortcuts::metric_derivative + 5];
      F[0]       = -jacobian * (r_x * sigma_xx + r_y * sigma_xy + r_z * sigma_xz);
      F[1]       = -jacobian * (r_x * sigma_xy + r_y * sigma_yy + r_z * sigma_yz);
      F[2]       = -jacobian * (r_x * sigma_xz + r_y * sigma_yz + r_z * sigma_zz);
      F[3]       = 0.0;
      F[4]       = 0.0;
      F[5]       = 0.0;
      F[6]       = 0.0;
      F[7]       = 0.0;
      F[8]       = 0.0;
    } break;
    case 2: {
      auto s_x = Q[Shortcuts::metric_derivative + 6];
      auto s_y = Q[Shortcuts::metric_derivative + 7];
      auto s_z = Q[Shortcuts::metric_derivative + 8];
      F[0]       = -jacobian * (s_x * sigma_xx + s_y * sigma_xy + s_z * sigma_xz);
      F[1]       = -jacobian * (s_x * sigma_xy + s_y * sigma_yy + s_z * sigma_yz);
      F[2]       = -jacobian * (s_x * sigma_xz + s_y * sigma_yz + s_z * sigma_zz);
      F[3]       = 0.0;
      F[4]       = 0.0;
      F[5]       = 0.0;
      F[6]       = 0.0;
      F[7]       = 0.0;
      F[8]       = 0.0;
    }
  }
"""

def ncp():
  return """
  switch (normal) {
    case 0: {
      auto u_q = deltaQ[0];
      auto v_q = deltaQ[1];
      auto w_q = deltaQ[2];
      auto q_x = Q[Shortcuts::metric_derivative + 0];
      auto q_y = Q[Shortcuts::metric_derivative + 1];
      auto q_z = Q[Shortcuts::metric_derivative + 2];
      BTimesDeltaQ[0]  = 0;
      BTimesDeltaQ[1]  = 0;
      BTimesDeltaQ[2]  = 0;
      BTimesDeltaQ[3]  = -q_x * u_q;
      BTimesDeltaQ[4]  = -q_y * v_q;
      BTimesDeltaQ[5]  = -q_z * w_q;
      BTimesDeltaQ[6]  = -(q_y * u_q + q_x * v_q); // sigma_xy
      BTimesDeltaQ[7]  = -(q_z * u_q + q_x * w_q); // sigma_xz
      BTimesDeltaQ[8]  = -(q_z * v_q + q_y * w_q); // sigma_yz
    } break;
    case 1: {
      auto u_r = deltaQ[0];
      auto v_r = deltaQ[1];
      auto w_r = deltaQ[2];
      auto r_x = Q[Shortcuts::metric_derivative + 3];
      auto r_y = Q[Shortcuts::metric_derivative + 4];
      auto r_z = Q[Shortcuts::metric_derivative + 5];
      BTimesDeltaQ[0]  = 0;
      BTimesDeltaQ[1]  = 0;
      BTimesDeltaQ[2]  = 0;
      BTimesDeltaQ[3]  = -r_x * u_r;
      BTimesDeltaQ[4]  = -r_y * v_r;
      BTimesDeltaQ[5]  = -r_z * w_r;
      BTimesDeltaQ[6]  = -(r_y * u_r + r_x * v_r); // sigma_xy
      BTimesDeltaQ[7]  = -(r_z * u_r + r_x * w_r); // sigma_xz
      BTimesDeltaQ[8]  = -(r_z * v_r + r_y * w_r); // sigma_yz
    } break;
    case 2: {
      auto u_s = deltaQ[0];
      auto v_s = deltaQ[1];
      auto w_s = deltaQ[2];
      auto s_x = Q[Shortcuts::metric_derivative + 6];
      auto s_y = Q[Shortcuts::metric_derivative + 7];
      auto s_z = Q[Shortcuts::metric_derivative + 8];
      BTimesDeltaQ[0]  = 0;
      BTimesDeltaQ[1]  = 0;
      BTimesDeltaQ[2]  = 0;
      BTimesDeltaQ[3]  = -s_x * u_s;
      BTimesDeltaQ[4]  = -s_y * v_s;
      BTimesDeltaQ[5]  = -s_z * w_s;
      BTimesDeltaQ[6]  = -(s_y * u_s + s_x * v_s); // sigma_xy
      BTimesDeltaQ[7]  = -(s_z * u_s + s_x * w_s); // sigma_xz
      BTimesDeltaQ[8]  = -(s_z * v_s + s_y * w_s); // sigma_yz
    }
  }
"""

def multiplyMaterialParameterMatrix():
  return """
  auto rho      = Q[Shortcuts::rho];
  auto c_p      = Q[Shortcuts::cp];
  auto c_s      = Q[Shortcuts::cs];
  auto mu       = rho * c_s * c_s;
  auto lambda   = rho * c_p * c_p - 2 * mu;
  auto jacobian = Q[Shortcuts::jacobian];
  auto rho_inv  = 1.0 / (rho * jacobian);

  // identical in all dimensions so no need for switch
  rhs[0] = rho_inv * rhs[0];
  rhs[1] = rho_inv * rhs[1];
  rhs[2] = rho_inv * rhs[2];

  auto lam_temp = lambda * (rhs[3] + rhs[4] + rhs[5]);
  rhs[3]          = (2 * mu) * rhs[3] + lam_temp;
  rhs[4]          = (2 * mu) * rhs[4] + lam_temp;
  rhs[5]          = (2 * mu) * rhs[5] + lam_temp;

  rhs[6] = mu * rhs[6];
  rhs[7] = mu * rhs[7];
  rhs[8] = mu * rhs[8];
"""

def refinement_criterion():
  return """
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  return result;
"""

def point_source():
  return """
  std::fill_n(forceVector, 9, 0.);

  auto jacobian = Q[Shortcuts::jacobian];

  assertion2(n == 0, "Only a single pointSource for LOH1",n);

  constexpr double t0 = 0.1;
  constexpr double M0 = 1000.0;
  
  double f = M0*t/(t0*t0)*std::exp(-t/t0);
  
  forceVector[Shortcuts::sigma + 4] = f;

  for (int i = 0; i < NumberOfUnknowns; i++) {
    forceVector[i] = forceVector[i] / jacobian;
  }
"""
def init_point_source():
  return """
  pointSourceLocation[0][0] = 0.0;
  pointSourceLocation[0][1] = 2.0;
  pointSourceLocation[0][2] = 0.0;

  context->correctPointSourceLocation(pointSourceLocation);
"""

def riemann_solver():
  return """
  Numerics::riemannSolver<VariableShortcuts{{SOLVER_NAME}}, {{CORRECTOR_COMPUTATION_PRECISION}}, Order + 1, NumberOfUnknowns, (NumberOfUnknowns + NumberOfAuxiliaryVariables), 1>(
    FL, FR, QL, QR, dt, direction, isBoundaryFace, faceIndex
  );
"""

def userIncludes():
  return """
#define _CUSTOM_COORDINATES
#define ASAGI_NOMPI
#define _TOP 1

#include "../ExaSeis_core/Curvilinear/ContextCurvilinear.h"

#include "../ExaSeis_core/Numerics/curvilinearRoutines.h"
#include "../ExaSeis_core/Numerics/riemannsolverIsotropic.h"

#include "peano4/datamanagement/CellMarker.h"
"""

def abstractUserDefinitions():
  return """
ContextCurvilinear<{{NAMESPACE | join("::")}}::VariableShortcuts{{SOLVER_NAME}},
  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Order + 1,
  ({{NAMESPACE | join("::")}}::{{CLASSNAME}}::NumberOfUnknowns + {{NAMESPACE | join("::")}}::{{CLASSNAME}}::NumberOfAuxiliaryVariables),
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
  static ContextCurvilinear<VariableShortcuts{{SOLVER_NAME}}, Order + 1, (NumberOfAuxiliaryVariables + NumberOfUnknowns), {{SOLUTION_STORAGE_PRECISION}}>* context;//(NumberOfUnknowns+NumberOfAuxiliaryVariables)>* context;
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

  context = new ContextCurvilinear<VariableShortcuts{{SOLVER_NAME}}, Order + 1, (NumberOfAuxiliaryVariables + NumberOfUnknowns), {{SOLUTION_STORAGE_PRECISION}}>(
    topography_string,
    coarsestMeshLevel,
    realCoarsestMeshSize,
    depth,
    DomainOffset, DomainSize,
    kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    kernels::{{SOLVER_NAME}}::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudx
  );
"""