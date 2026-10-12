import argparse
import sys
import os
import numpy as np

import exahype2

from atmosphereE_pertubations import Atmosphere
from exahype2.fv.scheme import Limiter          # add near the top
from f_wave import f_wave_riemannsolver, max_eigenavalue
from modal_filter import modal_filter_kernel


from run_config import embed_run_config

from exahype2.dg.ader.actionsets.brissaud_lim import LimitBeforePredict



stratified_atmosphere = Atmosphere()


size = [1200e3, 220e3, 1]
# size = [1200e3, 600e3, 1]
end_time = 6000
order = 5
# order = 2
unknowns = {"rho": 1, "j": 2, "E": 1}
aux_var = {"height": 1, "backgroundstate": 3, "O": 1, "O2": 1, "N2": 1, "T": 1, "viscosity": 1, "unknowns_avg": 4, "troubled": 1}
use_limiting = True
use_modal_filter = False

file = '/work_fast/go78vuz/EXAHYPE/GW/applications/gravity_waves/AMR/patch/ADERDGSolver.vtkhdf'
# restart_from = '/work_fast/go78vuz/EXAHYPE/mario_new_limiting/applications/gravity_waves/restart_26_08/patch/ADERDGSolver.vtkhdf'
restart_from = file
restart_step = 30
restart_time = 3000


restart_from = None

if restart_from is None:
    initial_conditions = stratified_atmosphere.initial_conditions()
    restart_time = 0

else:
    initial_conditions = stratified_atmosphere.initial_conditions_from_file(
        restart_from, restart_step)



name = "Thunderstorm_Euler_pertub"


project = exahype2.Project(
    namespace=["applications", "gravity_waves", "thunderstorm"],
    project_name=name,
    executable=name,
    dimensions=2,
    offset=[0, 0, 0],
    size=size,
    periodic_bc=[False, False, False],
    min_end_time=end_time,
    max_end_time=end_time,
    min_grid_levels=4,
    max_grid_levels=5,
)

solver = exahype2.dg.ader.Solver(
    name                 = "ADERDGSolver",
    order                = order,
    time_stepping = exahype2.TimeStepping.Adaptive(0.5),
    unknowns             = unknowns,
    auxiliary_variables = aux_var
    # auxiliary_variables  = {"grad": 8, "height": 1, "backgroundstate": 2},
)


solver.set_pde_terms(
    initial_condition         = initial_conditions,
    boundary_conditions        = stratified_atmosphere.boundary_conditions_pertb_1_3(),
    flux                       = stratified_atmosphere.flux(),
    max_eigenvalue             = stratified_atmosphere.eigenvalues(),
    riemann_solver=stratified_atmosphere.riemann_solver(),
    diffusive_flux=False,
    source=stratified_atmosphere.source_with_TS(
        2*np.pi / (6.05*60),   # omega = 1.731e-2 rad/s  (tau = 6.05 min)
        2*np.pi / 31.4e3,      # k_x   = 2.001e-4 rad/m  (lambda_x = 31.4 km)
        600e3,                 # x_0
        12e3,                  # z_0
        600,                   # t_0
        1e-7,                  # F_0/rho
        30e3,                  # sigma_x
        3e3,                   # sigma_z
        2000,                  # sigma_t
    time_offset=restart_time),
physical_admissibility = stratified_atmosphere.physical_admissibility_criterion_true(),
    #dmp_observables = stratified_atmosphere.map_observerables()

)
solver.add_includes(
    "tarch/reader/CSVReader.h",
    "tarch/reader/VTKSampler.h",
    "iostream",
    "cmath",
    "algorithm",
    "cstdlib"
)

solver.add_declarations(Atmosphere.CSV_CONSTANTS)

solver.add_declarations(f"""
  // current simulation time, read by isPhysicallyAdmissible
  static inline double limiterTime = {float(restart_time)};
""")
solver.on_start_time_step(f"""
  limiterTime = getMinTimeStamp() + {float(restart_time)};
""")


# TEST: slope limiting like suggested by brissaud 2017 (hybrid galerkin numerical modelling ...) 


# solver.postprocess_updated_patch = """ //Compute the state average for each cell
#     // cell average of the unknowns: sum_nodes (prod_d w_d) * Q / sum of weights
#     constexpr int NodesPerAxis = {{DG_ORDER}} + 1;
#     const auto* const w = {{QUADRATURE_TABLE}}::QuadratureWeights1D;

#     double avg[{{NUMBER_OF_UNKNOWNS}}] = {0.0};
#     double weightSum = 0.0;
#     for (int node = 0; node < {{NUMBER_OF_DOFS_PER_CELL}}; node++) {
#     double weight = 1.0;
#     for (int d = 0, rest = node; d < DIMENSIONS; d++, rest /= NodesPerAxis) {
#         weight *= w[rest % NodesPerAxis];
#     }
#     for (int v = 0; v < {{NUMBER_OF_UNKNOWNS}}; v++) {
#         avg[v] += weight * QNew[node * {{NUMBER_OF_DATA}} + v];
#     }
#     weightSum += weight;
#     }

#     // store the average in every node's unknowns_avg slots
#     for (int node = 0; node < {{NUMBER_OF_DOFS_PER_CELL}}; node++) {
#     for (int v = 0; v < {{NUMBER_OF_UNKNOWNS}}; v++) {
#         QNew[node * {{NUMBER_OF_DATA}} + {{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::unknowns_avg + v] = avg[v] / weightSum;
#     }
#     }


#     constexpr int NodesPerFace = DIMENSIONS == 2 ? ({{DG_ORDER}} + 1) : ({{DG_ORDER}} + 1) * ({{DG_ORDER}} + 1);        
#     for (int f = 0; f < 2 * DIMENSIONS; f++) {
#         auto* const face = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(f).data();
#         auto* const own  = face + (f < DIMENSIONS ? 1 : 0) * NodesPerFace * {{NUMBER_OF_DATA}};   // QR on x-/z-, QL on x+/z+
#         for (int n = 0; n < NodesPerFace; n++) {
#             for (int v = 0; v < {{NUMBER_OF_UNKNOWNS}}; v++) {
#                 own[n * {{NUMBER_OF_DATA}} + {{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::unknowns_avg + v] = avg[v] / weightSum;
#             }
#         }
#     }




# """


# project.add_action_set_to_timestepping(
#     LimitBeforePredict(solver, kernel="""
#     // get average values at faces of current cell

#     constexpr int N = {{DG_ORDER}} + 1;
#     const auto* const w = {{QUADRATURE_TABLE}}::QuadratureWeights1D;
#     const auto* const L = kernels::{{SOLVER_NAME}}::DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::BasisFunctionValuesLeft1D;   // φ_i(0)
#     const auto* const R = kernels::{{SOLVER_NAME}}::DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::BasisFunctionValuesRight1D;  // φ_i(1)

#     // faceAvg[f][v], faces: 0 = x-, 1 = z-, 2 = x+, 3 = z+ ; node (i, j) = Q[(j * N + i) * NUMBER_OF_DATA + v]
#     double faceAvg[2 * DIMENSIONS][{{NUMBER_OF_UNKNOWNS}}] = {};
#     for (int k = 0; k < N; k++) {              // position along the face
#     for (int i = 0; i < N; i++) {            // position normal to the face
#         for (int v = 0; v < {{NUMBER_OF_UNKNOWNS}}; v++) {
#         const double qx = Q[(k * N + i) * {{NUMBER_OF_DATA}} + v];   // row j = k, walking in x
#         const double qz = Q[(i * N + k) * {{NUMBER_OF_DATA}} + v];   // column i = k, walking in z
#         faceAvg[0][v] += w[k] * L[i] * qx;
#         faceAvg[2][v] += w[k] * R[i] * qx;
#         faceAvg[1][v] += w[k] * L[i] * qz;
#         faceAvg[3][v] += w[k] * R[i] * qz;
#         }
#     }
#     }

#     // cell average of current cell (2D and 3D): sum_nodes (prod_d w_d) * Q, weights on [0,1] sum to 1
#     double cellAvg[{{NUMBER_OF_UNKNOWNS}}] = {};
#     for (int node = 0; node < {{NUMBER_OF_DOFS_PER_CELL}}; node++) {
#         double weight = 1.0;
#         for (int d = 0, rest = node; d < DIMENSIONS; d++, rest /= N) {
#             weight *= w[rest % N];
#         }
#         for (int v = 0; v < {{NUMBER_OF_UNKNOWNS}}; v++) {
#             cellAvg[v] += weight * Q[node * {{NUMBER_OF_DATA}} + v];
#         }
#     }

#     auto sign = [](double x) { return (x > 0.0) - (x < 0.0); };


#     [[maybe_unused]] auto minmod = [&](double a1, double a2, double a3) -> double {
#         double s = (sign(a1) + sign(a2) + sign(a3)); 
#         if (std::abs(s) == 3){return (s/3)*std::min(std::abs(a1), std::min(std::abs(a2),std::abs(a3))); }
#         else return 0;

#     };

#     // local scales per conserved variable from the background state of this cell (node 0 suffices)
#     const double rho0 = Q[{{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::backgroundstate + 0];
#     const double p0   = Q[{{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::backgroundstate + 1];
#     const double E0   = Q[{{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::backgroundstate + 2];
#     const double c0   = std::sqrt(1.4 * p0 / rho0);           // sound speed as a velocity scale

#     // rho', jx, jz, E'
#     const double scale[{{NUMBER_OF_UNKNOWNS}}] = { rho0, rho0 * c0, rho0 * c0, E0 };
#     //const double Mrel = 1e-6;                                   // dimensionless, tune once

#     auto cellsize = marker.getH();
#      [[maybe_unused]] auto minmod_alternative = [&](double a1, double a2, double a3,  double Mh2) -> double {
#         a2 += Mh2*sign(a2); 
#         a3 += Mh2*sign(a3);
#         double s = (sign(a1) + sign(a2) + sign(a3)); 
#         if (std::abs(s) == 3){return (s/3)*std::min(std::abs(a1), std::min(std::abs(a2),std::abs(a3))); }
#         else return 0;

#     };

#     auto minmodTVB = [&](double a1, double a2, double a3, double Mh2) -> double {
#         if (std::abs(a1) <= Mh2) return a1;    // smooth enough: don't touch it
#         return minmod(a1, a2, a3);
#     };

      
      

#     // set the troubled flag on every node of the current cell
#     auto setTroubled = [&](double value) {
#       for (int node = 0; node < {{NUMBER_OF_DOFS_PER_CELL}}; node++) {
#         Q[node * {{NUMBER_OF_DATA}} + {{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::troubled] = value;
#       }
#     };

#     // defaults, overridable at runtime via LAMBDA_MIN_X / LAMBDA_MIN_Z (used by sweep_lambda.sh)
#     auto envOr = [](const char* key, double fallback) {
#         const char* value = std::getenv(key);
#         return value ? std::atof(value) : fallback;
#     };
#     static const double lambdaMin_x = envOr("LAMBDA_MIN_X", 45000);
#     static const double lambdaMin_z = envOr("LAMBDA_MIN_Z", 15000);
#     const double kmin[2] = { 2.0 * M_PI / lambdaMin_x, 2.0 * M_PI / lambdaMin_z };
    
#     setTroubled(0.0);
#     bool troubled = false;
#     for (int k = 0; k < DIMENSIONS; k++) { 
        
#         int left_face = k; 
#         int right_face = k + DIMENSIONS; 
#         const double* right_neighbour = QR[right_face]; 
#         const double* left_neighbour = QL[left_face]; 
        

            
#         for (int v = 0; v < {{NUMBER_OF_UNKNOWNS}}; v++) {

#             const double qL = faceAvg[left_face][v]; 
#             const double qR = faceAvg[right_face][v]; 

            
#             const double avgL = left_neighbour [{{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::unknowns_avg + v];
#             const double avgR = right_neighbour[{{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::unknowns_avg + v];
            
#             const double qcenter = cellAvg[v];

#             const double A   = std::max({std::abs(qcenter), std::abs(avgL), std::abs(avgR)});
#             const double Mh2 = (2.0/3.0) * A * kmin[k]*kmin[k] * cellsize[k]*cellsize[k];
#             double vl = qcenter - minmodTVB(qcenter - qL, qcenter - avgL, avgR - qcenter, Mh2);
#             double vr = qcenter + minmodTVB(qR - qcenter, qcenter - avgL, avgR - qcenter, Mh2);
#             const double eps = 1e-3 * A + 1e-12 * scale[v];   // scale[v] is now the background array again

#             if (std::abs(vl - qL) > eps || std::abs(vr - qR) > eps) {
#                 setTroubled(1.0);
#                 troubled = true;
#             }
            
#         }

#     }

#         // for troubled cells --> linear reconstruction
#         if (troubled) {
#             const auto* const xi = {{QUADRATURE_TABLE}}::QuadraturePoints1D;   // nodes on [0,1]

#             for (int v = 0; v < {{NUMBER_OF_UNKNOWNS}}; v++) {
#                 // slope of the L2 projection onto P1, per reference cell width, in each direction
#                 double slopeDG[DIMENSIONS] = {};
#                 for (int node = 0; node < {{NUMBER_OF_DOFS_PER_CELL}}; node++) {
#                     double weight = 1.0;
#                     for (int d = 0, rest = node; d < DIMENSIONS; d++, rest /= N) {
#                         weight *= w[rest % N];
#                     }
#                     for (int d = 0, rest = node; d < DIMENSIONS; d++, rest /= N) {
#                         slopeDG[d] += 12.0 * weight * (xi[rest % N] - 0.5) * Q[node * {{NUMBER_OF_DATA}} + v];
#                     }
#                 }

#                 // limit each slope against the differences of the neighbour averages
#                 double slope[DIMENSIONS];
#                 for (int d = 0; d < DIMENSIONS; d++) {
#                     const double avgL = QL[d             ][{{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::unknowns_avg + v];
#                     const double avgR = QR[d + DIMENSIONS][{{FULL_QUALIFIED_SOLVER_NAME}}::Shortcuts::unknowns_avg + v];
#                     const double A   = std::max({std::abs(cellAvg[v]), std::abs(avgL), std::abs(avgR)});
#                     const double Mh2 = (2.0/3.0) * A * kmin[d]*kmin[d] * cellsize[d]*cellsize[d];
#                     slope[d] = minmodTVB(slopeDG[d], avgR - cellAvg[v], cellAvg[v] - avgL,  Mh2);
#                 }

#                 // overwrite the nodal values with the limited linear function
#                 for (int node = 0; node < {{NUMBER_OF_DOFS_PER_CELL}}; node++) {
#                     double value = cellAvg[v];
#                     for (int d = 0, rest = node; d < DIMENSIONS; d++, rest /= N) {
#                         value += slope[d] * (xi[rest % N] - 0.5);
#                     }
#                     Q[node * {{NUMBER_OF_DATA}} + v] = value;
#                 }
#             }
#         }


    

#     """)
# )


if use_modal_filter:
    solver.postprocess_updated_patch = modal_filter_kernel(
        order              = order,
        number_of_unknowns = sum(unknowns.values()),
        number_of_data     = sum(unknowns.values()) + sum(aux_var.values()),
        damping_rate       = 0.05,   # 1/s, e-folding rate of the highest mode
        filter_order       = 4,
        cutoff_mode        = None,   # N//2: lower modes untouched
        min_height         = None,   # e.g. 80e3 to filter only above 80 km
    )
    solver.add_includes("cmath")

if use_limiting:
    solver.add_subcell_limiter(
        exahype2.fv.MusclHancock(limiter=Limiter.mclim),
        number_of_dmp_observables = 0,
        dmp_relaxation_parameter  = 0.0,
        dmp_differences_scaling   = 1e-3,
    )


# the executable writes these into <outputPath>/run_config/ when a run starts
here = os.path.dirname(os.path.abspath(__file__))
embed_run_config(
    solver,
    name,
    source_files=[os.path.abspath(__file__), os.path.join(here, "atmosphereE_pertubations.py")],
    settings={
        "project": name,
        "size": size,
        "end_time": end_time,
        "order": order,
        "unknowns": unknowns,
        "auxiliary_variables": aux_var,
        "restart_from": restart_from,
        "restart_step": restart_step,
        "restart_time": restart_time,
    },
)

project.add_solver(solver)



project.generate_and_build()
