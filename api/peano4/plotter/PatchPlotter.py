# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.solversteps.ActionSet import ActionSet

from .BasePlotter import BasePlotter


class PatchPlotter(BasePlotter):
    """! The nodal representation gives every patch its own mapped vertices and
    connects neighbouring vertices within that patch, so a patch boundary keeps
    two traces. The cell representation writes one cell-data value per finite
    volume and ignores mapping.

    The description argument maps field names to component counts, and an empty
    description writes all unknowns under the dataset name.
    """

    def __init__(
        self,
        filename,
        patch,
        dataset_name,
        description,
        time_stamp_evaluation,
        guard,
        representation="nodal",
        mapping=None,
        file_format=None,
        additional_cell_data_declarations="",
        additional_cell_data_construction="",
        additional_cell_data_plots="",
        alternative_data_guard="",
        alternative_data_access="",
        alternative_data_dofs_per_axis=0,
    ):
        """Whenever ``alternative_data_guard`` evaluates to true, every plot point
        reads the nearest degree of freedom of the array that
        ``alternative_data_access`` yields, on a grid of
        ``alternative_data_dofs_per_axis`` entries per axis."""
        super().__init__(
            filename,
            dataset_name,
            description,
            time_stamp_evaluation,
            guard,
            file_format=file_format,
            additional_cell_data_declarations=additional_cell_data_declarations,
            additional_cell_data_construction=additional_cell_data_construction,
            additional_cell_data_plots=additional_cell_data_plots,
        )
        self._dictionary["OUTPUT_SUBFOLDER"] = "patch/"

        if representation not in ("nodal", "cell"):
            raise ValueError(
                'The representation must be "nodal" or "cell" and is "{}".'.format(
                    representation
                )
            )
        if any(dimension != patch.dim[0] for dimension in patch.dim):
            raise ValueError("The patch must span the same extent in every dimension.")

        if alternative_data_guard and representation != "nodal":
            raise ValueError(
                "An alternative data source requires the nodal representation."
            )
        if alternative_data_guard and (
            not alternative_data_access or alternative_data_dofs_per_axis <= 0
        ):
            raise ValueError(
                "An alternative data source requires its data access and a "
                "positive number of degrees of freedom per axis."
            )
        self._dictionary["ALTERNATIVE_DATA_GUARD"] = alternative_data_guard
        self._dictionary["ALTERNATIVE_DATA_ACCESS"] = alternative_data_access
        self._dictionary["ALTERNATIVE_DATA_DOFS_PER_AXIS"] = str(
            alternative_data_dofs_per_axis
        )

        self._representation = representation
        degrees_of_freedom_per_axis = patch.dim[0]
        self._dictionary["DOFS_PER_AXIS"] = str(degrees_of_freedom_per_axis)
        self._dictionary["UNKNOWNS"] = str(patch.no_of_unknowns)
        field_offset = 0
        self._dictionary["PLOT_FIELDS"] = []
        if description:
            for field_name, component_count in description.items():
                self._dictionary["PLOT_FIELDS"].append(
                    (field_name, component_count, field_offset)
                )
                field_offset += component_count

        if representation == "nodal":
            if mapping is None or len(mapping) == 0:
                raise ValueError(
                    "The nodal representation requires mapping coordinates."
                )
            mapping_2D = []
            mapping_3D = []
            for point_index, point in enumerate(mapping):
                if len(point) < 2:
                    raise ValueError(
                        "Each mapping point needs at least two coordinates."
                    )
                if point_index < degrees_of_freedom_per_axis**2:
                    mapping_2D.extend((point[0], point[1]))
                mapping_3D.extend(
                    (point[0], point[1], point[2] if len(point) > 2 else 0.0)
                )
            self._dictionary["MAPPING_2D"] = (
                "{" + ", ".join(str(value) for value in mapping_2D) + "}"
            )
            self._dictionary["MAPPING_3D"] = (
                "{" + ", ".join(str(value) for value in mapping_3D) + "}"
            )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_body_of_operation(self, operation_name):
        self._dictionary["ADDITIONAL_CELL_DATA_PLOTS"] = (
            self.additional_cell_data_plots()
        )
        if self._representation == "nodal":
            return self._get_body_of_operation_nodal(operation_name)
        return self._get_body_of_operation_cell(operation_name)

    def _get_body_of_operation_nodal(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = (
                super().get_body_of_operation(operation_name)
                + jinja2.Template(
                    """
  _vertexPlotter.reset();
  _vertexPlotter = _fileWriter->createVertexPlotter(_pieceId, tarch::plotter::FileWriter::VertexMerging::Distinct);

  _cellPlotter.reset();
  _cellPlotter = _fileWriter->createCellPlotter(_pieceId);

  _curvePositionPlotter.reset();
  _curvePositionPlotter = _fileWriter->createCellDataPlotter(_pieceId, "CurvePosition", 1);

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  _{{name}}DataPlotter.reset();
  _{{name}}DataPlotter = _fileWriter->createVertexDataPlotter(_pieceId, "{{name}}", {{count}});
{% endfor %}
{% else %}
  _vertexDataPlotter.reset();
  _vertexDataPlotter = _fileWriter->createVertexDataPlotter(_pieceId, "{{NAME}}", {{UNKNOWNS}});
{% endif %}
""",
                    trim_blocks=True,
                    lstrip_blocks=True,
                ).render(**self._dictionary)
            )
        elif operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = jinja2.Template(
                """
  if ({{GUARD_PREDICATE}}) {
    assertion(_vertexPlotter);
    assertion(_cellPlotter);
    assertion(_subdomainPlotter);
    assertion(_subtreeTaskPlotter);
    assertion(_refinementLevelPlotter);
{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
    assertion(_{{name}}DataPlotter);
{% endfor %}
{% else %}
    assertion(_vertexDataPlotter);
{% endif %}

    constexpr int PointsPerAxis = {{DOFS_PER_AXIS}};
    constexpr int TotalPoints =
      DIMENSIONS == 2 ? PointsPerAxis * PointsPerAxis : PointsPerAxis * PointsPerAxis * PointsPerAxis;
#if DIMENSIONS == 2
    constexpr std::array<double, 2 * TotalPoints> Mapping{{MAPPING_2D}};
#else
    constexpr std::array<double, 3 * TotalPoints> Mapping{{MAPPING_3D}};
#endif

    const auto&                  patchSize       = marker.getH();
    const auto                   patchOffset     = marker.getX() - patchSize * 0.5;
    const auto                   refinementLevel = marker.level;
    const auto                   curvePosition   = peano4::sfc::ActiveCurve::curveParameter(
      tarch::la::divideComponents(marker.getX() - DomainOffset, DomainSize), refinementLevel
    );
    std::array<int, TotalPoints> vertexIndices{};
    const auto* const            data = fineGridCell{{NAME}}.data();
{% if ALTERNATIVE_DATA_GUARD %}
    const bool        useAlternativeData = {{ALTERNATIVE_DATA_GUARD}};
    const auto* const alternativeData = {{ALTERNATIVE_DATA_ACCESS}};
    constexpr int     AlternativePointsPerAxis = {{ALTERNATIVE_DATA_DOFS_PER_AXIS}};
    const auto        alternativeIndexAlongAxis = [](const double referenceCoordinate) {
      return std::clamp(
        static_cast<int>(referenceCoordinate * AlternativePointsPerAxis),
        0,
        AlternativePointsPerAxis - 1
      );
    };
{% endif %}

#if DIMENSIONS == 3
    for (int pointZ = 0; pointZ < PointsPerAxis; pointZ++) {
#endif
      for (int pointY = 0; pointY < PointsPerAxis; pointY++) {
        for (int pointX = 0; pointX < PointsPerAxis; pointX++) {
#if DIMENSIONS == 2
          const int pointIndex = pointX + pointY * PointsPerAxis;
#else
          const int pointIndex =
            pointX + pointY * PointsPerAxis + pointZ * PointsPerAxis * PointsPerAxis;
#endif
          const int                             mappingOffset = pointIndex * DIMENSIONS;
          tarch::la::Vector<DIMENSIONS, double> position;
          position(0) = patchOffset(0) + patchSize(0) * Mapping[mappingOffset];
          position(1) = patchOffset(1) + patchSize(1) * Mapping[mappingOffset + 1];
#if DIMENSIONS == 3
          position(2) = patchOffset(2) + patchSize(2) * Mapping[mappingOffset + 2];
#endif
          vertexIndices[pointIndex] = _vertexPlotter->plotVertex(position);
          const int unknownOffset = pointIndex * {{UNKNOWNS}};
{% if ALTERNATIVE_DATA_GUARD %}
          const int alternativeOffset =
            (alternativeIndexAlongAxis(Mapping[mappingOffset])
             + alternativeIndexAlongAxis(Mapping[mappingOffset + 1]) * AlternativePointsPerAxis
#if DIMENSIONS == 3
             + alternativeIndexAlongAxis(Mapping[mappingOffset + 2])
               * AlternativePointsPerAxis * AlternativePointsPerAxis
#endif
            ) * {{UNKNOWNS}};
          const auto* const pointData =
            useAlternativeData ? alternativeData + alternativeOffset : data + unknownOffset;
{% else %}
          const auto* const pointData = data + unknownOffset;
{% endif %}
{% if DESCRIPTION %}
{% for name, count, offset in PLOT_FIELDS %}
          std::array<double, {{count}}> {{name}}Values{};
          for (int unknown = 0; unknown < {{count}}; unknown++) {
            {{name}}Values[unknown] = pointData[{% if offset > 0 %}{{offset}} + {% endif %}unknown];
          }
          _{{name}}DataPlotter->plotVertexData(vertexIndices[pointIndex], {{name}}Values.data(), {{count}});
{% endfor %}
{% else %}
          std::array<double, {{UNKNOWNS}}> values{};
          for (int unknown = 0; unknown < {{UNKNOWNS}}; unknown++) {
            values[unknown] = pointData[unknown];
          }
          _vertexDataPlotter->plotVertexData(vertexIndices[pointIndex], values.data(), {{UNKNOWNS}});
{% endif %}
        }
      }
#if DIMENSIONS == 3
    }
#endif

#if DIMENSIONS == 3
    for (int cellZ = 0; cellZ < PointsPerAxis - 1; cellZ++) {
#endif
      for (int cellY = 0; cellY < PointsPerAxis - 1; cellY++) {
        for (int cellX = 0; cellX < PointsPerAxis - 1; cellX++) {
#if DIMENSIONS == 2
          const int firstVertex = cellX + cellY * PointsPerAxis;
#else
          const int firstVertex =
            cellX + cellY * PointsPerAxis + cellZ * PointsPerAxis * PointsPerAxis;
#endif
          constexpr int XStride = 1;
          constexpr int YStride = PointsPerAxis;
#if DIMENSIONS == 2
          const std::array<int, 4> vertices{
            vertexIndices[firstVertex],
            vertexIndices[firstVertex + XStride],
            vertexIndices[firstVertex + XStride + YStride],
            vertexIndices[firstVertex + YStride]
          };
          const int cellIndex = _cellPlotter->plotQuadrangle(vertices.data());
#else
          constexpr int ZStride = PointsPerAxis * PointsPerAxis;
          const std::array<int, 8> vertices{
            vertexIndices[firstVertex],
            vertexIndices[firstVertex + XStride],
            vertexIndices[firstVertex + XStride + YStride],
            vertexIndices[firstVertex + YStride],
            vertexIndices[firstVertex + ZStride],
            vertexIndices[firstVertex + XStride + ZStride],
            vertexIndices[firstVertex + XStride + YStride + ZStride],
            vertexIndices[firstVertex + YStride + ZStride]
          };
          const int cellIndex = _cellPlotter->plotHexahedron(vertices.data());
#endif
          _subdomainPlotter->plotCellData(cellIndex, tarch::mpi::Rank::getInstance().getRank());
          _subtreeTaskPlotter->plotCellData(cellIndex, getSubtreeId());
          _refinementLevelPlotter->plotCellData(cellIndex, refinementLevel);
          _curvePositionPlotter->plotCellData(cellIndex, curvePosition);
{% if ADDITIONAL_CELL_DATA_PLOTS %}
{{ADDITIONAL_CELL_DATA_PLOTS}}
{% endif %}
        }
      }
#if DIMENSIONS == 3
    }
#endif
  }
""",
                trim_blocks=True,
                lstrip_blocks=True,
            ).render(**self._dictionary)
        return result

    def _get_body_of_operation_cell(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = (
                super().get_body_of_operation(operation_name)
                + jinja2.Template(
                    """
  _vertexPlotter.reset();
  _vertexPlotter = _fileWriter->createVertexPlotter(_pieceId);

  _cellPlotter.reset();
  _cellPlotter = _fileWriter->createCellPlotter(_pieceId);

  _curvePositionPlotter.reset();
  _curvePositionPlotter = _fileWriter->createCellDataPlotter(_pieceId, "CurvePosition", 1);

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  _{{name}}DataPlotter.reset();
  _{{name}}DataPlotter = _fileWriter->createCellDataPlotter(_pieceId, "{{name}}", {{count}});
{% endfor %}
{% else %}
  _cellDataPlotter.reset();
  _cellDataPlotter = _fileWriter->createCellDataPlotter(_pieceId, "{{NAME}}", {{UNKNOWNS}});
{% endif %}
""",
                    trim_blocks=True,
                    lstrip_blocks=True,
                ).render(**self._dictionary)
            )
        elif operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = jinja2.Template(
                """
  if ({{GUARD_PREDICATE}}) {
    assertion(_vertexPlotter);
    assertion(_cellPlotter);
    assertion(_subdomainPlotter);
    assertion(_subtreeTaskPlotter);
    assertion(_refinementLevelPlotter);
{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
    assertion(_{{name}}DataPlotter);
{% endfor %}
{% else %}
    assertion(_cellDataPlotter);
{% endif %}

    const auto& patchSize       = marker.getH();
    const auto  patchOffset     = marker.getX() - patchSize * 0.5;
    const auto  refinementLevel = marker.level;
    const auto  cellSize        = patchSize / static_cast<double>({{DOFS_PER_AXIS}});
    const auto  curvePosition   = peano4::sfc::ActiveCurve::curveParameter(
      tarch::la::divideComponents(marker.getX() - DomainOffset, DomainSize), refinementLevel
    );

    peano4::dForEach({{DOFS_PER_AXIS}}, [&](const tarch::la::Vector<DIMENSIONS, int>& volume) {
      tarch::la::Vector<DIMENSIONS, double> volumeAsDouble;
      for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
        volumeAsDouble(dimension) = static_cast<double>(volume(dimension));
      }
      const auto cellOffset = patchOffset + tarch::la::multiplyComponents(volumeAsDouble, cellSize);

#if DIMENSIONS == 2
      const std::array<int, 4> vertices{
        _vertexPlotter->plotVertex(tarch::la::Vector<2, double>{cellOffset(0), cellOffset(1)}),
        _vertexPlotter->plotVertex(tarch::la::Vector<2, double>{cellOffset(0) + cellSize(0), cellOffset(1)}),
        _vertexPlotter->plotVertex(
          tarch::la::Vector<2, double>{cellOffset(0) + cellSize(0), cellOffset(1) + cellSize(1)}
        ),
        _vertexPlotter->plotVertex(tarch::la::Vector<2, double>{cellOffset(0), cellOffset(1) + cellSize(1)})
      };
      const int cellIndex = _cellPlotter->plotQuadrangle(vertices.data());
#else
      const std::array<int, 8> vertices{
        _vertexPlotter->plotVertex(
          tarch::la::Vector<3, double>{cellOffset(0), cellOffset(1), cellOffset(2)}
        ),
        _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{
          cellOffset(0) + cellSize(0),
          cellOffset(1),
          cellOffset(2)
        }),
        _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{
          cellOffset(0) + cellSize(0),
          cellOffset(1) + cellSize(1),
          cellOffset(2)
        }),
        _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{
          cellOffset(0),
          cellOffset(1) + cellSize(1),
          cellOffset(2)
        }),
        _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{
          cellOffset(0),
          cellOffset(1),
          cellOffset(2) + cellSize(2)
        }),
        _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{
          cellOffset(0) + cellSize(0),
          cellOffset(1),
          cellOffset(2) + cellSize(2)
        }),
        _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{
          cellOffset(0) + cellSize(0),
          cellOffset(1) + cellSize(1),
          cellOffset(2) + cellSize(2)
        }),
        _vertexPlotter->plotVertex(tarch::la::Vector<3, double>{
          cellOffset(0),
          cellOffset(1) + cellSize(1),
          cellOffset(2) + cellSize(2)
        })
      };
      const int cellIndex = _cellPlotter->plotHexahedron(vertices.data());
#endif
      const auto        unknownOffset = peano4::dLinearised(volume, {{DOFS_PER_AXIS}}) * {{UNKNOWNS}};
      const auto* const data          = fineGridCell{{NAME}}.data() + unknownOffset;
{% if DESCRIPTION %}
{% for name, count, offset in PLOT_FIELDS %}
      _{{name}}DataPlotter->plotCellData(cellIndex, data{% if offset > 0 %} + {{offset}}{% endif %}, {{count}});
{% endfor %}
{% else %}
      _cellDataPlotter->plotCellData(cellIndex, data, {{UNKNOWNS}});
{% endif %}

      _subdomainPlotter->plotCellData(cellIndex, tarch::mpi::Rank::getInstance().getRank());
      _subtreeTaskPlotter->plotCellData(cellIndex, getSubtreeId());
      _refinementLevelPlotter->plotCellData(cellIndex, refinementLevel);
      _curvePositionPlotter->plotCellData(cellIndex, curvePosition);
{% if ADDITIONAL_CELL_DATA_PLOTS %}
{{ADDITIONAL_CELL_DATA_PLOTS}}
{% endif %}
    });
  }
""",
                trim_blocks=True,
                lstrip_blocks=True,
            ).render(**self._dictionary)
        return result

    def get_includes(self):
        return (
            super().get_includes()
            + """
#include "Constants.h"
#include "peano4/sfc/ActiveCurve.h"
"""
        )

    def get_attributes(self):
        if self._representation == "nodal":
            return (
                super().get_attributes()
                + jinja2.Template(
                    """
  std::unique_ptr<tarch::plotter::FileWriter::CellPlotter>       _cellPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter>     _vertexPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter>   _curvePositionPlotter{};
{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> _{{name}}DataPlotter{};
{% endfor %}
{% else %}
  std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> _vertexDataPlotter{};
{% endif %}
""",
                    trim_blocks=True,
                    lstrip_blocks=True,
                ).render(**self._dictionary)
            )
        return (
            super().get_attributes()
            + jinja2.Template(
                """
  std::unique_ptr<tarch::plotter::FileWriter::CellPlotter>     _cellPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter>   _vertexPlotter{};
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _curvePositionPlotter{};
{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _{{name}}DataPlotter{};
{% endfor %}
{% else %}
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _cellDataPlotter{};
{% endif %}
""",
                trim_blocks=True,
                lstrip_blocks=True,
            ).render(**self._dictionary)
        )
