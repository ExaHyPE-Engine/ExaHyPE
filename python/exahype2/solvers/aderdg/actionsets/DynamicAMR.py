# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2
import jinja2

from peano4.solversteps.ActionSet import ActionSet
from .AbstractAderDGActionSet import AbstractAderDGActionSet


class DynamicAMR(AbstractAderDGActionSet):
    """

    The ExaHyPE 2 AderDG handling of adaptive meshes.
    Despite the name this currently only handles static AMR, so this probably desperately needs to be renamed.

    """

    def __init__(
        self,
        solver,
        clear_guard="true",
        interpolate_guard="true",
        restrict_guard="true",
    ):
        super(DynamicAMR, self).__init__(solver)
        self._clear_guard = clear_guard
        self._interpolate_guard = interpolate_guard
        self._restrict_guard = restrict_guard

        self._Template_TouchFaceFirstTime = """

    if ( {{CLEAR_GUARD}} ) {

      const int Order = {{ORDER}};
      const int NumberOfVariables = {{NUMBER_OF_UNKNOWNS}};
      const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
      const int strideQ = NumberOfVariables+NumberOfParameters;
      
      #if Dimensions==2
      constexpr int spaceFaceSize      = (Order+1);
      #else
      constexpr int spaceFaceSize      = (Order+1)*(Order+1);
      #endif

      const int basisElementsPerFace = spaceFaceSize*strideQ;
      const int fluxElementsPerFace = spaceFaceSize*NumberOfVariables;

      std::fill_n(fineGridFace{{SOLVER_NAME}}QEstimates.value, 2*basisElementsPerFace, 0.0);
      std::fill_n(fineGridFace{{SOLVER_NAME}}QFluxEstimates.value, 2*fluxElementsPerFace, 0.0);

    }

    """

        self._Template_CreateHangingFace = """

  if ( {{INTERPOLATE_GUARD}} ) {
  
    const int Order = {{ORDER}};
    const int NumberOfVariables = {{NUMBER_OF_UNKNOWNS}};
    
    #if Dimensions==2
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif

    int facePosition = marker.getSelectedFaceNumber()<Dimensions ? spaceFaceSize*NumberOfVariables : 0;

    if(!marker.isInteriorFaceWithinPatch()){

      kernels::aderdg::generic::c::singleLevelFaceUnknownsProlongation<
        {{SOLVER_NAME}},
        {{CORRECTOR_COMPUTATION_PRECISION}},        
        NumberOfVariables,+
        Order+1>(
          fineGridFace{{SOLVER_NAME}}QFluxEstimates.value+facePosition,
          coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).value+facePosition,
          marker.getRelativePositionWithinFatherFace()
      );

   }

  }

    """

        self._Template_DestroyHangingFace = """

  if({{RESTRICT_GUARD}}){

    const int Order = {{ORDER}};
    const int NumberOfVariables = {{NUMBER_OF_UNKNOWNS}};
    const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    const int strideQ = NumberOfVariables+NumberOfParameters;
    
    #if Dimensions==2
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif

    const int basisElementsPerFace  = spaceFaceSize*strideQ;
    const int fluxElementsPerFace   = spaceFaceSize*NumberOfVariables;

    int facePosition = marker.getSelectedFaceNumber()<Dimensions ? 1 : 0;

    //estimates
    kernels::aderdg::generic::c::singleLevelFaceUnknownsRestriction<
      {{SOLVER_NAME}},
      {{CORRECTOR_COMPUTATION_PRECISION}},      
      strideQ,
      Order+1>(
        coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).value+facePosition*basisElementsPerFace, //const double* lQhbndCoarse
        fineGridFace{{SOLVER_NAME}}QEstimates.value+facePosition*basisElementsPerFace, //double* lQhbndFine          
        marker.getRelativePositionWithinFatherFace() //const tarch::la::Vector<Dimensions-1, int>& subfaceIndex
    );

    //fluxes
    kernels::aderdg::generic::c::singleLevelFaceUnknownsRestriction<
      {{SOLVER_NAME}}, //SolverType
      {{CORRECTOR_COMPUTATION_PRECISION}}, //pStoreType      
      NumberOfVariables,      //numberOfVariables
      Order+1>(
        coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).value+facePosition*fluxElementsPerFace, //const double* lQhbndCoarse
        fineGridFace{{SOLVER_NAME}}QFluxEstimates.value+facePosition*fluxElementsPerFace, //double* lQhbndFine          
        marker.getRelativePositionWithinFatherFace() //const tarch::la::Vector<Dimensions-1, int>& subfaceIndex
    );

  }

    """

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_body_of_operation(self, operation_name):
        result = "\n"
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            d["CLEAR_GUARD"] = self._clear_guard
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(**d)
            pass
        if operation_name == ActionSet.OPERATION_CREATE_HANGING_FACE:
            d["INTERPOLATE_GUARD"] = self._interpolate_guard
            result = jinja2.Template(self._Template_CreateHangingFace).render(**d)
            pass
        if operation_name == ActionSet.OPERATION_DESTROY_HANGING_FACE:
            d["RESTRICT_GUARD"] = self._restrict_guard
            result = jinja2.Template(self._Template_DestroyHangingFace).render(**d)
            pass
        return result

    def get_includes(self):
        return (
            super(DynamicAMR, self).get_includes()
            + """
#include <cstring>
"""
        )
