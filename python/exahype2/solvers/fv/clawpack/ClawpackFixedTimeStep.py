# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.SingleSweep import SingleSweep
from exahype2.solvers.fv.PDETerms    import PDETerms

import jinja2
import peano4

# from .kernels import create_source_term_kernel_for_Rusanov
# from .kernels import create_compute_Riemann_kernel_for_Rusanov
# from .kernels import create_abstract_solver_declarations
# from .kernels import create_abstract_solver_definitions
# from .kernels import create_solver_declarations
# from .kernels import create_solver_definitions
# from .kernels import create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor

class UpdateCell(ReconstructPatchAndApplyFunctor):
  RiemannCallOverPatch = """
    double cellTimeStepSize = -1.0;
    double cellTimeStamp    = -1.0;

    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    assertion2( tarch::la::greaterEquals( cellTimeStepSize, 0.0 ), cellTimeStepSize, cellTimeStamp );
    assertion2( tarch::la::greaterEquals( cellTimeStamp, 0.0 ), cellTimeStepSize, cellTimeStamp );

    const double usedTimeStepSize = cellTimeStepSize;

    ::exahype2::fv::copyPatch(
      reconstructedPatch,
      targetPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1 // halo size
    );

    {% if USE_SPLIT_LOOP %}
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
    #endif
    {% else %}
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
    #endif
    {% endif %}
      [&](
        const double * __restrict__                  QL,
        const double * __restrict__                  QR,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) -> void {
        double wave[{{NUMBER_OF_UNKNOWNS}}][{{NUMBER_OF_UNKNOWNS}}];
        double speed[{{NUMBER_OF_UNKNOWNS}}];

        int num_eqn   = {{NUMBER_OF_UNKNOWNS}};
        int num_aux   = {{NUMBER_OF_AUXILIARY_VARIABLES}};
        int num_waves = {{NUMBER_OF_UNKNOWNS}};

        {{CLAWPACK_RIEMANN_SOLVER}}_(
          {%if DISCRIMINATE_NORMAL %}
            &normal,
          {% endif %}
          &num_eqn,
          &num_aux,
          &num_waves,
          QL,                                 // double* q_l
          QR,                                 // double* q_r
          QL+{{NUMBER_OF_UNKNOWNS}},          // double* aux_l
          QR+{{NUMBER_OF_UNKNOWNS}},          // double* aux_r
          wave,
          speed,
          FL,                                 // double* amdq
          FR                                  // double* apdq
        );

        for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) {
          FL[i] = -FL[i];
        }
      },
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        double * __restrict__                        S
      ) -> void {
      // S[0] =0;
      // S[1] =0;
     // S[2] =0;
      // S[3] =0;
      },
      marker.x(),
      marker.h(),
      cellTimeStamp,
      cellTimeStepSize,
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      reconstructedPatch,
      targetPatch
    );




    {{POSTPROCESS_UPDATED_PATCH}}
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(cellTimeStamp + usedTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(cellTimeStepSize);

    repositories::{{SOLVER_INSTANCE}}.update(cellTimeStepSize, cellTimeStamp + usedTimeStepSize, marker.h()(0) );


  """


  def __init__(self,solver):
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)

    # Manual insertions
    d[ "USE_SPLIT_LOOP" ] = solver._use_split_loop
    d["CLAWPACK_RIEMANN_SOLVER"] = solver.clawpack_Riemann_solver
    d["DISCRIMINATE_NORMAL"] = solver._discriminate_normal
    d["PREPROCESS_RECONSTRUCTED_PATCH"] = solver._preprocess_reconstructed_patch





    ReconstructPatchAndApplyFunctor.__init__(self,
      solver._patch,
      solver._patch_overlap_new,
      jinja2.Template( self.RiemannCallOverPatch ).render(**d),
      solver._reconstructed_array_memory_location,
      "not marker.isRefined()"
    )

    self._solver = solver

  def get_user_includes(self):
    return """
#include "exahype2/fv/Generica.h"
"""

  def get_includes(self):
    return """
#include "exahype2/fv/BoundaryConditions.h"
#include "exahype2/NonCriticalAssertions.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes()


class ClawpackFixedTimeStep( SingleSweep ):
  def __init__(self,
    name, patch_size, unknowns, auxiliary_variables, min_volume_h,
    max_volume_h, time_step_size, clawpack_name, clawpack_files,
    plot_grid_properties=False, discriminate_normal=False,
    kernel_implemenatation=None
  ):
    # Mandatory
    super(ClawpackFixedTimeStep,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties)
    self.clawpack_Riemann_solver             = clawpack_name
    self.Riemann_solver_implementation_files = clawpack_files
    self._time_step_size = time_step_size


    # Defaults
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False

    self._preprocess_reconstructed_patch      = "cellTimeStepSize = marker.h()(0) / repositories::{}.MaxAdmissibleVolumeH * {};\n".format(self.get_name_of_global_instance(), time_step_size)
    self._preprocess_reconstructed_patch     += "cellTimeStamp    = fineGridCell{}CellLabel.getTimeStamp();".format(self._name)

    # self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation
    # self._source_term_implementation          = PDETerms.Empty_Implementation

    # self._source_term_call          = ""
    # self._Riemann_solver_call       = ""

    self._discriminate_normal = discriminate_normal
    # self.set_implementation()

  # def get_includes(self):
    # return 'include "exahype2/fv/BoundaryConditions.h\n#include "TopologyParser.h\n{}{}'.format(self._solver._get_default_includes(), self._solver.get_user_includes())

  def add_entries_to_text_replacement_dictionary(self,d):
    super(ClawpackFixedTimeStep,self).add_entries_to_text_replacement_dictionary(d)
    d["ADDITIONALINCLUDES"] = '#include "TopologyParser.h"'

    rpn2 = 'extern "C" int rpn2_(int* ixy, int* num_eqn, int* num_aux, int* num_waves,'
    rpn2 += 'const double * __restrict__ q_l, const double * __restrict__ q_r,'
    rpn2 += 'const double * __restrict__ aux_l, const double * __restrict__ aux_r,'
    rpn2 += 'double wave[3][3], double* s, double* amdq, double* apdq);\n'
    d["ADDITIONALDEFS"]     = rpn2
    # d["POSTPROCESS_UPDATED_PATCH"] = self._postp

  def add_implementation_files_to_project(self,namespace,output):
    super(ClawpackFixedTimeStep,self).add_implementation_files_to_project(namespace,output)
    for f in self.Riemann_solver_implementation_files:
      output.makefile.add_Fortran_file(f)



  # def get_user_includes(self):
    # return """
# #include "exahype2/fv/Generic.h"
# // #include "exahype2/fv/Rusanov.h"
# """
  def set_implementation(self,
          boundary_conditions=None,eigenvalues=None,refinement_criterion=None,initial_conditions=None,source_term=None,
          memory_location = None, use_split_loop = False):
    """
    Overwrite default settings and call create_action_sets.
    """

    if boundary_conditions is not None:
      self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion is not None:
      self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions is not None:
      self._initial_conditions_implementation         = initial_conditions
    if source_term is not None:
      self._source_term_implementation                = source_term
    if eigenvalues is not None:
      self._eigenvalues_implementation                = eigenvalues
    if memory_location is not None:
      self._reconstructed_array_memory_location       = memory_location
    if use_split_loop is not None:
      self._use_split_loop = use_split_loop


    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "Memory mode ({}) without appropriate delete chosen, i.e. this will lead to a memory leak".format(self._reconstructed_array_memory_location) )

    super(ClawpackFixedTimeStep,self).create_action_sets()
    self._action_set_update_cell = UpdateCell(self)




    # self._source_term_call    = create_source_term_kernel_for_Rusanov(self._source_term_implementation)
    # self._Riemann_solver_call = create_compute_Riemann_kernel_for_Rusanov(self._flux_implementation, self._ncp_implementation)

    # self._abstract_solver_user_declarations = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    # self._abstract_solver_user_definitions  = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    # self._solver_user_declarations          = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    # self._solver_user_definitions           = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)

    # super(ClawpackFixedTimeStep,self).set_implementation(self, boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop)

  def set_pointwise_constraint(self, kernel):
    """
    This is a routine to implement some a posteriori pointwise constraints on solution.
    So you can make kernel hold a C++
    string similar to
    if (Q[1]>43.2) {
     Q[2] = Q[4];
    }
    """
    code  = "int postProcessingIndex = 0;\n"
    code += "dfor(volume, %i) "%self._patch_size;
    code += "{\n  double* Q = targetPatch + postProcessingIndex;\n"
    code += kernel
    code += "  postProcessingIndex += {};\n}}\n".format(self._unknowns + self._auxiliary_variables);
    self.set_postprocess_updated_patch_kernel( code )


  # def set_implementation(self, **kwargs):
    # if flux                 is not None:  self._flux_implementation                       = flux
    # if ncp                  is not None:  self._ncp_implementation                        = ncp
    # if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    # if source_term          is not None:  self._source_term_implementation                = source_term

    # self._source_term_call    = create_source_term_kernel_for_Rusanov(self._source_term_implementation)
    # self._Riemann_solver_call = create_compute_Riemann_kernel_for_Rusanov(self._flux_implementation, self._ncp_implementation)

    # self._abstract_solver_user_declarations = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    # self._abstract_solver_user_definitions  = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    # self._solver_user_declarations          = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    # self._solver_user_definitions           = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)

    # SingleSweep.set_implementation(self, boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop)


