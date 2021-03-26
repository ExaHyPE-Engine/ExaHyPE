# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import jinja2

from abc import abstractmethod

from peano4.solversteps.ActionSet import ActionSet

from peano4.toolbox.blockstructured.ProjectPatchOntoFaces import ProjectPatchOntoFaces
from peano4.toolbox.blockstructured.BackupPatchOverlap    import BackupPatchOverlap


class AbstractFVActionSet( ActionSet ):
  def __init__(self,solver):
    """
   
    solver: ADERDG
      Reference to creating class 
   
    """
    self._solver = solver
    pass
  
  
  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_includes(self):
    return """
#include <functional>
#include "exahype2/PatchUtils.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes() 


class AMROnPatch(AbstractFVActionSet):
  TemplateAMR = """  
  if ({{PREDICATE}}) { 
    ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();

    if (tarch::la::max( marker.h() ) > repositories::{{SOLVER_INSTANCE}}.getMaxMeshSize() ) {
      refinementCriterion = ::exahype2::RefinementCommand::Refine;
    } 
    else {
      int index = 0;
      dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
        refinementCriterion = refinementCriterion and repositories::{{SOLVER_INSTANCE}}.refinementCriterion(
          fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
          ::exahype2::getVolumeCentre( marker.x(), marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume), 
          ::exahype2::getVolumeSize( marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
          repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp()
        );
        index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
      }
     
      if (refinementCriterion==::exahype2::RefinementCommand::Refine and tarch::la::max( marker.h() ) < repositories::{{SOLVER_INSTANCE}}.getMinMeshSize() ) {
        refinementCriterion = ::exahype2::RefinementCommand::Keep;
      } 
      else if (refinementCriterion==::exahype2::RefinementCommand::Coarsen and 3.0* tarch::la::max( marker.h() ) > repositories::{{SOLVER_INSTANCE}}.getMaxMeshSize() ) {
        refinementCriterion = ::exahype2::RefinementCommand::Keep;
      } 
    }
    
    _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion );
  }
  """
  
    
  def __init__(self,solver, predicate):
    AbstractFVActionSet.__init__(self,solver)
    self._predicate = predicate

  
  def get_body_of_getGridControlEvents(self):
    return """
  return repositories::refinementControl.getGridControlEvents();
""" 


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = """
  _localRefinementControl.clear();
"""

    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      result = """
  repositories::refinementControl.merge( _localRefinementControl );
"""
    
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      if self._solver._patch.dim[0] != self._solver._patch.dim[1]:
        raise Exception( "Error: Can only handle square patches." )
      
      d[ "UNKNOWNS" ]           = str(self._solver._patch.no_of_unknowns)
      d[ "DOFS_PER_AXIS" ]      = str(self._solver._patch.dim[0])
      d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_2D" ] = str(self._solver._patch.no_of_unknowns * self._solver._patch.dim[0] * self._solver._patch.dim[0])
      d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_3D" ] = str(self._solver._patch.no_of_unknowns * self._solver._patch.dim[0] * self._solver._patch.dim[0] * self._solver._patch.dim[0])
      d[ "CELL_ACCESSOR" ]                                = "fineGridCell" + self._solver._patch.name
      d[ "PREDICATE" ]          = self._predicate
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)      
      result = jinja2.Template( self.TemplateAMR ).render(**d)

    return result


  def get_attributes(self):
    return """
    ::exahype2::RefinementControl         _localRefinementControl;
"""


class AdjustPatch(AbstractFVActionSet):
  TemplateAdjustCell = """
  if ({{PREDICATE}}) { 
    int index = 0;
    dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
      repositories::{{SOLVER_INSTANCE}}.adjustSolution(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
        ::exahype2::getVolumeCentre( marker.x(), marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume), 
        ::exahype2::getVolumeSize( marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize()
      );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  } 
"""
  
  def __init__(self,solver,predicate):
    AbstractFVActionSet.__init__(self,solver)
    self._predicate = predicate


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ] = self._predicate      
      result = jinja2.Template(self.TemplateAdjustCell).render(**d)
      pass 
    return result


class HandleBoundary(AbstractFVActionSet):
  """
  
    The global periodic boundary conditions are set in the Constants.h. 
   
  """
  TemplateHandleBoundary = """
    logDebug( 
      "touchFaceFirstTime(...)", 
      "label=" << fineGridFaceLabel.toString() << ", " << repositories::{{SOLVER_INSTANCE}}.PeriodicBC[marker.getSelectedFaceNumber()%Dimensions] << 
      ",marker=" << marker.toString()
    );
    if (
      {{PREDICATE}}
      and
      not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[marker.getSelectedFaceNumber()%Dimensions]
      and
      not marker.isRefined() 
      and 
      fineGridFaceLabel.getBoundary()
    ) {
      ::exahype2::fv::applyBoundaryConditions(
        [&](
          const double * __restrict__                  Qinside,
          double * __restrict__                        Qoutside,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.boundaryConditions( Qinside, Qoutside, faceCentre, volumeH, t, normal );
        },  
        marker.x(),
        marker.h(),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
        {{TIME_STEP_SIZE}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        fineGridFace{{UNKNOWN_IDENTIFIER}}.value
      );
    }
"""
  def __init__(self,solver,predicate):
    AbstractFVActionSet.__init__(self,solver)
    self._predicate = predicate


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ] = self._predicate      
      result = jinja2.Template(self.TemplateHandleBoundary).render(**d)
      pass 
    return result


  def get_includes(self):
    return """
#include "exahype2/PatchUtils.h"
#include "exahype2/fv/BoundaryConditions.h"
""" + AbstractFVActionSet.get_includes(self) 


class ProjectPatchOntoFaces( peano4.toolbox.blockstructured.ProjectPatchOntoFaces ):
  def __init__(self,solver, predicate, project_onto_new = True):
    if project_onto_new:
      peano4.toolbox.blockstructured.ProjectPatchOntoFaces.__init__(
        self,
        solver._patch,
        solver._patch_overlap_new,
        predicate, 
        solver._get_default_includes() + solver.get_user_includes()
      )
    else:
      peano4.toolbox.blockstructured.ProjectPatchOntoFaces.__init__(
        self,
        solver._patch,
        solver._patch_overlap,
        predicate, 
        solver._get_default_includes() + solver.get_user_includes()
      )
    
    
class CopyNewPatchOverlapIntoCurrentOverlap( BackupPatchOverlap ):
  def __init__(self,solver, predicate):
    BackupPatchOverlap.__init__(self, 
      solver._patch_overlap_new,
      solver._patch_overlap,
      False,
      predicate,
      solver._get_default_includes() + solver.get_user_includes()
    )
    

class FV(object):
  """ 
    An abstract finite volume solver step sizes that works on patch-based
    AMR with a halo layer of one.
  
    We use two overlaps in this case: the standard one and one we call new. In the
    time stepping, we use the new one to project our data to. Then we roll it over
    at the end of the iteration. This way, we ensure that the one from the previous
    iteration is not overwritten by some adjacent cell halfway through the 
    computation.

    
    Data flow:
    
    The class has a number of guards. They are boolean expression which control 
    whether data is stored and communicated. If you want to redefine these guards,
    you have to redefine create_data_structures() and reset the predicates after
    you have called the superclass' operation. I recommend that you refrain from
    defining completely new predicates. Use the predefined predicates instead and
    refine them by adding more and and or clauses.
  
  
    Parallelisation:
    
    I do equip both Q and NewQ with proper merge routines. However, all merge guards
    set to "never" by default. If you need some data exchange, you have to activate
    them manually.
    
    
    Add further data structures:
    
    To add more data structures, plug into create_data_structures() and 
    create_action_sets().
    
  """
  
      
  def __init__(self, name, patch_size, overlap, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties):
    """
  name: string
     A unique name for the solver. This one will be used for all generated 
     classes. Also the C++ object instance later on will incorporate this 
     name.
     
  patch_size: int
     Size of the patch in one dimension. All stuff here's dimension-generic.
     
  overlap: int
     That's the size of the halo layer which is half of the overlap with a 
     neighbour. A value of 1 means that a patch_size x patch_size patch in 
     2d is surrounded by one additional cell layer. The overlap has to be 
     bigger or equal to one. It has to be smaller or equal to patch_size.
     
  unknowns: int
     Number of unknowns per Finite Volume voxel.
     
  auxiliary_variables: int 
     Number of auxiliary variables per Finite Volume voxel. Eventually, both
     unknowns and auxiliary_variables are merged into one big vector if we 
     work with AoS. But the solver has to be able to distinguish them, as 
     only the unknowns are subject to a hyperbolic formulation.
     
  min_h: double
  
  max_h: double
  
  plot_grid_properties: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
 
    """
    self._name              = name
    
    self._min_h                = min_h
    self._max_h                = max_h 
    self._plot_grid_properties = plot_grid_properties

    self._preprocess_reconstructed_patch      = ""
    self._postprocess_updated_patch           = ""
    
    self.additional_includes   = ""
 
    self._patch_size           = patch_size  
    self._unknowns             = unknowns
    self._auxiliary_variables  = auxiliary_variables

    self.solver_constants_ = ""
    
    if min_h>max_h:
       print( "Error: min_h (" + str(min_h) + ") is bigger than max_h (" + str(max_h) + ")" )

    self._reconstructed_array_memory_location=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack

    self._solver_template_file_class_name = None

    self.plot_description = ""
    
    self.create_data_structures()
    self.create_action_sets()


  def __str__(self):
    result = """
Name:                   """ + self._name + """
Type:                   """ + self.__class__.__name__ + """
Patch size:             """ + str( self._patch_size ) + """  
Unknowns:               """ + str( self._unknowns ) + """
Auxiliary variables:    """ + str( self._auxiliary_variables ) + """
h_min:                  """ + str( self._min_h ) + """
h_max:                  """ + str( self._max_h ) + """
In-situ preprocessing:  """ 
    if self._preprocess_reconstructed_patch:
      result += """yes
"""
    else:
      result += """no
"""
    result += "In-situ postprocessing: """ 
    if self._postprocess_updated_patch:
      result += """yes
"""
    else:
      result += """no
"""
    return result


  __repr__ = __str__
  

  @abstractmethod
  def create_data_structures(self):
    """
    
     Recall in subclasses if you wanna change the number of unknowns
     or auxiliary variables. See class description's subsection on 
     data flow.
     
    """
    self._patch             = peano4.datamodel.Patch( (self._patch_size,self._patch_size,self._patch_size), self._unknowns+self._auxiliary_variables, self._unknown_identifier() )
    self._patch_overlap     = peano4.datamodel.Patch( (2,self._patch_size,self._patch_size),                self._unknowns+self._auxiliary_variables, self._unknown_identifier() )
    self._patch_overlap_new = peano4.datamodel.Patch( (2,self._patch_size,self._patch_size),                self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "New" )
    
    self._patch_overlap.generator.merge_method_definition     = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap)
    self._patch_overlap_new.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap)
    
    self._patch_overlap.generator.includes     += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""
    self._patch_overlap_new.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""
   
    self._patch.generator.store_persistent_condition = self._store_cell_data_default_predicate()
    self._patch.generator.load_persistent_condition  = self._load_cell_data_default_predicate()
    
    self._patch_overlap.generator.send_condition               = "false"
    self._patch_overlap.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_new.generator.send_condition               = "false"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "false"

    self._patch_overlap.generator.store_persistent_condition   = "false"
    self._patch_overlap.generator.load_persistent_condition    = "false"

    self._patch_overlap_new.generator.store_persistent_condition   = "false"
    self._patch_overlap_new.generator.load_persistent_condition    = "false"

    self._patch.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._patch_overlap.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._patch_overlap_new.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    


  @abstractmethod
  def create_action_sets(self):
    """
    
     Overwrite in subclasses if you wanna create different
     action sets.
     
    """
    self._action_set_adjust_cell              = AdjustPatch(self, "not marker.isRefined()")
    self._action_set_AMR                      = AMROnPatch(self, "not marker.isRefined()")
    self._action_set_handle_boundary          = HandleBoundary(self, self._store_face_data_default_predicate() )
    self._action_set_project_patch_onto_faces = ProjectPatchOntoFaces(self, self._store_cell_data_default_predicate())
    self._action_set_copy_new_patch_overlap_into_overlap     = CopyNewPatchOverlapIntoCurrentOverlap(self, self._store_face_data_default_predicate())
    self._action_set_update_cell              = None


  def _store_cell_data_default_predicate(self):
    return "not marker.isRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  def _load_cell_data_default_predicate(self):
    return "not marker.isRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"


  def _store_face_data_default_predicate(self):
    return "not marker.isRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  def _load_face_data_default_predicate(self):
    return "not marker.isRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"
  
  
  def _unknown_identifier(self):
    return self._name+"Q"
  

  def get_name_of_global_instance(self):
    return "InstanceOf" + self._name

  
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """
    
      Add all required data to the Peano4 project's datamodel 
      so it is properly built up
      
    """
    if verbose:
      print( "Patch data" )
      print( "----------" )
      print( str(self._patch) )
      print( "Patch overlap data" )
      print( "----------" )
      print( str(self._patch_overlap) )
      print( "Patch overlap data" )
      print( "----------" )
      print( str(self._patch_overlap_new) )
    datamodel.add_cell(self._patch)
    datamodel.add_face(self._patch_overlap)
    datamodel.add_face(self._patch_overlap_new)
 
 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    """
      Tell Peano what data to move around
      
      Inform Peano4 step which data are to be moved around via the 
      use_cell and use_face commands. This operation is generic from
      ExaHyPE's point of view, i.e. I use it for all grid sweep types. 
    
    """
    step.use_cell(self._patch)
    step.use_face(self._patch_overlap)
    step.use_face(self._patch_overlap_new)

  
  def _get_default_includes(self):
    return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""


  @abstractmethod
  def get_user_includes(self):
    """
  
    Use this to add include statements to the generated C++ code. Is there for
    subclasses to hook in.
  
    """
    return ""
  
  
  @abstractmethod
  def set_preprocess_reconstructed_patch_kernel(self,kernel):
    """
  
    Most subclasses will redefine/overwrite this operation as they have
    to incorporate the kernel into their generated stuff
  
    """
    self._preprocess_reconstructed_patch = kernel


  @abstractmethod
  def set_postprocess_updated_patch_kernel(self,kernel):
    """
  
    Most subclasses will redefine/overwrite this operation as they have
    to incorporate the kernel into their generated stuff
  
    """
    self._postprocess_updated_patch = kernel

  
  def add_actions_to_init_grid(self, step):
    step.add_action_set( self._action_set_adjust_cell ) 
    step.add_action_set( self._action_set_project_patch_onto_faces )
    step.add_action_set( self._action_set_copy_new_patch_overlap_into_overlap )

    
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    step.add_action_set( self._action_set_adjust_cell )
    if evaluate_refinement_criterion:
      step.add_action_set( self._action_set_AMR )
  
  
  def set_plot_description(self,description):
    """
    
     Use this one to set a description within the output patch file that tells
     the vis solver what the semantics of the entries are. Typicallly, I use 
     a comma-separated list here. 
    
    """
    self.plot_description = description
    
  
  def add_actions_to_plot_solution(self, step, output_path):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    
    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat( 
      filename=output_path + "solution-" + self._name, 
      patch=self._patch, 
      dataset_name=self._unknown_identifier(), 
      description=self.plot_description,
      guard_predicate="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_predicate(),
      additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
      precision="PlotterPrecision"
    ))

    if self._plot_grid_properties:    
      step.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( 
        filename = output_path + "grid-" + self._name,
        cell_unknown=None,
        guard_predicate="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_predicate(),
        additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
"""
      ))

    pass
   
 
  def add_actions_to_perform_time_step(self, step):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)

    # @todo vertauschen

    step.add_action_set( self._action_set_handle_boundary )
    step.add_action_set( self._action_set_adjust_cell )
    step.add_action_set( self._action_set_update_cell )
    step.add_action_set( self._action_set_project_patch_onto_faces )
    step.add_action_set( self._action_set_AMR )
    step.add_action_set( self._action_set_copy_new_patch_overlap_into_overlap )


  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass


  def add_implementation_files_to_project(self,namespace,output):
    """
     The ExaHyPE2 project will call this operation when it sets
     up the overall environment.

     output: peano4.output.Output
    """
    templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/"

    if self._solver_template_file_class_name is None:
      templatefile_prefix += self.__class__.__name__
    else:
      templatefile_prefix += self._solver_template_file_class_name

    abstractHeaderDictionary = {}
    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(abstractHeaderDictionary)
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(abstractHeaderDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    generated_abstract_header_file = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + "Abstract.template.h",
      templatefile_prefix + "Abstract.template.cpp",
      "Abstract" + self._name, 
      namespace,
      ".", 
      abstractHeaderDictionary,
      True)
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self._name, 
      namespace,
      ".", 
      implementationDictionary,
      False)

    output.add( generated_abstract_header_file )
    output.add( generated_solver_files )
    output.makefile.add_cpp_file( "Abstract" + self._name + ".cpp" )
    output.makefile.add_cpp_file( self._name + ".cpp" )

  def setSolverConstants(self, datastring): self.solver_constants_ = datastring
  def getSolverConstants(self): return self.solver_constants_


  def _init_dictionary_with_default_parameters(self,d):
    """

      This one is called by all algorithmic steps before I invoke
      add_entries_to_text_replacement_dictionary().

    """
    d["NUMBER_OF_VOLUMES_PER_AXIS"]     = self._patch.dim[0]
    d["HALO_SIZE"]                      = int(self._patch_overlap.dim[0]/2)
    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                    = self._name
    d["UNKNOWN_IDENTIFIER"]             = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]             = self._unknowns
    d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._auxiliary_variables
    d["SOLVER_NUMBER"]                  = 22

    d[ "PREPROCESS_RECONSTRUCTED_PATCH" ]  = self._preprocess_reconstructed_patch
    d[ "POSTPROCESS_UPDATED_PATCH" ]       = self._postprocess_updated_patch

    if self._patch_overlap.dim[0]/2!=1:
      print( "ERROR: Finite Volume solver currently supports only a halo size of 1")

    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "nonCriticalAssertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "nonCriticalAssertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "nonCriticalAssertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "nonCriticalAssertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "nonCriticalAssertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "nonCriticalAssertion6"

    d[ "MAX_H"] = self._min_h
    d[ "MIN_H"] = self._max_h

    d[ "INCLUDES"] = self.additional_includes

    d[ "SOLVER_CONSTANTS" ] = self.solver_constants_
