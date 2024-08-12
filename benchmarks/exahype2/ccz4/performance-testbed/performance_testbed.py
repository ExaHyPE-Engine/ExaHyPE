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

modes = { 
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts, "stats":  peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

floatparams = {
        "GLMc0":1.5, "GLMc":1.2, "GLMd":2.0, "GLMepsA":1.0, "GLMepsP":1.0,
        "GLMepsD":1.0, 
	"itau":1.0, "k1":0.1, "k2":0.0, "k3":0.5, "eta":1.0,
        "f":0.75, "g":0.0, "xi":1.0, "e":1.0, "c":1.0, "mu":0.2, "ds":1.0,
        "sk":1.0, "bs":1.0,
        "domain_r":1.5, "smoothing":0.0, "KOSigma":8.0 #, \
	#"itau":1.0, "k1":0.1, "k2":0.0, "k3":0.5, "eta":1.0,
        #"f":1.0, "g":0.0, "xi":1.0, "e":1.0, "c":1.0, "mu":0.2, "ds":1.0,
        #"sk":1.0, "bs":1.0#, \
	#"par_b":666.0, "center_offset_x":-1.0, "center_offset_y":0.0, "center_offset_z":0.0, \
	#"target_m_plus":1.0, "par_p_plus_x":0.0, "par_p_plus_y":0.0, "par_p_plus_z":0.0, \
	#"par_s_plus_x":0.0, "par_s_plus_y":0.0, "par_s_plus_z":0.0, \
	#"target_m_minus":1.0, "par_p_minus_x":0.0, "par_p_minus_y":0.0, "par_p_minus_z":0.0, \
	#"par_s_minus_x":0.0, "par_s_minus_y":0.0, "par_s_minus_z":0.0, \
	#"tp_epsilon":1e-6
}

intparams = {"BBHType":2, "LapseType":1, "tp_grid_setup":0, "swi":99, "ReSwi":1, "SO":0}
#sss
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ExaHyPE 2 - CCZ4 script')
    parser.add_argument("-maxh",   "--max-h",       dest="max_h",           type=float, default=0.05,  help="upper limit for refinement. Refers to volume size, i.e. not to patch size" )
    parser.add_argument("-minh",   "--min-h",       dest="min_h",           type=float, default=0.02,  help="lower limit for refinement (set to 0 to make it equal to max_h - default). Refers to volume size, i.e. not to patch size" )
    parser.add_argument("-ps",   "--patch-size",      dest="patch_size",      type=int, default=9,    help="Patch size, i.e. number of volumes per patch per direction" )
    parser.add_argument("-plt",  "--plot-step-size",  dest="plot_step_size",  type=float, default=0, help="Plot step size (0 to switch it off)" )
    parser.add_argument("-m",    "--mode",            dest="mode",            default="release",  help="|".join(modes.keys()) )
    parser.add_argument( "--gpu",            dest="GPU",            default=False, action="store_true",  help="Run with accelerator support" )
    parser.add_argument("-ext",  "--extension",       dest="extension",       choices=["none", "adm", "Psi4","phy-debug", "PT"],   default="none",  help="Pick extension, i.e. what should be plotted on top. Default is none" )
    parser.add_argument("-impl", "--implementation",  dest="implementation",  default="fd4-rk1-adaptive", choices=["fd4-rk1-adaptive", "fd4-rk1-adaptive-enclave"], help="Pick solver type" )
    parser.add_argument("-no-pbc",  "--no-periodic-boundary-conditions",      dest="periodic_bc", action="store_false", default=True,  help="switch on or off the periodic BC" )
    parser.add_argument("-sommerfeld",  "--sommerfeld-boundary-conditions",      dest="sommerfeld_bc", action="store_true", default=False,  help="switch on or off the Sommerfeld radiative BC" )
    parser.add_argument("-AMRMarker",  "--AMR-marker",      dest="marker", choices=["none", "poisson"],   default="none",  help="switch on or off the AMR boundary marker" )
    parser.add_argument("-et",   "--end-time",        dest="end_time",        type=float, default=0.01, help="End (terminal) time" )
    parser.add_argument("-pst",   "--plot-start-time",        dest="plot_start_time",  type=float, default=0.0, help="start time for plot" )
    parser.add_argument("-s",    "--scenario",        dest="scenario",      default="single-puncture",  choices=["gauge", "dia_gauge", "linear", "single-puncture","two-punctures", "flat"], help="Scenario" )
    parser.add_argument("-cfl",      "--CFL-ratio",           dest="cfl",             type=float, default=0.1, help="Set CFL ratio" )
    parser.add_argument("-tracer", "--add-tracer",    dest="add_tracer", type=int, default=0,  help="Add tracers and specify the seeds. 0-switch off, 1-static point tracer, 2-moving point tracer" )
    parser.add_argument("-tn", "--tracer-name",       dest="tra_name",    type=str, default="de",  help="name of output tracer file (temporary)" )
    parser.add_argument("-exn", "--exe-name",        dest="exe_name",    type=str, default="test",  help="name of output executable file" )
    parser.add_argument("-outdir", "--output-directory",        dest="path",    type=str, default="./",  help="specify the output directory, include the patch file and tracer file" )
    parser.add_argument("-interp",   "--interpolation", dest="interpolation",     choices=["constant","order-2","linear-constant-extrap","linear-linear-extrap","linear-con-extrap-lin-normal-interp","linear-lin-extrap-lin-normal-interp"], default="linear-lin-extrap-lin-normal-interp",  help="interpolation scheme for AMR" )
    parser.add_argument("-restrict", "--restriction",   dest="restriction",       choices=["average", "inject" ], default="average",  help="restriction scheme for AMR" )
    parser.add_argument("-so", "--second-order",  dest="SO_flag",           default=True, action="store_true",  help="enable double communication per timestep, used in the soccz4 formulation.")


    for k, v in floatparams.items(): parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=float, default=v, help="default: %(default)s")
    for k, v in intparams.items():
      if k=="ReSwi":
        parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=int, default=v, help="default: %(default)s, choose refinement criterion, 0-no refinement, 1-radius based, 2-SBH phi gradient based, 3-BBH phi gradient based. Notice: 2 and 3 only work with -ext Full")
      else: parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=int, default=v, help="default: %(default)s")

    args = parser.parse_args()

    SuperClass = None

    print(args.implementation)
    if args.implementation=="fd4-rk1-adaptive" or args.implementation=="fd4-rk4-adaptive":
       SuperClass = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep
    if args.implementation=="fd4-rk1-adaptive-enclave":
       SuperClass = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking
    if args.implementation=="fd4-rk1-fixed" or args.implementation=="fd4-rk4-fixed":
       SuperClass = exahype2.solvers.rkfd.fd4.GlobalFixedTimeStep


    class CCZ4Solver( SuperClass ):
      def __init__(self, name, patch_size, min_volume_h, max_volume_h, cfl, domain_r, KOSig):
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
        if SuperClass==exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep:
          if args.implementation=="fd4-rk1-adaptive" or args.implementation=="fd4-rk1-adaptive-enclave":
            rk_order = 1
          else:
            rk_order = 4
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size, rk_order=rk_order,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_meshcell_h=min_volume_h, max_meshcell_h=max_volume_h,
            time_step_relaxation=cfl,
            KOSigma=KOSig
          )
        elif SuperClass==exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking:
          if args.implementation=="fd4-rk1-adaptive" or args.implementation=="fd4-rk1-adaptive-enclave":
            rk_order = 1
          else:
            rk_order = 4
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size, rk_order=rk_order,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_meshcell_h=min_volume_h, max_meshcell_h=max_volume_h,
            time_step_relaxation=cfl,
            KOSigma=KOSig,
            pde_terms_without_state=True
          )
        elif SuperClass==exahype2.solvers.rkfd.fd4.GlobalFixedTimeStep or SuperClass==exahype2.solvers.rkfd.fd4.GlobalFixedTimeStepWithEnclaveTasking:
          if args.implementation=="fd4-rk1-fixed":
            rk_order = 1
          else:
            rk_order = 4
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size, rk_order=rk_order,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_meshcell_h=min_volume_h, max_meshcell_h=max_volume_h,
            normalised_time_step_size=0.01,
            KOSigma=KOSig
          )
        elif SuperClass == exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep:
          SuperClass.__init__(
            self,
            name=name,
            rk_order     = 2,
            polynomials  = exahype2.solvers.GaussLegendreBasis(2),
            face_projections   = exahype2.solvers.rkdg.FaceProjections.Solution,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_cell_h=min_volume_h, max_cell_h=max_volume_h, #not a good name, but use for now.
            time_step_relaxation=cfl
          )
        else:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_volume_h=min_volume_h, max_volume_h=max_volume_h,
            time_step_relaxation=cfl
            #use_gpu =args.GPU #=="fv-fixed-gpu" else False
#                        use_gpu = True if args.implementation=="fv-adaptive-gpu" else False
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

        self.postprocess_updated_patch = """
{
"""
        
        if (
            SuperClass==exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep 
            or SuperClass==exahype2.solvers.rkfd.fd4.GlobalFixedTimeStep
            or SuperClass==exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking
            #or SuperClass==exahype2.solvers.rkfd.fd4.GlobalFixedTimeStepWithEnclaveTasking no fixed enclave version yet
            ):
          self.postprocess_updated_patch += """
    #if Dimensions==2
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    #endif

    #if Dimensions==3
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    #endif
"""
        #elif SuperClass == exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep: fix it after rkdg is back
        #  self.postprocess_updated_patch = """
    #constexpr int itmax = ({{ORDER}}+1) * ({{ORDER}}+1) * ({{ORDER}}+1); // only support 3d
#"""
        else:
          self.postprocess_updated_patch += """
    #if Dimensions==2
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    #endif

    #if Dimensions==3
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    #endif
"""

        self.postprocess_updated_patch += """
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""

   
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
    project = exahype2.Project( ["benchmarks", "exahype2", "ccz4"], "ccz4", executable=exe)

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
        #msg = "Warning: ADER-DG no supported on this machine"
        #print(msg)
        #userinfo.append((msg,e))

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
        
    if args.interpolation=="order-2":
      my_solver.overlap=2

    if args.interpolation=="constant":
      my_solver.interpolation = "piecewise_constant"
      print( "Interpolation rule: piecewise_constant" )
    if args.interpolation=="linear-constant-extrap":
      my_solver.interpolation = "linear_with_constant_extrapolation" 
      print( "Interpolation rule: linear constant extrapolation" )
    if args.interpolation=="linear-linear-extrap":
      my_solver.interpolation = "linear_with_linear_extrapolation"
      print( "Interpolation rule: linear extrapolation" )
    if args.interpolation=="linear-con-extrap-lin-normal-interp":
      my_solver.interpolation = "linear_with_constant_extrapolation_and_linear_normal_interpolation"
      print( "Interpolation rule: linear+constant extrapolation and linear normal interpolation" )

    if args.interpolation=="order-2":
      my_solver.interpolation = "linear" 

    tem_interp=["TP_constant","TP_linear_with_linear_extrap_normal_interp"]
    tem_restrict=["TP_inject_normal_extrap","TP_average_normal_extrap"]
    if ("fd4-rk" in args.implementation):
      exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_interpolation( my_solver, tem_interp[1] ) 
      exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_restriction(   my_solver, tem_restrict[1] ) 
      userinfo.append(("FD4 Interpolation: " + tem_interp[1] + " & Restriction: " + tem_restrict[1], None))

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

    solverconstants=""

    if args.scenario=="gauge":
      solverconstants+= "static constexpr int Scenario=0; /* Gauge wave */ \n "
      userinfo.append(("picking gauge wave scenario",None))
      floatparams.update({"sk":0.0}); floatparams.update({"bs":0.0})
      intparams.update({"LapseType":0})
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

    project.set_Peano4_installation("../../../..", build_mode)

########################################################################################
#output dir and proble
########################################################################################
    path="./"
    if not args.path=="./":
        path=args.path 
    #path="/cosma5/data/durham/dc-zhan3/bbh-c5-1"
    #path="/cosma6/data/dp004/dc-zhan3/exahype2/sbh-fv3"
    project.set_output_path(path)
    probe_point = [-12,-12,0.0]
    project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )
    if args.extension=="Psi4":
      my_solver.select_dofs_to_print = [0,12,16,17,18,53,54,59,60]
    elif args.extension=="adm" or args.extension=="phy-debug":
      pass
    else:
      pass#my_solver.select_dofs_to_print = [0,12,16,17,18,53,54]
   
    #project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.95))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::SplitOversizedTree","(new ::exahype2::LoadBalancingConfiguration(0.95,500,16))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOut","(new ::exahype2::LoadBalancingConfiguration(0.95))" )
    project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutHierarchically","(new ::exahype2::LoadBalancingConfiguration(0.95,1000,16))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates","(new ::exahype2::LoadBalancingConfiguration(0.98,1000,16))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates","(new ::exahype2::LoadBalancingConfiguration(0.95))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::cascade::SpreadOut_RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.95))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::cascade::SpreadOutOnceGridStagnates_SplitOversizedTree","(new ::exahype2::LoadBalancingConfiguration(0.95))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::cascade::SpreadOut_SplitOversizedTree",
    #    "new ::exahype2::LoadBalancingConfiguration(0.98, 1, 8,::exahype2::LoadBalancingConfiguration::UseNumberOfThreads), new toolbox::loadbalancing::metrics::CellCount()",)

########################################################################################
#linking stuff
########################################################################################
    peano4_project = project.generate_Peano4_project(verbose=True)
    peano4_project.output.makefile.add_header_search_path("../../../../applications/exahype2/ccz4/")

    if args.scenario=="gauge" or args.scenario=="linear" or args.scenario=="dia_gauge" or args.scenario=="flat":
      pass
    elif args.scenario=="two-punctures" or args.scenario=="single-puncture":
      peano4_project.output.makefile.add_linker_flag( "-lm -lgsl -lgslcblas" )
      peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Utilities.cpp" )
      peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Parameters.cpp" )
      peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Logging.cpp" )
      peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/TwoPunctures.cpp" )
      peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/CoordTransf.cpp" )
      peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/Equations.cpp" )
      peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/FuncAndJacobian.cpp" )
      peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/libtwopunctures/Newton.cpp" )
      peano4_project.output.makefile.add_CXX_flag( "-DIncludeTwoPunctures" )
    else:
      raise Exception( "Scenario " + args.scenario + " is now unknown")

    peano4_project.output.makefile.add_CXX_flag( "-DCCZ4EINSTEIN" )

    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/InitialValues.cpp" )
    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp" )

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
