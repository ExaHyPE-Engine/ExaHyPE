# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

# import peano4
import peano4.solversteps
import jinja2


class HandleFVBoundary(AbstractAderDGActionSet):

    TemplateHandleFVBoundary = """
  //Finite volumes boundary
  if ( {{PREDICATE}} )
   {
    logTraceInWith3Arguments( "touchFaceFirstTime(...)", fineGridFace{{SOLVER_NAME}}FaceLabel.toString(), (repositories::{{SOLVER_INSTANCE}}.PeriodicBC[marker.getSelectedFaceNumber()%Dimensions]), marker.toString() );

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
      fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(marker.getSelectedFaceNumber()<Dimensions ? 1 : 0),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      {{2*ORDER+1}},
      1, //Halo size
      {{NUMBER_OF_UNKNOWNS+NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      fineGridFace{{SOLVER_NAME}}Q_FV_Projections.value
    );

    bool isLeftEntryOutside = marker.getSelectedFaceNumber() < Dimensions;
    double innerTimeStamp;
    
    innerTimeStamp = fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp( isLeftEntryOutside ? 1 : 0 );
    fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdatedTimeStamp( isLeftEntryOutside ? 0 : 1, innerTimeStamp );
    
    innerTimeStamp = fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp( isLeftEntryOutside ? 1 : 0 );
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp( isLeftEntryOutside ? 0 : 1, innerTimeStamp );

    innerTimeStamp = fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp( isLeftEntryOutside ? 1 : 0 );
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp( isLeftEntryOutside ? 0 : 1, innerTimeStamp );

    logTraceOut( "touchFaceFirstTime(...)" );
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
        super(HandleFVBoundary, self).__init__(solver)
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
            result += jinja2.Template(self.TemplateHandleFVBoundary).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(HandleFVBoundary, self).get_includes()
            + """
#include "exahype2/fv/BoundaryConditions.h"
      """
        )
