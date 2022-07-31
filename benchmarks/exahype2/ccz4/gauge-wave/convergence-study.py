import os
import argparse

import peano4
import exahype2
import dastgen2

import peano4.toolbox.particles



parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-j",   "--parallel-builds",   dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-s",   "--solver",            dest="solver",         choices=["fv", "fd4-rk1", "fd4-rk2", "fd4-rk3", "fd4-rk4", "dg1-rk1", "dg2-rk2", "dg3-rk3", "dg4-rk4"], required=True, help="Pick solver type" )
parser.add_argument("-et",  "--end-time",          dest="end_time",       type=float, default=1.0, help="End of simulation" )
parser.add_argument("-cs",  "--cell-size",         dest="cell_size",      type=float, default=0.3, help="Cell size" )
parser.add_argument("-asserts",  "--asserts",      dest="asserts",        action="store_true", default=False, help="Switch on assertions" )
args = parser.parse_args()




#
# This part si only for convenience. We could use these symbols to augment
# the output files, but I don't use this feature here.
#
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


#
# Create solver class
#
my_solver = None


if args.solver=="fv":
  my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(name                 = "CCZ4FV", 
                                                                                   patch_size           = 5,
                                                                                   unknowns             = sum(unknowns.values()),
                                                                                   auxiliary_variables  = 0,
                                                                                   min_volume_h         = args.cell_size / 5, 
                                                                                   max_volume_h         = args.cell_size / 5,
                                                                                   time_step_relaxation = 0.1)
if "fd4-rk" in args.solver:
  order = int(args.solver[-1])
  my_solver = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep(name                 = "CCZ4FD4RK" + str(order), 
                                                               patch_size           = 5,
                                                               rk_order             = order,
                                                               unknowns             = sum(unknowns.values()),
                                                               auxiliary_variables  = 0,
                                                               min_meshcell_h       = args.cell_size / 5, 
                                                               max_meshcell_h       = args.cell_size / 5,
                                                               time_step_relaxation = 0.1)
if "rk" in args.solver and "dg" in args.solver:
  space_order = int(args.solver[2])
  time_order  = int(args.solver[6])
  my_solver = exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep(name             = "CCZ4DG"  + str(space_order) + "RK" + str(time_order),
                                                               rk_order             = time_order,
                                                               polynomials          = exahype2.solvers.GaussLegendreBasis(space_order),
                                                               unknowns             = sum(unknowns.values()),
                                                               auxiliary_variables  = 0,
                                                               min_cell_h       = args.cell_size, 
                                                               max_cell_h       = args.cell_size,
                                                               time_step_relaxation = 0.1)

#
# Add the compute kernels and initial condition implementations from the 
# real application to this project, so we we can use them.
#
my_solver.add_user_action_set_includes( """
#include "CCZ4Kernels.h"
""")
my_solver.add_user_solver_includes( """
#include "CCZ4Kernels.h"
#include "InitialValues.h"
#include <cstring>
""")


#
# I need a couple of constants. I could either replace them directly
# within the Python snippets below, but I prefer here to go a different
# way and to export them as proper C++ constants.
#
# There are two ways to inject solver constants into Peano: We can either
# add them to the Makefile as global const expressions, or we can add 
# them to the ExaHyPE2 solver. The latter is the route we go down here, 
# as these constants logically belong to the solver and not to the project.
#
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
  my_solver.add_solver_constants("static constexpr int {} = {};".format(key, value))
for key, value in double_constants.items():   
  my_solver.add_solver_constants("static constexpr double {} = {};".format(key, value))


#
# All of our solver routines (events) will basically delegate to some 
# functions in the CCZ4 kernels. So we refrain from creating any meaningful
# C++ implementation and tell the solver directly which routines to delegate
# to.
#
# boundary conditions: remain empty, as we employ periodic BCs anyway
# ncp:                 call kernel routine
#
if ("rk" in args.solver and "dg" in args.solver):
  my_solver.set_implementation(
          boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          ncp                  = """
  double dQdxSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    dQdxSerialised[i+0*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+1*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+2*NumberOfUnknowns] = 0.0;

    dQdxSerialised[i+normal*NumberOfUnknowns] = dQdx[i];
  }
  ::applications::exahype2::ccz4::ncp(BgradQ, Q, dQdxSerialised, normal, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
          flux                 = exahype2.solvers.fv.PDETerms.None_Implementation,
          source_term          = """
  memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
          refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          eigenvalues          = """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""          ,
          initial_conditions   = """
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
  ::applications::exahype2::ccz4::gaugeWave(Q, x, 0);
"""
        )
else:    
  my_solver.set_implementation(
          boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          ncp                  = """
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;

    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, deltaQSerialised, normal, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
          flux                 = exahype2.solvers.fv.PDETerms.None_Implementation,
          source_term          = """
  memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
          refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          eigenvalues          = """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""          ,
          initial_conditions   = """
for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
"""
        )    

    
# ===============
# Postprocessing
# ===============
if args.solver=="fv":
  my_solver.postprocess_updated_patch = """
{
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( targetPatch+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""
if "fd4-rk" in args.solver:
  my_solver.postprocess_updated_patch = """
{
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( targetPatch+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""
if "rk" in args.solver and "dg" in args.solver:
  my_solver.postprocess_updated_patch = """
{
    constexpr int itmax = ({{ORDER}}+1) * ({{ORDER}}+1) * ({{ORDER}}+1);
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( targetPatch+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""



project = exahype2.Project( ["benchmarks", "exahype2", "ccz4"], "ccz4", executable=args.solver + "-" + str(args.cell_size))



########################################################################################
#parameter setting according to scenarios
########################################################################################
#    for k, v in intparams.items():
#      intparams.update({k:eval("args.CCZ4{}".format(k))})
#    for k, v in floatparams.items():
#      floatparams.update({k:eval("args.CCZ4{}".format(k))})



solverconstants=""

#    if args.scenario=="gauge":
#      solverconstants+= "static constexpr int Scenario=0; /* Gauge wave */  "
#      userinfo.append(("picking gauge wave scenario",None))
#      floatparams.update({"sk":0.0}); floatparams.update({"bs":0.0})
#      intparams.update({"LapseType":0})

project.add_solver(my_solver)



########################################################################################
#Tracer setting 
########################################################################################
tracer_particles = project.add_tracer( name="Tracer",attribute_count=59 )
project.add_action_set_to_initialisation( 
  exahype2.tracer.InsertParticlesbyCoordinates( 
    particle_set=tracer_particles, coordinates=[[-0.4251,0,0],[0,0,0],[0.4251,0,0]]))
project.add_action_set_to_timestepping(
  peano4.toolbox.particles.UpdateParallelState(
    particle_set=tracer_particles
  )
)

if args.solver=="fv" or "fd4-rk" in args.solver:
  project.add_action_set_to_timestepping(exahype2.tracer.FiniteVolumesTracing(tracer_particles,
                                                                              my_solver,
                                                                              [],        # velocity indices: don't move them [17,18,19]
                                                                              range(59), # data indices
                                                                              0,         # scaling of velocities
                                                                              time_stepping_kernel="toolbox::particles::StaticPosition"))

if "rk" in args.solver and "dg" in args.solver:
  print( "WARNING: Haven't written the tracer yet")
#  project.add_action_set_to_timestepping(exahype2.tracer.DiscontinuousGalerkinTracing(tracer_particles,
#                                                                                      my_solver,
#                                                                                      [],        # velocity indices: don't move them [17,18,19]
#                                                                                      range(59), # data indices
#                                                                                      0,         # scaling of velocities
#                                                                                      time_stepping_kernel="toolbox::particles::StaticPosition"))


project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
  particle_set=tracer_particles,
  solver=my_solver,
  filename="Tracer-" + args.solver + "-" + str(args.cell_size),
  data_delta_between_two_snapsots=1e16,
  time_delta_between_two_snapsots=0.0001
  ))


########################################################################################
# Build the project
########################################################################################

if args.asserts:
  build_mode = peano4.output.CompileMode.Asserts
else:
  build_mode = peano4.output.CompileMode.Release
    
dimensions  = 3
offset      = [-0.5, -0.5, -0,5] 
domain_size = [   1,    1,    1]
periodic_boundary_conditions = [True,True,True]          # Periodic BC

project.set_global_simulation_parameters(
  dimensions,               # dimensions
  offset,  domain_size,
  args.end_time,
  0.0, 
  0.1,                      #args.plot_step_size,   # snapshots
  periodic_boundary_conditions,
  8  # plotter precision
)

project.set_Peano4_installation(args.peanodir, build_mode)


project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision","(new ::exahype2::LoadBalancingConfiguration(0.98,0))" )


peano4_project = project.generate_Peano4_project(verbose=args.verbose)


peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/InitialValues.cpp" )
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp" )
peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/ccz4" )

peano4_project.generate( throw_away_data_after_generation=False )
peano4_project.build( make_clean_first = True, number_of_parallel_builds=args.j )

