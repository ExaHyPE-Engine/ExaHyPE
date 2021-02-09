import os
import peano4
import exahype2
import admc

class CCZ4Solver( exahype2.solvers.fv.GenericRusanovAdaptiveTimeStepSizeWithEnclaves ):
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
    print( "number of unknowns=", number_of_unknowns )
    
    #number_of_auxiliary_variables = 0
    number_of_auxiliary_variables  = 6

    exahype2.solvers.fv.GenericRusanovAdaptiveTimeStepSizeWithEnclaves.__init__( 
      self,
      name=name, patch_size=patch_size, 
      unknowns=number_of_unknowns, 
      auxiliary_variables=number_of_auxiliary_variables, 
      min_h=min_h, max_h=max_h, 
      time_step_relaxation=0.3
    )
    
    self._solver_template_file_class_name = exahype2.solvers.fv.GenericRusanovAdaptiveTimeStepSizeWithEnclaves.__name__
    self.add_constraint_verification()


  def add_constraint_verification(self):
    """
     
     Add the constraint verification code
     
     We introduce new auxiliary variables. Prior to each time step, I 
     compute the Laplacian and store it in the auxiliary variable. This
     is kind of a material parameter F(Q) which does not feed back into
     the solution.
     
    """
    if self._auxiliary_variables != 6:
      raise Exception( "number of constraints does not match." )
    self._preprocess_reconstructed_patch  = """
    const int patchSize = """ + str(self._patch.dim[0]) + """;
    dfor(sourceCell,patchSize) {
      tarch::la::Vector<Dimensions,int> destinationCell = sourceCell + tarch::la::Vector<Dimensions,int>(1);
      
      tarch::la::Vector<Dimensions,int> cell;
      int cellSerialised;
      /*double* Q_deri;
      double* Q_putin;
      double* constraints;
      memset(Q_putin, 0.1, 59*sizeof(double));
      memset(Q_deri, 0.1, 3*59*sizeof(double));
      memset(constraints, .0, 6*sizeof(double));*/
      double Q_deri[59*3]; double Q_putin[59]; double constraints[6]={ 0 };
      for(int i=0; i<59; i++){
      	  cell = destinationCell;
	      cellSerialised  = peano4::utils::dLinearised(destinationCell,patchSize + 2*1);
	      double centralValue = reconstructedPatch[cellSerialised*(59+6)+i];
	      Q_putin[i]=centralValue;

	      cell            = destinationCell;
	      cell(0) = cell(0)-1;
	      cellSerialised  = peano4::utils::dLinearised(destinationCell,patchSize + 2*1);
	      double leftValue = reconstructedPatch[cellSerialised*(59+6)+i]; 
	      cell            = destinationCell;
	      cell(0) = cell(0)+1;
	      cellSerialised  = peano4::utils::dLinearised(destinationCell,patchSize + 2*1);
	      double rightValue = reconstructedPatch[cellSerialised*(59+6)+i]; 
	      Q_deri[i+59*0]=(rightValue-leftValue)/(2*marker.h()(0));

	      cell            = destinationCell;
	      cell(0) = cell(0)-1*(patchSize+2);
	      cellSerialised  = peano4::utils::dLinearised(destinationCell,patchSize + 2*1);
	      leftValue = reconstructedPatch[cellSerialised*(59+6)+i]; 
	      cell            = destinationCell;
	      cell(0) = cell(0)+1*(patchSize+2);
	      cellSerialised  = peano4::utils::dLinearised(destinationCell,patchSize + 2*1);
	      rightValue = reconstructedPatch[cellSerialised*(59+6)+i]; 
	      Q_deri[i+59*1]=(rightValue-leftValue)/(2*marker.h()(1));
	      
	      cell            = destinationCell;
	      cell(0) = cell(0)-1*(patchSize+2)*(patchSize+2);
	      cellSerialised  = peano4::utils::dLinearised(destinationCell,patchSize + 2*1);
	      leftValue = reconstructedPatch[cellSerialised*(59+6)+i]; 
	      cell            = destinationCell;
	      cell(0) = cell(0)+1*(patchSize+2)*(patchSize+2);
	      cellSerialised  = peano4::utils::dLinearised(destinationCell,patchSize + 2*1);
	      rightValue = reconstructedPatch[cellSerialised*(59+6)+i]; 
	      Q_deri[i+59*2]=(rightValue-leftValue)/(2*marker.h()(2));
	  }
	  
	  admconstraints_(constraints,Q_putin,Q_deri);
	  
	  for(int i=0;i<6,i++;){
	      reconstructedPatch[cellSerialised*(59+6)+59+i] = constraints[i]+3;
	  }
      //free(Q_putin); free(Q_deri); free(constraints);
    }
"""
    
    
if __name__ == "__main__":
    project = exahype2.Project( ["examples", "exahype2", "ccz4"], "ccz4" )

    time_step_size      = 0.001
    max_h               = 0.4
    min_h               = 0.4
    patch_size          = 6
    
    my_solver = CCZ4Solver(
      "CCZ4", patch_size, min_h, max_h
    )
    
    project.add_solver(my_solver)
    
    import exahype2.sympy
    
    pde = exahype2.sympy.PDE(unknowns=my_solver._unknowns,auxiliary_variables=my_solver._auxiliary_variables,dimensions=3)
    
    my_solver.set_implementation(
      boundary_conditions=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
      flux=exahype2.solvers.fv.PDETerms.None_Implementation,
      ncp=exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
      source_term=exahype2.solvers.fv.PDETerms.User_Defined_Implementation
    )   
    
    build_mode = peano4.output.CompileMode.Asserts
    #build_mode = peano4.output.CompileMode.Release
    
    dimensions = 3
    end_time = 1.0
    snapshots = time_step_size*40
        
    project.set_global_simulation_parameters(
      dimensions,               # dimensions
      [-0.5, -0.5, -0.5],  [1.0, 1.0, 1.0],
      end_time,                 # end time
      0.0, snapshots,   # snapshots
      [True,True,True]          # Periodic BC
    )
    
    project.set_Peano4_installation("../../..", build_mode)
    
    peano4_project = project.generate_Peano4_project()
    
    peano4_project.output.makefile.add_Fortran_flag( "-DCCZ4EINSTEIN -DDim3" )
    
    # This is for GNU
    peano4_project.output.makefile.add_Fortran_flag( "-lstdc++ -fdefault-real-8 -fdefault-double-8 -cpp -std=legacy -ffree-line-length-512 -fPIC" )
#add routie to find the gfortran
#need to create a link first:sudo ln -s /usr/lib/x86_64-linux-gnu/libgfortran.so.4 /usr/lib/x86_64-linux-gnu/libgfortran.so
    peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC -L/usr/lib/x86_64-linux-gnu -lgfortran" )
    
    # This might work for Intel (not tested)
    #peano4_project.output.makefile.add_Fortran_flag( "-r8 -cpp -auto -qopenmp-simd -O2" )
    #peano4_project.output.makefile.add_linker_flag( "-lstdc++ -fPIC" )
    # you might need -lifcore
    
    peano4_project.output.makefile.add_Fortran_module( "MainVariables.f90" )
    
    peano4_project.output.makefile.add_Fortran_files( 
      ["PDE.f90 ", "EinsteinConstraints.f90 ", "Properties.f90",
        "Metric.f90 ", "C2P-FOCCZ4.f90 ","ADMConstraints.f90"] 
    )
    
    peano4_project.constants.export_string( "Scenario", "gaugewave-c++" )
    
    peano4_project.generate( throw_away_data_after_generation=False )
    
    parallel_builds = 4   # I don't use a massively parallel build here as my laptop otherwise becomes too hot.
                          # Without any arguments, the build process will grab all of your cores.
    peano4_project.build( make_clean_first = False, number_of_parallel_builds = parallel_builds )
    #!make -j4

