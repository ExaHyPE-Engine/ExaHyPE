import os, sys

import peano4
import exahype2

project = exahype2.Project( ["benchmarks", "exahype2", "euler", "sod_shocktube"], "sod-shocktube-benchmark", ".", executable="sod-shocktube-benchmark" )

solver_name = "euler_fv_sod"
unknowns       = 4
max_h          = 0.01
min_h          = 0.01
auxiliary_variables = 0
accuracy = 1e-6

thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  solver_name,
  10,
  unknowns, auxiliary_variables,
  min_volume_h = 0.01, 
  max_volume_h = 0.01,
  time_step_relaxation = 0.01,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  pde_terms_without_state = True
)

project.add_solver( thesolver )

dimensions = 2
build_mode = peano4.output.CompileMode.Release
project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0],
  size = [1.0,1.0],
  min_end_time = 0.1,
  max_end_time = 0.1,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0,
  periodic_BC           = [False, False, False]
)
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../../../", build_mode )

peano4_project = project.generate_Peano4_project(False)
peano4_project.constants.export_const_with_type( "Accuracy", str(accuracy), "double" )
peano4_project.generate()
os.system( "make clean" )
os.system( "make -j{}".format(12) )

# import matplotlib.pyplot as plt

# #      "in-situ"

# Algorithms = [ 
#   "patch-wise",
#   "batched",
#   "task-graph",
# ]
# # Take care of the fact that AoS and SoA are contained within AoSoA
# DataStructures = [ 
#   "AoSoA",
#   "AoS",
#   "SoA",
#   "in-situ"
# ]
# # Take care of comma which ensures things don't go wrong
# Implementation  = [
#   "call stack,",
#   "heap,",
#   "omp,", 
# #      "sycl", 
# ]
# Machines = [
#   "host",
#   "single thread on device 0",
#   "4 threads on device 0",
#   "single thread on device 1",
#   "4 threads on device 1",
#   "single thread on device 2",
#   "4 threads on device 2",
# ]


# for machine in Machines:
#   for implementation in Implementation:
#     plt.clf()
#     added_data = False  
#     print( "process machine {} and implementation {}".format(machine,implementation) )
#     plotted_ticks = False
#     for algorithm in Algorithms:
#       for data_structure in DataStructures:
#       input_file = open( args.file, "r" )
#       x_labels = []
#       y_values = []
#       for line in input_file:
#         if "Number of patches fused:" in line:
#           x_labels.append( int(line.split(":")[-1]) )
#         new_measurement = "completed measurements" in line
#         new_measurement = new_measurement and algorithm in line
#         if data_structure=="SoA" or data_structure=="AoS":
#           new_measurement = new_measurement and data_structure in line
#           new_measurement = new_measurement and not "AoSoA" in line
#         else:
#           new_measurement = new_measurement and data_structure in line
#         new_measurement = new_measurement and machine in line
#         new_measurement = new_measurement and implementation in line
#         #if  and machine in line and metric in line and prefix in line:
#         if new_measurement:
#           y_values.append( float(line.split("/")[-1]) ) 
#     if len(y_values)>0: # and len(x_values)==len(y_values)
#       bar_width = 1.0/(len(Algorithms)*len(DataStructures)+1)
#       linearised_index = Algorithms.index(algorithm) * len(DataStructures) + DataStructures.index(data_structure) 
#       x_values = [i+linearised_index*bar_width for i in range(0,len(y_values))]
#       if len(x_labels)>len(y_values):
#         print( "shorten x range for metric {} x {}".format(algorithm,data_structure) )
#         x_values = x_values[0:len(y_values)]
#       if len(x_labels)<len(y_values):
#         raise Exception( "parser error for {}x{} as x-len is {} vs y-len {}".format(algorithm,data_structure,len(x_labels),len(y_values)) )
#       plt.bar( x_values, y_values, width=bar_width, label=algorithm + ", " + data_structure )
#       added_data = True 
#     if len(y_values)>0 and not plotted_ticks:
#       plotted_ticks = True
#       plt.xticks(range(0,len(x_labels)), x_labels)  
#   plt.yscale( "log" ) 
#   plt.legend()
#   plt.xlabel( "assembled patches")
#   plt.ylabel( "time per FV update [t]=s")
#   plt.title( "Throughput - size per patch: $ {}^{} $".format( args.patch_size, args.dim) )
#   if added_data:
#     plt.savefig( args.file.replace(".","_") + "_" + machine.replace(" ","_").replace(",","") + "_" + implementation.replace(" ","_").replace(",","") + ".pdf" )
#   else:
#     print( "no data found for {} x {}".format(machine,implementation) ) 

print("Done")
