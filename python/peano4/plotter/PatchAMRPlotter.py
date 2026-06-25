# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .BasePlotter import BasePlotter

from peano4.solversteps.ActionSet import ActionSet

import jinja2


class PatchAMRPlotter(BasePlotter):
    """!

    Plotter with AMR Block Visualisation

    This plotter visualises Peano patches as collections of "AMR blocks"
    (2.5D extruded blocks in 2D, or shrunk blocks in 3D)
    to highlight the grid structure and refinement levels.

    """

    def __init__(
        self,
        output_path,
        filename,
        patch,
        dataset_name,
        description,
        time_stamp_evaluation,
        amr_scale_factor,
        amr_base_thickness,
        guard,
    ):
        super().__init__(
            output_path,
            filename,
            dataset_name,
            description,
            time_stamp_evaluation,
            guard,
        )

        for i in patch.dim:
            assert i == patch.dim[0]

        self.d["DOFS_PER_AXIS"] = str(patch.dim[0])
        self.d["UNKNOWNS"] = str(patch.no_of_unknowns)
        self.d["SCALE_FACTOR"] = str(amr_scale_factor)
        self.d["BASE_THICKNESS"] = str(amr_base_thickness)

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_body_of_operation(self, operation_name):
        result = "\n"

        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = (
                super().get_body_of_operation(operation_name)
                + jinja2.Template(
                    """
  _amrPlotter.reset();
  _amrPlotter = std::make_unique<tarch::plotter::AMRPlotter>(*_fileWriter, _localTreeNumber, {{SCALE_FACTOR}}, {{BASE_THICKNESS}});

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  _{{name}}DataPlotter.reset();
  _{{name}}DataPlotter = _fileWriter->createCellDataPlotter(_localTreeNumber, "{{name}}", {{count}});
{% endfor %}
{% else %}
  _cellDataPlotter = _fileWriter->createCellDataPlotter(_localTreeNumber, "{{NAME}}", {{UNKNOWNS}});
{% endif %}
"""
                ).render(**self.d)
            )

        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = jinja2.Template(
                """
  if ({{GUARD_PREDICATE}}) {
    assertion(_amrPlotter);
    assertion(_subdomainPlotter);
    assertion(_refinementLevelPlotter);

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
    assertion(_{{name}}DataPlotter);
{% endfor %}
{% else %}
    assertion(_cellDataPlotter);
{% endif %}

    const auto& patchSize       = marker.getH();
    const auto& patchOffset     = marker.getX() - patchSize * 0.5;
    const auto& refinementLevel = marker.level;

    // Size of an individual cell within the patch
    const auto cellSize = patchSize / static_cast<double>({{DOFS_PER_AXIS}});

    // Loop over all cells in the patch
    int cellIndex = -1;
    dfor(k, {{DOFS_PER_AXIS}}) {
      tarch::la::Vector<DIMENSIONS, double> kDouble;
      for (int d = 0; d < DIMENSIONS; d++) {
        kDouble(d) = static_cast<double>(k(d));
      }

      tarch::la::Vector<DIMENSIONS, double> cellOffset = patchOffset + tarch::la::multiplyComponents(kDouble, cellSize);

      #if DIMENSIONS == 2
      cellIndex = _amrPlotter->plotCell2D(
        tarch::la::Vector<2, double>({cellOffset(0), cellOffset(1)}),
        tarch::la::Vector<2, double>({cellSize(0), cellSize(1)}),
        refinementLevel
      );
      #else
      cellIndex = _amrPlotter->plotCell3D(
        tarch::la::Vector<3, double>({cellOffset(0), cellOffset(1), cellOffset(2)}),
        tarch::la::Vector<3, double>({cellSize(0), cellSize(1), cellSize(2)}),
        refinementLevel
      );
      #endif

      int currentDoF = ::peano4::dLinearised(k, {{DOFS_PER_AXIS}}) * {{UNKNOWNS}};
      const auto* data = fineGridCell{{NAME}}.data() + currentDoF;

{% if DESCRIPTION %}
      int dataOffset = 0;
{% for name, count in DESCRIPTION.items() %}
      _{{name}}DataPlotter->plotCellData(cellIndex, data + dataOffset, {{count}});
      dataOffset += {{count}};
{% endfor %}
{% else %}
      _cellDataPlotter->plotCellData(cellIndex, data, {{UNKNOWNS}});
{% endif %}

      _subdomainPlotter->plotCellData(cellIndex, _treeNumber);
      _refinementLevelPlotter->plotCellData(cellIndex, refinementLevel);
    }
  }
"""
            ).render(**self.d)

        return result

    def get_attributes(self):
        return (
            super().get_attributes()
            + jinja2.Template(
                """
  std::unique_ptr<tarch::plotter::AMRPlotter> _amrPlotter;

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _{{name}}DataPlotter;
{% endfor %}
{% else %}
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _cellDataPlotter;
{% endif %}
"""
            ).render(**self.d)
        )

    def get_includes(self):
        return super().get_includes() + (
            """
#include "tarch/plotter/AMRPlotter.h"
"""
        )
