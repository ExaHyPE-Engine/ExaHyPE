#include "ElasticSolver.h"

#include "exahype2/RefinementControl.h"

// #include "../ExaSeis_core/Context/SolverInformationADERDG.h"
// #include "../ExaSeis_core/Context/DomainInformation.h"
#include "curvi/kdTree/innerNode.h"
#include "curvi/kdTree/root.h"
#include "../ExaSeis_core/Context/DomainInformation.h"
#include "../ExaSeis_core/Context/SolverInformationADERDG.h"
#include "exahype2/aderdg/kernels/Basis/GaussLegendreBasis.h"

tarch::logging::Log exahype2::elastic::ElasticSolver::_log("exahype2::elastic::ElasticSolver");

/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.v] instead of Q[0]
 */
exahype2::elastic::VariableShortcuts s;

exahype2::elastic::ElasticSolver::ElasticSolver():
  AbstractElasticSolver() {

  std::string scenario_string   = "Loh1";
  std::string topography_string = "loh1.yaml";

  pml_cell_width   = 3;
  pml_power        = 1;
  pml_alpha_const  = 1.5;
  pml_alpha_scalar = 0.0;
  pml_rel_error    = 0.001;

  DomainInformation* domain_info = new DomainInformation(this);

  SolverInformationADERDG<Order>* solver_info = new SolverInformationADERDG<Order>(this);

  logTraceOutWith1Argument("Freesurface set at ", std::to_string(_TOP * 2));

  context = new ContextCurvilinear<VariableShortcuts, Order + 1>(
    scenario_string, topography_string, domain_info, solver_info
  );

  context->initTransformation();
  context->initTree();
  context->setRefinementCriteria(this, this->refinementCriteria);

  std::cout << "PML is using " << pml_cell_width << " elements\n";
  std::cout << "Freesurface set at " << _TOP * 2 << "\n";

  initPointSourceLocations(pointSourceLocation);
}

::exahype2::RefinementCommand exahype2::elastic::ElasticSolver::refinementCriterion(
  const double* __restrict__ Q, // Q[36+19]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {

  // for (auto refinementCriterion : refinementCriteria) {
  //   if(refinementCriterion->eval(luh, center , dx , t ,level,refinement)){
  //     _log.info("refinementCriterion()", "Refine cell
  //     "+std::to_string(center[0])+","+std::to_string(center[1])+","+std::to_string(center[2])+" at level
  //     "+std::to_string(level)); return refinement;
  //   }
  // }

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
}

void exahype2::elastic::ElasticSolver::initialCondition(
  double* __restrict__ luh, // Q[36+19]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    point,
  bool                                         gridIsConstructed
) {

  std::fill_n(luh, (s.SizeParameters + s.SizeVariables) * (Order + 1) * (Order + 1) * (Order + 1), 0);

  context->initUnknownsPatch(luh, x, h, 0.0, 0.0);

  // Initialisation of PML parameters

  double dmp_pml_width_x = pml_cell_width * h[0];
  double dmp_pml_width_y = pml_cell_width * h[1];
  double dmp_pml_width_z = pml_cell_width * h[2];

  double dmp_pml_right_x = getDomainOffset()[0] + getDomainSize()[0] - dmp_pml_width_x;
  double dmp_pml_right_y = getDomainOffset()[1] + getDomainSize()[1] - dmp_pml_width_y;
  double dmp_pml_right_z = getDomainOffset()[2] + getDomainSize()[2] - dmp_pml_width_z;

  double dmp_pml_left_x = getDomainOffset()[0] + dmp_pml_width_x;
  double dmp_pml_left_y = getDomainOffset()[1] + dmp_pml_width_y;
  double dmp_pml_left_z = getDomainOffset()[2] + dmp_pml_width_z;

  // constexpr double amplitude_nominator =   6.0;
  // constexpr int dmp_power = 2;

  constexpr double amplitude_nominator = 6.0;

  double dmp_amplitude_x = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_x) * log(1.0 / pml_rel_error);
  double dmp_amplitude_y = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_y) * log(1.0 / pml_rel_error);
  double dmp_amplitude_z = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_z) * log(1.0 / pml_rel_error);

  double offset_x = x[0] - h[0] * 0.5;
  double offset_y = x[1] - h[1] * 0.5;
  double offset_z = x[2] - h[2] * 0.5;

  kernels::idx4 idx(Order + 1, Order + 1, Order + 1, NumberOfVariables + NumberOfParameters);

  for (int k = 0; k < Order + 1; k++) {
    double computational_z = (offset_z + h[2] * nodes[Order][k]);
    for (int j = 0; j < Order + 1; j++) {
      double computational_y = (offset_y + h[1] * nodes[Order][j]);
      for (int i = 0; i < Order + 1; i++) {
        double computational_x = (offset_x + h[0] * nodes[Order][i]);
        if (_TOP != 0) { //  No PML on surface
          if (computational_x < dmp_pml_left_x) {
            luh[idx(
              k, j, i, s.dmp_pml + 0
            )] = dmp_amplitude_x * pow((dmp_pml_left_x - computational_x) / dmp_pml_width_x, pml_power);
          }
        }
        if (_TOP != 1) { //  No PML on surface
          if (computational_y <= dmp_pml_left_y) {
            luh[idx(
              k, j, i, s.dmp_pml + 1
            )] = dmp_amplitude_y * pow((dmp_pml_left_y - computational_y) / dmp_pml_width_y, pml_power);
          }
        }
        if (_TOP != 2) { //  No PML on surface
          if (computational_z <= dmp_pml_left_z) {
            luh[idx(
              k, j, i, s.dmp_pml + 2
            )] = dmp_amplitude_z * pow((dmp_pml_left_z - computational_z) / dmp_pml_width_z, pml_power);
          }
        }
        if (computational_x >= dmp_pml_right_x) {
          luh[idx(
            k, j, i, s.dmp_pml + 0
          )] = dmp_amplitude_x * pow((computational_x - dmp_pml_right_x) / dmp_pml_width_x, pml_power);
        }
        if (computational_y >= dmp_pml_right_y) {
          luh[idx(
            k, j, i, s.dmp_pml + 1
          )] = dmp_amplitude_y * pow((computational_y - dmp_pml_right_y) / dmp_pml_width_y, pml_power);
        }
        if (computational_z >= dmp_pml_right_z) {
          luh[idx(
            k, j, i, s.dmp_pml + 2
          )] = dmp_amplitude_z * pow((computational_z - dmp_pml_right_z) / dmp_pml_width_z, pml_power);
        }
      }
    }
  }
}

void exahype2::elastic::ElasticSolver::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[36+19]
  double* __restrict__ Qoutside,      // Qoutside[36+19]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {

  for (int i = 0; i < s.SizeVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}

double ::exahype2::elastic::ElasticSolver::maxEigenvalue(
  const double* __restrict__ Q, // Q[36+19]
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {

  // Check for NaN in any of the velocities
  assert(("Check for Nans in velocity", !(isnan(Q[s.v + 0]) || isnan(Q[s.v + 1]) || isnan(Q[s.v + 2]))));
  // assert(("Time for failure has been exceeded", t<0.1));

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

  double lambda[10]{1.0};

  lambda[0] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cp;
  lambda[1] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs;
  lambda[2] = std::sqrt(q_x * q_x + q_y * q_y + q_z * q_z) * cs;

  lambda[3] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cp;
  lambda[4] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs;
  lambda[5] = std::sqrt(r_x * r_x + r_y * r_y + r_z * r_z) * cs;

  lambda[6] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cp;
  lambda[7] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs;
  lambda[8] = std::sqrt(s_x * s_x + s_y * s_y + s_z * s_z) * cs;
  lambda[9] = 1.0;

  return *std::max_element(lambda, lambda + 10);
}

void ::exahype2::elastic::ElasticSolver::flux(
  const double* __restrict__ Q, // Q[36+19]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[36]
) {

  double sigma_xx = Q[s.sigma + 0];
  double sigma_yy = Q[s.sigma + 1];
  double sigma_zz = Q[s.sigma + 2];
  double sigma_xy = Q[s.sigma + 3];
  double sigma_xz = Q[s.sigma + 4];
  double sigma_yz = Q[s.sigma + 5];

  double jacobian = Q[s.jacobian];

  double rho     = Q[s.rho];
  double rho_inv = 1.0 / rho;

  std::fill_n(F, NumberOfVariables, 0.0);

  kernels::idx2 idx_pml(Dimensions, 9);

  switch (normal) {
  case 0: {
    double q_x                     = Q[s.metric_derivative + 0];
    double q_y                     = Q[s.metric_derivative + 1];
    double q_z                     = Q[s.metric_derivative + 2];
    F[s.v + 0]                     = -jacobian * (q_x * sigma_xx + q_y * sigma_xy + q_z * sigma_xz);
    F[s.v + 1]                     = -jacobian * (q_x * sigma_xy + q_y * sigma_yy + q_z * sigma_yz);
    F[s.v + 2]                     = -jacobian * (q_x * sigma_xz + q_y * sigma_yz + q_z * sigma_zz);
    F[s.pml + idx_pml(0, s.v + 0)] = F[s.v + 0];
    F[s.pml + idx_pml(0, s.v + 1)] = F[s.v + 1];
    F[s.pml + idx_pml(0, s.v + 2)] = F[s.v + 2];
  } break;
  case 1: {
    double r_x                     = Q[s.metric_derivative + 3];
    double r_y                     = Q[s.metric_derivative + 4];
    double r_z                     = Q[s.metric_derivative + 5];
    F[s.v + 0]                     = -jacobian * (r_x * sigma_xx + r_y * sigma_xy + r_z * sigma_xz);
    F[s.v + 1]                     = -jacobian * (r_x * sigma_xy + r_y * sigma_yy + r_z * sigma_yz);
    F[s.v + 2]                     = -jacobian * (r_x * sigma_xz + r_y * sigma_yz + r_z * sigma_zz);
    F[s.pml + idx_pml(1, s.v + 0)] = F[s.v + 0];
    F[s.pml + idx_pml(1, s.v + 1)] = F[s.v + 1];
    F[s.pml + idx_pml(1, s.v + 2)] = F[s.v + 2];
  } break;
  case 2: {
    double s_x                     = Q[s.metric_derivative + 6];
    double s_y                     = Q[s.metric_derivative + 7];
    double s_z                     = Q[s.metric_derivative + 8];
    F[s.v + 0]                     = -jacobian * (s_x * sigma_xx + s_y * sigma_xy + s_z * sigma_xz);
    F[s.v + 1]                     = -jacobian * (s_x * sigma_xy + s_y * sigma_yy + s_z * sigma_yz);
    F[s.v + 2]                     = -jacobian * (s_x * sigma_xz + s_y * sigma_yz + s_z * sigma_zz);
    F[s.pml + idx_pml(2, s.v + 0)] = F[s.v + 0];
    F[s.pml + idx_pml(2, s.v + 1)] = F[s.v + 1];
    F[s.pml + idx_pml(2, s.v + 2)] = F[s.v + 2];
  }
  }
}

void ::exahype2::elastic::ElasticSolver::nonconservativeProduct(
  const double* __restrict__ Q,      // Q[36+19]
  const double* __restrict__ deltaQ, // deltaQ[36+19]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ BgradQ // BgradQ[36]
) {

  kernels::idx2 idx_pml(Dimensions, 9);
  double        rho       = Q[s.rho];
  double        cp        = Q[s.cp];
  double        cs        = Q[s.cs];
  double        jacobian  = Q[s.jacobian];
  double        dmp_pml_x = Q[s.dmp_pml + 0];
  double        dmp_pml_y = Q[s.dmp_pml + 1];
  double        dmp_pml_z = Q[s.dmp_pml + 2];
  double        mu        = rho * cs * cs;
  double        lambda    = rho * cp * cp - 2 * mu;
  double        rho_inv   = 1.0 / (rho * jacobian);

  std::fill_n(BgradQ, NumberOfVariables, 0.0);

  switch (normal) {
  case 0: {
    double u_q                              = deltaQ[s.v + 0];
    double v_q                              = deltaQ[s.v + 1];
    double w_q                              = deltaQ[s.v + 2];
    double q_x                              = Q[s.metric_derivative + 0];
    double q_y                              = Q[s.metric_derivative + 1];
    double q_z                              = Q[s.metric_derivative + 2];
    double lam_temp                         = lambda * (-q_x * u_q - q_y * v_q - q_z * w_q);
    BgradQ[s.sigma + 0]                     = -2 * mu * q_x * u_q + lam_temp;
    BgradQ[s.sigma + 1]                     = -2 * mu * q_y * v_q + lam_temp;
    BgradQ[s.sigma + 2]                     = -2 * mu * q_z * w_q + lam_temp;
    BgradQ[s.sigma + 3]                     = -mu * (q_y * u_q + q_x * v_q); // sigma_xy
    BgradQ[s.sigma + 4]                     = -mu * (q_z * u_q + q_x * w_q); // sigma_xz
    BgradQ[s.sigma + 5]                     = -mu * (q_z * v_q + q_y * w_q); // sigma_yz
    BgradQ[s.pml + idx_pml(0, s.sigma + 0)] = -dmp_pml_x * q_x * u_q;
    BgradQ[s.pml + idx_pml(0, s.sigma + 1)] = -dmp_pml_x * q_y * v_q;
    BgradQ[s.pml + idx_pml(0, s.sigma + 2)] = -dmp_pml_x * q_z * w_q;
    BgradQ[s.pml + idx_pml(0, s.sigma + 3)] = -dmp_pml_x * (q_y * u_q + q_x * v_q);
    BgradQ[s.pml + idx_pml(0, s.sigma + 4)] = -dmp_pml_x * (q_z * u_q + q_x * w_q);
    BgradQ[s.pml + idx_pml(0, s.sigma + 5)] = -dmp_pml_x * (q_z * v_q + q_y * w_q);
  } break;
  case 1: {
    double u_r                              = deltaQ[s.v + 0];
    double v_r                              = deltaQ[s.v + 1];
    double w_r                              = deltaQ[s.v + 2];
    double r_x                              = Q[s.metric_derivative + 3];
    double r_y                              = Q[s.metric_derivative + 4];
    double r_z                              = Q[s.metric_derivative + 5];
    double lam_temp                         = lambda * (-r_x * u_r - r_y * v_r - r_z * w_r);
    BgradQ[s.sigma + 0]                     = -2 * mu * r_x * u_r + lam_temp;
    BgradQ[s.sigma + 1]                     = -2 * mu * r_y * v_r + lam_temp;
    BgradQ[s.sigma + 2]                     = -2 * mu * r_z * w_r + lam_temp;
    BgradQ[s.sigma + 3]                     = -mu * (r_y * u_r + r_x * v_r); // sigma_xy
    BgradQ[s.sigma + 4]                     = -mu * (r_z * u_r + r_x * w_r); // sigma_xz
    BgradQ[s.sigma + 5]                     = -mu * (r_z * v_r + r_y * w_r); // sigma_yz
    BgradQ[s.pml + idx_pml(1, s.sigma + 0)] = -dmp_pml_y * r_x * u_r;
    BgradQ[s.pml + idx_pml(1, s.sigma + 1)] = -dmp_pml_y * r_y * v_r;
    BgradQ[s.pml + idx_pml(1, s.sigma + 2)] = -dmp_pml_y * r_z * w_r;
    BgradQ[s.pml + idx_pml(1, s.sigma + 3)] = -dmp_pml_y * (r_y * u_r + r_x * v_r);
    BgradQ[s.pml + idx_pml(1, s.sigma + 4)] = -dmp_pml_y * (r_z * u_r + r_x * w_r);
    BgradQ[s.pml + idx_pml(1, s.sigma + 5)] = -dmp_pml_y * (r_z * v_r + r_y * w_r);
  } break;
  case 2: {
    double u_s                              = deltaQ[s.v + 0];
    double v_s                              = deltaQ[s.v + 1];
    double w_s                              = deltaQ[s.v + 2];
    double s_x                              = Q[s.metric_derivative + 6];
    double s_y                              = Q[s.metric_derivative + 7];
    double s_z                              = Q[s.metric_derivative + 8];
    double lam_temp                         = lambda * (-s_x * u_s - s_y * v_s - s_z * w_s);
    BgradQ[s.sigma + 0]                     = -2 * mu * s_x * u_s + lam_temp;
    BgradQ[s.sigma + 1]                     = -2 * mu * s_y * v_s + lam_temp;
    BgradQ[s.sigma + 2]                     = -2 * mu * s_z * w_s + lam_temp;
    BgradQ[s.sigma + 3]                     = -mu * (s_y * u_s + s_x * v_s); // sigma_xy
    BgradQ[s.sigma + 4]                     = -mu * (s_z * u_s + s_x * w_s); // sigma_xz
    BgradQ[s.sigma + 5]                     = -mu * (s_z * v_s + s_y * w_s); // sigma_yz
    BgradQ[s.pml + idx_pml(2, s.sigma + 0)] = -dmp_pml_z * s_x * u_s;
    BgradQ[s.pml + idx_pml(2, s.sigma + 1)] = -dmp_pml_z * s_y * v_s;
    BgradQ[s.pml + idx_pml(2, s.sigma + 2)] = -dmp_pml_z * s_z * w_s;
    BgradQ[s.pml + idx_pml(2, s.sigma + 3)] = -dmp_pml_z * (s_y * u_s + s_x * v_s);
    BgradQ[s.pml + idx_pml(2, s.sigma + 4)] = -dmp_pml_z * (s_z * u_s + s_x * w_s);
    BgradQ[s.pml + idx_pml(2, s.sigma + 5)] = -dmp_pml_z * (s_z * v_s + s_y * w_s);
  }
  }
}

void ::exahype2::elastic::ElasticSolver::algebraicSource(
  const tarch::la::Vector<Dimensions, double>& x, double t, const double* const Q, double* S
) {

  double rho = Q[s.rho];
  double cp  = Q[s.cp];
  double cs  = Q[s.cs];

  double dmp_pml_x = Q[s.dmp_pml + 0];
  double dmp_pml_y = Q[s.dmp_pml + 1];
  double dmp_pml_z = Q[s.dmp_pml + 2];

  double mu      = rho * cs * cs;
  double lambda  = rho * cp * cp - 2 * mu;
  double rho_inv = 1.0 / rho;

  double alpha_x = (pml_alpha_const + pml_alpha_scalar * dmp_pml_x);
  double alpha_y = (pml_alpha_const + pml_alpha_scalar * dmp_pml_y);
  double alpha_z = (pml_alpha_const + pml_alpha_scalar * dmp_pml_z);

  kernels::idx2 idx_pml(Dimensions, 9);

  const double* Q_pml = Q + s.pml;
  double*       S_pml = S + s.pml;

  S[0] = rho_inv * (Q_pml[idx_pml(0, 0)] + Q_pml[idx_pml(1, 0)] + Q_pml[idx_pml(2, 0)]);
  S[1] = rho_inv * (Q_pml[idx_pml(0, 1)] + Q_pml[idx_pml(1, 1)] + Q_pml[idx_pml(2, 1)]);
  S[2] = rho_inv * (Q_pml[idx_pml(0, 2)] + Q_pml[idx_pml(1, 2)] + Q_pml[idx_pml(2, 2)]);

  S[3] = (2 * mu + lambda) * Q_pml[idx_pml(0, 3)] + lambda * (Q_pml[idx_pml(0, 4)] + Q_pml[idx_pml(0, 5)])
         + (2 * mu + lambda) * Q_pml[idx_pml(1, 3)] + lambda * (Q_pml[idx_pml(1, 4)] + Q_pml[idx_pml(1, 5)])
         + (2 * mu + lambda) * Q_pml[idx_pml(2, 3)] + lambda * (Q_pml[idx_pml(2, 4)] + Q_pml[idx_pml(2, 5)]);

  S[4] = (2 * mu + lambda) * Q_pml[idx_pml(0, 4)] + lambda * (Q_pml[idx_pml(0, 3)] + Q_pml[idx_pml(0, 5)])
         + (2 * mu + lambda) * Q_pml[idx_pml(1, 4)] + lambda * (Q_pml[idx_pml(1, 3)] + Q_pml[idx_pml(1, 5)])
         + (2 * mu + lambda) * Q_pml[idx_pml(2, 4)] + lambda * (Q_pml[idx_pml(2, 3)] + Q_pml[idx_pml(2, 5)]);

  S[5] = (2 * mu + lambda) * Q_pml[idx_pml(0, 5)] + lambda * (Q_pml[idx_pml(0, 3)] + Q_pml[idx_pml(0, 4)])
         + (2 * mu + lambda) * Q_pml[idx_pml(1, 5)] + lambda * (Q_pml[idx_pml(1, 3)] + Q_pml[idx_pml(1, 4)])
         + (2 * mu + lambda) * Q_pml[idx_pml(2, 5)] + lambda * (Q_pml[idx_pml(2, 3)] + Q_pml[idx_pml(2, 4)]);

  S[6] = mu * (Q_pml[idx_pml(0, 6)] + Q_pml[idx_pml(1, 6)] + Q_pml[idx_pml(2, 6)]);
  S[7] = mu * (Q_pml[idx_pml(0, 7)] + Q_pml[idx_pml(1, 7)] + Q_pml[idx_pml(2, 7)]);
  S[8] = mu * (Q_pml[idx_pml(0, 8)] + Q_pml[idx_pml(1, 8)] + Q_pml[idx_pml(2, 8)]);

  for (int j = 0; j < 9; j++) {
    S_pml[idx_pml(0, j)] = (dmp_pml_x + alpha_x) * Q_pml[idx_pml(0, j)];
    S_pml[idx_pml(1, j)] = (dmp_pml_y + alpha_y) * Q_pml[idx_pml(1, j)];
    S_pml[idx_pml(2, j)] = (dmp_pml_z + alpha_z) * Q_pml[idx_pml(2, j)];
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

  double rho = Q[s.rho];
  double cp  = Q[s.cp];
  double cs  = Q[s.cs];

  double jacobian = Q[s.jacobian];

  double dmp_pml_x = Q[s.dmp_pml + 0];
  double dmp_pml_y = Q[s.dmp_pml + 1];
  double dmp_pml_z = Q[s.dmp_pml + 2];

  double mu      = rho * cs * cs;
  double lambda  = rho * cp * cp - 2 * mu;
  double rho_inv = 1.0 / (rho * jacobian);

  kernels::idx2 idx_pml(Dimensions, 9);

  // Rhs uses the same formula regardless of dimension, hence no switch necessary

  rhs[s.v + 0] = rho_inv * rhs[s.v + 0];
  rhs[s.v + 1] = rho_inv * rhs[s.v + 1];
  rhs[s.v + 2] = rho_inv * rhs[s.v + 2];

  for (int j = 0; j < 3; j++) {
    rhs[s.pml + idx_pml(0, j)] = dmp_pml_x / jacobian * rhs[s.pml + idx_pml(0, j)];
    rhs[s.pml + idx_pml(1, j)] = dmp_pml_y / jacobian * rhs[s.pml + idx_pml(1, j)];
    rhs[s.pml + idx_pml(2, j)] = dmp_pml_z / jacobian * rhs[s.pml + idx_pml(2, j)];
  }
}

void ::exahype2::elastic::ElasticSolver::initPointSourceLocations(
  double sourceLocation[NumberOfPointSources][Dimensions]
) {
  context->initPointSourceLocation(pointSourceLocation);
}

void ::exahype2::elastic::ElasticSolver::pointSource(
  const double* const Q, // Q[36+19]
  const double* const x,
  const double        t,
  const double        dt,
  double* const       forceVector, // Q[36]
  int                 n
) {
  std::fill_n(forceVector, NumberOfVariables, 0.0);
  double jacobian = Q[s.jacobian];
  context->setPointSourceVector(Q, x, t, dt, forceVector, n);

  for (int i = 0; i < s.SizeVariables; i++) {
    forceVector[i] = forceVector[i] / jacobian;
  }
}

void exahype2::elastic::ElasticSolver::riemannSolver(
  double* const                                FL, // FL[36
  double* const                                FR, // FR[36
  const double* const                          QL, // QL[36+19]
  const double* const                          QR, // QR[36+19]
  const double                                 t,
  const double                                 dt,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const int                                    direction,
  bool                                         isBoundaryFace,
  int                                          faceIndex
) {

  // //Local lax-friedrich
  // generated::kernels::AderDG::linear::riemannSolver(
  //   *this,
  //   FL,
  //   FR,
  //   QL,
  //   QR,
  //   t,
  //   dt,
  //   x,
  //   h,
  //   direction
  // );

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

  Numerics::riemannSolver<VariableShortcuts, Order, NumberOfVariables, NumberOfParameters, _TOP * 2>(
    FL, FR, QL, QR, dt, direction, isBoundaryFace, myFaceIndex
  );
}