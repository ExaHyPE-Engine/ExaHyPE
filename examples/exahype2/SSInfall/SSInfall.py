import os
import argparse

import peano4
import exahype2

import peano4.toolbox.particles
import dastgen2

from Probe_file_gene import tracer_seeds_generate
import numpy as np

modes = { 
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts, "stats":  peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

floatparams = {
         "G":1, "tilde_rho_ini":1, "r_ini":0.2, "delta_rho":0.05, "tilde_P_ini":1e-6, "gamma":5.0/3.0, "Omega_m":1, "delta_m":0.15, "r_point":0.05, "a_i":0.001, "v_scale":1.0, "domain_r":0.5}

intparams = {"swi":0, "ReSwi":0, "sample_number":10, "iseed":0, "ReSwi":0, "MassCal":0, "extrapolate_bc":0}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ExaHyPE 2 - SSInfall script')
    parser.add_argument("-maxh",   "--max-h",       dest="max_h",           type=float, required="True",  help="upper limit for refinement. Refers to volume size, i.e. not to patch size" )
    parser.add_argument("-minh",   "--min-h",       dest="min_h",           type=float, default=0,  help="lower limit for refinement (set to 0 to make it equal to max_h - default). Refers to volume size, i.e. not to patch size" )
    parser.add_argument("-ps",   "--patch-size",      dest="patch_size",      type=int, default=6,    help="Patch size, i.e. number of volumes per patch per direction" )
    parser.add_argument("-plt",  "--plot-step-size",  dest="plot_step_size",  type=float, default=0.04, help="Plot step size (0 to switch it off)" )
    parser.add_argument("-m",    "--mode",            dest="mode",            default="release",  help="|".join(modes.keys()) )
    parser.add_argument("-ext",  "--extension",       dest="extension",       choices=["cellcount","rhointer"],   default="cellcount",  help="Pick extension, i.e. the way to calculate the mass. Default is cell counting" )
    parser.add_argument("-impl", "--implementation",  dest="implementation",  choices=["fv-global-fixed", "fv-global-adaptive", "fv-global-fixed-enclave", "fv-global-adaptive-enclave","fv-subcyling-adaptive-enclave","fv-local-enclave"], required="True",  help="Pick solver type" )
    #parser.add_argument("-no-pbc",  "--no-periodic-boundary-conditions",      dest="periodic_bc", action="store_false", default="True",  help="switch on or off the periodic BC" )
    parser.add_argument("-bc", "--boundary-condition",  dest="boundary",  choices=["periodic","neumann","extrapolate", "hybrid"], default="neumann",  help="Pick the type of Boundary condtion" )
    parser.add_argument("-et",   "--end-time",        dest="end_time",        type=float, default=1.0, help="End (terminal) time" )
    parser.add_argument("-tn", "--tracer-name",       dest="tra_name",    type=str, default="de",  help="name of output tracer file (temporary)" )
    parser.add_argument("-exn", "--exe-name",        dest="exe_name",    type=str, default="",  help="name of output executable file" )
    parser.add_argument("-outdir", "--output-directory",        dest="path",    type=str, default="./",  help="specify the output directory, include the patch file and tracer file" )
    parser.add_argument("-tracer", "--add-tracer",    dest="add_tracer", type=int, default=0,  help="Add tracers and specify the seeds. 0-switch off, 1-x axis, 2-xy plane, 3-over domain (evenly)" )
    parser.add_argument("-iseed", "--initial-seed",    dest="seed", type=str, default="tophat",  help="specify the overdensity seeds. tophat/point" )
    parser.add_argument("-eigen", "--eigenvalue-control",    dest="eigen", choices=["none", "exp"],  default="none",  help="Modified the formula of eigenvalue to suppress initial time-stepping size" )
    parser.add_argument("-cfl",  "--CFL-ratio",  dest="cfl",  type=float, default=0.1, help="Set CFL ratio" )
    parser.add_argument("-interp",   "--interpolation", dest="interpolation",     choices=["constant", "linear-slow", "linear", "outflow" ], default="linear-slow",  help="interpolation scheme for AMR" )
    parser.add_argument("-restrict", "--restriction",   dest="restriction",       choices=["average", "inject"], default="average",  help="restriction scheme for AMR" )

    for k, v in floatparams.items(): parser.add_argument("--{}".format(k), dest="{}".format(k), type=float, default=v, help="default: %(default)s")
    for k, v in intparams.items():
      if k=="ReSwi":
        parser.add_argument("--{}".format(k), dest="{}".format(k), type=int, default=v, help="default: %(default)s, choose refinement criterion, 0-no refinement, 1-radius based, 2-SBH phi gradient based, 3-BBH phi gradient based. Notice: 2 and 3 only work with -ext Full")
      else: parser.add_argument("--{}".format(k), dest="{}".format(k), type=int, default=v, help="default: %(default)s")

    args = parser.parse_args()

    SuperClass = None

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

    class SSInfallSolver( SuperClass ):
      def __init__(self, name, patch_size, min_volume_h, max_volume_h, cfl, domain_r):
        unknowns = {
          "rho":1,
          "j":3,
          "E":1,
        }

        number_of_unknowns = sum(unknowns.values())

        self._my_user_includes = """
#include "exahype2/PatchUtils.h"
"""
        #if SuperClass==exahype2.solvers.fv.GenericRusanovFixedTimeStepSize or \
        #   SuperClass==exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves:
        if SuperClass==exahype2.solvers.fv.rusanov.GlobalFixedTimeStep or SuperClass==exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_volume_h=min_volume_h, max_volume_h=max_volume_h,
            time_step_size=1e-2
          )
        else:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_volume_h=min_volume_h, max_volume_h=max_volume_h,
            time_step_relaxation=cfl
          )

        #self._solver_template_file_class_name = SuperClass.__name__

        self.set_implementation(
          boundary_conditions=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          flux=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          ncp=exahype2.solvers.fv.PDETerms.None_Implementation,
          source_term=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          refinement_criterion = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
        )

        self._patch_size = patch_size
        self._domain_r = domain_r  

        self.set_postprocess_updated_patch_kernel( """

""" )
      def create_action_sets(self):
        SuperClass.create_action_sets(self)

        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_restriction_scheme( "averaging" )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_interpolation_scheme( "linear" )

        if args.interpolation=="constant":
          self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_interpolation_scheme( "piecewise_constant" )
          print( "Interpolation rule: piecewise_constant" )
        if args.interpolation=="linear-slow":
          self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_interpolation_scheme( "linear" )
          print( "Interpolation rule: linear interpolation without optimisation" )
        if args.interpolation=="linear":
          self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_interpolation_scheme( "linear_precomputed_operators<" + str(self._patch_size) +">" )
          print( "Interpolation rule: optimised linear interpolation with patch size " + str(self._patch_size) )
        if args.interpolation=="outflow":
          self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_interpolation_scheme( "outflow" )
          print( "Interpolation rule: outflow (from fine grid into coarse grid)" )

        if args.restriction=="average":
          self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_restriction_scheme( "averaging" )
          print( "Restiction rule: averaging" )
        if args.restriction=="inject":
          self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_restriction_scheme( "inject" )
          print( "Restiction rule: injection" )


      def get_user_action_set_includes(self):
        """
         We take this routine to add a few additional include statements.
        """
        return SuperClass.get_user_action_set_includes(self) + self._my_user_includes
        

      def add_derivative_calculation(self):
        """

         Add the constraint verification code

         We introduce new auxiliary variables. Prior to each time step, I
         compute the Laplacian and store it in the auxiliary variable. This
         is kind of a material parameter F(Q) which does not feed back into
         the solution.

         Changing the number of unknowns a posteriori is a delicate update
         to the solver, so we invoke the constructor again to be on the safe
         side.

        """
        self._auxiliary_variables = 5*3

        self.set_preprocess_reconstructed_patch_kernel( """
        const int patchSize = """ + str( self._patch.dim[0] ) + """;
        int aux_var=""" + str( self._auxiliary_variables ) + """;
        int real_var=5;
        double volumeH = ::exahype2::getVolumeLength(marker.h(),patchSize);
        dfor(cell,patchSize) {
          tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(1);
          const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*1);

          // Lets look left vs right and compute the gradient. Then, lets
          // loop up and down. So we look three times for the respective
          // directional gradients
          for (int d=0; d<3; d++) {
            tarch::la::Vector<Dimensions,int> leftCell  = currentCell;
            tarch::la::Vector<Dimensions,int> rightCell = currentCell;
            leftCell(d)  -= 1;
            rightCell(d) += 1;
            const int leftCellSerialised  = peano4::utils::dLinearised(leftCell, patchSize + 2*1);
            const int rightCellSerialised = peano4::utils::dLinearised(rightCell,patchSize + 2*1);
            for (int i=0; i<real_var; i++) {
              reconstructedPatch[cellSerialised*(real_var+aux_var)+real_var+i*3+d] =
                ( reconstructedPatch[rightCellSerialised*(real_var+aux_var)+i] - reconstructedPatch[leftCellSerialised*(real_var+aux_var)+i] ) / 2.0 / volumeH;
              if (isnan(reconstructedPatch[cellSerialised*(real_var+aux_var)+real_var+i*3+d])) {std::cout <<reconstructedPatch[rightCellSerialised*(real_var+aux_var)+i]<<" "<< reconstructedPatch[leftCellSerialised*(real_var+aux_var)+i] << std::endl;}
            }

          }
        }
    """)
        self.create_data_structures()
        self.create_action_sets()

      def add_mass_cal_cellcount(self):
        """

        """
        self._my_user_includes += """
#include "../SSInfall.h"
#include <math.h>
    """
        self._auxiliary_variables = 5*3

        self.set_preprocess_reconstructed_patch_kernel( """
        const int patchSize = """ + str( self._patch.dim[0] ) + """;
        double volumeH = ::exahype2::getVolumeLength(marker.h(),patchSize);
        int aux_var=""" + str( self._auxiliary_variables ) + """;
        double domain_r=""" + str( self._domain_r ) + """;
        int sample=repositories::{{SOLVER_INSTANCE}}.sample_number;
        tarch::la::Vector<Dimensions,double> center=repositories::{{SOLVER_INSTANCE}}.center;
        dfor(cell,patchSize) {
          tarch::la::Vector<Dimensions,double> coor;
          //tarch::la::Vector<Dimensions,double> vH=(volumeH,volumeH,volumeH);
          for (int i=0;i<Dimensions;i++) coor(i) = marker.getOffset()(i)+ (cell(i)+0.5)*volumeH;
          
          tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(1);
          const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*1);
          
          double r_coor=(coor(0)-center(0))*(coor(0)-center(0))+(coor(1)-center(1))*(coor(1)-center(1))+(coor(2)-center(2))*(coor(2)-center(2));
          r_coor=pow(r_coor,0.5);
          repositories::{{SOLVER_INSTANCE}}.add_mass(r_coor,reconstructedPatch[cellSerialised*(5+aux_var)+0],volumeH);  
          //if (isnan(reconstructedPatch[cellSerialised*(5+aux_var)+0])){std::cout << "NaN inside domain" << std::endl; std::abort();}
          //if (r_coor>0.84) {std::cout << "add here" << std::endl;}

          double rho =  reconstructedPatch[cellSerialised*(5+aux_var)+0];      
          double m1  =  reconstructedPatch[cellSerialised*(5+aux_var)+1];
          double m2  =  reconstructedPatch[cellSerialised*(5+aux_var)+2];
          double m3  =  reconstructedPatch[cellSerialised*(5+aux_var)+3];
          double e   =  reconstructedPatch[cellSerialised*(5+aux_var)+4];
          
          double p   =  (5.0/3.0-1)*(e-0.5*(m1*m1+m2*m2+m3*m3)/rho);
          if (p<0) {
            reconstructedPatch[cellSerialised*(5+aux_var)+4]=0.5*(m1*m1+m2*m2+m3*m3)/rho+1e-14; 
          }
          
          //int test=0;
          if (abs(coor(0))>0.8*domain_r or abs(coor(1))>0.8*domain_r or abs(coor(2))>0.8*domain_r){ 
					for (int d=0; d<3; d++) {
            tarch::la::Vector<Dimensions,int> leftCell  = currentCell;
            tarch::la::Vector<Dimensions,int> rightCell = currentCell;
            leftCell(d)  -= 1;
            rightCell(d) += 1;
            const int leftCellSerialised  = peano4::utils::dLinearised(leftCell, patchSize + 2*1);
            const int rightCellSerialised = peano4::utils::dLinearised(rightCell,patchSize + 2*1);
            for (int i=0; i<5; i++){
            	if (cell(d)==0) {reconstructedPatch[cellSerialised*(5+aux_var)+5+i*3+d] =( reconstructedPatch[rightCellSerialised*(5+aux_var)+i] - reconstructedPatch[cellSerialised*(5+aux_var)+i] )/ volumeH;}
            	else if (cell(d)==(patchSize-1)) {reconstructedPatch[cellSerialised*(5+aux_var)+5+i*3+d] =( reconstructedPatch[cellSerialised*(5+aux_var)+i] - reconstructedPatch[leftCellSerialised*(5+aux_var)+i] )/ volumeH;}
            	else {reconstructedPatch[cellSerialised*(5+aux_var)+5+i*3+d] =( reconstructedPatch[rightCellSerialised*(5+aux_var)+i] - reconstructedPatch[leftCellSerialised*(5+aux_var)+i] )/ 2.0 / volumeH;}
            }
          }
          } else{
            for (int d=0; d<15; d++) {reconstructedPatch[cellSerialised*(5+aux_var)+5+d]=0;}
          }
          
          
          /*for (int d=0; d<15; d++) {
            if (isnan(reconstructedPatch[cellSerialised*(5+aux_var)+5+d])){
              std::cout << reconstructedPatch[cellSerialised*(5+aux_var)+5+d] <<" "<< test <<" "<<d<< std::endl;
            }
          }*/

        }
        
    """)
        self.create_data_structures()
        self.create_action_sets()

      def add_mass_cal_rhointer(self):
        """

        """
        self._my_user_includes += """
#include "../SSInfall.h"
#include <math.h>
    """
        self._auxiliary_variables = 0

        self.set_preprocess_reconstructed_patch_kernel( """
        const int patchSize = """ + str( self._patch.dim[0] ) + """;
        double volumeH = ::exahype2::getVolumeLength(marker.h(),patchSize);
        int aux_var=0;
        int sample=repositories::{{SOLVER_INSTANCE}}.sample_number;
        if ( marker.isContained((0,0,0)) ){
          tarch::la::Vector<Dimensions,int> centerCell = tarch::la::Vector<Dimensions,int>(1+patchSize/2);
          const int cellSerialised  = peano4::utils::dLinearised(centerCell, patchSize + 2*1);
          repositories::{{SOLVER_INSTANCE}}.rho_0=reconstructedPatch[cellSerialised*(5+aux_var)+0];
          //std::cout << repositories::{{SOLVER_INSTANCE}}.rho_0 << std::endl;
        }
        for (int i=0;i<sample;i++){
          tarch::la::Vector<Dimensions,double> coor; coor(0)=repositories::{{SOLVER_INSTANCE}}.r_s[i];
          if ( marker.isContained(coor) ){
            //std::cout << coor << std::endl;
            for (int xindex=0; xindex<(patchSize+2);xindex++){
              if ( (marker.getOffset()(0)+(xindex-1)*volumeH)<repositories::{{SOLVER_INSTANCE}}.r_s[i] and (marker.getOffset()(0)+(xindex-0.5)*volumeH)>repositories::{{SOLVER_INSTANCE}}.r_s[i] ){
                //std::cout <<  (marker.getOffset()(0)+(xindex-1)*volumeH)<<" "<< (marker.getOffset()(0)+(xindex-0.5)*volumeH)<< std::endl;
                tarch::la::Vector<Dimensions,int> cell1=tarch::la::Vector<Dimensions,int>(1+patchSize/2); cell1(0)=xindex;
                tarch::la::Vector<Dimensions,int> cell2=tarch::la::Vector<Dimensions,int>(1+patchSize/2); cell2(0)=xindex-1;
                //std::cout << cell1 <<cell2<< std::endl;
                double rho1=reconstructedPatch[peano4::utils::dLinearised(cell1, patchSize + 2*1)*(5+aux_var)+0], x1=marker.getOffset()(0)+(xindex-0.5)*volumeH;
                double rho2=reconstructedPatch[peano4::utils::dLinearised(cell1, patchSize + 2*1)*(5+aux_var)+0], x2=marker.getOffset()(0)+(xindex-1.5)*volumeH;
                repositories::{{SOLVER_INSTANCE}}.rho_x[i]=rho1*(x2-repositories::{{SOLVER_INSTANCE}}.r_s[i])/(x2-x1)+rho2*(repositories::{{SOLVER_INSTANCE}}.r_s[i]-x1)/(x2-x1);
                //std::cout<<repositories::{{SOLVER_INSTANCE}}.rho_x[i]<<std::endl;
              }
              else if ( (marker.getOffset()(0)+(xindex-0.5)*volumeH)<repositories::{{SOLVER_INSTANCE}}.r_s[i] and (marker.getOffset()(0)+(xindex)*volumeH)>repositories::{{SOLVER_INSTANCE}}.r_s[i] ){
                //std::cout <<  (marker.getOffset()(0)+(xindex-0.5)*volumeH)<<" "<< (marker.getOffset()(0)+(xindex)*volumeH)<< std::endl;
                tarch::la::Vector<Dimensions,int> cell1=tarch::la::Vector<Dimensions,int>(1+patchSize/2); cell1(0)=xindex;
                tarch::la::Vector<Dimensions,int> cell2=tarch::la::Vector<Dimensions,int>(1+patchSize/2); cell2(0)=xindex+1;
                //std::cout << cell1 <<cell2 <<std::endl;
                double rho1=reconstructedPatch[peano4::utils::dLinearised(cell1, patchSize + 2*1)*(5+aux_var)+0], x1=marker.getOffset()(0)+(xindex-0.5)*volumeH;
                double rho2=reconstructedPatch[peano4::utils::dLinearised(cell1, patchSize + 2*1)*(5+aux_var)+0], x2=marker.getOffset()(0)+(xindex+0.5)*volumeH;
                repositories::{{SOLVER_INSTANCE}}.rho_x[i]=rho1*(x2-repositories::{{SOLVER_INSTANCE}}.r_s[i])/(x2-x1)+rho2*(repositories::{{SOLVER_INSTANCE}}.r_s[i]-x1)/(x2-x1);
                //std::cout<<repositories::{{SOLVER_INSTANCE}}.rho_x[i]<<std::endl;
              }
            }
          }                   
        }
    """)

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
    project = exahype2.Project( ["examples", "exahype2", "SSInfall"], "SSInfall", executable=exe)

########################################################################################
#Pick solver
########################################################################################
    is_aderdg = False
    solver_name = "SSInfall"
    #try:
    #  if SuperClass==exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize:
    #    is_aderdg = True
    #    order = 3
    #    unknowns = 59
    #    time_step_size = 0.001
    #except Exception as e:
    #    msg = "Warning: ADER-DG no supported on this machine"
    #    print(msg)
    #    userinfo.append((msg,e))

    if is_aderdg:
      solver_name    = "ADERDG" + solver_name
    else:
      solver_name    = solver_name

    min_h = args.min_h
    if min_h <=0.0:
      print( "No minimal mesh size chosen. Set it to max mesh size (regular grid)" )
      min_h = args.max_h

    if is_aderdg:
      my_solver = exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize(
          solver_name, order, unknowns, 0, #auxiliary_variables
          exahype2.solvers.aderdg.Polynomials.Gauss_Legendre,
          min_h, args.max_h, time_step_size,
          flux = exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation,
          ncp  = exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation,
          sources = exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation
      )
    else:
      my_solver = SSInfallSolver(solver_name, args.patch_size, min_h, args.max_h,args.cfl,args.domain_r)
      userinfo.append(("CFL ratio set as "+str(args.cfl), None))
      
    if args.extension=="cellcount":  
      my_solver.add_mass_cal_cellcount()
      userinfo.append(("mass calculation schme: cell counting",None))
    elif args.extension=="rhointer":
      my_solver.add_mass_cal_rhointer()
      userinfo.append(("mass calculation schme: rho interpolation",None))
    
    #my_solver.add_derivative_calculation()     
########################################################################################
#Domain settings
########################################################################################
    floatparams.update({"domain_r":args.domain_r})
    #if args.ReSwi==0:
      #offset=[-0.5, -0.5, -0.5]; domain_size=[1.0, 1.0, 1.0]
      #offset=[-0.75, -0.75, -0.75]; domain_size=[1.5, 1.5, 1.5]
      #offset=[-1.5, -1.5, -1.5]; domain_size=[3, 3, 3]
    #elif args.ReSwi==2:
      #offset=[-3, -3, -3]; domain_size=[6, 6, 6]
      #offset=[-4.5, -4.5, -4.5]; domain_size=[9, 9, 9]
    dr=floatparams["domain_r"]
    offset=[-dr, -dr, -dr]; domain_size=[2*dr, 2*dr, 2*dr]
    msg = "domain set as "+str(offset)+" and "+str(domain_size)
    print(msg)
    userinfo.append((msg,None))
      
########################################################################################
#parameter setting according to scenarios
########################################################################################
    for k, v in intparams.items():
      intparams.update({k:eval("args.{}".format(k))})
    for k, v in floatparams.items():
      floatparams.update({k:eval("args.{}".format(k))})
      
    if args.boundary=="periodic":
      msg = "Periodic BC set"
      print(msg)
      periodic_boundary_conditions = [True,True,True]          # Periodic BC
      userinfo.append((msg,None))
    elif args.boundary=="extrapolate":
      msg = "Extrapolate BC set"
      print(msg)
      periodic_boundary_conditions = [False,False,False]
      intparams.update({"extrapolate_bc":1})
      userinfo.append((msg,None))
    elif args.boundary=="hybrid":
      msg = "hybrid BC set"
      print(msg)
      periodic_boundary_conditions = [False,False,False]
      intparams.update({"extrapolate_bc":2})
      userinfo.append((msg,None))
    elif args.boundary=="neumann":
      msg = "Neumann BC set"
      print(msg)
      periodic_boundary_conditions = [False,False,False]
      userinfo.append((msg,None))
     
    if args.seed=="tophat":
      intparams.update({"iseed":0})
      userinfo.append(("Tophat overdensity region set",None))
    if args.seed=="point":
      intparams.update({"iseed":1})
      userinfo.append(("Point mass in tophat seed set",None))

    if args.eigen=="exp":
      #floatparams["C_1"]=(1*1e-4)/floatparams["tilde_P_ini"]*(floatparams["a_i"]/1e-3)**0.5
      #floatparams["C_2"]=(10*1e-5)/floatparams["tilde_P_ini"]
      floatparams["C_1"]=20
      floatparams["C_2"]=50
      userinfo.append(("Use exponential formula for eigenvalues",None))
    if args.eigen=="none":
      floatparams["C_1"]=0
      floatparams["C_2"]=0
      userinfo.append(("Use original formula for eigenvalues",None))

    solverconstants=""
    #if args.extension=="cellcount": 
    solverconstants+= "int global_cell_tot[{}]={};\n".format(args.sample_number,"{0}")
    solverconstants+= "double global_m_tot[{}]={};\n".format(args.sample_number,"{0}")
    solverconstants+= "int cell_tot[{}]={};\n".format(args.sample_number,"{0}")
    solverconstants+= "double m_tot[{}]={};\n".format(args.sample_number,"{0}")
    solverconstants+= "double m_tot_copy[{}]={};\n".format(args.sample_number,"{0}")
    #elif args.extension=="rhointer":
    solverconstants+= "double rho_0=0, rho_x[{}]={};\n".format(args.sample_number,"{0}")
    solverconstants+= "double rho_x_copy[{}]={};\n".format(args.sample_number,"{0}") 
    if args.extension=="rhointer":    
      intparams.update({"MassCal":1})
      
    r_list=np.linspace(0,(domain_size[0]+offset[0]),(args.sample_number+1))[1:]
    solverconstants+= "double r_s[{}]={}".format(args.sample_number,"{")
    for r in r_list:
      solverconstants+= str(r)+", "
    solverconstants=solverconstants[:-2]
    solverconstants+= "};\n"
    
    for k, v in floatparams.items(): solverconstants+= "static constexpr double {} = {};\n".format("{}".format(k), v)
    for k, v in intparams.items():   solverconstants+= "static constexpr int {} = {};\n".format("{}".format(k), v)  
     
    my_solver.set_solver_constants(solverconstants)

    project.add_solver(my_solver)

    build_mode = modes[args.mode]
    
    dimensions = 3

    project.set_global_simulation_parameters(
      dimensions,               # dimensions
      offset,  domain_size,
      args.end_time,                 # end time
      0.0, args.plot_step_size,   # snapshots
      periodic_boundary_conditions,
      12  # plotter precision
    )

    project.set_Peano4_installation("../../..", build_mode)

########################################################################################
#output dir and proble
########################################################################################
    path="./"
    if not args.path=="./":
        path=args.path 
    #path="/cosma5/data/durham/dc-zhan3/SSInfall1"
    #path="/cosma6/data/dp004/dc-zhan3/exahype2/sbh-fv3"
    project.set_output_path(path)
    probe_point = [-0,-0,-1e-6]
    project.add_plot_filter( probe_point,[40.0,40.0,2e-6],1 )

    project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision","(new ::exahype2::LoadBalancingConfiguration(0.9,0,16))" )

########################################################################################
#Tracer setting 
########################################################################################
    if not args.add_tracer==0:
      tracer_name = {1:"line", 2:"slide", 3:"volume", 6:"Gauss_Legendre_quadrature", 7:"t-design"}
      userinfo.append(("Tracer added, Type: "+tracer_name[args.add_tracer],None))
      tracer_particles = project.add_tracer( name="MyTracer",attribute_count=5, plot=False)
       #project.add_action_set_to_timestepping(exahype2.tracer.FiniteVolumesTracing(tracer_particles,my_solver,[17,18,19],[16],-1,time_stepping_kernel="toolbox::particles::explicitEulerWithoutInterpolation"))
      project.add_action_set_to_timestepping(
        exahype2.tracer.FiniteVolumesTracing(
          tracer_particles,my_solver,
          [17,18,19],range(5),-1,
          #time_stepping_kernel="toolbox::particles::LinearInterp",
          time_stepping_kernel="toolbox::particles::StaticPosition",
          #observer_kernel="toolbox::particles::ObLinearInterp"
        )
      )
      if args.add_tracer==1 or args.add_tracer==2 or args.add_tracer==3 :
        tracer_seeds_generate(Type=args.add_tracer, a=0.0, b=(offset[0]+domain_size[0]), N_x=210,N_y=50,N_z=1)
        project.add_action_set_to_initialisation( exahype2.tracer.InsertParticlesFromFile( particle_set=tracer_particles, filename=tracer_name[args.add_tracer]+".dat", scale_factor=1)) #"line.dat" #slide.dat #volume.dat

      if path=="./": path1="."
      else: path1=path
      project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
        particle_set=tracer_particles,
        solver=my_solver,
        filename=path1+"zz"+args.tra_name,
        number_of_entries_between_two_db_flushes=20000,
        output_precision=10,
        position_delta_between_two_snapsots=1e16,
        data_delta_between_two_snapsots=1e16,
        time_delta_between_two_snapsots=1.0,
        use_relative_deltas=False
      ))
      #data_delta_between_two_snapsots,position_delta_between_two_snapsots,filename,          
      #,,-1,"zz",1000))

########################################################################################
#compile for the real executable
########################################################################################
    peano4_project = project.generate_Peano4_project(verbose=True)
    peano4_project.generate( throw_away_data_after_generation=False )
    peano4_project.build( make_clean_first = True )

    # Remind the user of warnings!
    userinfo.append(("the executable file name: "+exe, None))
    userinfo.append(("output directory: "+path, None))
    print("=========================================================")
    if not args.add_tracer==0:
        userinfo.append(("tracer output file: "+path1+"zz"+args.tra_name, None))
    if len(userinfo) >0:
        print("The building infomation:")
        for msg, exception in userinfo:
            if exception is None:
                print(msg)
            else: print(msg, "Exception: {}".format(exception))
    print(intparams)
    print(floatparams)
    print("=========================================================")
