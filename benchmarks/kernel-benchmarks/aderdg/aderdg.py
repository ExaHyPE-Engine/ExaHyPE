# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import sys, os, argparse
import peano4, exahype2

sys.path.insert(0, os.path.abspath("../../../tests/aderdg"))
import scenarios


parser = exahype2.ArgumentParser("ExaHyPE 2 ADER-DG Kernel Benchmarking Script")
parser.set_defaults(
    min_depth=3,
    degrees_of_freedom=6,
)

parser.add_argument(
    "-s",
    "--samples",
    type=int,
    default=0,
    help="Set number of samples for benchmarking, if not set, run until scenario end",
)
parser.add_argument(
    "-pr",
    "--precision",
    choices=["double", "float", "fp16", "bf16"],
    default="double",
    help="Precision in which the solver should be computed.",
)
parser.add_argument(
    "-st_pr",
    "--storage_precision",
    choices=["double", "float", "fp16", "bf16"],
    default=None,
    help="Precision in which the solution should be stored, supersedes the precision argument.",
)
parser.add_argument(
    "-pr_pr",
    "--predictor_precision",
    choices=["double", "float", "fp16", "bf16"],
    default=None,
    help="Precision in which the predictor should be computed, supersedes the precision argument.",
)
parser.add_argument(
    "-cr_pr",
    "--corrector_precision",
    choices=["double", "float", "fp16", "bf16"],
    default=None,
    help="Precision in which the corrector should be computed, supersedes the precision argument.",
)

args = parser.parse_args()

scenario = scenarios.ElasticPlanarWaves(dimensions=args.dimensions)

boundary_conditions = """
 // Lamé parameters
 constexpr double mu     =  4.0;
 constexpr double lambda = -4.0;

 // Constant auxiliary parameters
 constexpr double rho  = 1.0;
 constexpr double cp   = 2.0;
 constexpr double cs   = 2.0;

 constexpr double w = -2*M_PI;

 const double val_x = cos( w*t -std::numbers::pi*x[0]);
 const double val_y = cos( w*t -std::numbers::pi*x[1]);
#if DIMENSIONS==2
  // Stresses
  Qoutside[2] = val_x*(lambda+2*mu)  + val_y*lambda;         //xx
  Qoutside[3] = val_x*lambda         + val_y*(lambda+2*mu);  //yy
  Qoutside[4] = val_x*mu + val_y*mu;                         //xy

  // Velocities
  Qoutside[0] = val_x*cp + val_y*cs; //vx
  Qoutside[1] = val_y*cp + val_x*cs; //vy

  Qoutside[5] = rho;
  Qoutside[6] = cp;
  Qoutside[7] = cs;
#else
  const double val_z = cos( w*t -std::numbers::pi*x[2]);

  // Stresses
  Qoutside[3] = val_x*(lambda+2*mu)  + val_y*lambda        + val_z*lambda;         //xx
  Qoutside[4] = val_x*lambda         + val_y*(lambda+2*mu) + val_z*lambda;         //yy
  Qoutside[5] = val_x*lambda         + val_y*lambda        + val_z*(lambda+2*mu);  //zz
  Qoutside[6] = val_x*mu; //xy
  Qoutside[7] = val_z*mu; //xz
  Qoutside[8] = val_y*mu; //yz

  // Velocities
  Qoutside[0] = val_x*cp + val_z*cs; //vx
  Qoutside[1] = val_y*cp + val_x*cs; //vy
  Qoutside[2] = val_z*cp + val_y*cs; //vz

  Qoutside[9] = rho;
  Qoutside[10] = cp;
  Qoutside[11] = cs;
#endif
"""

ader_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="AderSolver",
    order=args.degrees_of_freedom - 1,
    unknowns=scenario._equation.num_unknowns,
    auxiliary_variables=scenario._equation.num_auxiliary_variables,
    min_cell_h=0.001,  # max_cell_size -> arbitrary value
    max_cell_h=0.001,  # min_cell_size -> arbitrary value
    time_step_relaxation=0.5,
)

ader_solver.add_kernel_optimisations(
    polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre,
    is_linear=scenario._equation.is_linear,
    predictor_computation_precisions=[
        (
            args.predictor_precision
            if args.predictor_precision is not None
            else args.precision
        )
    ],
    corrector_computation_precision=(
        args.corrector_precision
        if args.corrector_precision is not None
        else args.precision
    ),
    solution_persistent_storage_precision=(
        args.storage_precision if args.storage_precision is not None else args.precision
    ),
)

ader_solver.set_implementation(
    initial_conditions=scenario.initial_conditions(),
    boundary_conditions=boundary_conditions,
    max_eigenvalue=scenario._equation.eigenvalues(),
    flux=scenario._equation.flux(),
    ncp=scenario._equation.ncp(),
    riemann_solver=scenario._equation.riemann_solver(),
)

project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "kernelbenchmarks"],
    project_name="KernelBenchmarks",
    directory=".",
    executable="KernelBenchmarks",
)
project.add_solver(ader_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = scenario._end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=scenario._dimensions,
    size=[scenario._domain_size, scenario._domain_size, scenario._domain_size][
        0 : scenario._dimensions
    ],
    offset=[scenario._offset, scenario._offset, scenario._offset][
        0 : scenario._dimensions
    ],
    min_end_time=scenario._end_time,
    max_end_time=scenario._end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[False, False, False][0 : scenario._dimensions],
)

project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)

project.constants.export_constexpr_with_type(
    "GridLength", str(3**args.min_depth), "int"
)

project.constants.export_constexpr_with_type(
    "ShouldPlot", "true" if args.number_of_snapshots > 0 else "false", "bool"
)

project.constants.export_constexpr_with_type(
    "UseAMR", "true" if args.amr_levels > 0 else "false", "bool"
)

project.constants.export_constexpr_with_type("Samples", str(args.samples), "int")

makefile = project.output.makefile
makefile.add_cpp_file("KernelBenchmarks-main.cpp")

makefile.add_h_file("DataRepository.h")
makefile.add_cpp_file("DataRepository.cpp")

makefile.add_h_file("EnclaveDataRepository.h")
makefile.add_cpp_file("EnclaveDataRepository.cpp")

makefile.add_h_file("EnclaveTasks.h")
makefile.add_cpp_file("EnclaveTasks.cpp")

makefile.add_CXX_flag(
    "-DSolverStoragePrecision=" + ader_solver._solution_persistent_storage_precision
)
makefile.add_CXX_flag(
    "-DSolverPredictorPrecision=" + ader_solver._predictor_computation_precisions[0]
)
makefile.add_CXX_flag(
    "-DSolverCorrectorPrecision=" + ader_solver._corrector_computation_precision
)

print(ader_solver._predictor_computation_precisions[0])

project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
