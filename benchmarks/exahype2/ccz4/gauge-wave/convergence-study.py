import os

import peano4
import exahype2
import dastgen2

import peano4.toolbox.particles


#       SuperClass = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking
#       SuperClass = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep

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


my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(name                 = "CCZ4FV", 
                                                                                 patch_size           = 5,
                                                                                 unknowns             = sum(unknowns.values()),
                                                                                 auxiliary_variables  = 0,
                                                                                 min_volume_h         = 0.05, 
                                                                                 max_volume_h         = 0.05,
                                                                                 time_step_relaxation = 0.1)



my_solver._my_user_includes = """
#include "../CCZ4Kernels.h"
"""


my_solver.set_implementation(
          boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          ncp                  = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          flux                 = exahype2.solvers.fv.PDETerms.None_Implementation,
          source_term          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          eigenvalues          = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
        )


my_solver.add_user_action_set_includes( """
#include "CCZ4Kernels.h"
""")
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

   
my_solver.add_solver_constants( """static constexpr int Scenario=0; /* Gauge wave */  """ )


project = exahype2.Project( ["benchmarks", "exahype2", "ccz4"], "ccz4", executable="finitevolumes")




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

#build_mode = peano4.output.CompileMode.Debug
build_mode = peano4.output.CompileMode.Release
    
dimensions  = 3
offset      = [-0.5, -0.5, -0,5] 
domain_size = [   1,    1,    1]
periodic_boundary_conditions = [True,True,True]          # Periodic BC

project.set_global_simulation_parameters(
  dimensions,               # dimensions
  offset,  domain_size,
  1.0,                      # end time
  0.0, 
  0.1,                      #args.plot_step_size,   # snapshots
  periodic_boundary_conditions,
  8  # plotter precision
)

project.set_Peano4_installation("../../../..", build_mode)


project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision","(new ::exahype2::LoadBalancingConfiguration(0.98,0))" )

########################################################################################
#Tracer setting 
########################################################################################
#    if not args.add_tracer==0:
#      tracer_name = {1:"line", 2:"slide", 3:"volume", 6:"Gauss_Legendre_quadrature", 7:"t-design"}
#      tracer_particles = project.add_tracer( name="MyTracer",attribute_count=65 )
#       #project.add_action_set_to_timestepping(exahype2.tracer.FiniteVolumesTracing(tracer_particles,my_solver,[17,18,19],[16],-1,time_stepping_kernel="toolbox::particles::explicitEulerWithoutInterpolation"))
#      project.add_action_set_to_timestepping(
#        exahype2.tracer.FiniteVolumesTracing(
#          tracer_particles,my_solver,
#          [17,18,19],range(65),-1,
#          #time_stepping_kernel="toolbox::particles::LinearInterp",
#          time_stepping_kernel="toolbox::particles::StaticPosition"#,
#          #observer_kernel="toolbox::particles::ObLinearInterp"
#        )
#      )
#        project.add_action_set_to_initialisation( exahype2.tracer.InsertParticlesbyCoordinates ( particle_set=tracer_particles, N=3, coor_s=[[0.4251,0,0],[-0.4251,0,0],[0.2,0.2,0]]))

#      project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
#        particle_set=tracer_particles,
#        solver=my_solver,
#        filename=path1+"zz"+args.tra_name,
#        number_of_entries_between_two_db_flushes=2000,
#        output_precision=10,
#        position_delta_between_two_snapsots=1e16,
#        data_delta_between_two_snapsots=1e16,
#        time_delta_between_two_snapsots=0.02,
#        use_relative_deltas=False,
#        initial_record_time=55#args.end_time*2.0/3.0
#        #last_record_time=1e3
#      ))
      #data_delta_between_two_snapsots,position_delta_between_two_snapsots,filename,          
      #,,-1,"zz",1000))


peano4_project = project.generate_Peano4_project(verbose=True)


peano4_project.output.makefile.add_CXX_flag( "-DCCZ4EINSTEIN" )
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/InitialValues.cpp" )
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp" )
peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/ccz4" )

peano4_project.generate( throw_away_data_after_generation=False )
peano4_project.build( make_clean_first = True )

