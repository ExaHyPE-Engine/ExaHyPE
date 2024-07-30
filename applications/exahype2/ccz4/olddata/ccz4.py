import os
import argparse

import peano4
import exahype2

import peano4.toolbox.particles
import dastgen2

from Probe_file_gene import tracer_seeds_generate

from peano4.toolbox.blockstructured.DynamicAMR                 import DynamicAMR
import numpy as np
#todo no mpmath here so rkdg not working

def CoordListFromFile(file,scale):
  f=open(file)
  data=[]
  tem=f.readlines()
  for line in tem:
    data.append(list(map(float,line.split(' ')[1:])))
  print(len(data))
  data=np.array(data)
  return data*scale


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
        "domain_r":0.5, "smoothing":0.0, "KOSigma":4.0 #, \
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

intparams = {"BBHType":2, "LapseType":1, "tp_grid_setup":0, "swi":99, "ReSwi":0, "source":0}
#sss
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ExaHyPE 2 - CCZ4 script')
    parser.add_argument("-maxh",   "--max-h",       dest="max_h",           type=float, required="True",  help="upper limit for refinement. Refers to volume size, i.e. not to patch size" )
    parser.add_argument("-minh",   "--min-h",       dest="min_h",           type=float, default=0,  help="lower limit for refinement (set to 0 to make it equal to max_h - default). Refers to volume size, i.e. not to patch size" )
    parser.add_argument("-ps",   "--patch-size",      dest="patch_size",      type=int, default=6,    help="Patch size, i.e. number of volumes per patch per direction" )
    parser.add_argument("-plt",  "--plot-step-size",  dest="plot_step_size",  type=float, default=0.04, help="Plot step size (0 to switch it off)" )
    parser.add_argument("-m",    "--mode",            dest="mode",            default="release",  help="|".join(modes.keys()) )
    parser.add_argument( "--gpu",            dest="GPU",            default=False, action="store_true",  help="Run with accelerator support" )
    parser.add_argument("-ext",  "--extension",       dest="extension",       choices=["none", "adm", "Psi4","phy-debug", "PT"],   default="none",  help="Pick extension, i.e. what should be plotted on top. Default is none" )
    parser.add_argument("-impl", "--implementation",  dest="implementation",  choices=["fv-global-fixed", "fv-global-adaptive", "fv-global-fixed-enclave", "fv-global-adaptive-enclave","fv-subcyling-adaptive-enclave","fv-local-enclave", "fv-musclhancock", "fd4-rk1-adaptive", "fd4-rk1-adaptive-enclave", "fd4-rk1-fixed", "fd4-rk4-adaptive", "fd4-rk4-fixed", "RKDG"], required="True",  help="Pick solver type" )
    parser.add_argument("-no-pbc",  "--no-periodic-boundary-conditions",      dest="periodic_bc", action="store_false", default=True,  help="switch on or off the periodic BC" )
    parser.add_argument("-sommerfeld",  "--sommerfeld-boundary-conditions",      dest="sommerfeld_bc", action="store_true", default=False,  help="switch on or off the Sommerfeld radiative BC" )
    parser.add_argument("-AMRMarker",  "--AMR-marker",      dest="marker", choices=["none", "poisson"],   default="none",  help="switch on or off the AMR boundary marker" )
    parser.add_argument("-et",   "--end-time",        dest="end_time",        type=float, default=1.0, help="End (terminal) time" )
    parser.add_argument("-pst",   "--plot-start-time",        dest="plot_start_time",        type=float, default=0.0, help="start time for plot" )
    parser.add_argument("-s",    "--scenario",        dest="scenario",        choices=["gauge", "dia_gauge", "linear", "single-puncture","two-punctures", "flat"], required="True", help="Scenario" )
    parser.add_argument("-cfl",      "--CFL-ratio",           dest="cfl",             type=float, default=0.1, help="Set CFL ratio" )
    parser.add_argument("-tracer", "--add-tracer",    dest="add_tracer", type=int, default=0,  help="Add tracers and specify the seeds. 0-switch off, 1-static point tracer, 2-moving point tracer" )
    parser.add_argument("-tn", "--tracer-name",       dest="tra_name",    type=str, default="de",  help="name of output tracer file (temporary)" )
    parser.add_argument("-exn", "--exe-name",        dest="exe_name",    type=str, default="",  help="name of output executable file" )
    parser.add_argument("-outdir", "--output-directory",        dest="path",    type=str, default="./",  help="specify the output directory, include the patch file and tracer file" )
    parser.add_argument("-interp",   "--interpolation", dest="interpolation",     choices=["constant","order-2","linear-constant-extrap","linear-linear-extrap","linear-con-extrap-lin-normal-interp","linear-lin-extrap-lin-normal-interp"], default="linear-lin-extrap-lin-normal-interp",  help="interpolation scheme for AMR" )
    parser.add_argument("-restrict", "--restriction",   dest="restriction",       choices=["average", "inject" ], default="average",  help="restriction scheme for AMR" )


    for k, v in floatparams.items(): parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=float, default=v, help="default: %(default)s")
    for k, v in intparams.items():
      if k=="ReSwi":
        parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=int, default=v, help="default: %(default)s, choose refinement criterion, 0-no refinement, 1-radius based, 2-SBH phi gradient based, 3-BBH phi gradient based. Notice: 2 and 3 only work with -ext Full")
      else: parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=int, default=v, help="default: %(default)s")

    args = parser.parse_args()

    SuperClass = None

    print(args.implementation)
    if args.implementation=="fv-global-fixed":
       SuperClass = exahype2.solvers.fv.rusanov.GlobalFixedTimeStep
    if args.implementation=="fv-global-adaptive":
       SuperClass = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep
    if args.implementation=="fv-global-fixed-enclave":
       SuperClass = exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking
    if args.implementation=="fv-global-adaptive-enclave":
       SuperClass = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking
    if args.implementation=="fv-subcyling-adaptive-enclave": 
       SuperClass = exahype2.solvers.fv.rusanov.SubcyclingAdaptiveTimeStepWithEnclaveTasking
    if args.implementation=="fv-local-enclave":
       SuperClass = exahype2.solvers.fv.rusanov.LocalTimeStepWithEnclaveTasking
    if args.implementation=="fv-musclhancock":
       SuperClass = exahype2.solvers.fv.musclhancock.GlobalAdaptiveTimeStep
    if args.implementation=="fd4-rk1-adaptive" or args.implementation=="fd4-rk4-adaptive":
       SuperClass = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep
    if args.implementation=="fd4-rk1-adaptive-enclave":
       SuperClass = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking
    if args.implementation=="fd4-rk1-fixed" or args.implementation=="fd4-rk4-fixed":
       SuperClass = exahype2.solvers.rkfd.fd4.GlobalFixedTimeStep
    if args.implementation=="RKDG":
       SuperClass = exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking


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

        if SuperClass==exahype2.solvers.fv.rusanov.GlobalFixedTimeStep:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_volume_h=min_volume_h, max_volume_h=max_volume_h,
            normalised_time_step_size=1e-2
          )
        elif SuperClass==exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_volume_h=min_volume_h, max_volume_h=max_volume_h,
            time_step_size=1e-2,
            use_gpu =args.GPU #=="fv-fixed-gpu" else False
          )
        elif SuperClass==exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_volume_h=min_volume_h, max_volume_h=max_volume_h,
            time_step_relaxation=cfl
          )
        elif SuperClass==exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep:
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

          self.switch_to_heap_storage(True,True)

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

      def add_adm_constriants(self):
        """
          @add adm constriants (Hamilton and Momentum)
        """
        self._auxiliary_variables = 7

        self._my_user_includes += """
  #include "../CCZ4Kernels.h"
    """

        self._preprocess_reconstructed_patch = """ 
        const int patchSize = """ + str( self._patch.dim[0] ) + """;
        double volumeH = ::exahype2::fv::getVolumeLength(marker.h(),patchSize);

    const int n_a_v=7;
    const int overlap=3; //make sure you are using fd4 solver!

    //if (not marker.willBeRefined() and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()!=FiniteVolumeCCZ4::SolverState::GridConstruction and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()==FiniteVolumeCCZ4::SolverState::RungeKuttaSubStep0) {
    if (not marker.willBeRefined() and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()!=FiniteVolumeCCZ4::SolverState::GridConstruction) {
      dfor(cell,patchSize) {
        tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(overlap);

        double gradQ[3*59]={ 0 };

        for (int d=0; d<3; d++) {
          tarch::la::Vector<Dimensions,int> leftCell  = currentCell;
          tarch::la::Vector<Dimensions,int> rightCell = currentCell;
          leftCell(d)  -= 1;
          rightCell(d) += 1;
          const int leftCellSerialised  = peano4::utils::dLinearised(leftCell, patchSize + 2*overlap);
          const int rightCellSerialised = peano4::utils::dLinearised(rightCell,patchSize + 2*overlap);
          for(int i=0; i<59; i++) {
            gradQ[d*59+i] = ( oldQWithHalo[rightCellSerialised*(59+n_a_v)+i] - oldQWithHalo[leftCellSerialised*(59+n_a_v)+i] ) / 2.0 / volumeH;
          }
        }

        const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*overlap);
        
        double constraints[7]={0,0,0,0,0,0,0};
        admconstraints(constraints, oldQWithHalo+cellSerialised*(59+n_a_v), gradQ);
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables ( 1, patchSize, 0, 59, n_a_v);
        for (int i=0;i<7;i++){
          fineGridCellFiniteVolumeCCZ4Q.value[ enumeratorWithAuxiliaryVariables(0,cell,59+i) ] = constraints[i]; 
        }
      }  
    } 
    """

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

        self._preprocess_reconstructed_patch = """ 
        const int patchSize = """ + str( self._patch.dim[0] ) + """;
        double volumeH = ::exahype2::fv::getVolumeLength(marker.h(),patchSize);

    const int n_a_v=2;
    const int overlap=3; //make sure you are using fd4 solver!

    //if (not marker.willBeRefined() and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()!=FiniteVolumeCCZ4::SolverState::GridConstruction and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()==FiniteVolumeCCZ4::SolverState::RungeKuttaSubStep0) {
    if (not marker.willBeRefined() and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()!=FiniteVolumeCCZ4::SolverState::GridConstruction) {
      dfor(cell,patchSize) {
        tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(overlap);

        double gradQ[3*59]={ 0 };

        for (int d=0; d<3; d++) {
          tarch::la::Vector<Dimensions,int> leftCell  = currentCell;
          tarch::la::Vector<Dimensions,int> rightCell = currentCell;
          leftCell(d)  -= 1;
          rightCell(d) += 1;
          const int leftCellSerialised  = peano4::utils::dLinearised(leftCell, patchSize + 2*overlap);
          const int rightCellSerialised = peano4::utils::dLinearised(rightCell,patchSize + 2*overlap);
          for(int i=0; i<59; i++) {
            gradQ[d*59+i] = ( oldQWithHalo[rightCellSerialised*(59+n_a_v)+i] - oldQWithHalo[leftCellSerialised*(59+n_a_v)+i] ) / 2.0 / volumeH;
          }
        }

        const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*overlap);
        
        double Psi4[2]={0,0};
        double currentPosition[3]; 
        for (int d=0; d<3; d++) currentPosition[d]=marker.getOffset()(d)+(cell(d)+0.5)*volumeH;
        Psi4Calc(Psi4, oldQWithHalo+cellSerialised*(59+n_a_v), gradQ, currentPosition);
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables   ( 1, patchSize, 0, 59, n_a_v);
        fineGridCellFiniteVolumeCCZ4Q.value[ enumeratorWithAuxiliaryVariables(0,cell,59+0) ] = Psi4[0]; 
        fineGridCellFiniteVolumeCCZ4Q.value[ enumeratorWithAuxiliaryVariables(0,cell,59+1) ] = Psi4[1];
      }  
    } 
    """

        self.create_data_structures()
        self.create_action_sets()

      def add_intermediate_quantities(self):
        """
          @add adm intermediate quantites
        """
        self._auxiliary_variables = 15

        self._my_user_includes += """
  #include "../CCZ4Kernels.h"
    """

        self._preprocess_reconstructed_patch = """ 
        const int patchSize = """ + str( self._patch.dim[0] ) + """;
        double volumeH = ::exahype2::fv::getVolumeLength(marker.h(),patchSize);

    const int n_a_v=""" + str( self._auxiliary_variables ) + """;
    const int overlap=3; //make sure you are using fd4 solver!

    //if (not marker.willBeRefined() and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()!=FiniteVolumeCCZ4::SolverState::GridConstruction and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()==FiniteVolumeCCZ4::SolverState::RungeKuttaSubStep0) {
    if (not marker.willBeRefined() and repositories::InstanceOfFiniteVolumeCCZ4.getSolverState()!=FiniteVolumeCCZ4::SolverState::GridConstruction) {
      dfor(cell,patchSize) {
        tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(overlap);

        double gradQ[3*59]={ 0 };

        for (int d=0; d<3; d++) {
          tarch::la::Vector<Dimensions,int> leftCell  = currentCell;
          tarch::la::Vector<Dimensions,int> leftCell2  = currentCell;
          tarch::la::Vector<Dimensions,int> rightCell = currentCell;
          tarch::la::Vector<Dimensions,int> rightCell2 = currentCell;
          leftCell(d)  -= 1; leftCell2(d)  -= 2; 
          rightCell(d) += 1; rightCell2(d) += 2;
          const int leftCellSerialised  = peano4::utils::dLinearised(leftCell, patchSize + 2*overlap);
          const int rightCellSerialised = peano4::utils::dLinearised(rightCell,patchSize + 2*overlap);
          const int leftCellSerialised2  = peano4::utils::dLinearised(leftCell2, patchSize + 2*overlap);
          const int rightCellSerialised2 = peano4::utils::dLinearised(rightCell2,patchSize + 2*overlap);
          for(int i=0; i<59; i++) {
            gradQ[d*59+i] = ( (2.0/3.0)*oldQWithHalo[rightCellSerialised*(59+n_a_v)+i] - (2.0/3.0)*oldQWithHalo[leftCellSerialised*(59+n_a_v)+i]
                              +(1.0/12.0)*oldQWithHalo[leftCellSerialised2*(59+n_a_v)+i] - (1.0/12.0)*oldQWithHalo[rightCellSerialised2*(59+n_a_v)+i] )/volumeH;
          }
        }

        const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*overlap);
        
        double terms[""" + str( self._auxiliary_variables ) + """]={0,0,0,0,0,0,0};
        TestingOutput(terms, oldQWithHalo+cellSerialised*(59+n_a_v), gradQ);
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables ( 1, patchSize, 0, 59, n_a_v);
        for (int i=0;i<""" + str( self._auxiliary_variables ) + """;i++){
          fineGridCellFiniteVolumeCCZ4Q.value[ enumeratorWithAuxiliaryVariables(0,cell,59+i) ] = terms[i]; 
        }
      }  
    } 
    """

        self.create_data_structures()
        self.create_action_sets()

      def add_puncture_tracker(self):
        """
          @add 
        """
        self._auxiliary_variables = 0

        self._user_solver_includes += """
  #include "libtwopunctures/TP_PunctureTracker.h"
  #include "CCZ4Kernels.h"
  #include <fstream>
  #include <iomanip>
    """

#        self._user_action_set_includes += """
#  #include "../libtwopunctures/TP_PunctureTracker.h"
#  #include "../CCZ4Kernels.h"
#  #include <fstream>
#  #include <iomanip>
#    """

        self._preprocess_reconstructed_patch = """
        const int patchSize = """ + str( self._patch.dim[0] ) + """;
        double volumeH = ::exahype2::fv::getVolumeLength(marker.h(),patchSize);

    std::fstream fin;
    std::string att="_"""+args.exe_name+""".txt"; std::string p1="puncture1"; std::string p2="puncture2"; std::string tem="ztem";
    const int n_a_v=""" + str( self._auxiliary_variables ) + """;
    const double overlap=3;

    if (tarch::la::equals(timeStamp,0.0)){//initialization
      fin.open((p1+att),std::ios::out|std::ios::trunc);
      fin << "2.0 0.0 0.0 0.0 0.0" << std::endl;//4.461538
      fin.close();
      fin.open((p2+att),std::ios::out|std::ios::trunc);
      fin << "-2.0 0.0 0.0 0.0 0.0" << std::endl;//-5.538462
      fin.close();
      //fin.open((tem+att),std::ios::out|std::ios::trunc);
      //fin << "tem file" << std::endl;
      //fin.close();
    } else {
      fin.open((p1+att),std::ios::in);
      std::string pos=getLastLine(fin);
      fin.close();
      double coor1[5]={0};//read in previous coordinates
      CoorReadIn(coor1,pos);
      fin.open((p2+att),std::ios::in);
      std::string pos2=getLastLine(fin);
      fin.close();
      double coor2[5]={0};
      CoorReadIn(coor2,pos2);
      int inter_number=4;
      if (marker.isContained(coor1)){
        tarch::la::Vector<Dimensions*2,int> IndexOfCell=FindCellIndex(coor1,marker.getOffset(),volumeH,patchSize); //find where the puncture is
        tarch::la::Vector<Dimensions,int> IndexForInterpolate[8];
        FindInterIndex(IndexForInterpolate,IndexOfCell);//find the closest 8 cells
        double raw[32]={0};
        for (int i=0;i<8;i++){
          int Lindex=peano4::utils::dLinearised(IndexForInterpolate[i], patchSize + 2*overlap);
          for (int j=0;j<Dimensions;j++) {raw[i*inter_number+j]=oldQWithHalo[Lindex*(59+n_a_v)+17+j];} //read in corresponding beta
          raw[i*inter_number+3]=oldQWithHalo[Lindex*(59+n_a_v)+16];
            //std::cout << IndexForInterpolate[i](0) << " " << IndexForInterpolate[i](1) << " " << IndexForInterpolate[i](2) << std::endl;
            //std::cout << raw[i*inter_number+0] << " " << raw[i*inter_number+1] << " " << raw[i*inter_number+2] << " " << raw[i*inter_number+3] << std::endl;
            //::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables ( 1, patchSize, overlap, 59, n_a_v);
            //std::cout << oldQWithHalo[enumeratorWithAuxiliaryVariables(0,IndexForInterpolate[i],0)] << std::endl;
        }
        double result[inter_number];
        Interpolation(result,IndexForInterpolate,raw,coor1,marker.getOffset(),volumeH,patchSize);
        
        coor1[0]-=timeStepSize*result[0]; coor1[1]-=timeStepSize*result[1]; coor1[2]-=timeStepSize*result[2];//updates position
        fin.open((p1+att),std::ios::app);//output
        fin << std::setprecision (17) << coor1[0] << " " << coor1[1];
        fin << " " << coor1[2] << " " << timeStamp << " " << result[3] << std::endl;
        fin.close();
        //fin.open((tem+att),std::ios::app);
        //fin << "for cellindex" << IndexOfCell(0) << " " << IndexOfCell(1) << " " << IndexOfCell(2) << " " << IndexOfCell(3) << " " << IndexOfCell(4) << " " << IndexOfCell(5) << std::endl;
        //for (int i=0;i<8;i++){
        //fin << IndexForInterpolate[i](0) << " " << IndexForInterpolate[i](1) << " " << IndexForInterpolate[i](2) << std::endl;
        //fin << raw[i*inter_number+0] << " " << raw[i*inter_number+1] << " " << raw[i*inter_number+2] << " " << raw[i*inter_number+3] << std::endl;
        //}

        //fin << "after inter" << result[0] << " " << result[1] << " " << result[2] << " " << result[3] << " " << timeStamp << std::endl;
        //fin.close();
      }
      if (marker.isContained(coor2)){//do the same for the second puncutre
        tarch::la::Vector<Dimensions*2,int> IndexOfCell=FindCellIndex(coor2,marker.getOffset(),volumeH,patchSize);
        tarch::la::Vector<Dimensions,int> IndexForInterpolate[8];
        FindInterIndex(IndexForInterpolate,IndexOfCell);
        double raw[8*inter_number];
        for (int i=0;i<8;i++){
          int Lindex=peano4::utils::dLinearised(IndexForInterpolate[i], patchSize + 2*overlap);
          for (int j=0;j<Dimensions;j++) {raw[i*inter_number+j]=oldQWithHalo[Lindex*(59+n_a_v)+17+j];}
          raw[i*inter_number+3]=oldQWithHalo[Lindex*(59+n_a_v)+16];
        }
        double result[inter_number];
        Interpolation(result,IndexForInterpolate,raw,coor2,marker.getOffset(),volumeH,patchSize);
        
        coor2[0]-=timeStepSize*result[0]; coor2[1]-=timeStepSize*result[1]; coor2[2]-=timeStepSize*result[2];
        fin.open((p2+att),std::ios::app);
        fin << std::setprecision (17) << coor2[0] << " " << coor2[1];
        fin << " " << coor2[2] << " " << timeStamp << " " << result[3] << std::endl;
        fin.close();
      }
    }
    """

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
      solver_name    = "FiniteVolume" + solver_name


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
#AMR boundary tracing
########################################################################################
    print(args.marker)
    if args.marker=="poisson":
      amr_label = exahype2.solvers.elliptic.AMRMarker( name="AMRMarker" )
      project.add_solver( amr_label )
      my_solver._auxiliary_variables+=1
      print( "Add Poisson AMR marker for variable {}".format(my_solver._auxiliary_variables-1) )
      amr_label.couple_with_FV_solver( my_solver, my_solver._auxiliary_variables-1 )

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
    #if args.interpolation=="linear+enforce" or args.interpolation=="linear-slow+enforce":
    #  self.point_wise_postprocessing_of_interpolation = "examples::exahype2::ccz4::enforceCCZ4constraints(targetVolume)" )
    #if args.restriction=="average+enforce" or args.restriction=="inject+enforce":
    #  self.point_wise_postprocessing_of_restriction = "examples::exahype2::ccz4::enforceCCZ4constraints(targetVolume)"



########################################################################################
#Add postpocessing function
########################################################################################
    if args.extension=="adm":
      my_solver.add_adm_constriants()
    if args.extension=="Psi4":
      my_solver.add_Psi4W()    
    if args.extension=="phy-debug":
      my_solver.add_intermediate_quantities()
    if args.extension=="PT":
      my_solver.add_puncture_tracker()

      #my_solver._auxiliary_variables = 59*3

########################################################################################
#parameter setting according to scenarios
########################################################################################
    for k, v in intparams.items():
      intparams.update({k:eval("args.CCZ4{}".format(k))})
    for k, v in floatparams.items():
      floatparams.update({k:eval("args.CCZ4{}".format(k))})

    if args.scenario=="two-punctures":
      msg = "Periodic BC deactivated because you pick Puncture scenario\nInitialize binary black holes"
      print(msg)
      periodic_boundary_conditions = [False,False,False]
      intparams.update({"swi":2})  #notice it may change, see according function in FiniteVolumeCCZ4.cpp
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
      if args.scenario=="single-puncture":  #please notice the default black hole mass (1.0) is hard-coded. need to be modified accordingly if necessary.
        my_solver._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = """
      ::exahype2::fd::applySommerfeldConditions(
        [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, gridCellH, t, dt, normal );
        },
        [&](
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH
        ) -> void {
          for (int i=0; i<"""+str(my_solver._unknowns+my_solver._auxiliary_variables)+"""; i++) {
            Q[i] = 0.0;
          }
          Q[0] = 1.0; Q[3] = 1.0; Q[5] = 1.0;
          //const double r=tarch::la::norm2(faceCentre);
          //Q[16] = 0.5*(1+(1-1.0/2/r)/(1+1.0/2/r)); 
          //Q[54] = 1/(1+1.0/2/r)/(1+1.0/2/r);
          Q[16] = 1.0; Q[54] = 1.0;
        },
        marker.x(),
        marker.h(),
        {{FACE_METADATA_ACCESSOR}}.getOldTimeStamp(marker.getSelectedFaceNumber()<Dimensions ? 1 : 0),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        {{OVERLAP}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        {0.0, 0.0, 0.0},
        fineGridFace{{UNKNOWN_IDENTIFIER}}Old.value,
        fineGridFace{{UNKNOWN_IDENTIFIER}}New.value
      );
        """
      else:
        my_solver._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = """
      ::exahype2::fd::applySommerfeldConditions(
        [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, gridCellH, t, dt, normal );
        },
        [&](
          double * __restrict__                        Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH
        ) -> void {
          for (int i=0; i<"""+str(my_solver._unknowns+my_solver._auxiliary_variables)+"""; i++) {
            Q[i] = 0.0;
          }
          Q[0] = 1.0; Q[3] = 1.0; Q[5] = 1.0;
          Q[16] = 1.0; Q[54] = 1.0;
        },
        marker.x(),
        marker.h(),
        {{FACE_METADATA_ACCESSOR}}.getOldTimeStamp(marker.getSelectedFaceNumber()<Dimensions ? 1 : 0),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        {{OVERLAP}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        {0.0, 0.0, 0.0},
        fineGridFace{{UNKNOWN_IDENTIFIER}}Old.value,
        fineGridFace{{UNKNOWN_IDENTIFIER}}New.value
      );
        """



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
    probe_point = [-12,-12,0.0]
    project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )
    if args.extension=="Psi4":
      my_solver.select_dofs_to_print = [0,12,16,17,18,53,54,59,60]
    elif args.extension=="adm" or args.extension=="phy-debug":
      pass
    else:
      pass#my_solver.select_dofs_to_print = [0,12,16,17,18,53,54]
   

    #project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveSubdivision","(new ::exahype2::LoadBalancingConfiguration(0.95,100,-1,32))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.9,1000,16))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.98))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOut","(new ::exahype2::LoadBalancingConfiguration(0.95))" )
    project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutHierarchically","(new ::exahype2::LoadBalancingConfiguration(0.98))" )
    #project.set_load_balancing("toolbox::loadbalancing::strategies::cascade::SpreadOut_RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.98))" )


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
        init_action_set.priority = 0
        project.add_action_set_to_initialisation( init_action_set )


        # Not required anymore, as will be done by tracer routine itself. See
        # exahype2.Project.add_tracer()
        # -------------------------------------------------------------------
        #project.add_action_set_to_timestepping(
        #  peano4.toolbox.particles.UpdateParallelState(
        #    particle_set=tracer_particles
        #  )
        #)
        
        tracer_action_set=exahype2.tracer.FiniteVolumesTracing(tracer_particles,
                                                               my_solver,
                                                               project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear",
                                                               )
        tracer_action_set.priority = my_solver._action_set_update_cell.priority+1
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
        dump_action_set.priority = my_solver._action_set_update_cell.priority+2
        project.add_action_set_to_timestepping(dump_action_set)

    if args.add_tracer==2:
        tracer_particles = project.add_tracer( name="Tracer",attribute_count=59 )
        init_action_set = exahype2.tracer.InsertParticlesByCoordinates( 
            particle_set=tracer_particles, coordinates=[[2.0,0,0],[-2.0,0,0]])
        init_action_set.priority = 0
        project.add_action_set_to_initialisation( init_action_set )

        # Not required anymore, as will be done by tracer routine itself. See
        # exahype2.Project.add_tracer()
        # -------------------------------------------------------------------
        #project.add_action_set_to_timestepping(
        #  peano4.toolbox.particles.UpdateParallelState(
        #    particle_set=tracer_particles
        #  )
        #)
        
        tracer_action_set=exahype2.tracer.FiniteVolumesTracing(tracer_particles,
                                                               my_solver,
                                                               project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear_explicit_Euler",
                                                               projection_kernel_arguments="""
                                                                marker,
                                                                {{PATCH_SIZE}},
                                                                {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
                                                                fineGridCell{{SOLVER_NAME}}Q.value,
                                                                fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
                                                                {17,18,19},
                                                                {-1,-1,-1},
                                                                *p
                                                               """
                                                               )
        tracer_action_set.priority = my_solver._action_set_compute_final_linear_combination.priority+1
        project.add_action_set_to_timestepping(tracer_action_set)
        project.add_action_set_to_initialisation(tracer_action_set)
        
        dump_action_set=exahype2.tracer.DumpTracerIntoDatabase(
          particle_set=tracer_particles,
          solver=my_solver,
          filename=args.path+"Tracer-test",
          number_of_entries_between_two_db_flushes=500,
          output_precision=8,
          data_delta_between_two_snapsots=1e16,
          position_delta_between_two_snapsots=1e16,
          time_delta_between_two_snapsots=0.02
          )
        dump_action_set.priority = my_solver._action_set_compute_final_linear_combination.priority+2
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

    # NOTE these lines are required to build with the fortran routines --- this will also require to uncomment some
    # includes etc
    #
    # peano4_project.output.makefile.add_Fortran_flag( "-DCCZ4EINSTEIN -DDim3" )
    # peano4_project.output.makefile.add_Fortran_flag( "-lstdc++ -fdefault-real-8 -fdefault-double-8 -cpp -std=legacy -ffree-line-length-512 -fPIC" )
    # peano4_project.output.makefile.add_CXX_flag( "-fPIE -DCCZ4EINSTEIN" )
    # peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC -lgfortran" )
    # peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC -L/usr/lib/x86_64-linux-gnu -lgfortran" )

    # This might work for Intel (not tested)
    #peano4_project.output.makefile.add_Fortran_flag( "-r8 -cpp -auto -qopenmp-simd -O2" )
    #peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC" )
    # you might need -lifcore

    # peano4_project.output.makefile.add_Fortran_module( "MainVariables.f90" )

    # peano4_project.output.makefile.add_Fortran_files(
      # ["PDE.f90 ", "EinsteinConstraints.f90 ", "Properties.f90","ADMConstraints.f90"]
    # )


    # peano4_project.constants.export_string( "Scenario", "gaugewave-c++" )
    # peano4_project.constants.add_target_begin()
    # for k, v in floatparams.items(): peano4_project.constants.export_constexpr_with_type("CCZ4{}".format(k), eval('args.CCZ4{}'.format(k)), "double")
    # peano4_project.constants.add_target_end()
    # peano4_project.constants.add_target_begin()
    # for k, v in intparams.items():   peano4_project.constants.export_constexpr_with_type("CCZ4{}".format(k), eval('args.CCZ4{}'.format(k)), "int")
    # peano4_project.constants.add_target_end()
    # project.export_const_with_type("NumberOfUnknowns", 59, "int")
    #peano4_project.constants.export_string( "Scenario", "linearwave-c++" )

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
