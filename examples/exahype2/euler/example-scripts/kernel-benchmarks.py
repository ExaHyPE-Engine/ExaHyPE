"""

 A very simple example which demonstrates how to configure a patch-based
 Finite Volume solver in Peano4. The code relies on snippets from ExaHyPE2.
 However, it relies only on ExaHyPE's C/FORTRAN compute kernels, i.e. the
 sophisticated build environment of this H2020 project is not used. The
 solver simulates the simple Euler equations.

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os, sys
import argparse


print( """
Please call this script from the directory hosting the Makefile and the
sources. Typically, I invoke the script via

python3 example-scripts/finitevolumes-with-ExaHyPE2-benchmark.py arguments
""")

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-j",   "--parallel-builds",   dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-d",   "--dim",               dest="dim",            type=int, default=2, help="Dimensions" )
parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",                 default="../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument(        "--debug",             dest="debug"  ,        action="store_true", default=False, help="Verbose")
parser.add_argument("-ps",  "--patch-size",        dest="patch_size",     type=int, default=17, help="Dimensions" )
parser.add_argument("-a",   "--accuracy",          dest="accuracy",       type=float, default=1e-10, help="Floating point accuracy to which the different kernel variants have to match (absolute)" )
parser.add_argument("-f",   "--file",              dest="file",           default="", help="Specify to convert an output file into a bar plot. If you omit this flag, the script will actually build the benchmark" )
args = parser.parse_args()

if args.dim not in [2,3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    import sys
    sys.exit(1)

if args.file=="":
    import peano4
    import exahype2
    
    #
    # Create a project and configure it to end up in a subnamespace (and thus
    # subdirectory). 
    #
    project = exahype2.Project( ["examples", "exahype2", "euler"], "finitevolumes", ".", executable="kernel-benchmark" )
    
    #
    # Add the Finite Volumes solver
    #
    solver_name = "EulerOnGPU"
    unknowns       = 5
    max_h          = 0.01
    min_h          = 0.01
    auxiliary_variables = 0
    
    thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
      solver_name,
      args.patch_size,
      unknowns, auxiliary_variables,
      min_volume_h = 0.01, 
      max_volume_h = 0.01,
      time_step_relaxation = 0.01,
      flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
      eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
      pde_terms_without_state = True
    )
    
    thesolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation )
    
    project.add_solver( thesolver )
    
    
    #
    # Configure build
    #
    dimensions = args.dim
    if args.debug:
      build_mode = peano4.output.CompileMode.Debug
    else:
      build_mode = peano4.output.CompileMode.Release
    project.set_global_simulation_parameters(
      dimensions = dimensions,
      offset = [0.0,0.0,0.0],
      size = [1.0,1.0,1.0],
      min_end_time = 1.0,
      max_end_time = 1.0,
      first_plot_time_stamp = 0.0,
      time_in_between_plots = 0.0,
      periodic_BC           = [False, False, False]
    )
    project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
    project.set_Peano4_installation( args.peanodir, build_mode )
    
    
    #
    # Generate code and then copy cpp file over main
    #
    peano4_project = project.generate_Peano4_project(args.verbose)
    peano4_project.constants.export_const_with_type( "Accuracy", str(args.accuracy), "double" )
    peano4_project.generate()
    benchmark_main_file = os.path.dirname(os.path.realpath(__file__)) + "/kernel-benchmarks.cpp"
    print( "replace generated main with benchmark main file {}".format(benchmark_main_file) )
    os.system( "cp {} finitevolumes-main.cpp".format(benchmark_main_file) )
    os.system( "make clean" )
    if args.j>0:
      os.system( "make -j{}".format(args.j) )
    else:
      os.system( "make -j" )
else:
    import matplotlib.pyplot as plt
    
    Prefix  = [
      "call stack,",
      "heap,",
      "omp,"
    ]
    Metrics = [ 
      "baseline:",
      "patch-wise, AoS:",
      "patch-wise, SoA:",
      "patch-wise, AoSoA:",
      "batched, AoS:",
      "batched, SoA:",
      "batched, AoSoA:",
      "patch-wise, in-situ:",
      "batched, in-situ:"
    ]
    
    for prefix in Prefix:
      plt.clf()
      print( "handle prefix {}".format(prefix) )
      plotted_ticks = False
      for metric in Metrics:
        input_file = open( args.file, "r" )
        x_labels = []
        y_values = []
        for line in input_file:
          if "Number of patches fused:" in line:
            x_labels.append( int(line.split(":")[-1]) )
          if metric in line and prefix in line:
            y_values.append( float(line.split("/")[-1]) ) 
        if len(y_values)>0: # and len(x_values)==len(y_values)
          bar_width = 1.0/(len(Metrics)+1)
          x_values = [i+(Metrics.index(metric)-len(Metrics)/2.0)*bar_width for i in range(0,len(y_values))]
          if len(x_labels)>len(y_values):
            print( "shorten x range for metric {} x {}".format(prefix,metric) )
            x_values = x_values[0:len(y_values)]
          if len(x_labels)<len(y_values):
            raise Exception( "parser error for {}x{} for metric {}".format(x_labels,y_values,metric) )
          plt.bar( x_values, y_values, width=bar_width, label=metric.replace(":","") ) 
        if len(y_values)>0 and not plotted_ticks:
          plotted_ticks = True
          plt.xticks(range(0,len(x_labels)), x_labels)  
      plt.yscale( "log" ) 
      plt.legend()
      plt.xlabel( "assembled patches")
      plt.ylabel( "time per FV update [t]=s")
      plt.title( "Throughput for $ {}^{} $ Cartesian patches".format( args.patch_size, args.dim) )
      plt.savefig( args.file.replace(".","_")+"_"+prefix.replace(" ","").replace(",","")+".pdf" ) 
  
print("Done")
