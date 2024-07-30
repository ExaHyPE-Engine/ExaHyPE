import os
import argparse
import dastgen2
import shutil
import numpy as np

import peano4
import exahype2
import peano4.toolbox.particles
from peano4.toolbox.blockstructured.DynamicAMR        import DynamicAMR


from Probe_file_gene          import tracer_seeds_generate
from ComputeFirstDerivatives  import ComputeFirstDerivativesFD4RK
import CCZ4Helper



modes = { 
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts, 
  "stats":  peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

floatparams = {
  "GLMc0":1.5, "GLMc":1.2, "GLMd":2.0, "GLMepsA":1.0, "GLMepsP":1.0,
  "GLMepsD":1.0, 
	"itau":1.0, "k1":0.1, "k2":0.0, "k3":0.5, "eta":1.0,
  "f":0.75, "g":0.0, "xi":1.0, "e":1.0, "c":1.0, "mu":0.2, "ds":1.0,
  "sk":1.0, "bs":1.0,
  "domain_r":0.5, "smoothing":0.0, "KOSigma":8.0 #, \
}

intparams = {"BBHType":2, "LapseType":1, "tp_grid_setup":0, "swi":99, "ReSwi":0, "SO":0}
#
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ExaHyPE 2 - CCZ4 script')
    parser.add_argument("-maxh", "--max-h",           dest="max_h",           type=float, required="True",  help="upper limit for refinement. Refers to volume/meshcell size, i.e. not to patch size" )
    parser.add_argument("-minh", "--min-h",           dest="min_h",           type=float, default=0,        help="lower limit for refinement (set to 0 to make it equal to max_h - default). Refers to volume/meshcell size, i.e. not to patch size" )
    parser.add_argument("-ps",   "--patch-size",      dest="patch_size",      type=int,   default=6,        help="Patch size, i.e. number of volumes per patch per direction" )
    parser.add_argument("-plt",  "--plot-step-size",  dest="plot_step_size",  type=float, default=0.04,     help="Plot step size (0 to switch it off)" )
    parser.add_argument("-m",    "--mode",            dest="mode",            default="release",            help="|".join(modes.keys()) )
    parser.add_argument(         "--gpu",             dest="GPU",             default=False, action="store_true",  help="Run with accelerator support" )
    parser.add_argument("-ext",  "--extension",       dest="extension",       choices=["none", "adm", "Psi4"],   default="none",  help="Pick extension, i.e. what should be plotted on top of the evolving solution. Default is none" )
    parser.add_argument("-impl", "--implementation",  dest="implementation",  choices=["fv-adaptive", "fd4-rk1-adaptive", "fd4-rk1-adaptive-enclave", "fd4-rk1-fixed", "fd4-rk4-adaptive", "fd4-rk2-adaptive", "RKDG"], required="True",  help="Pick solver type" )
    parser.add_argument("-no-pbc",  "--no-periodic-boundary-conditions",      dest="periodic_bc", action="store_false", default=True,  help="switch on or off the periodic BC" )
    parser.add_argument("-sommerfeld",  "--sommerfeld-boundary-conditions",   dest="sommerfeld_bc", action="store_true", default=False,  help="switch on or off the Sommerfeld radiative BC" )
    parser.add_argument("-et",   "--end-time",        dest="end_time",        type=float, default=1.0, help="End (terminal) time" )
    parser.add_argument("-pst",  "--plot-start-time", dest="plot_start_time", type=float, default=0.0, help="start time for plot" )
    parser.add_argument("-s",    "--scenario",        dest="scenario",        choices=["gauge", "dia_gauge", "linear", "single-puncture","two-punctures", "flat"], required="True", help="Scenario" )
    parser.add_argument("-cfl",  "--CFL-ratio",       dest="cfl",             type=float, default=0.1, help="Set CFL ratio" )
    parser.add_argument("-tracer", "--add-tracer",    dest="add_tracer",  type=int, default=0,  help="Add tracers and specify the seeds. 0-switch off, 1-static point tracer, 2-moving point tracer" )
    parser.add_argument("-tn",   "--tracer-name",     dest="tra_name",    type=str, default="de",  help="name of output tracer file (temporary)" )
    parser.add_argument("-exn",  "--exe-name",        dest="exe_name",    type=str, default="",  help="name of output executable file" )
    parser.add_argument("-outdir", "--output-directory", dest="path",     type=str, default="./",  help="specify the output directory, include the patch file and tracer file" )
    parser.add_argument("-so",   "--second-order",    dest="SO_flag",     default=False, action="store_true",  help="enable double communication per timestep, used in the soccz4 formulation.")


    for k, v in floatparams.items(): parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=float, default=v, help="default: %(default)s")
    for k, v in intparams.items():
      if k=="ReSwi":
        parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=int, default=v, help="default: %(default)s, choose refinement criterion.")
      else: parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=int, default=v, help="default: %(default)s")

    args = parser.parse_args()

    SuperClass = None

    print(args.implementation)
    if args.implementation=="fv-adaptive":
       SuperClass = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep
    if args.implementation=="fd4-rk1-adaptive" or args.implementation=="fd4-rk4-adaptive" or args.implementation=="fd4-rk2-adaptive":
       SuperClass = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep
    if args.implementation=="fd4-rk1-adaptive-enclave":
       SuperClass = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking
    if args.implementation=="RKDG":
       SuperClass = exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking


    class CCZ4Solver( SuperClass ):
      def __init__(self, name, patch_size, min_mesh_unit_h, max_mesh_unit_h, cfl, domain_r, KOSig):
        unknowns = {
          "G":6,      #0-5
          "K":6,      #6-11
          "theta":1,  #12
          "Z":3,      #13-15
          "lapse":1,  #16
          "shift":3,  #17-19
          "b":3,      #20-22
          "dLapse":3, #23-25
          "dxShift":3,#26-28
          "dyShift":3,#29-31
          "dzShift":3,#32-34
          "dxG":6,    #35-40
          "dyG":6,    #41-46
          "dzG":6,    #47-52
          "traceK":1, #53
          "phi":1,    #54
          "P":3,      #55-57
          "K0":1,     #58
        }

        number_of_unknowns = sum(unknowns.values())

        self._my_user_includes = """
#include "../CCZ4Kernels.h"
"""
        if SuperClass==exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_volume_h=min_mesh_unit_h, max_volume_h=max_mesh_unit_h,
            time_step_relaxation=cfl
          )
        elif SuperClass==exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep:
          if args.implementation=="fd4-rk1-adaptive":
            rk_order = 1
          elif args.implementation=="fd4-rk2-adaptive":
            rk_order = 2
          else:
            rk_order = 4
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size, rk_order=rk_order,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_meshcell_h=min_mesh_unit_h, max_meshcell_h=max_mesh_unit_h,
            time_step_relaxation=cfl,
            KOSigma=KOSig
          )
        elif SuperClass==exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking:
          if args.implementation=="fd4-rk1-adaptive-enclave":
            rk_order = 1
          else:
            rk_order = 4
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size, rk_order=rk_order,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_meshcell_h=min_mesh_unit_h, max_meshcell_h=max_mesh_unit_h,
            time_step_relaxation=cfl,
            KOSigma=KOSig,
            pde_terms_without_state=True
          )
        else: # only rkdg left. i.e., SuperClass == exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep:
          SuperClass.__init__(
            self,
            name=name,
            rk_order     = 2,
            polynomials  = exahype2.solvers.GaussLegendreBasis(2),
            face_projections   = exahype2.solvers.rkdg.FaceProjections.Solution,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_cell_h=min_mesh_unit_h, max_cell_h=max_mesh_unit_h,
            time_step_relaxation=cfl
          )

        self._patch_size = patch_size
        self._domain_r = domain_r

        self.set_implementation(
          boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
          ncp=exahype2.solvers.PDETerms.User_Defined_Implementation,
          flux=exahype2.solvers.PDETerms.None_Implementation,
          source_term=exahype2.solvers.PDETerms.User_Defined_Implementation,
          refinement_criterion = exahype2.solvers.PDETerms.User_Defined_Implementation,
          eigenvalues = exahype2.solvers.PDETerms.User_Defined_Implementation
        )

        #self._compute_kernel_call = exahype2.solvers.rkfd.fd4.kernels.create_compute_kernel_for_FD4(
        #    self._flux_implementation, 
        #    self._ncp_implementation, 
        #    self._source_term_implementation, 
        #    compute_max_eigenvalue_of_next_time_step = True,
        #    solver_variant                           = exahype2.solvers.rkfd.kernels.SolverVariant.Multicore,
        #    kernel_variant                           = exahype2.solvers.rkfd.kernels.KernelVariant.BatchedAoSHeap,
        #    KOSigma                                  = self._KO_Sigma
        #  )

        if SuperClass==exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking:
          self._fused_compute_kernel_call_cpu = exahype2.solvers.rkfd.fd4.kernels.create_compute_kernel_for_FD4(
            self._flux_implementation, 
            self._ncp_implementation, 
            self._source_term_implementation, 
            compute_max_eigenvalue_of_next_time_step = True,
            solver_variant                           = exahype2.solvers.rkfd.kernels.SolverVariant.Multicore,
            kernel_variant                           = exahype2.solvers.rkfd.kernels.KernelVariant.BatchedAoSHeap,
            KOSigma                                  = self._KO_Sigma
          )
        
        if SuperClass==exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep:
          self.postprocess_updated_patch += """{
    #if Dimensions==2
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    #endif

    #if Dimensions==3
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    #endif

    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""




        else:
          self.postprocess_updated_patch += CCZ4Helper.get_body_of_enforceCCZ4constraint()
   
      def create_action_sets(self):
        SuperClass.create_action_sets(self)
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.additional_includes += """ 
#include "../CCZ4Kernels.h"
            """

      def get_user_action_set_includes(self):
        """
         We take this routine to add a few additional include statements.
        """
        return SuperClass.get_user_action_set_includes(self) + self._my_user_includes

      def add_adm_constriants(self):
        """
          @add adm constriants (Hamilton and Momentum)
        """
        self._auxiliary_variables = 4

        self._my_user_includes += """
  #include "../CCZ4Kernels.h"
    """

        self._preprocess_reconstructed_patch = CCZ4Helper.get_body_of_adm_constraints(
          self._patch.dim[0], self._auxiliary_variables)

        self.create_data_structures()
        self.create_action_sets()

      def add_Psi4W(self):
        """
  add psi4 writer
        """
        self._auxiliary_variables = 2

        self._my_user_includes += """
  #include "../libtwopunctures/TP_PunctureTracker.h"
  #include "../CCZ4Kernels.h"
    """

        self._preprocess_reconstructed_patch = CCZ4Helper.get_body_of_Psi_Calc(
          self._patch.dim[0], self._auxiliary_variables)

        self.create_data_structures()
        self.create_action_sets()

########################################################################################
#main starts here
########################################################################################
    userinfo = []
    exe="peano4"
    
    if args.exe_name!="":
      exe += "_"
      exe += args.exe_name
    if not args.tra_name=="de":
      exe += "_" + args.tra_name
    project = exahype2.Project( ["applications", "exahype2", "ccz4"], "ccz4", executable=exe)

########################################################################################
#Pick solver
########################################################################################
    is_aderdg = False
    is_rkdg   = False
    solver_name = "CCZ4"
    try:
      if SuperClass==exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize:
        is_aderdg = True
        order = 3
        unknowns = 59
        time_step_size = 0.001
    except Exception as e:
        pass
        msg = "Warning: ADER-DG no supported on this machine"
        print(msg)
        userinfo.append((msg,e))

    try:
      if SuperClass==exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep:
        is_rkdg = True
    except Exception as e:
        pass
        msg = "Warning: RKDG not supported on this machine"
        print(msg)
        userinfo.append((msg,e))

    if is_aderdg:
      solver_name    = "ADERDG" + solver_name
    elif is_rkdg:
      solver_name    = "RKDG" + solver_name
    else:
      solver_name    = solver_name

    #if args.SO_flag==True:
    #  args.cfl=args.cfl/2

    min_h = args.min_h
    if min_h <=0.0:
      print( "No minimal mesh size chosen. Set it to max mesh size (regular grid)" )
      min_h = args.max_h

    if is_aderdg:
      my_solver = exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize(
          solver_name, order, unknowns, 0, #auxiliary_variables
          exahype2.solvers.aderdg.Polynomials.Gauss_Legendre,
          min_h, args.max_h, time_step_size,
          flux = None,
          ncp  = exahype2.solvers.PDETerms.User_Defined_Implementation,
          sources = exahype2.solvers.PDETerms.User_Defined_Implementation
      )
    else:
      my_solver = CCZ4Solver(solver_name, args.patch_size, min_h, args.max_h, args.cfl, args.CCZ4domain_r,args.CCZ4KOSigma)
      userinfo.append(("CFL ratio set as "+str(args.cfl), None))

    userinfo.append(("The solver is "+args.implementation, None))

########################################################################################
#Pick interpolation scheme
########################################################################################
        
    #Below are the I&R set functions for FD4
    #These functions here do two things:
    #1. it set the solver.interpolation parameters similar to what we do for fv, so the code know what template shoule be named.
    #2. it computes and write the corresponding matrices for named schemes into the solver head files.
    #notice point 2 is not needed as for build-in function (for fv) the matrices are already hard-coded.  
    tem_interp=["TP_constant","TP_linear_with_linear_extrap_normal_interp"]
    tem_restrict=["TP_inject_normal_extrap","TP_average_normal_extrap"]
    if ("fd4-rk" in args.implementation):
      exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_interpolation( my_solver, tem_interp[1] ) 
      exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_restriction(   my_solver, tem_restrict[1] ) 
      userinfo.append(("FD4 Interpolation: " + tem_interp[1] + " & Restriction: " + tem_restrict[1], None))

########################################################################################
#Add postpocessing function
########################################################################################
    if args.extension=="adm":
      my_solver.add_adm_constriants()
    if args.extension=="Psi4":
      my_solver.add_Psi4W()    

########################################################################################
#parameter setting according to scenarios
########################################################################################
    for k, v in intparams.items():
      intparams.update({k:eval("args.CCZ4{}".format(k))})
    for k, v in floatparams.items():
      floatparams.update({k:eval("args.CCZ4{}".format(k))})

    if args.SO_flag==True:
      intparams.update({"SO":1})

    if args.scenario=="two-punctures":
      msg = "Periodic BC deactivated because you pick Puncture scenario\nInitialize binary black holes"
      print(msg)
      periodic_boundary_conditions = [False,False,False]
      intparams.update({"swi":2})  #notice it may change, see according function in CCZ4.cpp
      print(intparams)
      userinfo.append((msg,None))
    elif args.scenario=="single-puncture":
      msg = "Periodic BC deactivated because you pick Puncture scenario\nInitialize single black hole"
      print(msg)
      periodic_boundary_conditions = [False,False,False]
      intparams.update({"swi":0}) 
      userinfo.append((msg,None))
    elif args.periodic_bc==True:
      msg = "Periodic BC set"
      print(msg)
      periodic_boundary_conditions = [True,True,True]          # Periodic BC
      userinfo.append((msg,None))
    else:
      msg = "WARNING: Periodic BC deactivated by hand"
      print(msg)
      periodic_boundary_conditions = [False,False,False]
      userinfo.append((msg,None))

    if args.sommerfeld_bc==True:
      msg = "set Sommerfeld boundary condition"
      userinfo.append((msg,None))
      periodic_boundary_conditions = [False,False,False]
      my_solver._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = CCZ4Helper.get_body_of_SommerfeldCondition(
                                                                                  args.scenario,
                                                                                  my_solver._unknowns,
                                                                                  my_solver._auxiliary_variables)

    solverconstants=""

    if args.scenario=="gauge":
      solverconstants+= "static constexpr int Scenario=0; /* Gauge wave */ \n "
      userinfo.append(("picking gauge wave scenario",None))
      floatparams.update({"sk":0.0}); floatparams.update({"bs":0.0})
      intparams.update({"LapseType":0})
    elif args.scenario=="dia_gauge":
      solverconstants+= "static constexpr int Scenario=3; /* Diagonal Gauge wave */ \n "
      userinfo.append(("picking diagonal gauge wave scenario",None))
      floatparams.update({"sk":0.0}); floatparams.update({"bs":0.0})
      intparams.update({"LapseType":0})
    elif args.scenario=="linear":
      solverconstants+= "static constexpr int Scenario=1; /* Linear wave */ \n "
      userinfo.append(("picking linear wave scenario",None))
      floatparams.update({"sk":0.0}); floatparams.update({"bs":0.0})
      intparams.update({"LapseType":0})
    elif args.scenario=="flat":
      solverconstants+= "static constexpr int Scenario=4; /* flat spacetime */ \n "
      userinfo.append(("picking flat scenario",None))
    elif  (args.scenario=="two-punctures") or (args.scenario=="single-puncture"):
      solverconstants+= "static constexpr int Scenario=2; /* Two-puncture */ \n"
      userinfo.append(("picking black hole scenario",None))
    else:
      raise Exception( "Scenario " + args.scenario + " is now unknown")  

    for k, v in floatparams.items(): solverconstants+= "static constexpr double {} = {};\n".format("CCZ4{}".format(k), v)
    for k, v in intparams.items():   solverconstants+= "static constexpr int {} = {};\n".format("CCZ4{}".format(k), v)
    my_solver.add_solver_constants(solverconstants)

    project.add_solver(my_solver)

    build_mode = modes[args.mode]
    
    dimensions = 3

########################################################################################
#Domain settings
########################################################################################
    floatparams.update({"domain_r":args.CCZ4domain_r})
    dr=floatparams["domain_r"]
    offset=[-dr, -dr, -dr]; domain_size=[2*dr, 2*dr, 2*dr]
    msg = "domain set as "+str(offset)+" and "+str(domain_size)
    print(msg)
    userinfo.append((msg,None))

    project.set_global_simulation_parameters(
      dimensions,               # dimensions
      offset,  domain_size,
      args.end_time,                 # end time
      args.plot_start_time, args.plot_step_size,   # snapshots
      periodic_boundary_conditions,
      8  # plotter precision
    )
    userinfo.append(("plot start time: "+str(args.plot_start_time)+", plot step size: "+str(args.plot_step_size),None))
    userinfo.append(("Terminal time: "+str(args.end_time),None))

    project.set_Peano4_installation("../../..", build_mode)

########################################################################################
#output dir and proble
########################################################################################
    path="./"
    if not args.path=="./":
        path=args.path 
    #path="/cosma5/data/durham/dc-zhan3/bbh-c5-1"
    #path="/cosma6/data/dp004/dc-zhan3/exahype2/sbh-fv3"
    project.set_output_path(path)
    probe_point = [-100, -100, 0.0]
    project.add_plot_filter( probe_point,[200.0, 200.0, 0.01], 1 )
    #if args.extension=="Psi4":
    #  my_solver.select_dofs_to_print = [0,12,16,17,18,53,54,59,60]
    #elif args.extension=="adm" or args.extension=="phy-debug":
    #  pass
    #else:
    #  pass#my_solver.select_dofs_to_print = [0,12,16,17,18,53,54]
   

    #project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.95,0,32))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOut","(new ::exahype2::LoadBalancingConfiguration(0.95,0,32))" )
    project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutHierarchically","(new ::exahype2::LoadBalancingConfiguration(0.95))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates","(new ::exahype2::LoadBalancingConfiguration(0.95,0,128,128))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::SplitOversizedTree","(new ::exahype2::LoadBalancingConfiguration(0.95,0,32))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::cascade::SpreadOut_RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.95,0,32))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::cascade::SpreadOut_SplitOversizedTree",
    #  "new ::exahype2::LoadBalancingConfiguration(0.95, 32 ,::exahype2::LoadBalancingConfiguration::UseNumberOfThreads), new toolbox::loadbalancing::metrics::CellCount()",)

########################################################################################
#Tracer setting 
########################################################################################
    if args.add_tracer==1:
        tracer_particles = project.add_tracer( name="Tracer",attribute_count=61 )
        init_action_set =  exahype2.tracer.InsertParticlesByCoordinates( 
            particle_set=tracer_particles, coordinates=[[0,0,8.0],[0,0,-8.0],[8.0,8.0,0],[-8.0,-8.0,0]])
        #    particle_set=tracer_particles, coordinates=[[-0.1,0,0],[0.1,0,0],[0,0.1,0],[0,-0.1,0]])
        #init_action_set = exahype2.tracer.InsertParticlesFromFile( 
        #                            particle_set=tracer_particles, filename="t-design.dat", scale_factor=abs(offset[0])*0.8)
        #"Gauss_Legendre_quadrature.dat" #"t-design.dat" 
        init_action_set.descend_invocation_order = 0
        project.add_action_set_to_initialisation( init_action_set )
        
        tracer_action_set=exahype2.tracer.FiniteVolumesTracing(tracer_particles,
                                                               my_solver,
                                                               project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear",
                                                               )
        tracer_action_set.descend_invocation_order = my_solver._action_set_update_cell.descend_invocation_order+1
        project.add_action_set_to_timestepping(tracer_action_set)
        project.add_action_set_to_initialisation(tracer_action_set)
        
        dump_action_set=exahype2.tracer.DumpTracerIntoDatabase(
          particle_set=tracer_particles,
          solver=my_solver,
          filename=args.path+"Tracer-test",
          number_of_entries_between_two_db_flushes=1000,
          output_precision=8,
          data_delta_between_two_snapsots=1e16,
          position_delta_between_two_snapsots=1e16,
          time_delta_between_two_snapsots=0.02
          )
        dump_action_set.descend_invocation_order = my_solver._action_set_update_cell.descend_invocation_order+2
        project.add_action_set_to_timestepping(dump_action_set)

    if args.add_tracer==2:
        tracer_particles = project.add_tracer( name="Tracer",attribute_count=59 )
        init_action_set = exahype2.tracer.InsertParticlesByCoordinates( 
            particle_set=tracer_particles, coordinates=[[0.2,0,0],[-0.2,0,0]])
        init_action_set.descend_invocation_order = 0
        project.add_action_set_to_initialisation( init_action_set )
        
        tracer_action_set=exahype2.tracer.FiniteVolumesTracing(tracer_particles,
                                                               my_solver,
                                                               project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear_explicit_Euler",
                                                               projection_kernel_arguments="""marker,
                                                                {{PATCH_SIZE}},
                                                                {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
                                                                fineGridCell{{SOLVER_NAME}}Q.value,
                                                                fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
                                                                {17,18,19},
                                                                {-1,-1,-1},
                                                                *p
                                                               """
                                                               )
        tracer_action_set.descend_invocation_order = my_solver._action_set_compute_final_linear_combination.descend_invocation_order+1
        project.add_action_set_to_timestepping(tracer_action_set)
        project.add_action_set_to_initialisation(tracer_action_set)
        
        dump_action_set=exahype2.tracer.DumpTracerIntoDatabase(
          particle_set=tracer_particles,
          solver=my_solver,
          filename=args.path+"Tracer-BHTracker",
          number_of_entries_between_two_db_flushes=1000,
          output_precision=8,
          data_delta_between_two_snapsots=1e16,
          position_delta_between_two_snapsots=1e16,
          time_delta_between_two_snapsots=0.02,
          clear_database_after_flush      = False
          )
        dump_action_set.descend_invocation_order = my_solver._action_set_compute_final_linear_combination.descend_invocation_order+2
        project.add_action_set_to_timestepping(dump_action_set)

########################################################################################
#linking stuff
########################################################################################
    peano4_project = project.generate_Peano4_project(verbose=True)

    if args.scenario=="gauge" or args.scenario=="linear" or args.scenario=="dia_gauge" or args.scenario=="flat":
      pass
    elif args.scenario=="two-punctures" or args.scenario=="single-puncture":
      #
      # There are two different things to do when we pick a scneario: We have
      # to configure the solver accordingly (and keep in mind that every solver
      # needs its own config), and then we might have to adopt the build
      # environment.
      #
      peano4_project.output.makefile.add_linker_flag( "-lm -lgsl -lgslcblas" )
      peano4_project.output.makefile.add_cpp_file( "libtwopunctures/TP_Utilities.cpp" )
      peano4_project.output.makefile.add_cpp_file( "libtwopunctures/TP_Parameters.cpp" )
      peano4_project.output.makefile.add_cpp_file( "libtwopunctures/TP_Logging.cpp" )
      peano4_project.output.makefile.add_cpp_file( "libtwopunctures/TwoPunctures.cpp" )
      peano4_project.output.makefile.add_cpp_file( "libtwopunctures/CoordTransf.cpp" )
      peano4_project.output.makefile.add_cpp_file( "libtwopunctures/Equations.cpp" )
      peano4_project.output.makefile.add_cpp_file( "libtwopunctures/FuncAndJacobian.cpp" )
      peano4_project.output.makefile.add_cpp_file( "libtwopunctures/Newton.cpp" )
      peano4_project.output.makefile.add_CXX_flag( "-DIncludeTwoPunctures" )
    else:
      raise Exception( "Scenario " + args.scenario + " is now unknown")

    peano4_project.output.makefile.add_CXX_flag( "-DCCZ4EINSTEIN" )
    peano4_project.output.makefile.add_cpp_file( "InitialValues.cpp" )
    peano4_project.output.makefile.add_cpp_file( "CCZ4Kernels.cpp" )

    if args.SO_flag==True:
      userinfo.append(("Enable double communication, make sure you are using the Second formulism.", None))
      additional_mesh_traversal = peano4.solversteps.Step( name = "AdditionalMeshTraversal",
                                                           add_user_defined_actions=False,
                                                           )
      project_patch_onto_faces  = exahype2.solvers.rkfd.actionsets.ProjectPatchOntoFaces(my_solver)
      project_patch_onto_faces.guards     = [ "false" for x in range(0,my_solver.number_of_Runge_Kutta_steps()+1) ]
      project_patch_onto_faces.guards[-1] = my_solver._store_cell_data_default_guard()

      roll_over_projected_faces = exahype2.solvers.rkfd.actionsets.RollOverUpdatedFace(my_solver, 
                                                                                       my_solver._store_face_data_default_guard(),
                                                                                       )

      dynamic_AMR = exahype2.solvers.rkfd.actionsets.DynamicAMR(
            solver=my_solver,
            interpolation=my_solver.interpolation,
            restriction=my_solver.restriction,
        )

      additional_mesh_traversal.add_action_set( ComputeFirstDerivativesFD4RK(solver=my_solver,
                                                                             is_enclave_solver = ("enclave" in args.implementation),
                                                                             ))
      additional_mesh_traversal.add_action_set( project_patch_onto_faces )
      additional_mesh_traversal.add_action_set( roll_over_projected_faces )
      additional_mesh_traversal.add_action_set( dynamic_AMR )
      
      project.init_new_user_defined_algorithmic_step( additional_mesh_traversal )
      peano4_project.solversteps.add_step( additional_mesh_traversal )

      peano4_project.constants.define( "USE_ADDITIONAL_MESH_TRAVERSAL" )

    peano4_project.generate( throw_away_data_after_generation=False )
    peano4_project.build( make_clean_first = True )

    #Report the application information
    userinfo.append(("the executable file name: "+exe, None))
    userinfo.append(("output directory: "+path, None))
    print("=========================================================")
    if not args.add_tracer==0:
        userinfo.append(("tracer output file: "+args.path+"Tracer-test", None))
    if len(userinfo) >0:
        print("The building information:")
        for msg, exception in userinfo:
            if exception is None:
                print(msg)
            else: print(msg, "Exception: {}".format(exception))
    print(intparams)
    print(floatparams)
    print("=========================================================")
