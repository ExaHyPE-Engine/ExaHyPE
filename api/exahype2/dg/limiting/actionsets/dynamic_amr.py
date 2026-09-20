# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.actionsets

from exahype2.dg.limiting.projection_matrices import create_subcell_projection_matrices
from exahype2.dg.limiting.projection_matrices import render_matrix_table


class DynamicAMR(exahype2.actionsets.SolverActionSet):
    """Moves the subcell buffers and the limiter state across a level jump.

    A cell that runs no subcell scheme carries no valid subcell buffer, so the transfer
    projects its nodal solution onto the subcells first.
    """

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  ::exahype2::dg::limiting::resetCoarseningParentLabel(marker, fineGridCell{{SOLVER_NAME}}CellLabel);
"""

    _TEMPLATE_CREATE_CELL = """
  ::exahype2::dg::limiting::createCell<{{FULL_QUALIFIED_SOLVER_NAME}}>(
    marker,
    DgToFvMatrix,
    coarseGridCell{{SOLVER_NAME}}SubcellOld.data(),
    fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    coarseGridCell{{SOLVER_NAME}}CellLabel,
    fineGridCell{{SOLVER_NAME}}CellLabel
  );
"""

    _TEMPLATE_DESTROY_CELL = """
  ::exahype2::dg::limiting::destroyCell<{{FULL_QUALIFIED_SOLVER_NAME}}>(
    marker,
    _subcellRestrictionStage,
    DgToFvMatrix,
    coarseGridCell{{SOLVER_NAME}}SubcellOld.data(),
    fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    coarseGridCell{{SOLVER_NAME}}CellLabel,
    fineGridCell{{SOLVER_NAME}}CellLabel
  );
"""

    def __init__(self, solver):
        super().__init__(solver)

    def get_static_initialisations(self, full_qualified_classname):
        dg_to_fv, _ = create_subcell_projection_matrices(self.solver.basis)
        return (
            "namespace {\n"
            + render_matrix_table("DgToFvMatrix", dg_to_fv, "double")
            + "} // namespace\n"
        )

    def operation_body(self, operation_name):
        operations = exahype2.actionsets.ActionSet
        template = {
            operations.OPERATION_TOUCH_CELL_FIRST_TIME: self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
            operations.OPERATION_CREATE_CELL: self._TEMPLATE_CREATE_CELL,
            operations.OPERATION_DESTROY_CELL: self._TEMPLATE_DESTROY_CELL,
        }.get(operation_name)
        if template is None:
            return ""
        return self.render(template)

    def attributes(self):
        return self.render(
            """
  ::exahype2::amr::ChildDataStage<{{SOLUTION_STORAGE_PRECISION}}> _subcellRestrictionStage{
    peano4::sfc::ActiveCurve::ChildrenPerCell,
    ::exahype2::dg::limiting::ValuesPerSubcellPatch<{{SUBCELLS_PER_AXIS}}, {{NUMBER_OF_DATA}}>
  };
""",
            SUBCELLS_PER_AXIS=2 * self.solver.order + 1,
        )

    def includes(self):
        return super().includes() + ["exahype2/dg/limiting/DynamicAMR.h"]
