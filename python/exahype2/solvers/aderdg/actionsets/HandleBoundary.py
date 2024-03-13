# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

# import peano4
import peano4.solversteps
import jinja2


class HandleBoundary(AbstractAderDGActionSet):
    """

    The linear combination of the Runge Kutta trials has to be projected onto
    the faces, so we can then solve the Riemann problems. So the projection
    happens in one grid sweep, the corresponding Riemann solve in the next one.


    """

    TemplateHandleBoundary = """
  if ({{PREDICATE}}) {
  
    const double timeStamp = fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(marker.getSelectedFaceNumber()<Dimensions ? 1 : 0);
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    const int Order = {{ORDER}};
    const int NumberOfVariables = {{NUMBER_OF_UNKNOWNS}};
    const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    const int NumberOfData = NumberOfVariables+NumberOfParameters;
    
    #if Dimensions==2
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif
    
    const int basisElementsByFace = spaceFaceSize*NumberOfData;
    const int fluxElementsByFace = spaceFaceSize*NumberOfVariables;

    const int normal = marker.getSelectedFaceNumber()%Dimensions;
    const bool isLeftFace = marker.getSelectedFaceNumber()<Dimensions;

    {{CORRECTOR_COMPUTATION_PRECISION}}* FIn  = fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.value + (!isLeftFace ? 0 : fluxElementsByFace);
    {{CORRECTOR_COMPUTATION_PRECISION}}* FOut = fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.value + (!isLeftFace ? fluxElementsByFace : 0);
    {{CORRECTOR_COMPUTATION_PRECISION}}* QIn  = fineGridFace{{UNKNOWN_IDENTIFIER}}Estimates.value + (!isLeftFace ? 0 : basisElementsByFace);
    {{CORRECTOR_COMPUTATION_PRECISION}}* QOut = fineGridFace{{UNKNOWN_IDENTIFIER}}Estimates.value + (!isLeftFace ? basisElementsByFace : 0);    

    tarch::la::Vector<Dimensions,double> faceOffset = marker.x() - 0.5 * marker.h();
    faceOffset(normal) += 0.5 * marker.h()(normal);

    dfore(dof,Order+1,normal,0) {
        tarch::la::Vector<Dimensions,double> nodePosition;
        int dofSerialised = 0;
        int basis         = 1;
        for (int d=0; d<Dimensions; d++) {
          nodePosition(d)  = faceOffset(d);
          nodePosition(d) += (d==normal) ? 0.0 : repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d[dof(d)] * marker.h()(d);
          dofSerialised += (d==normal) ? 0 : dof(d)*basis;
          basis *= (d==normal) ? 1 : (Order+1);
        }

        repositories::{{SOLVER_INSTANCE}}.boundaryConditions(
          QIn  + dofSerialised*NumberOfData,
          QOut + dofSerialised*NumberOfData,
          nodePosition,
          marker.h(),
          timeStamp,
          normal
        );

        {% if USE_FLUX!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(
          QOut + dofSerialised*NumberOfData,
          nodePosition,
          marker.h(),
          timeStamp,
          timeStepSize,
          normal,
          FOut + dofSerialised*NumberOfVariables
        );
        {% else %}
        /*
         * Not formally correct, should still be able to contain contributions from things such as sources, point sources etc.
         * These must now be handled through the boundary conditions.
        */
        std::fill_n(FOut + dofSerialised*NumberOfVariables, NumberOfVariables, 0.0);
        {% endif %}
    }
   
  }
"""

    def __init__(self, solver, guard):
        """

        guard_project: String (C++ code)
          Predicate which controls if the solution is actually projected

        guard_safe_old_time_step: String (C++ code)
          Predicate which controls if the projection should be copied into
          the old solution and the time step should also be moved over

        """
        super(HandleBoundary, self).__init__(solver)
        self.guard = guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = self.guard
            result += jinja2.Template(self.TemplateHandleBoundary).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(HandleBoundary, self).get_includes()
            + """
#include "exahype2/aderdg/kernels/Kernels.h"
      """
        )
