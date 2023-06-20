#include "CurvilinearSolver.h"

#include "exahype2/RefinementControl.h"

// for pragma
#ifndef ALIGNMENT
#define ALIGNMENT 1
#endif

tarch::logging::Log exahype2::elastic::CurvilinearSolver::_log("exahype2::elastic::CurvilinearSolver");

/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.sigma] instead of Q[0]
 */

exahype2::elastic::CurvilinearSolver::CurvilinearSolver():
  AbstractCurvilinearSolver() {

  std::string scenario_string   = "Loh1";
  std::string topography_string = "loh1.yaml";

  DomainInformation*              domain_info = new DomainInformation(this);
  SolverInformationADERDG<Order>* solver_info = new SolverInformationADERDG<Order>(this);

  context = new ContextCurvilinear<VariableShortcuts, Order + 1>(
    scenario_string, topography_string, domain_info, solver_info
  );

  context->initTransformation();
  context->initTree();
  context->setRefinementCriteria(this, this->refinementCriteria);
}

::exahype2::RefinementCommand exahype2::elastic::CurvilinearSolver::refinementCriterion(
  const double* __restrict__ Q, // Q[9+16]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {
  logTraceInWith3Arguments("refinementCriterion(...)", x, h, t);
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument("refinementCriterion(...)", ::toString(result));
  return result;
}

void exahype2::elastic::CurvilinearSolver::initialCondition(
  double* __restrict__ Q, // Q[9+16]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    point,
  bool                                         gridIsConstructed
) {
  logTraceInWith2Arguments("initialCondition(...)", x, gridIsConstructed);

  std::fill_n(Q, (s.SizeParameters + s.SizeVariables) * (Order + 1) * (Order + 1) * (Order + 1), 0);
  context->initUnknownsPatch(Q, x, h, 0.0, 0.0);

  logTraceOut("initialCondition(...)");
}

void exahype2::elastic::CurvilinearSolver::boundaryValues(
  const double* const x,
  const double        t,
  const double        dt,
  const int           faceIndex,
  const int           direction,
  const double* const fluxIn,
  const double* const stateIn,
  const double* const gradStateIn,
  double* const       fluxOut,
  double* const       stateOut
) {
  logTraceInWith3Arguments("boundaryConditions(...)", x, t, direction);

  constexpr int numberOfVariables  = s.SizeVariables;
  constexpr int numberOfParameters = s.SizeParameters;
  constexpr int numberOfData       = numberOfVariables + numberOfParameters;

  for (int i = 0; i < numberOfData; i++) {
    stateOut[i] = stateIn[i];
  }

  for (int i = 0; i < numberOfVariables; i++) {
    fluxOut[i] = fluxIn[i];
  }

  logTraceOut("boundaryConditions(...)");
}

void ::exahype2::elastic::CurvilinearSolver::eigenvalues(
  const double* const Q, // Q[9+16]
  const int           direction,
  double* const       lambda
) {
  logTraceInWith3Arguments("eigenvalue(...)", x, t, normal);

  double cp  = Q[s.cp];
  double cs  = Q[s.cs];
  double q_x = Q[s.metric_derivative + 0];
  double q_y = Q[s.metric_derivative + 1];
  double q_z = Q[s.metric_derivative + 2];
  double r_x = Q[s.metric_derivative + 3];
  double r_y = Q[s.metric_derivative + 4];
  double r_z = Q[s.metric_derivative + 5];
  double s_x = Q[s.metric_derivative + 6];
  double s_y = Q[s.metric_derivative + 7];
  double s_z = Q[s.metric_derivative + 8];

  lambda[0] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cp;
  lambda[1] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs;
  lambda[2] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs;

  lambda[3] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cp;
  lambda[4] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs;
  lambda[5] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs;

  lambda[6] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cp;
  lambda[7] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs;
  lambda[8] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs;

  logTraceOut("eigenvalue(...)");
}

void ::exahype2::elastic::CurvilinearSolver::flux(
  const double* const Q, // Q[9+16]
  double** const      F  // F[Dimensions*9]
) {
  logTraceIn("flux(...)");

  double sigma_xx = Q[s.sigma + 0];
  double sigma_yy = Q[s.sigma + 1];
  double sigma_zz = Q[s.sigma + 2];
  double sigma_xy = Q[s.sigma + 3];
  double sigma_xz = Q[s.sigma + 4];
  double sigma_yz = Q[s.sigma + 5];

  double jacobian = Q[s.jacobian];

  double q_x = Q[s.metric_derivative + 0];
  double q_y = Q[s.metric_derivative + 1];
  double q_z = Q[s.metric_derivative + 2];
  double r_x = Q[s.metric_derivative + 3];
  double r_y = Q[s.metric_derivative + 4];
  double r_z = Q[s.metric_derivative + 5];
  double s_x = Q[s.metric_derivative + 6];
  double s_y = Q[s.metric_derivative + 7];
  double s_z = Q[s.metric_derivative + 8];

  F[0][0] = -jacobian * (q_x * sigma_xx + q_y * sigma_xy + q_z * sigma_xz);
  F[0][1] = -jacobian * (q_x * sigma_xy + q_y * sigma_yy + q_z * sigma_yz);
  F[0][2] = -jacobian * (q_x * sigma_xz + q_y * sigma_yz + q_z * sigma_zz);
  F[0][3] = 0.0;
  F[0][4] = 0.0;
  F[0][5] = 0.0;
  F[0][6] = 0.0;
  F[0][7] = 0.0;
  F[0][8] = 0.0;

  F[1][0] = -jacobian * (r_x * sigma_xx + r_y * sigma_xy + r_z * sigma_xz);
  F[1][1] = -jacobian * (r_x * sigma_xy + r_y * sigma_yy + r_z * sigma_yz);
  F[1][2] = -jacobian * (r_x * sigma_xz + r_y * sigma_yz + r_z * sigma_zz);
  F[1][3] = 0.0;
  F[1][4] = 0.0;
  F[1][5] = 0.0;
  F[1][6] = 0.0;
  F[1][7] = 0.0;
  F[1][8] = 0.0;

  F[2][0] = -jacobian * (s_x * sigma_xx + s_y * sigma_xy + s_z * sigma_xz);
  F[2][1] = -jacobian * (s_x * sigma_xy + s_y * sigma_yy + s_z * sigma_yz);
  F[2][2] = -jacobian * (s_x * sigma_xz + s_y * sigma_yz + s_z * sigma_zz);
  F[2][3] = 0.0;
  F[2][4] = 0.0;
  F[2][5] = 0.0;
  F[2][6] = 0.0;
  F[2][7] = 0.0;
  F[2][8] = 0.0;

  logTraceOut("flux(...)");
}

void ::exahype2::elastic::CurvilinearSolver::nonConservativeProduct(
  const double* const        Q,     // Q[9+16]
  const double* const* const gradQ, // Q[9+16]
  double** const             BgradQ // BgradQ[9]
) {
  logTraceInWith3Arguments("nonConservativeProduct(...)", x, t, normal);

  double u_q = gradQ[0][0];
  double v_q = gradQ[0][1];
  double w_q = gradQ[0][2];

  double u_r = gradQ[1][0];
  double v_r = gradQ[1][1];
  double w_r = gradQ[1][2];

  double u_s = gradQ[2][0];
  double v_s = gradQ[2][1];
  double w_s = gradQ[2][2];

  double q_x = Q[s.metric_derivative + 0];
  double q_y = Q[s.metric_derivative + 1];
  double q_z = Q[s.metric_derivative + 2];
  double r_x = Q[s.metric_derivative + 3];
  double r_y = Q[s.metric_derivative + 4];
  double r_z = Q[s.metric_derivative + 5];
  double s_x = Q[s.metric_derivative + 6];
  double s_y = Q[s.metric_derivative + 7];
  double s_z = Q[s.metric_derivative + 8];

  BgradQ[0][0] = 0;
  BgradQ[0][1] = 0;
  BgradQ[0][2] = 0;
  BgradQ[0][3] = -q_x * u_q;
  BgradQ[0][4] = -q_y * v_q;
  BgradQ[0][5] = -q_z * w_q;
  BgradQ[0][6] = -(q_y * u_q + q_x * v_q); // sigma_xy
  BgradQ[0][7] = -(q_z * u_q + q_x * w_q); // sigma_xz
  BgradQ[0][8] = -(q_z * v_q + q_y * w_q); // sigma_yz

  BgradQ[1][0] = 0;
  BgradQ[1][1] = 0;
  BgradQ[1][2] = 0;
  BgradQ[1][3] = -r_x * u_r;
  BgradQ[1][4] = -r_y * v_r;
  BgradQ[1][5] = -r_z * w_r;
  BgradQ[1][6] = -(r_y * u_r + r_x * v_r); // sigma_xy
  BgradQ[1][7] = -(r_z * u_r + r_x * w_r); // sigma_xz
  BgradQ[1][8] = -(r_z * v_r + r_y * w_r); // sigma_yz

  BgradQ[2][0] = 0;
  BgradQ[2][1] = 0;
  BgradQ[2][2] = 0;
  BgradQ[2][3] = -s_x * u_s;
  BgradQ[2][4] = -s_y * v_s;
  BgradQ[2][5] = -s_z * w_s;
  BgradQ[2][6] = -(s_y * u_s + s_x * v_s); // sigma_xy
  BgradQ[2][7] = -(s_z * u_s + s_x * w_s); // sigma_xz
  BgradQ[2][8] = -(s_z * v_s + s_y * w_s); // sigma_yz

  logTraceOut("nonConservativeProduct(...)");
}

void ::exahype2::elastic::CurvilinearSolver::multiplyMaterialParameterMatrix(
  const double* const Q, double** const rhs
) {

  double rho      = Q[s.rho];
  double c_p      = Q[s.cp];
  double c_s      = Q[s.cs];
  double mu       = rho * c_s * c_s;
  double lambda   = rho * c_p * c_p - 2 * mu;
  double jacobian = Q[s.jacobian];
  double rho_inv  = 1.0 / (rho * jacobian);

  for (int d = 0; d < Dimensions; d++) {
    rhs[d][0] = rho_inv * rhs[d][0];
    rhs[d][1] = rho_inv * rhs[d][1];
    rhs[d][2] = rho_inv * rhs[d][2];

    double lam_temp = lambda * (rhs[d][3] + rhs[d][4] + rhs[d][5]);
    rhs[d][3]       = (2 * mu) * rhs[d][3] + lam_temp;
    rhs[d][4]       = (2 * mu) * rhs[d][4] + lam_temp;
    rhs[d][5]       = (2 * mu) * rhs[d][5] + lam_temp;

    rhs[d][6] = mu * rhs[d][6];
    rhs[d][7] = mu * rhs[d][7];
    rhs[d][8] = mu * rhs[d][8];
  }
}

void ::exahype2::elastic::CurvilinearSolver::initPointSourceLocations(
  double sourceLocation[NumberOfPointSources][Dimensions]
) {
  //    sourceLocation[NumberOfPointSources][Dimensions] = 0.0;

  context->initPointSourceLocation(sourceLocation);
}

void ::exahype2::elastic::CurvilinearSolver::pointSource(
  const double* const Q, // Q[9+16]
  const double* const x,
  const double        t,
  const double        dt,
  double* const       forceVector, // Q[9
  int                 n
) {

  forceVector[0] = 0.0;
  forceVector[1] = 0.0;
  forceVector[2] = 0.0;
  forceVector[3] = 0.0;
  forceVector[4] = 0.0;
  forceVector[5] = 0.0;
  forceVector[6] = 0.0;
  forceVector[7] = 0.0;
  forceVector[8] = 0.0;

  double jacobian = Q[s.jacobian];

  context->setPointSourceVector(Q, x, t, dt, forceVector, n);

  for (int i = 0; i < s.SizeVariables; i++) {
    forceVector[i] = forceVector[i] / jacobian;
  }
}