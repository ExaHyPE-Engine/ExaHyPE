# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.solversteps.ActionSet import ActionSet


class CellDataWriter(ActionSet):
    """!

    Write one solver's cell data into the current checkpoint.

    The action set buffers one record per visited leaf cell, keyed by the
    cell's level and lattice position, and hands the buffers to
    peano4::checkpoint::CellDataWriter, which writes them into the group
    group_name of <checkpoint output path>/checkpoint-<n>.h5 with one
    collective write per dataset. n counts the checkpoints this run has
    written, and every writer of a checkpoint fills a group of that one file.

    group_name: str
      Path of the group inside the checkpoint file, such as "/solvers/Euler".

    output_path_expression: str
      C++ expression for the checkpoint output path; the caller names the
      configuration source, so this class depends on none.

    cell_data_name: str
      Name of the cell data container, e.g. the solver's current unknowns.

    values_per_cell_expression: str
      C++ constant expression for the number of doubles one cell stores.

    guard: str
      C++ predicate selecting the cells to record.

    scalars_expression: str
      C++ expression for the solver's scalar state, a std::vector<double>.

    time_stamp_expression: str
      C++ expression for the checkpoint time.

    capture_attributes, capture_statement, capture_static_initialisations: str
      Optional attribute declarations, a per-recorded-cell statement filling
      them, and their static initialisations. A solver whose scalar state
      lives on the mesh, such as a per-cell time step size, captures it here
      and names the attribute in scalars_expression.

    record_statements, record_expression: str
      Optional per-recorded-cell statements that assemble the record buffer,
      and the C++ expression for the buffer's value pointer. When
      record_expression is empty, the record comes from the cell data
      container named by cell_data_name.
    """

    def __init__(
        self,
        group_name,
        output_path_expression,
        cell_data_name,
        values_per_cell_expression,
        guard,
        scalars_expression,
        time_stamp_expression,
        additional_includes="",
        capture_attributes="",
        capture_statement="",
        capture_static_initialisations="",
        record_statements="",
        record_expression="",
    ):
        super().__init__(descend_invocation_order=1)
        self.d = {
            "GROUP_NAME": group_name,
            "OUTPUT_PATH": output_path_expression,
            "CELL_DATA_NAME": cell_data_name,
            "VALUES_PER_CELL": values_per_cell_expression,
            "GUARD_PREDICATE": guard,
            "SCALARS": scalars_expression,
            "TIMESTAMP": time_stamp_expression,
            "CAPTURE_STATEMENT": capture_statement,
            "RECORD_STATEMENTS": record_statements,
            "RECORD_EXPRESSION": record_expression,
        }
        self._additional_includes = additional_includes
        self._capture_attributes = capture_attributes
        self._capture_static_initialisations = capture_static_initialisations

    def get_action_set_name(self):
        return (
            __name__.replace(".py", "").replace(".", "_")
            + "_"
            + self.d["GROUP_NAME"].replace("/", "_")
        )

    def user_should_modify_template(self):
        return False

    def get_body_of_prepareTraversal(self):
        return jinja2.Template(
            """
  ::peano4::checkpoint::CheckpointFile::getInstance().open({{OUTPUT_PATH}});
  _writer.reset();
  _writer = std::make_unique<::peano4::checkpoint::CellDataWriter>("{{GROUP_NAME}}", {{VALUES_PER_CELL}});
"""
        ).render(**self.d)

    def get_body_of_unprepareTraversal(self):
        return jinja2.Template(
            """
  _writer->write({{SCALARS}}, {{TIMESTAMP}});
  _writer.reset();
  ::peano4::checkpoint::CheckpointFile::getInstance().close();
"""
        ).render(**self.d)

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = """
  _records.clear();
"""
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = """
  _writer->collect(std::move(_records));
  _records.clear();
"""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = jinja2.Template(
                """
  if ({{GUARD_PREDICATE}}) {
    // forCell() is the canonical map from a cell's centre and width onto its
    // level and lattice position; the refinement requests use the same one.
    const auto cellName = ::peano4::grid::GridControlEvent::forCell(
      ::peano4::grid::GridControlEvent::RefinementControl::Refine, marker.getX(), marker.getH(), DomainOffset, DomainSize
    );
{%- if RECORD_EXPRESSION == "" %}
    _records.append(cellName.level, cellName.latticeIndex, fineGridCell{{CELL_DATA_NAME}}.data(), {{VALUES_PER_CELL}});
{%- else %}
    {{RECORD_STATEMENTS}}
    _records.append(cellName.level, cellName.latticeIndex, {{RECORD_EXPRESSION}}, {{VALUES_PER_CELL}});
{%- endif %}
    {{CAPTURE_STATEMENT}}
  }
"""
            ).render(**self.d)
        return result

    def get_attributes(self):
        return (
            """
  ::peano4::checkpoint::CellDataWriter::Records _records;

  static std::unique_ptr<::peano4::checkpoint::CellDataWriter> _writer;
"""
            + self._capture_attributes
        )

    def get_static_initialisations(self, full_qualified_classname):
        return (
            f"""
std::unique_ptr<::peano4::checkpoint::CellDataWriter> {full_qualified_classname}::_writer = nullptr;
"""
            + self._capture_static_initialisations.replace(
                "{{FULL_QUALIFIED_CLASSNAME}}", full_qualified_classname
            )
        )

    def get_includes(self):
        return (
            """
#include "Constants.h"
#include "peano4/checkpoint/CellDataWriter.h"
#include "peano4/checkpoint/CheckpointFile.h"
#include "peano4/grid/GridControlEvent.h"
"""
            + self._additional_includes
        )
