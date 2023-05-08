import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

dimensions            = 2
width                 = [10, 10]
offset                = [0, 0]
end_time              = 0.02
time_step_relaxation  = 0.5 # taken from tutorial
plot_interval         = 0.005 # taken from tutorial
patch_size            = 3
maximum_cell_size     = 0.1
amr_levels            = 3 
auxiliary_variables   = 1 # bathymetry

solvers = {
  #"FVRusanovGlobalFixed":exahype2.solvers.fv.rusanov.GlobalFixedTimeStep,
  #"FVRusanovGlobalFixedEnclave":exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
  "FVRusanovGlobalAdaptive":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
  "FVRusanovGlobalAdaptiveEnclave":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
  #"FVRusanovLocalEnclave":exahype2.solvers.fv.rusanov.LocalTimeStepWithEnclaveTasking,
}

build_modes = {
  "Release":peano4.output.CompileMode.Release,
  "Trace":peano4.output.CompileMode.Trace,
  "Assert":peano4.output.CompileMode.Asserts,
  "Stats":peano4.output.CompileMode.Stats,
  "Debug":peano4.output.CompileMode.Debug,
}

supported_file_types_bathymetry = [".nc"]
supported_file_types_displacement = [".nc"]

scenarios = ["Radial_Dam_Break", "Radial_Bathymetry_Dam_Break", "Tsunami"]

parser = argparse.ArgumentParser(description='ExaHyPE 2 - SWE Application Script')
parser.add_argument("-s",       "--solver",            dest="solver",         choices=solvers.keys(),     required=True,              help="|".join(solvers.keys()) )
parser.add_argument("-m",       "--build-mode",        dest="build_mode",     choices=build_modes.keys(), default="Release",          help="|".join(build_modes.keys()) )
parser.add_argument("-d",       "--dim",               dest="dim",            type=int,                   default=dimensions,         help="Number of space dimensions")
parser.add_argument("-ps",      "--patch-size",        dest="patch_size",     type=int,                   default=patch_size,         help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-maxh",    "--max-cell-size",     dest="max_cell_size",  type=float,                 default=maximum_cell_size,  help="Maximum size of a single finite volume cell on each axis")
parser.add_argument("-pi",      "--plot-interval",     dest="plot_interval",  type=float,                 default=plot_interval,      help="Time between plots")
parser.add_argument("-et",      "--end-time",          dest="end_time",       type=float,                 default=end_time,           help="End time of simulation")
parser.add_argument("-amr",     "--amr-levels",        dest="amr_levels",     type=int,                   default=amr_levels,         help="Number of AMR grid levels on top of max. finite volume cell (3 by default)" )
parser.add_argument(            "--dynamic-amr",       dest="dynamic_amr",    action="store_true",        default=False,              help="Use dynamic AMR" )
parser.add_argument("-gpu",     "--gpu",               dest="gpu",            action="store_true",        default=False,              help="Enable GPU support (stateless PDE terms). Requires a GPU build." )
parser.add_argument(            "--no-compile",        dest="compile",        action="store_false",       default=True,               help="Compile (on by default)" )
parser.add_argument("-c",       "--scenario",          dest="scenario",       choices=scenarios,          default="Radial_Dam_Break", help="|".join(scenarios))
parser.add_argument("-bf",       "--bathymetry-file",  dest="bathymetry_file", type=str,                   required=False,             help="Path to file containing bathymetry relative to the directory this script is located in. Allowed file extensions: %s" % ("|".join(supported_file_types_bathymetry)))
parser.add_argument("-df",       "--displacement-file",dest="displacement_file",type=str,                  required=False,             help="Path to file containing displacement relative to the directory this script is located in. Allowed file extensions: %s" % ("|".join(supported_file_types_displacement)))
args = parser.parse_args()

my_pde = None
if args.scenario == "Radial_Bathymetry_Dam_Break":
  from scenarios.radial_bathymetry_dam_break import Radial_Bathymetry_Dam_Break
  my_pde = Radial_Bathymetry_Dam_Break(dimensions=args.dim, auxiliary_variables=auxiliary_variables, width=width)

elif args.scenario == "Tsunami":
  # check whether flags are set
  if args.bathymetry_file is None: raise Exception("No bathymetry file supplied for the tsunami scenario!")
  if args.displacement_file is None: raise Exception("No displacement file supplied for the tsunami scenario!")
  
  # check whether files exist in the first place
  if not os.path.exists(args.bathymetry_file): raise FileNotFoundError()
  if not os.path.exists(args.displacement_file): raise FileNotFoundError()
  
  # check for supported file extension
  bath_ext = os.path.splitext(args.bathymetry_file)[1]
  displ_ext = os.path.splitext(args.displacement_file)[1] 
  if bath_ext not in supported_file_types_bathymetry: raise Exception("Unsupported file type of bathymetry file! Supported extensions are: %s" % ("|".join(supported_file_types_bathymetry)))
  if displ_ext not in supported_file_types_displacement: raise Exception("Unsupported file type of displacement file! Supported extensions are: %s" % ("|".join(supported_file_types_displacement)))

  from scenarios.tsunami import Tsunami
  my_pde = Tsunami(dimensions=args.dim, auxiliary_variables=auxiliary_variables)

else:
  from scenarios.radial_dam_break import Radial_Dam_Break
  my_pde = Radial_Dam_Break(dimensions=args.dim, auxiliary_variables=auxiliary_variables, width=width)

my_pde.substitute_expression(my_pde.g, 9.81)

solver_type = args.solver
if args.gpu:
  if not "Enclave" in solver_type:
    solver_type += "Enclave"
solver_name = "SWESolver" + str(solver_type)
if args.gpu:
  solver_name += "GPU"
my_solver = solvers[solver_type](
  name                      = solver_name,
  patch_size                = args.patch_size,
  unknowns                  = args.dim+1,
  auxiliary_variables       = auxiliary_variables,
  max_volume_h              = args.max_cell_size, # TODO must be at least as fine as bathymetry file
  min_volume_h              = args.max_cell_size * 3.0**(-args.amr_levels),
  time_step_relaxation      = time_step_relaxation
)

if args.gpu:
  my_solver.set_pde_terms_without_state(True)

my_solver.add_solver_constants("static constexpr bool useDynamicAMR = {};".format("true" if args.dynamic_amr else "false"))
if args.scenario == "Tsunami":
  my_solver.add_user_solver_includes("#include \"parser/TopologyParser.h\"")
  my_solver.add_solver_constants("applications::exahype2::swe::parser::TopologyParser parser = applications::exahype2::swe::parser::TopologyParser(\"%s\", \"%s\");" % (args.bathymetry_file, args.displacement_file))
my_solver.set_implementation(
  initial_conditions    = my_pde.implementation_of_initial_conditions(),
  boundary_conditions   = my_pde.implementation_of_boundary_conditions(),
  flux                  = my_pde.implementation_of_flux(),
  eigenvalues           = my_pde.implementation_of_max_eigenvalue(),
  ncp                   = my_pde.implementation_of_ncp(),
  refinement_criterion  = my_pde.implementation_of_refinement_criterion(),
  source_term           = my_pde.implementation_of_sources()
)

my_project = exahype2.Project( ["applications", "exahype2", "swe"], "SWEApplication", ".", executable="swe-application-" + str(args.dim) + "d-patch-size-" + str(args.patch_size) + "-cell-size-" + str(args.max_cell_size))
my_project.add_solver( my_solver )
my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_Peano4_installation("../../../", build_modes[args.build_mode])

my_project.set_global_simulation_parameters(
  dimensions            = args.dim,
  size                  = width,
  offset                = offset,
  min_end_time          = args.end_time,
  max_end_time          = args.end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = args.plot_interval,
  periodic_BC           = [False, False, False]
)

peano4_project = my_project.generate_Peano4_project(verbose=True)
if args.scenario == "Tsunami": 
  peano4_project.output.makefile.add_cpp_file("parser/TopologyParser.cpp")
  peano4_project.output.makefile.add_cpp_file("parser/NetCDFReader.cpp")
peano4_project.generate()
if(args.compile):
  peano4_project.build(make_clean_first=True, throw_away_data_after_build=True)
