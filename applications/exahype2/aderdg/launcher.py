import peano4, exahype2
import os, sys
import argparse
import subprocess

sys.path.insert(0, os.path.abspath("./Scenarios"))
import Scenarios

modes = {
    "release": peano4.output.CompileMode.Release,
    "trace": peano4.output.CompileMode.Trace,
    "assert": peano4.output.CompileMode.Asserts,
    "stats": peano4.output.CompileMode.Stats,
    "debug": peano4.output.CompileMode.Debug,
}

available_scenarios = {
    "acoustic_planar_waves":  Scenarios.Acoustic_planar_waves(dimensions=2),
    "advection_linear":       Scenarios.Advection_linear(),
    "elastic_planar_waves":   Scenarios.Elastic_planar_waves(dimensions=2),
    "euler_gaussian_bell":    Scenarios.Euler_gaussian_bell(),
    "euler_isotropic_vortex": Scenarios.Euler_isotropic_vortex(),
    "swe_radial_dam_break":   Scenarios.SWE_radial_dam_break(),
    "swe_resting_lake":       Scenarios.SWE_resting_lake()
}

parser = argparse.ArgumentParser(description="ExaHyPE 2 - ADER testing script")

parser.add_argument("-md",    "--mesh-depth",       dest="md",                      type=int,     default=3,          help="Depth of coarsest mesh level, i.e if 2 is specified there will be 9 cells per dimension")
parser.add_argument("-amr",   "--adaptive-levels",  dest="adaptivity_levels",       type=int,     default=0,          help="Number of AMR grid levels on top of hmax (0 by default)",)
parser.add_argument("-o",     "--order",            dest="order",                   type=int,     default=3,          help="DG Order")
parser.add_argument("-p",     "--p",                dest="polynomials",             type=int,     default=0,          help="Polynomial type, 0 is gauss-legendre, 1 is gauss-lobatto",)
parser.add_argument("-m",     "--mode",             dest="mode",                                  default="release",  help="|".join(modes.keys()))
parser.add_argument("-s",     "--scenario",         dest="s",                                     default=None,       help="|".join(available_scenarios.keys()))
parser.add_argument("-v",     "--visualise",        action="store_true",                          default=False,      help="Visualise the output data using the postprocessor.",)

args = parser.parse_args()

if args.s is None:
  while True:
      try:
          s = input( "Which of the following scenarios would you like to try out?\n" + " - ".join(available_scenarios.keys()) + "\n" )
          scenario = available_scenarios[s]
      except KeyError:
          print("i'm afraid i can't do that Dave")
          continue
      else:
          #User has specified a valid scenario
          break
else:
  scenario = available_scenarios[args.s]

if args.visualise:
    output_patch_file = f"solutions/solution-{scenario.__class__.__name__}.peano-patch-file"
    if os.path.isfile(output_patch_file):
        subprocess.call(
            [
                "python3",
                f"../../../python/peano4/visualisation/render.py",
                str(output_patch_file),
            ]
        )
    sys.exit(0)

order = args.order

max_h = 1.1 * scenario._domain_size / (3.0**args.md)
min_h = max_h * 3.0 ** (-args.adaptivity_levels)

polynomials = (
  exahype2.solvers.aderdg.Polynomials.Gauss_Legendre if args.polynomials==0
  else exahype2.solvers.aderdg.Polynomials.Gauss_Lobatto
)

project = exahype2.Project(
  ["exahype2", "aderdg", scenario._equation.__class__.__name__], ".", executable=scenario.__class__.__name__.upper()
)

theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name  = scenario.__class__.__name__, order = order,
  min_cell_h = min_h, max_cell_h = max_h,
  time_step_relaxation  = 0.9,
  unknowns              = scenario._equation.num_unknowns,
  auxiliary_variables   = scenario._equation.num_auxiliary_variables,
  initial_conditions    = scenario.initial_conditions(),
  boundary_conditions   = scenario.boundary_conditions(),
  eigenvalues = scenario._equation.eigenvalues(),
  flux        = scenario._equation.flux(),
  ncp         = scenario._equation.ncp()
)

theSolver.add_kernel_optimisations(polynomials = polynomials, is_linear=scenario._equation.is_linear)

if(scenario.analytical_solution()!=exahype2.solvers.PDETerms.None_Implementation):
    exahype2.solvers.aderdg.ErrorMeasurement(
      theSolver,
      error_measurement_implementation = scenario.analytical_solution(),
      output_file_name = "Error_" + scenario.__class__.__name__
    )

if(scenario._plot_dt>0.0):
  project.set_output_path("solutions")

project.add_solver(theSolver)


scenario.set_global_simulation_parameters(project)

project.set_load_balancing(
  "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates",
  "new ::exahype2::LoadBalancingConfiguration(0.98)"
)
project.set_Peano4_installation("../../../", modes[args.mode])
peano4_project = project.generate_Peano4_project("False")
peano4_project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=16)
