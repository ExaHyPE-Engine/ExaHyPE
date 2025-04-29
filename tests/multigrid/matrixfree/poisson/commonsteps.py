import os, sys
import argparse
import peano4


def get_args():
  print( """
  
  Peano 4 multigrid solver using Discontinuous Galerkin
  
  """)
  
  parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
  parser.add_argument("-j",   "--parallel-builds",      dest="j",              type=int, default=-1, help="Parallel builds" )
  parser.add_argument("-pt",   "--plot-sweep",     dest="plot_after_each_mesh_sweep", action="store_true", default=False, help="Plot after each timestep. By default (False) we plot after initialisation and at the end" )
  parser.add_argument("-pd",  "--peano-dir",            dest="peanodir",       default="../../../../", help="Peano4 directory" )
  parser.add_argument("-v",   "--verbose",              dest="verbose",        action="store_true", default=False, help="Verbose")
  parser.add_argument("-d",   "--dimensions",           dest="dimensions",     type=int, default=2, help="Dimensions")
  parser.add_argument("-p",   "--poly_degree",          dest="poly_degree",    type=int, default=1, help="Polynomial Degree")
  parser.add_argument("-meshsize",  "--meshsize",       dest="meshsize",       default=0.3, help="Mesh size")
  parser.add_argument("-omega_c",  "--omega_c",       dest="omega_c",       default=0.5, help="Mesh size")
  parser.add_argument("-omega_f",  "--omega_f",       dest="omega_f",       default=1.0, help="Mesh size")
  parser.add_argument("-m",   "--mode",                 dest="mode",           choices=["release","stats","asserts", "trace"], required=True, help="Pick build type" )
  parser.add_argument("-t",   "--test-run",             dest="test_run",        action="store_true", default=False, help="Trigger test run")
  
  return parser.parse_args()

      
def configure_and_build_project(args,project):
  #
  # Configure build
  #
  if args.mode=="release":
    build_mode = peano4.output.CompileMode.Release
  if args.mode=="stats":
    build_mode = peano4.output.CompileMode.Stats
  if args.mode=="asserts":
    build_mode = peano4.output.CompileMode.Asserts
  if args.mode=="trace":
    build_mode = peano4.output.CompileMode.Trace
  
  
  cube_size = 1.0
  project.set_global_simulation_parameters(
    dimensions            = args.dimensions,
    offset                = [ 0.0       for _ in range(args.dimensions) ],
    domain_size           = [ cube_size for _ in range(args.dimensions) ],
    plot_after_each_mesh_sweep       = args.plot_after_each_mesh_sweep,
    max_iterations                   = 10000
  )
  
  project.set_load_balancing( "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates", "new ::exahype2::LoadBalancingConfiguration(0.9, 0, false, 8, 50)" )
  project.set_Peano4_installation( args.peanodir, build_mode )
  
  peano4_project = project.generate_Peano4_project(args.verbose)
  peano4_project.output.makefile.add_cpp_file( "Scenario.cpp" )
  peano4_project.generate()

  
def run_and_validate(args,executable):
  os.system( "make clean" )
  if args.j>0:
    os.system( "make -j{}".format(args.j) )
  else:
    os.system( "make -j$(nproc --all)" )
    
  if args.test_run:
    os.system( "./" + executable + " > " + executable + ".out")
    output_file = open(executable+".out", "r")
    r_max = 0.0
    for line in output_file.readlines():
      if "endMeshSweep()" in line:
        r_max = float( line.split( "|r|_max=" )[1].split(",")[0] )
    print( "I found final residual {}".format( r_max) )
    if r_max<2.11e-3:
      print( "SUCCESS")
    else:
      sys.exit( "FAILED: residual {} not within bounds".format(r_max) )
  