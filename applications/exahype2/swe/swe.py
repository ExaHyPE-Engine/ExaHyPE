import os
import sys
import argparse
import re

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2
from exahype2.solvers.PDETerms import PDETerms

dimensions            = 2 # this is fixed to 2 in pde.py
width                 = [10, 10]
offset                = [0, 0] # this is fixed to [0, 0]
end_time              = 0.02
time_step_relaxation  = 0.4
plot_interval         = 0.005 # taken from tutorial
patch_size            = 4 # 32
maximum_cell_size     = 0.1
amr_levels            = 3 
auxiliary_variables   = 1 # bathymetry, this is fixed to 1 in pde.py
amr_tolerance         = 1e-3
epsilon               = 1e-2

fv_rusanov_solvers = {
  #"FVRusanovGlobalFixed":exahype2.solvers.fv.rusanov.GlobalFixedTimeStep,
  #"FVRusanovGlobalFixedEnclave":exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
  "FVRusanovGlobalAdaptive":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
  "FVRusanovGlobalAdaptiveEnclave":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
  #"FVRusanovLocalEnclave":exahype2.solvers.fv.rusanov.LocalTimeStepWithEnclaveTasking,
}

rkdg_rusanov_solvers = {
  "RKDGRusanovGlobalAdaptive": exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep
}

user_defined_solvers = {
  "FVUserDefined" : exahype2.solvers.fv.riemann.UserDefinedRiemann
}

custom_solver_implementations = [
  "RusanovTsunami",
  "HLLEM",
  "FWave"
]

solvers = {}
solvers.update(rkdg_rusanov_solvers)
solvers.update(fv_rusanov_solvers)
solvers.update(user_defined_solvers)

build_modes = {
  "Release":peano4.output.CompileMode.Release,
  "Trace":peano4.output.CompileMode.Trace,
  "Assert":peano4.output.CompileMode.Asserts,
  "Stats":peano4.output.CompileMode.Stats,
  "Debug":peano4.output.CompileMode.Debug,
}

supported_file_types = [".nc"]

scenarios = [
  "Radial_Dam_Break", 
  "Radial_Bathymetry_Dam_Break", 
  "Radial_Obstacle_Dam_Break",
  "Tsunami", 
  "Artificial_Tsunami",
  "Sea_At_Rest",
  "Oscillating_Lake",
  "Three_Mounds_Channel",
  "Circular_Island"
  ]

coordinate_filter = "((-?[0-9]+\.?[0-9]*(e-?[0-9]+)?)|(inf))"
def parse_coordinate_pair(coords):
  if re.search("^\[%s\\s*\,\\s*%s\]$" % (coordinate_filter, coordinate_filter), coords):
      splits = re.split("[\[\]\,]", coords)
      return [float(splits[1]), float(splits[2])]
  else:
    raise ValueError("Could not parse the illegal coordinates: {}!".format(coords)) 


parser = argparse.ArgumentParser(description='ExaHyPE 2 - SWE Application Script')

parser.add_argument("-s",       "--solver",            dest="solver",         choices=solvers.keys(),     required=True               )
parser.add_argument("-o",       "--output",            dest="output_path",    type=str,                   default="./",               help="Output path for project output. The project will create a new folder at the given path. Default is the current directory.")
parser.add_argument("-m",       "--build-mode",        dest="build_mode",     choices=build_modes.keys(), default="Release",          help="|".join(build_modes.keys()) )

parser.add_argument("-et",      "--end-time",          dest="end_time",       type=float,                 default=end_time,           help="End time of simulation. Default is %f." % end_time)
parser.add_argument("-bf",      "--bathymetry-file",   dest="bathymetry_file", type=str,                  required=False,             help="Path to file containing bathymetry relative to the directory this script is located in. Allowed file extensions: %s" % ("|".join(supported_file_types)))
parser.add_argument("-df",      "--displacement-file", dest="displacement_file",type=str,                 required=False,             help="Path to file containing displacement relative to the directory this script is located in. Allowed file extensions: %s" % ("|".join(supported_file_types)))
parser.add_argument("-sc",      "--scenario",          dest="scenario",       choices=scenarios,          default="Radial_Dam_Break", help="Default is Radial_Dam_Break.")
parser.add_argument("-ps",      "--patch-size",        dest="patch_size",     type=int,                   default=patch_size,         help="Number of finite volumes per axis (dimension) per patch. Default is %f." % patch_size)
parser.add_argument("-pi",      "--plot-interval",     dest="plot_interval",  type=float,                 default=plot_interval,      help="Time in seconds between plots. Default is %f." % plot_interval)
parser.add_argument("-pri",     "--probe-interval",    dest="probe_interval", type=float,                 default=plot_interval,      help="Time in seconds between database entries when probes are used. Default is %f." % plot_interval)
parser.add_argument("-cs",      "--custom-solver",     dest="custom_solver",  choices=custom_solver_implementations,required=False,   help="Custom solver to be used as user defined solver. Has to be set if --solver has been set to some user defined option.")
parser.add_argument("-wd",      "--width",             dest="width",          type=str,                   required=False,             help="Specify size of domain in meters as [x, y] as string (e.g. [10, 10], [7e6, 4e6]). Needed to guarantee physicality of tsunami simulations. Note that some scenarios have a fixed width.")

parser.add_argument("-amr",     "--amr-levels",        dest="amr_levels",     type=int,                   default=amr_levels,         help="Number of AMR grid levels on top of max. finite volume cell Default is %f." % amr_levels)
parser.add_argument("-gpu",     "--gpu",               dest="gpu",            action="store_true",        default=False,              help="Enable GPU support (stateless PDE terms). Requires a GPU build." )

parser.add_argument("-maxd",    "--max_dimension_netcdf",dest="max_dimension_netcdf",type=int,            required=False,             help="Use when netCDF4 python package not installed to pass maximum dimension to the program.")
parser.add_argument("-maxh",    "--max-cell-size",     dest="max_cell_size",  type=float,                 default=maximum_cell_size,  help="Maximum size in meters of a single finite volume cell on each axis.")

parser.add_argument("-probes",  nargs='+',             dest="probe_coordinates",type=str,                 required=False,             help="Provide one or more tuples of coordinates [x, y] for probes (e.g. [15, 20.23] [5, 14.123]). Coordinates have to be in reference to the simulation domain.")

parser.add_argument("-rk-order","--rk-order",          dest="rk_order",       type=int,                   required=False,             help="Set order of time discretisation for Runge-Kutta scheme." )
parser.add_argument("-dg-order","--dg-order",          dest="dg_order",       type=int,                   required=False,             help="Set order of space discretisation for Discontinuous Galerkin." )
parser.add_argument("-amr-tol", "--amr-tolerance",     dest="amr_tol",        type=float,                 default=amr_tolerance,      help="When using AMR, please set a surface tolerance above which shall be refined and otherwise coarsened. Default is %f. Also used for guided AMR." % amr_tolerance)
parser.add_argument("-epsilon", "--epsilon",           dest="epsilon",        type=float,                 default=epsilon,            help="Can be used in the custom solver for something. Default is %f." % epsilon)

parser.add_argument("-nc-key-bath-x",                  dest="nc_key_bath_x",  type=str,                   default="x",                help="Key for x dimension values in bathymetry netCDF file. Default is x.")
parser.add_argument("-nc-key-bath-y",                  dest="nc_key_bath_y",  type=str,                   default="y",                help="Key for y dimension values in bathymetry netCDF file. Default is y.")
parser.add_argument("-nc-key-bath-bath",               dest="nc_key_bath_bath",type=str,                  default="z",                help="Key for bathymetry values in bathymetry netCDF file. Default is z.")
parser.add_argument("-nc-key-displ-x",                 dest="nc_key_displ_x",  type=str,                  default="x",                help="Key for x dimension values in displacement netCDF file. Default is x.")
parser.add_argument("-nc-key-displ-y",                 dest="nc_key_displ_y",  type=str,                  default="y",                help="Key for y dimension values in displacement netCDF file. Default is y.")
parser.add_argument("-nc-key-displ-displ",             dest="nc_key_displ_displ",type=str,                default="z",                help="Key for displacement values in displacement netCDF file. Default is z.")

parser.add_argument(            "--no-compile",        dest="compile",        action="store_false",       default=True,               help="Compile. Default is True.")
parser.add_argument(            "--uq-displacement-origin",dest="uq_displacement_origin",type=str,        required=False,             help="Move displacement for uncertainty quantification. Expects argument [x, y] denoting the center of the displacement file within the coordinate system of the bathymetry file. To move only one dimension, set the other one to inf.")
parser.add_argument(            "--dynamic-h",         dest="dynamic_h",      action="store_true",        default=False,              help="Dynamically adapt max cell size for highest netCDF resolution. Default is False.")

parser.add_argument(            "--guided-amr",        dest="guided_amr",      action="store_true",       default=False,              help="Use guided AMR, only available with the \"Tsunami\" scenario. Optionally pass in an already computed adjoint simulation file in netCDF format using the flag --gamr-file.")
parser.add_argument(            "--gamr-file",          dest="gamr_file",      type=str,                  required=False,             help="Pass in an already computed adjoint simulation output file in netCDF format. Expects to have keys \"x\",\"y\" and \"h\", \"hu\", \"hv\", \"b\".")
parser.add_argument(            "--gamr-start-time",    dest="gamr_start_time",type=float,                required=False,             help="Begin of temporal region of interest. Default is 0.")
parser.add_argument(            "--gamr-checkpoints",   dest="gamr_checkpoints",type=int,                 default=0,                  help="Number of times output should be written in adjoint simulation. Default is 0 (=Infinity)")
parser.add_argument("--gamr-area-of-interest", nargs="+",dest="gamr_area_of_interest",type=str,           required=False,             help="Define area of interest for guided AMR. Pass in either a square [x1, x2] [y1, y2] or a circle [x1, y1] [r, inf]. Coordinates are in the same system as the bathymetry.")
parser.add_argument(            "--gamr-writer-flush",  dest="gamr_writer_flush", type=int,               default=0,                  help="Define after how many writer operations the writer flushes the buffer to the disk.")
parser.add_argument(            "--adjoint-only",       dest="adjoint_only",    action="store_true",      default=False,              help="Set to only run adjoint simulation and no forward simulation. Used to simply generate file for guided amr.")
args = parser.parse_args()

maximum_cell_size = args.max_cell_size
patch_size = args.patch_size if args.patch_size != None else patch_size
if not args.output_path.endswith("/"):
  args.output_path += "/"

if args.width is not None:
  width = parse_coordinate_pair(args.width)

if args.probe_coordinates is not None:
  probe_interval = args.probe_interval

if args.solver in user_defined_solvers and args.custom_solver is None:
  raise Exception("No custom solver implementation supplied for user defined solver! Please set -cs flag to one of the following values: " + "|".join(custom_solver_implementations) + ".")

#########################################
# Set up scenario to use                #
#########################################  

my_pde = None
if args.scenario == "Radial_Dam_Break":
  # This is the default scenario
  width[1] = width[0]
  from scenarios.radial_dam_break import Radial_Dam_Break
  my_pde = Radial_Dam_Break(width=width, refinement_tolerance=args.amr_tol)

elif args.scenario == "Radial_Bathymetry_Dam_Break":
  width[1] = width[0]
  from scenarios.radial_bathymetry_dam_break import Radial_Bathymetry_Dam_Break
  my_pde = Radial_Bathymetry_Dam_Break(width=width, refinement_tolerance=args.amr_tol)

elif args.scenario == "Radial_Obstacle_Dam_Break":
  width[1] = width[0]
  from scenarios.radial_obstacle_dam_break import Radial_Obstacle_Dam_Break
  my_pde = Radial_Obstacle_Dam_Break(width=width, refinement_tolerance=args.amr_tol)

elif args.scenario == "Artificial_Tsunami":
  width = [10e3,10e3]
  from scenarios.artificial_tsunami import Artificial_Tsunami
  my_pde = Artificial_Tsunami(width=width, refinement_tolerance=args.amr_tol)

elif args.scenario == "Sea_At_Rest":
  from scenarios.sea_at_rest import Sea_At_Rest
  my_pde = Sea_At_Rest(width=width, refinement_tolerance=args.amr_tol)

elif args.scenario == "Oscillating_Lake":
  width = [4, 4]
  from scenarios.oscillating_lake import Oscillating_Lake
  my_pde = Oscillating_Lake(refinement_tolerance=args.amr_tol)

elif args.scenario == "Three_Mounds_Channel":
  width = [75, 30]
  from scenarios.three_mounds_channel import Three_Mounds_Channel
  my_pde = Three_Mounds_Channel(refinement_tolerance=args.amr_tol)

elif args.scenario == "Circular_Island":
  width = [25, 30]
  from scenarios.circular_island import Circular_Island
  my_pde = Circular_Island(refinement_tolerance=args.amr_tol)

elif args.scenario == "Tsunami":

  # check whether flags are set
  if args.bathymetry_file is None: raise Exception("No bathymetry file supplied for the tsunami scenario!")
  if args.displacement_file is None: raise Exception("No displacement file supplied for the tsunami scenario!")
  
  # check whether files exist in the first place
  if not os.path.exists(args.bathymetry_file): raise FileNotFoundError()
  if not os.path.exists(args.displacement_file): raise FileNotFoundError()
  if args.gamr_file is not None and not os.path.exists(args.gamr_file): raise FileNotFoundError() 
  
  # check for supported file extension
  bath_ext = os.path.splitext(args.bathymetry_file)[1]
  displ_ext = os.path.splitext(args.displacement_file)[1] 
  if bath_ext not in supported_file_types: raise Exception("Unsupported file type of bathymetry file! Supported extensions are: %s" % ("|".join(supported_file_types)))
  if displ_ext not in supported_file_types: raise Exception("Unsupported file type of displacement file! Supported extensions are: %s" % ("|".join(supported_file_types)))
  if args.gamr_file is not None and os.path.splitext(args.gamr_file)[1] not in supported_file_types: raise Exception("Unsupported file type of guided amr file! Supported extensions are: %s" % ("|".join(supported_file_types)))
  
  if args.amr_levels > 0 and args.dynamic_h:
    print("Overwriting max_cell_size to fit finest mesh in data files!")
    try:
      import netCDF4
      for file_path in [args.bathymetry_file, args.displacement_file]:
        nc_file = netCDF4.Dataset(file_path)
        # optimize cell size to exactly fit finest mesh width in data
        maximum_cell_size = min([min([w/d for (d,w) in zip([len(d) for d in nc_file.dimensions.values()], width)]), maximum_cell_size])
        nc_file.close()
    except ImportError:
      print("netCDF4 package not found. Checking -maxd flag.")
      if args.max_dimension_netcdf is None or args.max_dimension_netcdf <= 0: 
        raise ValueError("Neither netCDF4 package installed nor valid value passed to -maxd!") 
      else: 
        # generally estimates finer mesh width than finest mesh data width
        maximum_cell_size = min(width)/args.max_dimension_netcdf

  from scenarios.tsunami import Tsunami
  my_pde = Tsunami(refinement_tolerance=args.amr_tol, use_guided_amr=args.guided_amr)

my_pde.substitute_expression(my_pde.g, 9.81)

#########################################
# Set up solver for GPU                 #
#########################################  

solver_type = args.solver
if args.gpu:
  if not "Enclave" in solver_type:
    solver_type += "Enclave"
    if solver_type not in solvers:
      # This is necessary as "-s RKDGRusanovGlobalAdaptive -gpu" for example currently leads to this scenario
      raise ValueError("The requested solver {} is not available for GPUs!".format(solver_type))
    
solver_name = "SWESolver" + str(solver_type) + (args.custom_solver if solver_type in user_defined_solvers else "")
if args.gpu:
  solver_name += "GPU"

#########################################
# Set up solver                         #
#########################################  

my_solver = None
# finite volume rusanov solver?
if solver_type in fv_rusanov_solvers or solver_type == "FVUserDefined":

#########################################
# Set up solver parameters for FV       #
#########################################  

  my_solver = solvers[solver_type](
    name                      = solver_name,
    patch_size                = patch_size,
    unknowns                  = 3,
    auxiliary_variables       = 1,
    max_volume_h              = maximum_cell_size,
    min_volume_h              = maximum_cell_size * 3.0**(-args.amr_levels),
    time_step_relaxation      = time_step_relaxation
  )

# rkdg rusanov solver?
elif solver_type in rkdg_rusanov_solvers:
  # necessary flags set?
  if args.dg_order is None or args.rk_order is None:
    raise ValueError("When using RKDG, both the -dg-order and -rk-order have to be set!")
  
  # add flags to solver name for easier distinguishing between executables
  solver_name += "RK%dDG%d" % (args.rk_order, args.dg_order)
  
  from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections

#########################################
# Set up solver parameters for RKDG     #
#########################################  

  my_solver = solvers[solver_type](
    name                      = solver_name,
    rk_order                  = args.rk_order,
    polynomials               = exahype2.solvers.GaussLegendreBasis(args.dg_order),
    face_projections          = FaceProjections.Solution, # TODO according to python/exahype2/solvers/rkdg/RungeKuttaDG.py, this should be == 2? 
    unknowns                  = 3,
    auxiliary_variables       = 1,
    max_cell_h                = maximum_cell_size, 
    min_cell_h                = maximum_cell_size * 3.0**(-args.amr_levels),
    time_step_relaxation      = time_step_relaxation
  )

if args.gpu:
  my_solver.set_pde_terms_without_state(True)

#########################################
# Set up solver implementation          #
#########################################

if solver_type not in user_defined_solvers:
  my_solver.set_implementation(
    initial_conditions    = my_pde.implementation_of_initial_conditions(),
    boundary_conditions   = my_pde.implementation_of_boundary_conditions(),
    flux                  = my_pde.implementation_of_flux(),
    eigenvalues           = my_pde.implementation_of_max_eigenvalue(),
    ncp                   = my_pde.implementation_of_ncp(),
    refinement_criterion  = my_pde.implementation_of_refinement_criterion(),
    source_term           = my_pde.implementation_of_sources()
  )

else:
    
#########################################
# Set up custom solver                  #
#########################################

  custom_solver = None
  if args.custom_solver == "RusanovTsunami":
    from custom_solvers.custom_fv_solvers.rusanov_tsunami import RusanovTsunami
    custom_solver = RusanovTsunami(args.epsilon)
  
  elif args.custom_solver == "HLLEM":
    from custom_solvers.custom_fv_solvers.hllem import HLLEM
    custom_solver = HLLEM(args.epsilon)

  elif args.custom_solver == "FWave":
    from custom_solvers.custom_fv_solvers.fwave import FWave
    custom_solver = FWave(args.epsilon)

  implementation_of_flux = my_pde.implementation_of_flux() if custom_solver.implementation_of_flux() == PDETerms.None_Implementation else custom_solver.implementation_of_flux()
  implementation_of_eigenvalues = my_pde.implementation_of_eigenvalues() if custom_solver.implementation_of_eigenvalues() == PDETerms.None_Implementation else custom_solver.implementation_of_eigenvalues()

  my_solver.set_implementation(
    initial_conditions    = my_pde.implementation_of_initial_conditions(),
    boundary_conditions   = my_pde.implementation_of_boundary_conditions(),
    flux                  = implementation_of_flux,
    eigenvalues           = implementation_of_eigenvalues,
    refinement_criterion  = my_pde.implementation_of_refinement_criterion(),
    riemann_solver        = custom_solver.implementation_of_riemann_solver()
  )

executable_name = "swe-application-2d-solver-name-%s-scenario-%s-patch-size-%d-amr-%d-cell-size-%f-end-time-%f" % (solver_name, args.scenario, patch_size, args.amr_levels, maximum_cell_size, args.end_time)
output_path = args.output_path+"output-%s/" % executable_name

#########################################
# Add solver constants if necessary     #
#########################################

if args.scenario == "Artificial_Tsunami" or args.scenario == "Circular_Island":
  my_solver.add_user_solver_includes("""
                                     #include <cmath>
                                     """)

if args.scenario == "Tsunami":

  # Use guided AMR?
  if args.guided_amr:
    # Is no file provided?
    if args.gamr_file is None:
      
      #########################################
      # Add adjoint problem simulation        #
      #########################################
      
      my_solver.add_user_solver_includes("""
                                         #include \"adjoint/cpp/SWEAdjoint.hpp\"
                                         """)
      
      if args.gamr_area_of_interest is None:
        raise Exception("No file or area of interest specified for guided amr!")
      
      if args.gamr_start_time is None:
        print("No start time for temporal area of interest specified! Default to 0!")
        args.gamr_start_time = 0

      area_of_interest = []
      for coords in args.gamr_area_of_interest:
        area_of_interest.append(parse_coordinate_pair(coords))

      adjoint_output_path = "\"%s/%s-adjoint.nc\"" % (output_path, executable_name) # output file name
      swe_adjoint_parameters = adjoint_output_path
      swe_adjoint_parameters += ",\"%s\"" % args.bathymetry_file # bathymetry file

      swe_adjoint_parameters += ", %f" % width[0] # width x
      swe_adjoint_parameters += ", %f" % width[1] # width y
      swe_adjoint_parameters += ", %f" % maximum_cell_size # cell size
      swe_adjoint_parameters += ", %f" % args.end_time # end time

      swe_adjoint_parameters += ", %f" % area_of_interest[0][0]
      swe_adjoint_parameters += ", %f" % area_of_interest[0][1]
      swe_adjoint_parameters += ", %f" % area_of_interest[1][0]
      swe_adjoint_parameters += (", %f" % area_of_interest[1][1]) if area_of_interest[1][1] != float('inf') else ", INFINITY"

      swe_adjoint_parameters += ", %f" % args.gamr_start_time # start time
      swe_adjoint_parameters += ", %d" % args.gamr_checkpoints # checkpoints
      swe_adjoint_parameters += ", %d" % args.gamr_writer_flush # flush

      swe_adjoint_parameters += ",\"%s\"" % args.nc_key_bath_x
      swe_adjoint_parameters += ",\"%s\"" % args.nc_key_bath_y
      swe_adjoint_parameters += ",\"%s\"" % args.nc_key_bath_bath
      
      my_solver.add_solver_constants("applications::exahype2::swe::adjoint::SWEAdjoint swe_adjoint = applications::exahype2::swe::adjoint::SWEAdjoint(%s);" % swe_adjoint_parameters)

    #########################################
    # Add adjoint parser                    #
    #########################################
    
    my_solver.add_user_solver_includes("""
                                      #include \"parser/AdjointParser.h\"
                                      """)
    adjoint_parser_parameters = ""
    if args.gamr_file is not None:
      adjoint_parser_parameters = "\"%s\"" % args.gamr_file
      if args.gamr_start_time is not None:
        adjoint_parser_parameters += ", %f" % args.gamr_start_time
      else:
        print("No start time for temporal area of interest specified! Default to reading from provided file!")
    else:
      adjoint_parser_parameters = adjoint_output_path
    my_solver.add_solver_constants("applications::exahype2::swe::parser::AdjointParser adjoint_parser = applications::exahype2::swe::parser::AdjointParser(%s);" % adjoint_parser_parameters)

  #########################################
  # Add topology parser                   #
  #########################################

  my_solver.add_user_solver_includes("""
                                     #include \"parser/TopologyParser.h\"
                                     """)
  topology_parser_parameters = "\"%s\", \"%s\"" % (args.bathymetry_file, args.displacement_file);

  topology_parser_parameters += ",\"%s\"" % args.nc_key_bath_x
  topology_parser_parameters += ",\"%s\"" % args.nc_key_bath_y
  topology_parser_parameters += ",\"%s\"" % args.nc_key_bath_bath
  topology_parser_parameters += ",\"%s\"" % args.nc_key_displ_x
  topology_parser_parameters += ",\"%s\"" % args.nc_key_displ_y
  topology_parser_parameters += ",\"%s\"" % args.nc_key_displ_displ

  # is the displacement moved for uncertainty quantification?
  if args.uq_displacement_origin is not None:
    displacement_origin_x, displacement_origin_y = parse_coordinate_pair(args.uq_displacement_origin)
    topology_parser_parameters += (", %f" % displacement_origin_x) if displacement_origin_x != float('inf') else ", INFINITY"
    topology_parser_parameters += (", %f" % displacement_origin_y) if displacement_origin_y != float('inf') else ", INFINITY"
  
  my_solver.add_solver_constants("applications::exahype2::swe::parser::TopologyParser topology_parser = applications::exahype2::swe::parser::TopologyParser(%s);" % topology_parser_parameters)

#########################################
# Set up exahype2 project               #
#########################################

my_project = exahype2.Project( namespace=["applications", "exahype2", "swe"], project_name="SWEApplication", executable=executable_name)
my_project.add_solver( my_solver )
#my_project.set_load_balancing( "toolbox::loadbalancing::strategies::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutHierarchically", "new ::exahype2::LoadBalancingConfiguration(0.98)")
my_project.set_Peano4_installation("../../../", build_modes[args.build_mode])
my_project.set_output_path(output_path)
if not os.path.exists(output_path):
  os.mkdir(output_path)

#########################################
# Insert probes into simulation domain? #
#########################################

if args.probe_coordinates != None:

  # sanitize coordinates
  coordinates = []
  for coords in args.probe_coordinates:
    coordinates.append(parse_coordinate_pair(coords))
    
  # add particles/probes to simulation domain at given coordinates
  probes = my_project.add_tracer(name="Probes",attribute_count=4, plot=False) # maybe add a plot=False here to turn off vtu output
  my_project.add_action_set_to_initialisation(
    exahype2.tracer.InsertParticlesByCoordinates(
      particle_set = probes, coordinates=coordinates
    )
  )

  # add probes to be updated during timestepping
  my_project.add_action_set_to_timestepping(
    peano4.toolbox.particles.UpdateParallelState(
      particle_set = probes
    )
  )
  
  # add fv probe
  if solver_type in fv_rusanov_solvers:
    my_project.add_action_set_to_timestepping(
      exahype2.tracer.FiniteVolumesTracing(
        probes, 
        my_solver, 
        project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseConstant"
      )
    )

  #add dg probe
  else:
    my_project.add_action_set_to_timestepping(
      exahype2.tracer.DiscontinuousGalerkinTracing(
        probes, 
        my_solver,  
        project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle"
      )
    )

  # add data dumping at end of simulation to timestepping
  my_project.add_action_set_to_timestepping(
    exahype2.tracer.DumpTracerIntoDatabase(
      probes,
      my_solver,
      output_path + ("probes-%s" % executable_name) + ("-displacement-%f-%f" % (displacement_origin_x, displacement_origin_y) if args.uq_displacement_origin is not None else ""),
      65536,
      1e50,
      1e50,
      probe_interval, 
      8,
      False,
      probe_interval, # starting time of 0 leads to gibberish for t=0 in csv
      args.end_time
    )
  )

#########################################
# Set simulation parameters             #
#########################################

my_project.set_global_simulation_parameters(
  dimensions            = 2,
  size                  = width,
  offset                = [0, 0],
  min_end_time          = args.end_time,
  max_end_time          = args.end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = args.plot_interval,
  periodic_BC           = [False, False, False]
)

#########################################
# Generate and build peano4 project     #
#########################################

peano4_project = my_project.generate_Peano4_project(verbose=True)

# add additional cpps to makefile if necessary
if args.scenario == "Tsunami": 
  peano4_project.output.makefile.add_cpp_file("parser/TopologyParser.cpp")
  peano4_project.output.makefile.add_cpp_file("parser/NetCDFReader.cpp")
  peano4_project.output.makefile.add_cpp_file("parser/NetCDFHelper.cpp")

  peano4_project.output.makefile.add_h_file("parser/TopologyParser.h")
  peano4_project.output.makefile.add_h_file("parser/NetCDFReader.hpp")
  peano4_project.output.makefile.add_h_file("parser/NetCDFHelper.hpp")

  if args.guided_amr:
    if args.gamr_file is None:
      peano4_project.output.makefile.add_cpp_file("adjoint/cpp/SWEAdjoint.cpp")
      peano4_project.output.makefile.add_cpp_file("adjoint/cpp/NetCDFWriter.cpp")
      peano4_project.output.makefile.add_cpp_file("adjoint/cpp/DimensionalSplitting.cpp")

      peano4_project.output.makefile.add_h_file("adjoint/cpp/NetCDFWriter.hpp")
      peano4_project.output.makefile.add_h_file("adjoint/cpp/DimensionalSplitting.hpp")
      peano4_project.output.makefile.add_h_file("adjoint/cpp/SWEAdjoint.hpp")
      peano4_project.output.makefile.add_h_file("adjoint/cpp/AdjointSolver.hpp")

    peano4_project.output.makefile.add_cpp_file("parser/AdjointParser.cpp")
    peano4_project.output.makefile.add_h_file("parser/AdjointParser.h")

    if args.adjoint_only:
      peano4_project.constants.define("ADJOINT_ONLY")

# define project constants if necessary
if args.scenario == "Artificial_Tsunami":
  peano4_project.constants.define("_USE_MATH_DEFINES")

peano4_project.generate()
if(args.compile):
  peano4_project.build(make_clean_first=True, throw_away_data_after_build=True)
