#include "ElasticSolver.h"

#include "exahype2/RefinementControl.h"
#include "../ExaSeis_core/Context/ContextCurvilinear.h"
#include "../ExaSeis_core/Context/DomainInformation.h"
#include "../ExaSeis_core/Context/SolverInformationADERDG.h"

tarch::logging::Log exahype2::elastic::ElasticSolver::_log("exahype2::elastic::ElasticSolver");

/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.v] instead of Q[0]
 */
exahype2::elastic::VariableShortcuts s;

exahype2::elastic::ElasticSolver::ElasticSolver():
  AbstractElasticSolver() {

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

  initPointSourceLocations(pointSourceLocation);
}

void exahype2::elastic::ElasticSolver::initialCondition(
  double* __restrict__ Q, // Q[9+16]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    point,
  bool                                         gridIsConstructed
) {

  std::fill_n(Q, (s.SizeParameters + s.SizeVariables) * (Order + 1) * (Order + 1) * (Order + 1), 0);
  context->initUnknownsPatch(Q, x, h, 0.0, 0.0);
}

::exahype2::RefinementCommand exahype2::elastic::ElasticSolver::refinementCriterion(
  const double* __restrict__ Q, // Q[9+16]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // tarch::la::Vector<DIMENSIONS,double> center_curve;
  // tarch::la::Vector<DIMENSIONS,double> dx_curve;

  // if(tarch::la::equals(t,0.0)){
  //   context->getElementCenter(luh,center_curve);
  //   context->getElementSize  (luh,dx_curve);
  // }else{
  //   center_curve[0] = x[0];
  //   center_curve[1] = x[1];
  //   center_curve[2] = x[2];

  //   dx_curve[0] = h[0];
  //   dx_curve[1] = h[1];
  //   dx_curve[2] = h[2];
  // }

  // for (auto refinementCriterion : refinementCriteria) {
  //   if(refinementCriterion->eval(luh, center_curve , dx_curve , t ,level,refinement)){
  //     return refinement;
  //   }
  // }

  return result;
}

void exahype2::elastic::ElasticSolver::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[9+16]
  double* __restrict__ Qoutside,      // Qoutside[9+16]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {

  std::copy_n(Qinside, NumberOfVariables + NumberOfParameters, Qoutside);
}

double ::exahype2::elastic::ElasticSolver::maxEigenvalue(
  const double* __restrict__ Q, // Q[9+16]
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {

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
}

void ::exahype2::elastic::ElasticSolver::flux(
  const double* __restrict__ Q, // Q[9+16]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[9]
) {

  double sigma_xx = Q[s.sigma + 0];
  double sigma_yy = Q[s.sigma + 1];
  double sigma_zz = Q[s.sigma + 2];
  double sigma_xy = Q[s.sigma + 3];
  double sigma_xz = Q[s.sigma + 4];
  double sigma_yz = Q[s.sigma + 5];

  double jacobian = Q[s.jacobian];

  switch (normal) {
  case 0: {
    double q_x = Q[s.metric_derivative + 0];
    double q_y = Q[s.metric_derivative + 1];
    double q_z = Q[s.metric_derivative + 2];
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
    double r_x = Q[s.metric_derivative + 3];
    double r_y = Q[s.metric_derivative + 4];
    double r_z = Q[s.metric_derivative + 5];
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
    double s_x = Q[s.metric_derivative + 6];
    double s_y = Q[s.metric_derivative + 7];
    double s_z = Q[s.metric_derivative + 8];
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
}

void ::exahype2::elastic::ElasticSolver::nonconservativeProduct(
  const double* __restrict__ Q,      // Q[9+16]
  const double* __restrict__ deltaQ, // deltaQ[9+16]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ BgradQ // BgradQ[9]
) {

  switch (normal) {
  case 0: {
    double u_q = deltaQ[0];
    double v_q = deltaQ[1];
    double w_q = deltaQ[2];
    double q_x = Q[s.metric_derivative + 0];
    double q_y = Q[s.metric_derivative + 1];
    double q_z = Q[s.metric_derivative + 2];
    BgradQ[0]  = 0;
    BgradQ[1]  = 0;
    BgradQ[2]  = 0;
    BgradQ[3]  = -q_x * u_q;
    BgradQ[4]  = -q_y * v_q;
    BgradQ[5]  = -q_z * w_q;
    BgradQ[6]  = -(q_y * u_q + q_x * v_q); // sigma_xy
    BgradQ[7]  = -(q_z * u_q + q_x * w_q); // sigma_xz
    BgradQ[8]  = -(q_z * v_q + q_y * w_q); // sigma_yz
  } break;
  case 1: {
    double u_r = deltaQ[0];
    double v_r = deltaQ[1];
    double w_r = deltaQ[2];
    double r_x = Q[s.metric_derivative + 3];
    double r_y = Q[s.metric_derivative + 4];
    double r_z = Q[s.metric_derivative + 5];
    BgradQ[0]  = 0;
    BgradQ[1]  = 0;
    BgradQ[2]  = 0;
    BgradQ[3]  = -r_x * u_r;
    BgradQ[4]  = -r_y * v_r;
    BgradQ[5]  = -r_z * w_r;
    BgradQ[6]  = -(r_y * u_r + r_x * v_r); // sigma_xy
    BgradQ[7]  = -(r_z * u_r + r_x * w_r); // sigma_xz
    BgradQ[8]  = -(r_z * v_r + r_y * w_r); // sigma_yz
  } break;
  case 2: {
    double u_s = deltaQ[0];
    double v_s = deltaQ[1];
    double w_s = deltaQ[2];
    double s_x = Q[s.metric_derivative + 6];
    double s_y = Q[s.metric_derivative + 7];
    double s_z = Q[s.metric_derivative + 8];
    BgradQ[0]  = 0;
    BgradQ[1]  = 0;
    BgradQ[2]  = 0;
    BgradQ[3]  = -s_x * u_s;
    BgradQ[4]  = -s_y * v_s;
    BgradQ[5]  = -s_z * w_s;
    BgradQ[6]  = -(s_y * u_s + s_x * v_s); // sigma_xy
    BgradQ[7]  = -(s_z * u_s + s_x * w_s); // sigma_xz
    BgradQ[8]  = -(s_z * v_s + s_y * w_s); // sigma_yz
  }
  }
}

void ::exahype2::elastic::ElasticSolver::multiplyMaterialParameterMatrix(
  const double* const                          Q,
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* const                                rhs
) {

  double rho      = Q[s.rho];
  double c_p      = Q[s.cp];
  double c_s      = Q[s.cs];
  double mu       = rho * c_s * c_s;
  double lambda   = rho * c_p * c_p - 2 * mu;
  double jacobian = Q[s.jacobian];
  double rho_inv  = 1.0 / (rho * jacobian);

  // identical in all dimensions so no need for switch
  rhs[0] = rho_inv * rhs[0];
  rhs[1] = rho_inv * rhs[1];
  rhs[2] = rho_inv * rhs[2];

  double lam_temp = lambda * (rhs[3] + rhs[4] + rhs[5]);
  rhs[3]          = (2 * mu) * rhs[3] + lam_temp;
  rhs[4]          = (2 * mu) * rhs[4] + lam_temp;
  rhs[5]          = (2 * mu) * rhs[5] + lam_temp;

  rhs[6] = mu * rhs[6];
  rhs[7] = mu * rhs[7];
  rhs[8] = mu * rhs[8];
}

void ::exahype2::elastic::ElasticSolver::initPointSourceLocations(
  double sourceLocation[NumberOfPointSources][Dimensions]
) {

  dynamic_cast<ContextCurvilinear<VariableShortcuts, Order + 1>*>(context)->initPointSourceLocation(pointSourceLocation
  );
}

void ::exahype2::elastic::ElasticSolver::pointSource(
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

void exahype2::elastic::ElasticSolver::riemannSolver(
  double* const                                FL, // FL[9
  double* const                                FR, // FR[9
  const double* const                          QL, // QL[9+16]
  const double* const                          QR, // QR[9+16]
  const double                                 t,
  const double                                 dt,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const int                                    direction,
  bool                                         isBoundaryFace,
  int                                          faceIndex
) {

  if (isBoundaryFace) {
    double* FIn  = faceIndex < Dimensions ? FR : FL;
    double* FOut = faceIndex < Dimensions ? FL : FR;
    std::copy_n(FIn, s.Size, FOut);
  }

  int myFaceIndex = 0;
  switch (faceIndex) {
  case 0:
    myFaceIndex = 0;
    break;
  case 1:
    myFaceIndex = 2;
    break;
  case 2:
    myFaceIndex = 4;
    break;
  case 3:
    myFaceIndex = 1;
    break;
  case 4:
    myFaceIndex = 3;
    break;
  case 5:
    myFaceIndex = 5;
    break;
  }

  Numerics::
    riemannSolver<VariableShortcuts, Order + 1, NumberOfVariables, (NumberOfVariables + NumberOfParameters), _TOP * 2>(
      FL, FR, QL, QR, dt, direction, isBoundaryFace, myFaceIndex
    );
}