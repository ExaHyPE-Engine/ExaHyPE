# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

# import peano4
import peano4.solversteps
import jinja2


class Handle_boundary(AbstractAderDGActionSet):
    """

    The linear combination of the Runge Kutta trials has to be projected onto
    the faces, so we can then solve the Riemann problems. So the projection
    happens in one grid sweep, the corresponding Riemann solve in the next one.


    """

    TemplateProject = """
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

    tarch::la::Vector<Dimensions,double> bottomLeftPosition = marker.x()-marker.h();
    bottomLeftPosition[normal] += marker.h()[normal];

    for(int i=0; i<(Order+1); i++){
      #if Dimensions==3
      for(int j=0; j<(Order+1); j++){
      #endif

        tarch::la::Vector<Dimensions,double> nodePosition = bottomLeftPosition;

        #if Dimensions==2
          nodePosition[1-normal] += repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d[i]*marker.h()[1-normal];
        #else
          /*
          * These access the first and second components that are not in the normal direction
          * i.e. if normal = 0, they access components 1 and 2,
          *      if normal = 1, they access components 0 and 2,
          *      if normal = 2, they access components 0 and 1,
          */
          nodePosition[std::max((1-normal), 0)] += repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d[i]
                                                   *marker.h()[std::max((1-normal), 0)];
          nodePosition[std::min(2, (3-normal))] += repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d[j]
                                                   *marker.h()[std::min(2, (3-normal))];
        #endif

        repositories::{{SOLVER_INSTANCE}}.boundaryConditions(
          QIn,
          QOut,
          marker.x(),
          marker.h(),
          timeStamp,
          normal
        );

        repositories::{{SOLVER_INSTANCE}}.flux(
          QOut,
          marker.x(),
          marker.h(),
          timeStamp,
          timeStepSize,
          normal,
          FOut
        );

        FIn   += NumberOfVariables;
        QIn   += NumberOfData;
        FOut  += NumberOfVariables;
        QOut  += NumberOfData;

      #if Dimensions==3
      }
      #endif
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
        super(Handle_boundary, self).__init__(solver)
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
            result = jinja2.Template(self.TemplateProject).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(Handle_boundary, self).get_includes()
            + """
#include "exahype2/aderdg/kernels/Kernels.h"
      """
        )
