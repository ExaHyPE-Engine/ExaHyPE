# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import peano4.toolbox.multigrid.vertexbased.ScalarJacobiWithRediscretisation

import jinja2

from abc import abstractmethod

import exahype2

import dastgen2
import peano4.datamodel.DaStGen2


class AMRMarker(object):
  """ 
    A very simple Poisson equation system solver to identify AMR transitions
    
    Some codes need to know where AMR transitions are and want to flag the 
    areas around these transitions. This class implements a simple finite 
    differences diffusion implementation which eventually will become 
    stationary if your mesh does not change anymore. The code solves
    
    @f$ - \Delta u = f @f$
    
    for a fixed right-hand side f with a Jacobi solver. The solution is 
    subject to the constraints 

    @f$ 0 \leq u \leq 1 @f$

    and Dirichlet boundary conditions. Finally, the PDE has prescribed interior
    values of 1 for every hanging vertex. So we have 1 at AMR transitions (and
    the boundary if that's wanted) and we see the solution u fade away once we
    move away from the boundary, until is eventually becomes 0.
    
    As we use a Jacobi iterative solver, using this marker within ExaHyPE is 
    equivalent to a heat equation solver which becomes stationary eventually if
    the grid does not change. Regions with a small solution value (remember that
    all solution values are between 0 and 1) are far away from any AMR boundary.
    Regions with a value close to 1 are close to an AMR boundary.
    
    
    ## Usage
    
    Before you start, you have to reconfigure and add 
    
        --enable-fem
        
    to your configuration call. Run make afterwards to build the FEM toolbox of
    Peano.
    
    
    To use the marker within your project, add a new solver to your project:
    
         amr_label = exahype2.solvers.elliptic.AMRMarker( name="AMRMarker" )
         project.add_solver( amr_label )

    Take your ExaHyPE solver of choice and add one more auxiliary variable. This
    variable will, from now on, hold a copy of the actual iterate value of the 
    elliptic solver. 
    
    
    Call the couple_with_FV_solver() operation on the AMRMarker so ensure that
    the AMR marker pipes its data into the auxiliary variable. So if you solve
    Euler with 5 unknowns, you add one auxiliary variable. The additional 
    statement
    
         amr_label.couple_with_FV_solver( thesolver, 0 )

    then makes the elliptic solver write the patch average of its marker into
    the first (aka index 0) auxiliary quantity per volume. 
    
    
    ## Use cases
    
    ExaHyPE codes usually use this for some damping or smearing close to the
    AMR boundaries where instabilities due to reflecting waves can arise. There
    might be other use cases.
    
    In ExaHyPE, you add the AMRMarker and you project (couple) it onto an 
    auxiliary variable. When you evaluate the Poisson operator effectively 
    delivering a damping, you can now multiply the impact of this operator
    with the auxiliary variable. This way, it is active near AMR boundaries, 
    but it disappears far away from there.
    
    
    ## Pitfall
    
    Ensure that you initialise all additional auxiliary variables properly in 
    ExaHyPE. If you don't initialise them, they will contain garbage will will
    likely mess up follow-up calculations.
    
    The same holds for the boundary data. Even though we don't exchange 
    auxiliary data between patches, you have to set boundary data for them 
    to satisfy ExaHyPE's data correctness checks.

  """
  
      
  def __init__(self, name, relaxation_coefficient = 0.6, right_hand_side = -10.0, flag_domain_boundary = True, plot = False):
    """
    
  name: string
     A unique name for the solver. This one will be used for all generated 
     classes.
  
  plot: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
     
  relaxation_coefficient: Double (0< value <1)
     How quickly should the solution be relaxed against real solution. This
     is the damping parameter in the Jacobi update scheme and thus has to be
     bigger than 0 and smaller than 1. If the coefficient approaches 1, you 
     might see oscillations.
     
  right_hand_side: Double (negative)
     Basically describes how fast the solution is pulled down to 0 once we
     go away from the AMR transition area.
 
    """
    self._name                   = name
    self._relaxation_coefficient = relaxation_coefficient
    self._flag_domain_boundary   = flag_domain_boundary
    self._right_hand_side        = right_hand_side
    self._plot                   = plot
    
    self._volumetric_coupling_term = ""
    
    self.create_data_structures()
    self.create_action_sets()


  def __str__(self):
    return """
Name:                   {}
Type:                   {}
Relaxation coefficient: {}
""".format( 
  self._name,
  self.__class__.__name__,
  self._relaxation_coefficient
)


  __repr__ = __str__

  
  def create_readme_descriptor(self, domain_offset, domain_size):
    return """
### ExaHyPE 2 solver

""" + str(self)   

 
  def create_data_structures(self):
    """
    
       
    """
    self._data_model = peano4.datamodel.DaStGen2( self._unknown_identifier() )
    self._data_model.data.add_attribute( dastgen2.attributes.Double("u") )
    self._data_model.data.add_attribute( dastgen2.attributes.Double("residual") )
    
    self._data_model.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
        "true", "true", "true"
        )

    self._data_model.generator.send_condition               = "true"
    self._data_model.generator.receive_and_merge_condition  = "true"

    self._data_model.peano4_mpi_and_storage_aspect.merge_implementation = """
  setResidual( getResidual() + neighbour.getResidual() );
"""    
    

  @abstractmethod
  def create_action_sets(self):
    """
    
     Overwrite in subclasses if you wanna create different
     action sets.
     
     :: Call order and ownership
     
     This operation can be called multiple times. However, only the very
     last call matters. All previous calls are wiped out.
     
     If you have a hierarchy of solvers, every create_data_structure()
     should first(!) call its parent version. This way, you always ensure
     that all data are in place before you continue to alter the more
     specialised versions. So it is (logically) a top-down (general to
     specialised) run through all create_data_structure() variants 
     within the inheritance tree.
     
    """
    self._action_set_smoother = peano4.toolbox.multigrid.vertexbased.ScalarJacobiWithRediscretisation( 
      self._data_model, 
      self._relaxation_coefficient, 
      "{}".format(self._right_hand_side) 
      )
    self._action_set_smoother.Template_CreateHangingVertex = """
  fineGridVertex{{UNKNOWN}}.{{UNKNOWN_SETTER}}(1.0);
  fineGridVertex{{UNKNOWN}}.{{RESIDUAL_SETTER}}(0.0);
"""    

    updated_value_postprocessing = """
  bool isBoundary = false;
  tarch::la::Vector<Dimensions,double>  domainOffset(DomainOffset);
  tarch::la::Vector<Dimensions,double>  domainSize(DomainSize);
  for (int d=0; d<Dimensions; d++) {
    isBoundary |= tarch::la::equals( marker.x()(d), domainOffset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), domainOffset(d)+domainSize(d) );
  }
  if (isBoundary) {
    fineGridVertex{{UNKNOWN}}.{{UNKNOWN_SETTER}}(boundaryValue);
  }
  if ( fineGridVertex{{UNKNOWN}}.{{UNKNOWN_GETTER}}()<0.0 ) {
    fineGridVertex{{UNKNOWN}}.{{UNKNOWN_SETTER}}(0.0);
  }
""" 
    if self._flag_domain_boundary:
      self._action_set_smoother.Template_UpdateValue += """
const double boundaryValue = 1.0;
""" + updated_value_postprocessing
    else:
      self._action_set_smoother.Template_UpdateValue += """
const double boundaryValue = 0.0;
""" + updated_value_postprocessing
   
    self._action_set_smoother.Template_TouchCellFirstTime += self._volumetric_coupling_term
    self._action_set_smoother.Template_CreatePersistentVertex = """
  fineGridVertex{{UNKNOWN}}.{{UNKNOWN_SETTER}}(1.0);
"""    
    self._action_set_smoother.additional_includes = """
#include "Constants.h"
"""    

  def couple_with_FV_solver(self,FV_solver,auxiliary_variable):
    """
    
      FV_solver: exahype2.solver.fv.Solver
        Instance of the solver you want to couple this marker to
        
      auxiliary_variable: Integer
        index of the auxiliary variable that you want this class to dump its output
        to.
         
    """
    self._volumetric_coupling_term = """
  double cellAverage = 0.0;
  for(int k=0; k<TwoPowerD; k++) {{
    cellAverage += fineGridVertices{}(k).getU();
  }}
  cellAverage /= TwoPowerD;
  int index = {};
  dfor(k,{}) {{
    fineGridCell{}Q.value[index] = cellAverage;
    index += {};
  }}
""".format(self._unknown_identifier(),FV_solver._unknowns+auxiliary_variable,FV_solver.patch_size,FV_solver.name,FV_solver._unknowns+FV_solver._auxiliary_variables)    

    # @todo There's a bug somewhere. I can't access unkonwns as property (it always returns patch size, while _unknowns works
    
    self.create_action_sets()
  
  
  def _unknown_identifier(self):
    return self._name+"AMRValue"
  

  def get_name_of_global_instance(self):
    return "instanceOf" + self._name

  
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """
    
      Add all required data to the Peano4 project's datamodel 
      so it is properly built up
      
    """
    if verbose:
      print( "AMR marker" )
      print( "----------" )
      print( str(self._name) )
    datamodel.add_vertex(self._data_model)

 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    """
      Tell Peano what data to move around
      
      Inform Peano4 step which data are to be moved around via the 
      use_cell and use_face commands. This operation is generic from
      ExaHyPE's point of view, i.e. I use it for all grid sweep types. 
    
    """
    step.use_vertex(self._data_model)

  
#  def _get_default_includes(self):
#    return """
##include "tarch/la/Vector.h" 
#
##include "peano4/utils/Globals.h"
##include "peano4/utils/Loop.h"
#
##include "repositories/SolverRepository.h"
#
##include "Constants.h"
#"""


  def add_actions_to_init_grid(self, step):
    """
    
    
    """
    step.add_action_set( self._action_set_smoother )
    pass

    
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    """
    
     It is important to plug into the creation, as this is the place where
     we get the creational events.
     
    """
    step.add_action_set( self._action_set_smoother )

  
  def add_actions_to_plot_solution(self, step, output_path):
    """
    
     We plot if and only if we are asked to do so
     
    """
    ## Add it. I can't harm to do one more iteration, and we at least
    ## get the hanging nodes right this way
    step.add_action_set( self._action_set_smoother )
    if self._plot:
      step.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat( 
        filename=output_path + "amr-marker-" + self._name, 
        vertex_unknown=self._data_model,
        getter="getU()", 
        description="marker-value",
        time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
        additional_includes="""
#include "repositories/SolverRepository.h"       
"""        
      ))
   
 
  def add_actions_to_perform_time_step(self, step):
    """
    
    AMR
    
    It is important that we do the inter-grid transfer operators before we 
    apply the boundary conditions.
    
    """
    step.add_action_set( self._action_set_smoother )


  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass


  def add_implementation_files_to_project(self,namespace,output, dimensions, subdirectory=""):
    """
    
     The ExaHyPE2 project will call this operation when it sets
     up the overall environment.
     
     This routine is typically not invoked by a user.

     output: peano4.output.Output
     
    """
    templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/AMRMarker"

    if(subdirectory):
        subdirectory += "/"

    headerDictionary = {}
    implementationDictionary = {}
#    self._init_dictionary_with_default_parameters(headerDictionary)
#    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(headerDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self._name, 
      namespace,
      subdirectory + ".", 
      implementationDictionary,
      True,
      True)

    output.add( generated_solver_files )
    output.makefile.add_cpp_file( subdirectory + self._name + ".cpp", generated=True )


#  def _init_dictionary_with_default_parameters(self,d):
#    """
#
#      This one is called by all algorithmic steps before I invoke
#      add_entries_to_text_replacement_dictionary().
#
#      See the remarks on set_postprocess_updated_patch_kernel to understand why
#      we have to apply the (partially befilled) dictionary to create a new entry
#      for this very dictionary.##
##
#
#    """
#    d["RIGHT_HAND_SIDE"]        = self._right_hand_side
#    d["RELAXATION_COEFFICIENT"] = self._relaxation_coefficient
#    if self._flag_domain_boundary:
#      d["BOUNDARY_VALUE"]       = 1.0
#    else:
#      d["BOUNDARY_VALUE"]       = 0.0
#    
#    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
#    d["SOLVER_NAME"]                    = self._name


  @property
  def name(self):
    return self._name

