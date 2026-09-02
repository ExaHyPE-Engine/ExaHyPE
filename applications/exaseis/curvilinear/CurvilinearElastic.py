

def initial(pointwise_initial_conditions):
  return """
  std::fill_n(Q, (NumberOfAuxiliaryVariables + NumberOfUnknowns) * (Order + 1) * (Order + 1) * (Order + 1), 0);

  tarch::la::Vector<DIMENSIONS, double> center = (*_curvi)(x);

  auto initUnknownsPointwise = [&](
    const tarch::la::Vector<DIMENSIONS,double>& x,
    const tarch::la::Vector<DIMENSIONS,double>& center,
    {{SOLUTION_STORAGE_PRECISION}}* Q
  ) -> void {""" + pointwise_initial_conditions + """
  };

  int linearisedIndex = 0;
  peano4::dForEach(Order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
    tarch::la::Vector<DIMENSIONS, double> curviPos = (*_curvi)(
      ::exahype2::dg::getQuadraturePoint(
        x,
        h,
        index,
        Order + 1,
        kernels::ElasticSolver::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes
      )
    );
    Q[linearisedIndex+Shortcuts::curve_grid + 0] = curviPos[0];
    Q[linearisedIndex+Shortcuts::curve_grid + 1] = curviPos[1];
    Q[linearisedIndex+Shortcuts::curve_grid + 2] = curviPos[2];

    initUnknownsPointwise(
      curviPos, center, Q+linearisedIndex
    );

    linearisedIndex += NumberOfUnknowns + NumberOfAuxiliaryVariables;
  });

  const tarch::la::Vector<DIMENSIONS, {{SOLUTION_STORAGE_PRECISION}}> invDx = convertScalar<{{SOLUTION_STORAGE_PRECISION}}>(invertEntries(h));

  constexpr int nDof    = Order+1;
  constexpr int nDofPad = {{NUMBER_OF_DOF_PADDED}};
  constexpr int strideQ = (NumberOfUnknowns+NumberOfAuxiliaryVariables);

  // x-loop
  for (int yz = 0; yz < nDof*nDof; yz++) {
    UNROLL(nDof)
    for (int it_2 = 0; it_2 < nDof; it_2++) {
      UNROLL(nDof)
      for (int it_1 = 0; it_1 < nDof; it_1++) {
        // x derivatives
        for (int it_3 = 0; it_3 < DIMENSIONS; it_3++) {
          Q[yz*nDof*strideQ + it_1*strideQ + Shortcuts::metric_derivative + it_3] +=
            Q[yz*nDof*strideQ + it_2*strideQ + Shortcuts::curve_grid + it_3]
            * invDx[0] * kernels::ElasticSolver::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudx_T[it_1*nDofPad + it_2];
        }
      }
    } // it_2
  } // yz

  // y-loop
  for (int z = 0; z < nDof; z++) {
    for (int x = 0; x < nDof; x++) {
      UNROLL(nDof)
      for (int it_2 = 0; it_2 < nDof; it_2++) {
        UNROLL(nDof)
        for (int it_1 = 0; it_1 < nDof; it_1++) {
          for (int it_3 = 0; it_3 < DIMENSIONS; it_3++) {
            Q[z*nDof*nDof*strideQ + x*strideQ + it_1*nDof*strideQ + Shortcuts::metric_derivative + DIMENSIONS + it_3] +=
              Q[z*nDof*nDof*strideQ + x*strideQ + it_2*nDof*strideQ + Shortcuts::curve_grid + it_3]
              * invDx[1] * kernels::ElasticSolver::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudx_T[it_1*nDofPad + it_2];
          } // it_3
        } // it_1
      } // it_2
    } // x
  } // z

  // z-loop
  for (int xy = 0; xy < nDof*nDof; xy++) {
    UNROLL(nDof)
    for (int it_2 = 0; it_2 < nDof; it_2++) {
      UNROLL(nDof)
      for (int it_1 = 0; it_1 < nDof; it_1++) {
        for (int it_3 = 0; it_3 < DIMENSIONS; it_3++) {
          // gradQ[it_1 * 12 + it_3] +=
          Q[xy*strideQ + it_1*nDof*nDof*strideQ + Shortcuts::metric_derivative + 2*DIMENSIONS + it_3] +=
            Q[xy*strideQ + it_2*nDof*nDof*strideQ + Shortcuts::curve_grid + it_3]
            * invDx[2] * kernels::ElasticSolver::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudx_T[it_1*nDofPad + it_2];
        } // it_3
      } // it_2
    } // it_2
  } // xy

  linearisedIndex = 0;
  peano4::dForEach(Order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
    auto x_der_x = Q[linearisedIndex+Shortcuts::metric_derivative+0];
    auto y_der_x = Q[linearisedIndex+Shortcuts::metric_derivative+1];
    auto z_der_x = Q[linearisedIndex+Shortcuts::metric_derivative+2];

    auto x_der_y = Q[linearisedIndex+Shortcuts::metric_derivative+DIMENSIONS+0];
    auto y_der_y = Q[linearisedIndex+Shortcuts::metric_derivative+DIMENSIONS+1];
    auto z_der_y = Q[linearisedIndex+Shortcuts::metric_derivative+DIMENSIONS+2];

    auto x_der_z = Q[linearisedIndex+Shortcuts::metric_derivative+2*DIMENSIONS+0];
    auto y_der_z = Q[linearisedIndex+Shortcuts::metric_derivative+2*DIMENSIONS+1];
    auto z_der_z = Q[linearisedIndex+Shortcuts::metric_derivative+2*DIMENSIONS+2];

    auto jacobian =   x_der_x * (y_der_y * z_der_z - y_der_z * z_der_y)
                    - x_der_y * (y_der_x * z_der_z - y_der_z * z_der_x)
                    + x_der_z * (y_der_x * z_der_y - y_der_y * z_der_x);

    Q[linearisedIndex+Shortcuts::jacobian] = jacobian;
    Q[linearisedIndex+Shortcuts::metric_derivative+0] = (1 / jacobian) * (y_der_y * z_der_z - z_der_y * y_der_z);
    Q[linearisedIndex+Shortcuts::metric_derivative+3] = (1 / jacobian) * (z_der_x * y_der_z - y_der_x * z_der_z);
    Q[linearisedIndex+Shortcuts::metric_derivative+6] = (1 / jacobian) * (y_der_x * z_der_y - z_der_x * y_der_y);

    Q[linearisedIndex+Shortcuts::metric_derivative+1] = (1 / jacobian) * (z_der_y * x_der_z - x_der_y * z_der_z);
    Q[linearisedIndex+Shortcuts::metric_derivative+4] = (1 / jacobian) * (x_der_x * z_der_z - z_der_x * x_der_z);
    Q[linearisedIndex+Shortcuts::metric_derivative+7] = (1 / jacobian) * (z_der_x * x_der_y - x_der_x * z_der_y);

    Q[linearisedIndex+Shortcuts::metric_derivative+2] = (1 / jacobian) * (x_der_y * y_der_z - y_der_y * x_der_z);
    Q[linearisedIndex+Shortcuts::metric_derivative+5] = (1 / jacobian) * (y_der_x * x_der_z - x_der_x * y_der_z);
    Q[linearisedIndex+Shortcuts::metric_derivative+8] = (1 / jacobian) * (x_der_x * y_der_y - y_der_x * x_der_y);

    linearisedIndex += strideQ;
  });
"""


def boundary():
  return """
  std::copy_n(Qinside, NumberOfUnknowns + NumberOfAuxiliaryVariables, Qoutside);
"""

def eigenvalue():
  return """
  // Check for NaN in any of the velocities
  assert(("Check for Nans in velocity", !(std::isnan(Q[Shortcuts::v + 0]) || std::isnan(Q[Shortcuts::v + 1]) || std::isnan(Q[Shortcuts::v + 2]))));

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

  double lambda[10]{1.};

  lambda[0] = (double) (std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cp);
  lambda[1] = (double) (std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs);
  lambda[2] = (double) (std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs);

  lambda[3] = (double) (std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cp);
  lambda[4] = (double) (std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs);
  lambda[5] = (double) (std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs);

  lambda[6] = (double) (std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cp);
  lambda[7] = (double) (std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs);
  lambda[8] = (double) (std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs);

  lambda[9] = 1.;

  return *std::max_element(lambda, lambda + 10);
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
      F[Shortcuts::v+0] = -jacobian * (q_x * sigma_xx + q_y * sigma_xy + q_z * sigma_xz);
      F[Shortcuts::v+1] = -jacobian * (q_x * sigma_xy + q_y * sigma_yy + q_z * sigma_yz);
      F[Shortcuts::v+2] = -jacobian * (q_x * sigma_xz + q_y * sigma_yz + q_z * sigma_zz);
    } break;
    case 1: {
      auto r_x = Q[Shortcuts::metric_derivative + 3];
      auto r_y = Q[Shortcuts::metric_derivative + 4];
      auto r_z = Q[Shortcuts::metric_derivative + 5];
      F[Shortcuts::v+0] = -jacobian * (r_x * sigma_xx + r_y * sigma_xy + r_z * sigma_xz);
      F[Shortcuts::v+1] = -jacobian * (r_x * sigma_xy + r_y * sigma_yy + r_z * sigma_yz);
      F[Shortcuts::v+2] = -jacobian * (r_x * sigma_xz + r_y * sigma_yz + r_z * sigma_zz);
    } break;
    case 2: {
      auto s_x = Q[Shortcuts::metric_derivative + 6];
      auto s_y = Q[Shortcuts::metric_derivative + 7];
      auto s_z = Q[Shortcuts::metric_derivative + 8];
      F[Shortcuts::v+0] = -jacobian * (s_x * sigma_xx + s_y * sigma_xy + s_z * sigma_xz);
      F[Shortcuts::v+1] = -jacobian * (s_x * sigma_xy + s_y * sigma_yy + s_z * sigma_yz);
      F[Shortcuts::v+2] = -jacobian * (s_x * sigma_xz + s_y * sigma_yz + s_z * sigma_zz);
    }
  }
"""

def ncp():
  return """
  std::fill_n(BTimesDeltaQ, NumberOfUnknowns, 0.0);

  switch (normal) {
    case 0: {
      auto u_q = deltaQ[Shortcuts::v + 0];
      auto v_q = deltaQ[Shortcuts::v + 1];
      auto w_q = deltaQ[Shortcuts::v + 2];
      auto q_x = Q[Shortcuts::metric_derivative + 0];
      auto q_y = Q[Shortcuts::metric_derivative + 1];
      auto q_z = Q[Shortcuts::metric_derivative + 2];
      BTimesDeltaQ[Shortcuts::sigma + 0] = -q_x*u_q;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -q_y*v_q;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -q_z*w_q;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -(q_y*u_q+q_x*v_q); //sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4] = -(q_z*u_q+q_x*w_q); //sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5] = -(q_z*v_q+q_y*w_q); //sigma_yz
    } break;
    case 1: {
      auto u_r = deltaQ[Shortcuts::v + 0];
      auto v_r = deltaQ[Shortcuts::v + 1];
      auto w_r = deltaQ[Shortcuts::v + 2];
      auto r_x = Q[Shortcuts::metric_derivative + 3];
      auto r_y = Q[Shortcuts::metric_derivative + 4];
      auto r_z = Q[Shortcuts::metric_derivative + 5];
      BTimesDeltaQ[Shortcuts::sigma + 0] = -r_x*u_r;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -r_y*v_r;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -r_z*w_r;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -(r_y*u_r+r_x*v_r); //sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4] = -(r_z*u_r+r_x*w_r); //sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5] = -(r_z*v_r+r_y*w_r); //sigma_yz
    } break;
    case 2: {
      auto u_s = deltaQ[Shortcuts::v + 0];
      auto v_s = deltaQ[Shortcuts::v + 1];
      auto w_s = deltaQ[Shortcuts::v + 2];
      auto s_x = Q[Shortcuts::metric_derivative + 6];
      auto s_y = Q[Shortcuts::metric_derivative + 7];
      auto s_z = Q[Shortcuts::metric_derivative + 8];
      BTimesDeltaQ[Shortcuts::sigma + 0] = -s_x*u_s;
      BTimesDeltaQ[Shortcuts::sigma + 1] = -s_y*v_s;
      BTimesDeltaQ[Shortcuts::sigma + 2] = -s_z*w_s;
      BTimesDeltaQ[Shortcuts::sigma + 3] = -(s_y*u_s+s_x*v_s); //sigma_xy
      BTimesDeltaQ[Shortcuts::sigma + 4] = -(s_z*u_s+s_x*w_s); //sigma_xz
      BTimesDeltaQ[Shortcuts::sigma + 5] = -(s_z*v_s+s_y*w_s); //sigma_yz
    }
  }
"""

def multiplyMaterialParameterMatrix():
  return """
  auto rho = Q[Shortcuts::rho];
  auto cp  = Q[Shortcuts::cp];
  auto cs  = Q[Shortcuts::cs];
  auto jacobian = Q[Shortcuts::jacobian];

  auto mu      = rho * cs * cs;
  auto lambda  = rho * cp * cp - 2 * mu;
  auto rho_inv = 1/(rho*jacobian);

  // Rhs uses the same formula regardless of dimension, hence no switch necessary

  rhs[Shortcuts::v + 0] = rho_inv * rhs[Shortcuts::v + 0];
  rhs[Shortcuts::v + 1] = rho_inv * rhs[Shortcuts::v + 1];
  rhs[Shortcuts::v + 2] = rho_inv * rhs[Shortcuts::v + 2];

  auto lam_temp = lambda * (rhs[Shortcuts::sigma+0] + rhs[Shortcuts::sigma+1] + rhs[Shortcuts::sigma+2]);

  rhs[Shortcuts::sigma+0] = (2*mu) * rhs[Shortcuts::sigma+0] + lam_temp;
  rhs[Shortcuts::sigma+1] = (2*mu) * rhs[Shortcuts::sigma+1] + lam_temp;
  rhs[Shortcuts::sigma+2] = (2*mu) * rhs[Shortcuts::sigma+2] + lam_temp;

  rhs[Shortcuts::sigma+3] = mu*rhs[Shortcuts::sigma+3];
  rhs[Shortcuts::sigma+4] = mu*rhs[Shortcuts::sigma+4];
  rhs[Shortcuts::sigma+5] = mu*rhs[Shortcuts::sigma+5];
"""

def refinement_criterion():
  return """
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
"""

def riemann_solver():
  return """
  applications::exahype2::exaseis::riemannSolverIsotropic<VariableShortcuts{{SOLVER_NAME}}, {{CORRECTOR_COMPUTATION_PRECISION}}, Order + 1, NumberOfUnknowns, (NumberOfUnknowns + NumberOfAuxiliaryVariables), 1>(
    FL, FR, QL, QR, t, dt, direction, isBoundaryFace, faceIndex
  );
"""

def userIncludes():
  return """
#include <memory>

#include "../riemannSolvers/curvilinearRoutines.h"
#include "../riemannSolvers/riemannsolverIsotropic.h"

#include "exahype2/Curvi.h"
#include "exahype2/dg/DGUtils.h"

#include "tarch/reader/NetCDFFieldSampler.h"
"""

def abstractUserDefinitions():
  return """
std::unique_ptr<::exahype2::Curvi> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_curvi;

tarch::la::Vector<DIMENSIONS,double> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::invertProjection(const tarch::la::Vector<DIMENSIONS,double> coordinates){

  return _curvi->invertProjection(coordinates);

}
"""

def abstractDeclarations():
  return """
public:
  double QuadraturePoints1D[Order+1];

  tarch::la::Vector<DIMENSIONS,double> invertProjection(const tarch::la::Vector<DIMENSIONS,double> coordinates);

protected:
  static std::unique_ptr<::exahype2::Curvi> _curvi;
"""

def init_grid_step_implementation(scenario_perturbations):
  return """
  std::copy_n(
    kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    (Order+1),
    QuadraturePoints1D
  );

  //setting minCellSize
  tarch::la::Vector<DIMENSIONS, double> minCellSize = DomainSize;
  const double minAdmissiblePatchSize = ::runner::RuntimeParameters::getInstance().getMinAdmissiblePatchSize(DomainSize);

  //finding coarsest possible Mesh level
  while(min(minCellSize)>minAdmissiblePatchSize){
    minCellSize /= 3.0;
  }

  // Grid initialisation is re-run once per round of static AMR, so this hook fires more than once.
  // Assigning to the unique_ptr releases the Curvi the earlier round built, and the perturbations
  // below then rebuild it from scratch.
  _curvi = std::make_unique<::exahype2::Curvi>(
    DomainOffset,
    DomainSize,
    minCellSize
  );

""" + scenario_perturbations
