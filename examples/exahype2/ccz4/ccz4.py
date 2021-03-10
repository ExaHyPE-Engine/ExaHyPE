import os
import argparse

import peano4
import exahype2
import exahype2.sympy


modes = { 
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts,
  "debug":   peano4.output.CompileMode.Debug,
}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ExaHyPE 2 - CCZ4 script')
    parser.add_argument("-cs",   "--cell-size",       dest="max_h",           type=float, default=0.4,  help="Mesh size" )
    parser.add_argument("-ps",   "--patch-size",      dest="patch_size",      type=float, default=6,    help="Patch size, i.e. number of volumes per cell per direction" )
    parser.add_argument("-plt",  "--plot-step-size",  dest="plot_step_size",  type=float, default=0.01, help="Plot step size (0 to switch it off)" )
    parser.add_argument("-m",    "--mode",            dest="mode",            default="release",  help="|".join(modes.keys()) )
    parser.add_argument("-ext",  "--extension",       dest="extension",       choices=["none", "gradient", "adm"],   default="none",  help="Pick extension, i.e. what should be plotted on top. Default is none" )
    parser.add_argument("-impl", "--implementation",  dest="implementation",  choices=["fv-fixed", "fv-fixed-enclave", "fv-adaptive" ,"fv-adaptive-enclave", "fv-adaptive-optimistic"],   default="fv-adaptive-enclave",  help="Pick extension, i.e. what should be plotted on top. Default is none" )
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
    if args.implementation=="fv-adaptive-optimistic":
       SuperClass = exahype2.solvers.fv.GenericRusanovOptimisticTimeStepSizeWithEnclaves

    """
    Here is the aderdg test.
    """
    # SuperClass = exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize


    class CCZ4Solver( SuperClass ):
      def __init__(self, name, patch_size, min_h, max_h ):
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
        
        number_of_unknowns = 0
        for i in unknowns:
          number_of_unknowns += unknowns[i]
        
        if SuperClass==exahype2.solvers.fv.GenericRusanovFixedTimeStepSize or \
           SuperClass==exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves:
          SuperClass.__init__( 
            self,
            name=name, patch_size=patch_size, 
            unknowns=number_of_unknowns, 
            auxiliary_variables=0, 
            min_h=min_h, max_h=max_h,
            time_step_size=1e-6
          )
        else:
          SuperClass.__init__( 
            self,
            name=name, patch_size=patch_size, 
            unknowns=number_of_unknowns, 
            auxiliary_variables=0, 
            min_h=min_h, max_h=max_h, 
            time_step_relaxation=0.1
          )
          
        self._solver_template_file_class_name = SuperClass.__name__
    
        pde = exahype2.sympy.PDE(unknowns=self._unknowns,auxiliary_variables=self._auxiliary_variables,dimensions=3)
      
        self.set_implementation(
          boundary_conditions=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          flux=exahype2.solvers.fv.PDETerms.None_Implementation,
          ncp=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
          source_term=exahype2.solvers.fv.PDETerms.User_Defined_Implementation
        )   
        
    
      def get_user_includes(self):
        """
        
         We take this routine to add a few additional include statements.
         
        """
        return SuperClass.get_user_includes(self) + """
    #include "../PDE.h"
    #include "exahype2/PatchUtils.h"    
    """
    
    
      def add_constraint_verification(self):
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
        self._auxiliary_variables = 6
      
        self.set_preprocess_reconstructed_patch_kernel( """
        const int patchSize = """ + str( self._patch.dim[0] ) + """; 
        double volumeH = ::exahype2::getVolumeLength(marker.h(),patchSize);
        int n_a_v=6;
        dfor(cell,patchSize) {
          tarch::la::Vector<Dimensions,int> currentCell = cell + tarch::la::Vector<Dimensions,int>(1);
          
          // This constraint routine will evaluate both the solution per voxel
          // plus the derivative. The latter is something that we don't have, i.e.
          // we have to reconstruct it manually.
    
          // See the docu in PDE.h      
          double gradQ[3*59]={ 0 };
          
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
            for(int i=0; i<59; i++) {
              gradQ[d*59+i] = ( reconstructedPatch[rightCellSerialised*(59+n_a_v)+i] - reconstructedPatch[leftCellSerialised*(59+n_a_v)+i] ) / 2.0 / volumeH;
            }
          }
    
          // We will use a Fortran routine to compute the constraints per 
          // Finite Volume
          double constraints[n_a_v]={ 0 };
    
          // Central cell
          const int cellSerialised  = peano4::utils::dLinearised(currentCell, patchSize + 2*1);
         
          admconstraints_(constraints,reconstructedPatch+cellSerialised*(59+n_a_v),gradQ);
          
          for(int i=0;i<n_a_v;i++){
            reconstructedPatch[cellSerialised*(59+n_a_v)+59+i] = constraints[i];
          }
        }
    """)
        
        self.create_data_structures()
        self.create_action_sets()
    
      
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
        self._auxiliary_variables = 59*3
          
        self.set_preprocess_reconstructed_patch_kernel( """
        const int patchSize = """ + str( self._patch.dim[0] ) + """; 
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
            for (int i=0; i<59; i++) {
              reconstructedPatch[cellSerialised*(59*4)+59+i*3+d] = 
                ( reconstructedPatch[rightCellSerialised*(59*4)+i] - reconstructedPatch[leftCellSerialised*(59*4)+i] ) / 2.0 / volumeH;
            }
          }
        }
    """)
        
        self.create_data_structures()
        self.create_action_sets()

    
    project = exahype2.Project( ["examples", "exahype2", "ccz4"], "ccz4" )

   
    if SuperClass==exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize:
      my_solver = exahype2.solvers.aderdg.NonFusedGenericRusanovFixedTimeStepSize(
        "CCZ4", order, unknowns, 0, #auxiliary_variables
        exahype2.solvers.aderdg.Polynomials.Gauss_Legendre, 
        min_h, max_h, time_step_size,
        flux = None,
        ncp  = exahype2.solvers.aderdg.PDETerms.User_Defined_Implementation
      )
    else:
      my_solver = CCZ4Solver("CCZ4", args.patch_size, args.max_h, args.max_h)
      
      if args.extension=="gradient":
        my_solver.add_derivative_calculation()
      if args.extension=="adm":
        my_solver.add_constraint_verification()

   
    project.add_solver(my_solver)    
    
    build_mode = modes[args.mode]
    
    dimensions = 3
    end_time = 1.0

    periodic_boundary_conditions = [True,True,True]          # Periodic BC
    #periodic_boundary_conditions = [False,False,False]
        
    project.set_global_simulation_parameters(
      dimensions,               # dimensions
      [-0.5, -0.5, -0.5],  [1.0, 1.0, 1.0],
      end_time,                 # end time
      0.0, args.plot_step_size,   # snapshots
      periodic_boundary_conditions
    )
    
    project.set_Peano4_installation("../../..", build_mode)

    #project.set_output_path( "/cosma6/data/dp004/dc-zhan3/tem3" )

    project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision")

    peano4_project = project.generate_Peano4_project(verbose=True)
    
    peano4_project.output.makefile.add_Fortran_flag( "-DCCZ4EINSTEIN -DDim3" )
    
    use_intel = False
    if use_intel:
      pass
    else:
      peano4_project.output.makefile.add_Fortran_flag( "-lstdc++ -fdefault-real-8 -fdefault-double-8 -cpp -std=legacy -ffree-line-length-512 -fPIC" )
      peano4_project.output.makefile.add_CXX_flag( "-fPIE" )
      peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC -lgfortran" )
    

    
    #peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC -L/usr/lib/x86_64-linux-gnu -lgfortran" )

    # This might work for Intel (not tested)
    #peano4_project.output.makefile.add_Fortran_flag( "-r8 -cpp -auto -qopenmp-simd -O2" )
    #peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC" )
    # you might need -lifcore
    
    peano4_project.output.makefile.add_Fortran_module( "MainVariables.f90" )
    
    peano4_project.output.makefile.add_Fortran_files( 
      ["PDE.f90 ", "EinsteinConstraints.f90 ", "Properties.f90",
        "Metric.f90 ", "C2P-FOCCZ4.f90 ","ADMConstraints.f90"] 
    )
      
    #peano4_project.constants.export_string( "Scenario", "gaugewave-c++" )
    peano4_project.constants.export_string( "Scenario", "linearwave-c++" )

    peano4_project.generate( throw_away_data_after_generation=False )
    
    peano4_project.build( make_clean_first = True )
    #!make -j4

