import os, sys
import argparse

import peano4
import exahype2

print("""

ExaHyPE 2 - CCZ4 Finite Volume Rusanov Kernel Benchmarks

@author 2022 Tobias Weinzierl, Mario Wille

""")

parser = argparse.ArgumentParser(description='ExaHyPE 2 - CCZ4 FV Rusanov Kernel Benchmarking Script')
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose mode")
parser.add_argument(        "--debug",             dest="debug"  ,        action="store_true", default=False, help="Debug mode")
parser.add_argument(        "--managed",           dest="managed_memory", action="store_true", default=False, help="Use managed memory kernel variants")
parser.add_argument("-ps",  "--patch-size",        dest="patch_size",     type=int, default=9, help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-a",   "--accuracy",          dest="accuracy",       type=float, default=0.0, help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon.")
parser.add_argument("-i",   "--iterations",        dest="iterations",     type=int, default=100, help="Number of iterations to be done for each kernel invocation. Used for warmup and to give a realistic measurement.")
args = parser.parse_args()

accuracy = args.accuracy
if(args.accuracy < 0):
  import numpy
  accuracy = default=numpy.finfo(float).eps

dimensions = 3
project = exahype2.Project( ["benchmarks", "exahype2", "ccz4"], "CCZ4FVRusanovKernelBenchmarks", ".", executable="kernel-benchmark-fv-3d-patch-size-"+ str(args.patch_size) )

unknowns = {
          "G":6,
          "K":6,
          "theta":1,
          "Z":3,
          "lapse":1,
          "shift":3,
          "b":3,
          "dLapse":3,
          "dxShift":3,
          "dyShift":3,
          "dzShift":3,
          "dxG":6,
          "dyG":6,
          "dzG":6,
          "traceK":1,
          "phi":1,
          "P":3,
          "K0":1,
        }

fv_rusanov_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  name = "CCZ4FVRusanov",
  patch_size = args.patch_size,
  unknowns = sum(unknowns.values()),
  auxiliary_variables = 0,
  min_volume_h = 0.01,
  max_volume_h = 0.01,
  time_step_relaxation = 0.01,
  pde_terms_without_state = True
)

integer_constants = {
  "CCZ4LapseType":0,
}
double_constants = {
  "CCZ4ds":   1.0,
  "CCZ4c":    1.0,
  "CCZ4e":    1.0,
  "CCZ4f":    0.75,
  "CCZ4bs":   0.0,
  "CCZ4sk":   0.0,
  "CCZ4xi":   1.0,
  "CCZ4itau": 1.0,
  "CCZ4eta":  1.0,
  "CCZ4k1":   0.1,
  "CCZ4k2":   0.0,
  "CCZ4k3":   0.5,
  "CCZ4GLMc": 1.2,
  "CCZ4GLMd": 2.0,
  "CCZ4mu":   0.2,
}

for key, value in integer_constants.items():
  fv_rusanov_solver.add_solver_constants("static constexpr int {} = {};".format(key, value))
for key, value in double_constants.items():
  fv_rusanov_solver.add_solver_constants("static constexpr double {} = {};".format(key, value))

fv_rusanov_solver.add_user_action_set_includes( """
#include "CCZ4Kernels.h"
""")
fv_rusanov_solver.add_user_solver_includes( """
#include "CCZ4Kernels.h"
#include "InitialValues.h"
#include <cstring>
""")

fv_rusanov_solver.set_implementation(
  boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
  ncp                  = """
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
  flux                 = exahype2.solvers.fv.PDETerms.None_Implementation,
  source_term          = """
  memset(S, 0.0, NumberOfUnknowns*sizeof(double));
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
  refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
  eigenvalues          = """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
""",
  initial_conditions   = """
for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0;
::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
"""
)

project.add_solver(fv_rusanov_solver)

offset      = [-0.5, -0.5, -0.5]
domain_size = [   1,    1,    1]
periodic_boundary_conditions = [True,True,True]

project.set_global_simulation_parameters(
  dimensions   = dimensions,
  offset       = offset,
  size         = domain_size,
  min_end_time = 1.0,
  max_end_time = 1.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0,
  periodic_BC = periodic_boundary_conditions
)

project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision","(new ::exahype2::LoadBalancingConfiguration(0.98,0))" )

if args.debug:
  build_mode = peano4.output.CompileMode.Debug
else:
  build_mode = peano4.output.CompileMode.Release
project.set_Peano4_installation( "../../../../", build_mode )

peano4_project = project.generate_Peano4_project(args.verbose)

peano4_project.constants.export_constexpr_with_type( "Accuracy", str(accuracy), "double" )
peano4_project.constants.export_constexpr_with_type( "Iterations", str(args.iterations), "int" )
peano4_project.constants.export_boolean( "EnableManagedMemory", args.managed_memory )

peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/InitialValues.cpp" )
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp" )
peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/ccz4" )

peano4_project.generate()
peano4_project.build( make_clean_first = True)

print("Done")
