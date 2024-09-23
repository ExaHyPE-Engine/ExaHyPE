import argparse
import dastgen2

import peano4
import exahype2


floatparams = {
        "GLMc0":1.5, "GLMc":1.2, "GLMd":2.0, "GLMepsA":1.0, "GLMepsP":1.0,
        "GLMepsD":1.0, 
	"itau":1.0, "k1":0.1, "k2":0.0, "k3":0.5, "eta":1.0,
        "f":0.75, "g":0.0, "xi":1.0, "e":1.0, "c":1.0, "mu":0.2, "ds":1.0,
        "sk":0.0, "bs":0.0,
        "domain_r":0.5, "smoothing":0.0, "KOSigma":8.0
}

intparams = {"BBHType":2, "LapseType":0, "tp_grid_setup":0, "swi":99, "ReSwi":1, "SO":0}
#sss
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ExaHyPE 2 - CCZ4 script')
    parser.add_argument("-maxh",   "--max-h",       dest="max_h",           type=float, default=0.05,  help="upper limit for refinement. Refers to volume size, i.e. not to patch size" )
    parser.add_argument("-minh",   "--min-h",       dest="min_h",           type=float, default=0.05,  help="lower limit for refinement (set to 0 to make it equal to max_h - default). Refers to volume size, i.e. not to patch size" )
    parser.add_argument("-ps",   "--patch-size",      dest="patch_size",      type=int, default=9,    help="Patch size, i.e. number of volumes per patch per direction" )

    for k, v in floatparams.items(): parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=float, default=v, help="default: %(default)s")
    for k, v in intparams.items(): parser.add_argument("--{}".format(k), dest="CCZ4{}".format(k), type=int, default=v, help="default: %(default)s")

    args = parser.parse_args()

    class CCZ4Solver( exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking ):
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
#include "../../../../applications/exahype2/ccz4/CCZ4Kernels.h"
"""
        super(CCZ4Solver,self).__init__(
          name=name, patch_size=patch_size,
          unknowns=number_of_unknowns,
          auxiliary_variables=0,
          min_volume_h=min_volume_h, max_volume_h=max_volume_h,
          time_step_relaxation=cfl,
          pde_terms_without_state=True,
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


        self.postprocess_updated_patch = """
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
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""
   
      def create_action_sets(self):
        super(CCZ4Solver,self).create_action_sets()
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.additional_includes += """ 
#include "../../../../applications/exahype2/ccz4/CCZ4Kernels.h"
            """

      def get_user_action_set_includes(self):
        """
         We take this routine to add a few additional include statements.
        """
        return super(CCZ4Solver,self).get_user_action_set_includes() + self._my_user_includes

########################################################################################
#main starts here
########################################################################################
    userinfo = []   
    project = exahype2.Project( ["benchmarks", "exahype2", "ccz4"], "ccz4", executable="peano4_test")

    my_solver = CCZ4Solver("CCZ4", args.patch_size, args.min_h, args.max_h, 0.1, args.CCZ4domain_r,args.CCZ4KOSigma)
    userinfo.append(("CFL ratio set as "+str(0.1), None))
    userinfo.append(("The solver is Rusanov Finite Volume with EnclaveTasking", None))


########################################################################################
#parameter setting according to scenarios
########################################################################################
    for k, v in intparams.items():
      intparams.update({k:eval("args.CCZ4{}".format(k))})
    for k, v in floatparams.items():
      floatparams.update({k:eval("args.CCZ4{}".format(k))})

    solverconstants=""
    userinfo.append(("picking gauge wave scenario",None))

    for k, v in floatparams.items(): solverconstants+= "static constexpr double {} = {};\n".format("CCZ4{}".format(k), v)
    for k, v in intparams.items():   solverconstants+= "static constexpr int {} = {};\n".format("CCZ4{}".format(k), v)
    my_solver.add_solver_constants(solverconstants)

    project.add_solver(my_solver)
    
    dimensions = 3

########################################################################################
#Domain settings
########################################################################################
    project.set_global_simulation_parameters(
      dimensions,               # dimensions
      [-0.5,-0.5,-0.5],[1,1,1],
      0.1,                 # end time
      0, 0,   # snapshots
      [True,True,True],
      8  # plotter precision
    )

    project.set_Peano4_installation("../../../..", peano4.output.CompileMode.Release)

    project.set_output_path("./")   
    project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutHierarchically","(new ::exahype2::LoadBalancingConfiguration(0.95,1000,16))" )

########################################################################################
#linking stuff
########################################################################################
    peano4_project = project.generate_Peano4_project(verbose=True)
    peano4_project.output.makefile.add_header_search_path("../../../../applications/exahype2/ccz4/")

    peano4_project.output.makefile.add_CXX_flag( "-DCCZ4EINSTEIN" )

    peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp" )

    peano4_project.generate( throw_away_data_after_generation=False )
    peano4_project.build( make_clean_first = True )

    #Report the application information
    userinfo.append(("the executable file name: peano4_test", None))
    print("=========================================================")
    print("The building information:")
    for msg, exception in userinfo: print(msg)
    print(intparams)
    print(floatparams)
    print("=========================================================")
