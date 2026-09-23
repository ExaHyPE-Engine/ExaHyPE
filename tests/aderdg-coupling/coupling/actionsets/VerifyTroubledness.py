# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets import ActionSet

from .AbstractLimiterActionSet import AbstractLimiterActionSet


class VerifyTroubledness(AbstractLimiterActionSet):
    """Marks a cell whose solution fails the admissibility criterion as troubled."""

    TemplateVerifyTroubledness = """
    if ( {{PREDICATE}} ) {
        const double timeStamp     = fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStamp();
        const double timeStepSize  = fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStepSize();

        bool isUnTroubled = true;

        auto* luh = fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}.data();

        {% if USE_ADMISSIBILITY_CRITERION %}
        int linearisedIndex = 0;
        peano4::dForEach({{ORDER}} + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
          isUnTroubled &= repositories::{{SOLVER_INSTANCE}}.isPhysicallyAdmissible(
            luh + linearisedIndex,
            ::exahype2::dg::getQuadraturePoint(
              marker.getX(), marker.getH(), index,
              kernels::{{REGULAR_SOLVER_NAME}}::Quadrature<{{REGULAR_SOLVER_STORAGE_PRECISION}}>::QuadraturePoints1D
            ),
            marker.getH(),
            timeStamp + timeStepSize
          );

          linearisedIndex += repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfUnknowns + repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfAuxiliaryVariables;
        });
        {% endif %}

        if (not isUnTroubled) {
          repositories::{{SOLVER_INSTANCE}}.addTroubledCell();
          fineGridCell{{SOLVER_NAME}}CellLabel.setTroubledMarker(celldata::{{SOLVER_NAME}}CellLabel::TroubledMarker::TROUBLED);
          for (int d = 0; d < 2 * DIMENSIONS; d++) {
            fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubledMarker(facedata::{{SOLVER_NAME}}FaceLabel::TroubledMarker::TROUBLED);
          }
        }

        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp);
        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);

        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setHasUpdated(true);
    }
"""

    def __init__(self, solver, guard, use_admissibility_criterion=False):
        super(VerifyTroubledness, self).__init__(solver, guard)
        self.use_admissibility_criterion = use_admissibility_criterion

    def operation_body(self, operation_name):
        result = ""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.render(
                self.TemplateVerifyTroubledness,
                USE_ADMISSIBILITY_CRITERION=self.use_admissibility_criterion,
            )
        return result

    @property
    def name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def includes(self):
        return super().includes() + [
            "exahype2/dg/DGUtils.h",
            "kernels/{}/Quadrature.h".format(self._solver.regular_solver.name),
        ]
