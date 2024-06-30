# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class HandleBoundary(AbstractADERDGActionSet):
    """
    The linear combination of the Runge-Kutta trials has to be projected onto
    the faces, so we can then solve the Riemann problems. So the projection
    happens in one grid sweep, the corresponding Riemann solve in the next one.
    """

    _Template_HandleBoundary = """
  if ({{PREDICATE}}) {
    const double timeStamp = fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(marker.getSelectedFaceNumber() < Dimensions ? 1 : 0);

    // Needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}

    #if Dimensions == 2
    constexpr int SpaceFaceSize      = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize      = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    constexpr int FluxElementsByFace   = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};
    constexpr int BasisElementsByFace  = SpaceFaceSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});

    const int normal = marker.getSelectedFaceNumber() % Dimensions;
    const bool isLeftFace = marker.getSelectedFaceNumber() < Dimensions;

    {{CORRECTOR_COMPUTATION_PRECISION}}* FIn  = fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.value + (!isLeftFace ? 0 : FluxElementsByFace);
    {{CORRECTOR_COMPUTATION_PRECISION}}* FOut = fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.value + (!isLeftFace ? FluxElementsByFace : 0);
    {{CORRECTOR_COMPUTATION_PRECISION}}* QIn  = fineGridFace{{UNKNOWN_IDENTIFIER}}Estimates.value + (!isLeftFace ? 0 : BasisElementsByFace);
    {{CORRECTOR_COMPUTATION_PRECISION}}* QOut = fineGridFace{{UNKNOWN_IDENTIFIER}}Estimates.value + (!isLeftFace ? BasisElementsByFace : 0);

    tarch::la::Vector<Dimensions, double> faceOffset = marker.x() - 0.5 * marker.h();
    faceOffset(normal) += 0.5 * marker.h()(normal);

    dfore(dof, {{ORDER}} + 1, normal, 0) {
        tarch::la::Vector<Dimensions, double> nodePosition;
        int dofSerialised = 0;
        int basis         = 1;
        for (int d = 0; d < Dimensions; d++) {
          nodePosition(d)  = faceOffset(d);
          nodePosition(d) += (d == normal) ? 0.0 : kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes[dof(d)] * marker.h()(d);
          dofSerialised += (d == normal) ? 0 : dof(d) * basis;
          basis *= (d == normal) ? 1 : ({{ORDER}} + 1);
        }

        repositories::{{SOLVER_INSTANCE}}.boundaryConditions(
          QIn  + dofSerialised * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
          QOut + dofSerialised * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
          nodePosition,
          marker.h(),
          timeStamp,
          normal
        );

        {% if USE_FLUX %}
        repositories::{{SOLVER_INSTANCE}}.flux(
          QOut + dofSerialised * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
          nodePosition,
          marker.h(),
          timeStamp,
          timeStepSize,
          normal,
          FOut + dofSerialised * {{NUMBER_OF_UNKNOWNS}}
        );
        {% else %}
        // Not formally correct, should still be able to contain contributions from things such as sources, point sources etc.
        // These must now be handled through the boundary conditions.
        std::fill_n(FOut + dofSerialised * {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_UNKNOWNS}}, 0.0);
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
        self._guard = guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = self._guard
            result += jinja2.Template(self._Template_HandleBoundary).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(HandleBoundary, self).get_includes()
            + """
#include "kernels/"""
            + self._solver._name
            + """/Quadrature.h"
      """
        )
