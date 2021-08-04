import os
import argparse

import peano4
import exahype2

import peano4.toolbox.particles
import dastgen2

import numpy as np

modes = { 
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts, "stats":  peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

floatparams = {
        "sk":1.0, "bs":1.0
}

intparams = {"swi":99, "ReSwi":0}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ExaHyPE 2 - SSInfall script')
    parser.add_argument("-maxh",   "--max-h",       dest="max_h",           type=float, required="True",  help="upper limit for refinement" )
    parser.add_argument("-minh",   "--min-h",       dest="min_h",           type=float, default=0,  help="lower limit for refinement (set to 0 to make it equal to max_h - default)" )
    parser.add_argument("-ps",   "--patch-size",      dest="patch_size",      type=int, default=6,    help="Patch size, i.e. number of volumes per patch per direction" )
    parser.add_argument("-plt",  "--plot-step-size",  dest="plot_step_size",  type=float, default=0.04, help="Plot step size (0 to switch it off)" )
    parser.add_argument("-m",    "--mode",            dest="mode",            default="release",  help="|".join(modes.keys()) )
    parser.add_argument("-ext",  "--extension",       dest="extension",       choices=["none", "gradient", "AMRadm", "Full"],   default="none",  help="Pick extension, i.e. what should be plotted on top. Default is none" )
    parser.add_argument("-impl", "--implementation",  dest="implementation",  choices=["ader-fixed", "fv-fixed", "fv-fixed-enclave", "fv-adaptive" ,"fv-adaptive-enclave", "fv-optimistic-enclave", "fv-fixed-gpu"], required="True",  help="Pick solver type" )
    parser.add_argument("-no-pbc",  "--no-periodic-boundary-conditions",      dest="periodic_bc", action="store_false", default="True",  help="switch on or off the periodic BC" )
    parser.add_argument("-et",   "--end-time",        dest="end_time",        type=float, default=1.0, help="End (terminal) time" )
    parser.add_argument("-tn", "--tracer-name",       dest="tra_name",    type=str, default="de",  help="name of output tracer file (temporary)" )
    parser.add_argument("-exn", "--exe-name",        dest="exe_name",    type=str, default="",  help="name of output executable file" )
    parser.add_argument("-outdir", "--output-directory",        dest="path",    type=str, default="./",  help="specify the output directory, include the patch file and tracer file" )


    for k, v in floatparams.items(): parser.add_argument("--{}".format(k), dest="SSInfall{}".format(k), type=float, default=v, help="default: %(default)s")
    for k, v in intparams.items():
      if k=="ReSwi":
        parser.add_argument("--{}".format(k), dest="SSInfall{}".format(k), type=int, default=v, help="default: %(default)s, choose refinement criterion, 0-no refinement, 1-radius based, 2-SBH phi gradient based, 3-BBH phi gradient based. Notice: 2 and 3 only work with -ext Full")
      else: parser.add_argument("--{}".format(k), dest="SSInfall{}".format(k), type=int, default=v, help="default: %(default)s")

    args = parser.parse_args()

    SuperClass = None

    if args.implementation=="fv-fixed":
       SuperClass = exahype2.solvers.fv.GenericRusanovFixedTimeStepSize
    if args.implementation=="fv-fixed-enclave":
       SuperClass = exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves
    if args.implementation=="fv-adaptive":
       SuperClass = exahype2.solvers.fv.GenericRusanovAdaptiveTimeStepSize
    if args.implementation=="fv-adaptive-enclave":
       SuperClass = exahype2.solvers.fv.GenericRusanovAdaptiveTimeStepSizeWithEnclaves
    if args.implementation=="fv-optimistic-enclave":
       SuperClass = exahype2.solvers.fv.GenericRusanovOptimisticTimeStepSizeWithEnclaves
    if args.implementation=="ader-fixed":
       SuperClass = exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize
    if args.implementation=="fv-fixed-gpu":
       SuperClass = exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithAccelerator

    class SSInfallSolver( SuperClass ):
      def __init__(self, name, patch_size, min_h, max_h ):
        unknowns = {
          "rho":1,
          "j":3,
          "E":1,
        }

        number_of_unknowns = sum(unknowns.values())

        self._my_user_includes = """
#include "exahype2/PatchUtils.h"
"""
        if SuperClass==exahype2.solvers.fv.GenericRusanovFixedTimeStepSize or \
           SuperClass==exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_h=min_h, max_h=max_h,
            time_step_size=1e-2
          )
        else:
          SuperClass.__init__(
            self,
            name=name, patch_size=patch_size,
            unknowns=number_of_unknowns,
            auxiliary_variables=0,
            min_h=min_h, max_h=max_h,
            time_step_relaxation=0.25
          )

        self._solver_template_file_class_name = SuperClass.__name__

        self.set_implementation(
          boundary_conditions=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          flux=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          ncp=exahype2.solvers.fv.PDETerms.None_Implementation,
          source_term=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation
        )

        self.set_postprocess_updated_patch_kernel( """

  {
    #if Dimensions==2
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    #endif

    #if Dimensions==3
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    #endif

    int index = 0;
    for (int i=0;i<itmax;i++)
    {

    // dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
    //  examples::exahype2::ccz4::enforceCCZ4constraints( targetPatch+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
""" )
      def get_user_includes(self):
        """
         We take this routine to add a few additional include statements.
        """
        return SuperClass.get_user_includes(self) + self._my_user_includes

      def add_mass_cal(self):
        """

        """
        self._my_user_includes += """
#include "../SSInfall.h"
    """
        self._auxiliary_variables = 0

        self.set_preprocess_reconstructed_patch_kernel( """
        const int patchSize = """ + str( self._patch.dim[0] ) + """;
        double volumeH = ::exahype2::getVolumeLength(marker.h(),patchSize);
        if (not tarch::la::equals(t,repositories::{{SOLVER_INSTANCE}}.t_record)){
          repositories::{{SOLVER_INSTANCE}}.t_record=t;
          std::cout << repositories::{{SOLVER_INSTANCE}}.m_tot << std::endl;
          repositories::{{SOLVER_INSTANCE}}.m_tot=0;
        }
        dfor(cell,patchSize) {
          tarch::la::Vector<Dimensions,double> coor;
          for (int i=0;i<Dimensions;i++) coor(i) = marker.getOffset()(i)+ (cell(i)+0.5)*volumeH;
          tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(1);
          const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*1);
          double r_coor=(coor(0)-0)*(coor(0)-0)+(coor(1)-0)*(coor(1)-0)+(coor(2)-0)*(coor(2)-0);
          r_coor=pow(r_coor,0.5);
               
          repositories::{{SOLVER_INSTANCE}}.add_mass(r_coor,reconstructedPatch[cellSerialised*5+0],volumeH);       
          //std::cout << coor(0) << " " << coor(1) << " " << coor(2) << std::endl;
          //if (r_coor<r_s[0]) {std::cout << r_coor << std::endl;}

          
        }
    """)

        self.create_data_structures()
        self.create_action_sets()

########################################################################################
#main starts here
########################################################################################
    userwarnings = []
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
    try:
      if SuperClass==exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize:
        is_aderdg = True
        order = 3
        unknowns = 59
        time_step_size = 0.001
    except Exception as e:
        msg = "Warning: ADER-DG no supported on this machine"
        print(msg)
        userwarnings.append((msg,e))

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
      my_solver = SSInfallSolver(solver_name, args.patch_size, min_h, args.max_h)
      
    my_solver.add_mass_cal()

########################################################################################
#parameter setting according to scenarios
########################################################################################
    if args.periodic_bc=="True":
      msg = "Periodic BC set"
      print(msg)
      periodic_boundary_conditions = [True,True,True]          # Periodic BC
      userwarnings.append((msg,None))
    else:
      msg = "WARNING: Periodic BC deactivated by hand"
      print(msg)
      periodic_boundary_conditions = [False,False,False]
      userwarnings.append((msg,None))

    intparams.update({"ReSwi":args.SSInfallReSwi})

    solverconstants=""
    for k, v in floatparams.items(): solverconstants+= "static constexpr double {} = {};\n".format("SSInfall{}".format(k), v)
    for k, v in intparams.items():   solverconstants+= "static constexpr int {} = {};\n".format("SSInfall{}".format(k), v)
    my_solver.set_solver_constants(solverconstants)

    project.add_solver(my_solver)

    build_mode = modes[args.mode]
    
    dimensions = 3

########################################################################################
#Domain settings
########################################################################################
    if True:
      offset=[-0.5, -0.5, -0.5]; domain_size=[1.0, 1.0, 1.0]
      msg = "domain set as "+str(offset)+" and "+str(domain_size)
      print(msg)
      userwarnings.append((msg,None))

    project.set_global_simulation_parameters(
      dimensions,               # dimensions
      offset,  domain_size,
      args.end_time,                 # end time
      0.0, args.plot_step_size,   # snapshots
      periodic_boundary_conditions,
      8  # plotter precision
    )

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
    #probe_point = [-8,-8,-0.5]
    #project.add_plot_filter( probe_point,[16.0,16.0,0.5],1 )

    project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision")

########################################################################################
#compile for the real executable
########################################################################################
    peano4_project = project.generate_Peano4_project(verbose=True)
    peano4_project.generate( throw_away_data_after_generation=False )
    peano4_project.build( make_clean_first = True )

    # Remind the user of warnings!
    userwarnings.append(("the executable file name: "+exe, None))
    userwarnings.append(("output directory: "+path, None))
    if len(userwarnings) >0:
        print("Please note that these warning occured before the build:")
        for msg, exception in userwarnings:
            if exception is None:
                print(msg)
            else: print(msg, "Exception: {}".format(exception))
