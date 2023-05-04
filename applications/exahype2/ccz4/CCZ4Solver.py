import peano4
import exahype2
import dastgen2


from abc import abstractmethod


class AbstractCCZ4Solver(object):
  """!
  
  Abstract base class for any CCZ4 solver
  
  Each CCZ4 solver inherits from this abstract base class which really only 
  defines some generic stuff such as the unknowns and includes that every 
  single solver will use.
  
  If you use a CCZ4 solver, you will still have to add all the libraries to 
  your Peano project such that the Makefile picks them up. For this, the 
  solver offers an add_makefile_parameters().
    
  """
  
  
  """!
  
  Dictionary which specifies the unknown names plus their cardinality

  Has to be class attribute, as we need it in the constructor, i.e. before the 
  abstract object is created.

  """
  _FO_formulation_unknowns = {
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

# @todo Han, I need your help  
  _SO_formulation_unknowns = {
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
  
  # May not have a constructor. All the other numerical classes have 
  # constructors and their arguments always differ. So we cannot have a 
  # constructor here, as the arguments would have to match. Python's 
  # multiple inheritance is stupid here.
  #def __init__(self):
  #  pass


  def _add_standard_includes(self):
    """!
    
    Add the headers for the compute kernels and initial condition implementations 
    
    Usually called by the subclass constructor.

    """
    self.add_user_action_set_includes( """
#include "CCZ4Kernels.h"
#include "SecondOrderAuxiliaryVariablesReconstruction.h"
""")
    self.add_user_solver_includes( """
#include "CCZ4Kernels.h"
#include "InitialValues.h"
#include "SecondOrderAuxiliaryVariablesReconstruction.h"
#include <cstring>
""")
    
    
  def _add_solver_constants(self):
    """!

    Add domain-specific constants
        
    Usually called by the subclass constructor.

    I need a couple of constants. I could either replace them directly
    within the Python snippets below, but I prefer here to go a different
    way and to export them as proper C++ constants.

    There are two ways to inject solver constants into Peano: We can either
    add them to the Makefile as global const expressions, or we can add 
    them to the ExaHyPE2 solver. The latter is the route we go down here, 
    as these constants logically belong to the solver and not to the project.
    
    """
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
      self.add_solver_constants("static constexpr int {} = {};".format(key, value))
    for key, value in double_constants.items():   
      self.add_solver_constants("static constexpr double {} = {};".format(key, value))

  
  def add_makefile_parameters(self, 
                              peano4_project,
                              path_of_ccz4_application
                              ):
    """!
    
    Add include path and required cpp files to makefile
    
    If you have multiple CCZ4 solvers, i.e. different solvers of CCZ4 or multiple 
    instances of the CCZ4 type, please call this operation only once on one of 
    your solvers.
    
    """  
    if path_of_ccz4_application[-1]!="/":
      path_of_ccz4_application += "/"

    peano4_project.output.makefile.add_cpp_file( path_of_ccz4_application + "InitialValues.cpp", generated=False )
    peano4_project.output.makefile.add_cpp_file( path_of_ccz4_application + "CCZ4Kernels.cpp", generated=False )
    peano4_project.output.makefile.add_cpp_file( path_of_ccz4_application + "SecondOrderAuxiliaryVariablesReconstruction.cpp", generated=False )
    peano4_project.output.makefile.add_header_search_path( path_of_ccz4_application )


  @abstractmethod
  def add_tracer(name,
                 coordinates,
                 project):
    """!
    
    Add a tracer for the solver to the project solver
    
    The tracer is given a name and initial coordinates (list of three-tuples). 
    We need to know the underlying project as well, as we have to add the 
    tracing to the time stepping and the database update to the plotting.
    ~~~~~~~~~~~~~~~~~~~~~~~
    project.add_action_set_to_timestepping(my_interpolation)
    project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
      particle_set=tracer_particles,
      solver=my_solver,
       filename="Tracer-" + args.solver + "-" + str(args.cell_size),
      number_of_entries_between_two_db_flushes=5000,
      output_precision=10,
      data_delta_between_two_snapsots=1e16,
      time_delta_between_two_snapsots=0.01
      ))
    ~~~~~~~~~~~~~~~~~~~~~~~
    
    """
    assert "should not be called"
    pass
  
        
class CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking(AbstractCCZ4Solver,exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking):
  """!
  
  CCZ4 solver using finite volumes and global adaptive time stepping incl enclave tasking
  
  The constructor of this classs is straightforward and realises the standard 
  steps of any numerical implementation of the CCZ4 scheme:
  
  1. Init the actual numerical scheme. This happens through the constructor 
     of the base class.
     
  2. Add the header files that we need, i.e. those files which contain the 
     actual CCZ4 implementation.
     
  3. Add some constants that any CCZ4 C++ code requires.
  
  4. Set the actual implementation, i.e. link the generic PDE terms to the 
     CCZ4-specific function calls.
     
  5. Add the CCZ4-specific postprocessing.
    
  """
  def __init__(self,
               name,
               patch_size,
               min_volume_h,
               max_volume_h,
               use_heap_storage = True
               ):  
    super( CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking, self ).__init__(name                 = name,
                                                                                   patch_size           = patch_size,
                                                                                   unknowns             = sum(self._FO_formulation_unknowns.values()),
                                                                                   auxiliary_variables  = 0,
                                                                                   min_volume_h         = min_volume_h,
                                                                                   max_volume_h         = max_volume_h,
                                                                                   time_step_relaxation = 0.1
                                                                                   )
    self._add_standard_includes()

    if use_heap_storage:
      self.switch_to_heap_storage(True, True)

    self._add_solver_constants()

    self.set_implementation(
          boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          ncp                  = """
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;

    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
          flux                 = exahype2.solvers.fv.PDETerms.None_Implementation,
          source_term          = """
  memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
          refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          eigenvalues          = """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""          ,
          initial_conditions   = """
for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
"""
        )    

    self.postprocess_updated_patch = """
{
    constexpr int itmax = {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}};
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""

  def add_tracer(name,
                 coordinates,
                 project):
    tracer_particles = project.add_tracer( name=name,attribute_count=59 )
    project.add_action_set_to_initialisation( 
      exahype2.tracer.InsertParticlesByCoordinates( 
        particle_set=tracer_particles, coordinates=coordinates))
    project.add_action_set_to_timestepping(
      peano4.toolbox.particles.UpdateParallelState(
        particle_set=tracer_particles
      )
    )
    
    project.add_action_set_to_timestepping(exahype2.tracer.FiniteVolumesTracing(tracer_particles,
                                                self,
                                                time_stepping_kernel="toolbox::particles::staticPosition",
                                                project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
                                                ))

    project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
      particle_set=tracer_particles,
      solver=my_solver,
       filename="Tracer-" + args.solver + "-" + str(args.cell_size),
      number_of_entries_between_two_db_flushes=5000,
      output_precision=10,
      data_delta_between_two_snapsots=1e16,
      time_delta_between_two_snapsots=0.01
      ))


  
class CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking(AbstractCCZ4Solver,exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking):
  """!
  
  CCZ4 solver using fourth-order finite differences and global adaptive time stepping incl enclave tasking
  
  The constructor of this classs is straightforward and realises the standard 
  steps of any numerical implementation of the CCZ4 scheme:
  
  1. Init the actual numerical scheme. This happens through the constructor 
     of the base class.
     
  2. Add the header files that we need, i.e. those files which contain the 
     actual CCZ4 implementation.
     
  3. Add some constants that any CCZ4 C++ code requires.
  
  4. Set the actual implementation, i.e. link the generic PDE terms to the 
     CCZ4-specific function calls.
     
  5. Add the CCZ4-specific postprocessing.
  
  6. Switch to higher-order interpolation and restriction.
    
  """
  def __init__(self,
               name,
               patch_size,
               rk_order,
               min_meshcell_h,
               max_meshcell_h,
               use_heap_storage = True
               ):  
    super( CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking, self ).__init__(name                 = name,
                                                                                    patch_size           = patch_size,
                                                                                    rk_order             = rk_order,
                                                                                    unknowns             = sum(self._FO_formulation_unknowns.values()),
                                                                                    auxiliary_variables  = 0,
                                                                                    min_meshcell_h         = min_meshcell_h,
                                                                                    max_meshcell_h         = max_meshcell_h,
                                                                                    time_step_relaxation = 0.1
                                                                                    )
    
    self._add_standard_includes()

    if use_heap_storage:
      self.switch_to_heap_storage(True, True)

    self._add_solver_constants()

    self.set_implementation(
          boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          ncp                  = """
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;

    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
          flux                 = exahype2.solvers.fv.PDETerms.None_Implementation,
          source_term          = """
  memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
          refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          eigenvalues          = """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""          ,
          initial_conditions   = """
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
  ::applications::exahype2::ccz4::gaugeWave(Q, meshCellCentre, 0);
"""
        )

    self.postprocess_updated_patch = """
{
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""

    exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_interpolation( self, "TP_linear_with_linear_extrap_normal_interp" ) 
    exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_restriction(   self, "TP_average_normal_extrap" ) 


  def add_tracer(name,
                 coordinates):
    tracer_particles = project.add_tracer( name=name,attribute_count=59 )
    project.add_action_set_to_initialisation( 
      exahype2.tracer.InsertParticlesByCoordinates( 
        particle_set=tracer_particles, coordinates=coordinates))
    project.add_action_set_to_timestepping(
      peano4.toolbox.particles.UpdateParallelState(
        particle_set=tracer_particles
      )
    )    
    project.add_action_set_to_timestepping(exahype2.tracer.FiniteVolumesTracing(tracer_particles,
                                                self,
                                                time_stepping_kernel="toolbox::particles::staticPosition",
                                                project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
                                                ))

    project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
      particle_set=tracer_particles,
      solver=my_solver,
       filename="Tracer-" + args.solver + "-" + str(args.cell_size),
      number_of_entries_between_two_db_flushes=5000,
      output_precision=10,
      data_delta_between_two_snapsots=1e16,
      time_delta_between_two_snapsots=0.01
      ))



class CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking(AbstractCCZ4Solver,exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStepWithEnclaveTasking):
  """!
  
  Variation of classic FD4 which relies on second order PDE formulation
  
  The traditional ExaHyPE CCZ4 formulation is the first order formulation 
  introduced by Dumbser et al. In this formulation, the second order terms
  in CCZ4 are substituted with helper variables which represent first order
  derivatives. While formally straightforward, keeping the whole system 
  consistent and stricly hyperbolic is a different challenge.
  
  In this revised version, we have to evolve the primary quantities of CCZ4
  and also the helper variables, which blows the overall system up to 59
  equations in its simplest form. The work by Dumbser and others suggest that
  this is a consistent and stable approach, but limited work is actually 
  published on proper physical simulations. We therefore also implemented a 
  second order PDE version within ExaHyPE.
  
  This second order variant is not really second order from the start. 
  Instead, we use the first order formulation, and we reconstruct the helper
  term via finite differences prior to the compute kernel application. That is,
  the compute kernels see variables representing first order derivatives, and
  they also evolve these guys. Afterwards, we throw away the evolved quantities
  and reconstruct them from the primary unknowns prior to the next time step.
  
  This might not be super efficient (it would be faster to stick to the 
  second order formulation right from the start), but it allows us to reuse
  the compute kernels written for the first order PDE formulation.
  
  ## Data layout
  
  We have now a smaller number of real unknowns, i.e. only those guys who
  belong to the "original" second-order formulation. The remaining quantities
  compared to a first-order formulation are technically material or auxiliary
  quantities. We model them as such, which allows ExaHyPE's data management
  to deal more efficiently with them.


  reconstruction_type: "4thOrder", "centralDifferences", "leftDifference", "rightDifference"
      
  """
  def __init__(self,
               name,
               patch_size,
               rk_order,
               min_meshcell_h,
               max_meshcell_h,
               reconstruction_type,
               use_heap_storage = True
               ):  
    super( CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking, self ).__init__(name                 = name,
                                                                                    patch_size           = patch_size,
                                                                                    rk_order             = rk_order,
                                                                                    unknowns             = sum(self._SO_formulation_unknowns.values()),
                                                                                    auxiliary_variables  = sum(self._FO_formulation_unknowns.values())-sum(self._SO_formulation_unknowns.values()),
                                                                                    min_meshcell_h         = min_meshcell_h,
                                                                                    max_meshcell_h         = max_meshcell_h,
                                                                                    time_step_relaxation = 0.1
                                                                                    )
    
    self._add_standard_includes()

    if use_heap_storage:
      self.switch_to_heap_storage(True, True)

    self._add_solver_constants()

    self.set_implementation(
          boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          ncp                  = """
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;

    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncpSecondOrderFormulation(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
          flux                 = exahype2.solvers.fv.PDETerms.None_Implementation,
          source_term          = """
  memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::sourceSecondOrderFormulation(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
          refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
          eigenvalues          = """
          // do we only set Q
  return ::applications::exahype2::ccz4::maxEigenvalueSecondOrderFormulation(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""          ,
          initial_conditions   = """
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
  ::applications::exahype2::ccz4::gaugeWaveSecondOrderFormulation(Q, meshCellCentre, 0);
"""
        )

    self.postprocess_updated_patch = """
{
    constexpr int itmax = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraintsSecondOrderFormulation( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""

    exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_interpolation( self, "TP_linear_with_linear_extrap_normal_interp" ) 
    exahype2.solvers.rkfd.fd4.switch_to_FD4_tensor_product_restriction(   self, "TP_average_normal_extrap" ) 

    self.preprocess_reconstructed_patch = """
::exahype2::CellData  reconstructedPatchData( 
  oldQWithHalo, 
  marker.x(), 
  marker.h(), 
  timeStamp, 
  timeStepSize,
  nullptr // targetPatch 
);
::applications::exahype2::ccz4::recomputeAuxiliaryVariablesFD4_""" + reconstruction_type + """(
  reconstructedPatchData,
  {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
  3,                        // haloSize,
  {{NUMBER_OF_UNKNOWNS}},
  {{NUMBER_OF_AUXILIARY_VARIABLES}}
);
"""


  def add_tracer(name,
                 coordinates):
    tracer_particles = project.add_tracer( name=name,attribute_count=59 )
    project.add_action_set_to_initialisation( 
      exahype2.tracer.InsertParticlesByCoordinates( 
        particle_set=tracer_particles, coordinates=coordinates))
    project.add_action_set_to_timestepping(
      peano4.toolbox.particles.UpdateParallelState(
        particle_set=tracer_particles
      )
    )    
    project.add_action_set_to_timestepping(exahype2.tracer.FiniteVolumesTracing(tracer_particles,
                                                self,
                                                time_stepping_kernel="toolbox::particles::staticPosition",
                                                project_on_tracer_properties_kernel="::exahype2::fv::projectAllValuesOntoParticle_piecewiseLinear"
                                                ))

    project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
      particle_set=tracer_particles,
      solver=my_solver,
       filename="Tracer-" + args.solver + "-" + str(args.cell_size),
      number_of_entries_between_two_db_flushes=5000,
      output_precision=10,
      data_delta_between_two_snapsots=1e16,
      time_delta_between_two_snapsots=0.01
      ))
  
        
class CCZ4Solver_RKDG_GlobalAdaptiveTimeStepWithEnclaveTasking(AbstractCCZ4Solver,exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking):
  """!
  
  CCZ4 solver using Runge-Kutta Discontinuous Galerkin and global adaptive time stepping incl enclave tasking
  
  The constructor of this classs is straightforward and realises the standard 
  steps of any numerical implementation of the CCZ4 scheme:
  
  1. Init the actual numerical scheme. This happens through the constructor 
     of the base class.
     
  2. Add the header files that we need, i.e. those files which contain the 
     actual CCZ4 implementation.
     
  3. Add some constants that any CCZ4 C++ code requires.
  
  4. Set the actual implementation, i.e. link the generic PDE terms to the 
     CCZ4-specific function calls.
     
  5. Add the CCZ4-specific postprocessing.
  
  6. Switch to higher-order interpolation and restriction.
    
  """
  def __init__(self,
               name,
               rk_order,
               polynomials,
               min_cell_h,
               max_cell_h,
               use_heap_storage = True
               ):  
    super( CCZ4Solver_RKDG_GlobalAdaptiveTimeStepWithEnclaveTasking, self ).__init__(name                 = name,
                                                                                     rk_order             = rk_order,
                                                                                     polynomials          = polynomials,
                                                                                     face_projections     = exahype2.solvers.rkdg.FaceProjections.Solution,
                                                                                     unknowns             = sum(self._FO_formulation_unknowns.values()),
                                                                                     auxiliary_variables  = 0,
                                                                                     min_cell_h         = min_cell_h,
                                                                                     max_cell_h         = max_cell_h,
                                                                                     time_step_relaxation = 0.1
                                                                                     )
    
    self._add_standard_includes()

    if use_heap_storage:
      self.switch_to_heap_storage(True, True)

    self._add_solver_constants()

    self.set_implementation(
          boundary_conditions  = exahype2.solvers.rkdg.PDETerms.Empty_Implementation,
          ncp                  = """
  double dQdxSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    dQdxSerialised[i+0*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+1*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+2*NumberOfUnknowns] = 0.0;

    dQdxSerialised[i+normal*NumberOfUnknowns] = dQdx[i];
  }
  ::applications::exahype2::ccz4::ncp(BgradQ, Q, dQdxSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
          flux                 = exahype2.solvers.rkdg.PDETerms.None_Implementation,
          source_term          = """
  memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
          refinement_criterion = exahype2.solvers.rkdg.PDETerms.Empty_Implementation,
          eigenvalues          = """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
"""          ,
          initial_conditions   = """
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
  ::applications::exahype2::ccz4::gaugeWave(Q, x, 0);
"""
        )

    self.postprocess_updated_cell_after_final_linear_combination = """
{
    constexpr int itmax = ({{DG_ORDER}}+1) * ({{DG_ORDER}}+1) * ({{DG_ORDER}}+1);
    int index = 0;
    for (int i=0;i<itmax;i++)
    {
      applications::exahype2::ccz4::enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""



  def add_tracer(name,
                 coordinates):
      
    tracer_particles = project.add_tracer( name=name,attribute_count=59 )
    project.add_action_set_to_initialisation( 
      exahype2.tracer.InsertParticlesByCoordinates( 
        particle_set=tracer_particles, coordinates=coordinates))
    project.add_action_set_to_timestepping(
      peano4.toolbox.particles.UpdateParallelState(
        particle_set=tracer_particles
      )
    )
    
    project.add_action_set_to_timestepping(exahype2.tracer.DiscontinuousGalerkinTracing(tracer_particles,
                                                                                        my_solver,
                                                                                        time_stepping_kernel="toolbox::particles::staticPosition",
                                                                                        project_on_tracer_properties_kernel="::exahype2::dg::projectAllValuesOntoParticle"))

    project.add_action_set_to_timestepping(exahype2.tracer.DumpTrajectoryIntoDatabase(
      particle_set=tracer_particles,
      solver=my_solver,
       filename="Tracer-" + args.solver + "-" + str(args.cell_size),
      number_of_entries_between_two_db_flushes=5000,
      output_precision=10,
      data_delta_between_two_snapsots=1e16,
      time_delta_between_two_snapsots=0.01
      ))
    
    
