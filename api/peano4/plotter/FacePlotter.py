# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .BasePlotter import BasePlotter

from peano4.solversteps.ActionSet import ActionSet

import jinja2


class FacePlotter(BasePlotter):
    def __init__(
        self,
        filename,
        patch,
        dataset_name,
        description,
        time_stamp_evaluation,
        mapping,
        guard,
        file_format=None,
    ):
        super().__init__(
            filename,
            dataset_name,
            description,
            time_stamp_evaluation,
            guard,
            file_format=file_format,
        )

        self.d["OUTPUT_SUBFOLDER"] = "face/"

        self.d["DOFS_PER_AXIS"] = str(patch.dim[1])
        self.d["UNKNOWNS"] = str(patch.no_of_unknowns)

        assert len(mapping) > 0

        point_index = 0
        self.d["MAPPING_2D"] = "constexpr double Mapping[" + str(patch.dim[1]) + "] = {"
        self.d["MAPPING_3D"] = (
            "constexpr double Mapping[" + str(2 * patch.dim[1] * patch.dim[1]) + "] = {"
        )
        for point in mapping:
            if point_index > 0:
                separator = ", "
            else:
                separator = " "
            if point_index < patch.dim[1]:
                self.d["MAPPING_2D"] += separator + str(point[0]) + "\n"
            if point_index < patch.dim[1] * patch.dim[1]:
                self.d["MAPPING_3D"] += separator + str(point[0])
                self.d["MAPPING_3D"] += ", " + str(point[1])
                self.d["MAPPING_3D"] += "\n"
            point_index += 1

        self.d["MAPPING_2D"] += "};"
        self.d["MAPPING_3D"] += "};"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_body_of_operation(self, operation_name):
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

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  _{{name}}DataPlotterL.reset();
  _{{name}}DataPlotterL = _fileWriter->createVertexDataPlotter(_pieceId, "{{name}}L", {{count}});
  _{{name}}DataPlotterR.reset();
  _{{name}}DataPlotterR = _fileWriter->createVertexDataPlotter(_pieceId, "{{name}}R", {{count}});
{% endfor %}
{% else %}
  _vertexDataPlotterL.reset();
  _vertexDataPlotterL = _fileWriter->createVertexDataPlotter(_pieceId, "{{NAME}}L", {{UNKNOWNS}});
  _vertexDataPlotterR.reset();
  _vertexDataPlotterR = _fileWriter->createVertexDataPlotter(_pieceId, "{{NAME}}R", {{UNKNOWNS}});
{% endif %}
"""
                ).render(**self.d)
            )

        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            result = jinja2.Template(
                """
  if (not marker.hasBeenRefined() and
    {{GUARD_PREDICATE}}) {

    assertion(_vertexPlotter);
    assertion(_cellPlotter);
    assertion(_subdomainPlotter);
    assertion(_subtreeTaskPlotter);
    assertion(_refinementLevelPlotter);

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
    assertion(_{{name}}DataPlotterL);
    assertion(_{{name}}DataPlotterR);
{% endfor %}
{% else %}
    assertion(_vertexDataPlotterL);
    assertion(_vertexDataPlotterR);
{% endif %}

#if DIMENSIONS == 2
    {{MAPPING_2D}}
#else
    {{MAPPING_3D}}
#endif

    const int normalDimension = marker.getSelectedFaceNumber() % DIMENSIONS;

    auto faceSize = marker.getH();
    const auto& faceOffset = marker.getX() - faceSize * 0.5;
    faceSize[normalDimension] = 0.;

    // The tangential dimensions enumerate the axes other than the normal dimension.
    const int firstTangentialDimension = normalDimension == 0;
#if DIMENSIONS == 3
    const int secondTangentialDimension = 2 - (normalDimension == 2);
#endif

    constexpr int PointsPerAxis = {{DOFS_PER_AXIS}};
    constexpr int TotalPoints   = DIMENSIONS == 2 ? PointsPerAxis : PointsPerAxis * PointsPerAxis;

    int vertexIndices[TotalPoints];
    tarch::la::Vector<DIMENSIONS, double> position;
    position(normalDimension) = faceOffset(normalDimension);

#if DIMENSIONS == 3
    constexpr int ElementsPerFace = {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{UNKNOWNS}};
#else
    constexpr int ElementsPerFace = {{DOFS_PER_AXIS}} * {{UNKNOWNS}};
#endif

    const auto* const leftData = fineGridFace{{NAME}}.data();
    const auto* const rightData = fineGridFace{{NAME}}.data() + ElementsPerFace;

#if DIMENSIONS == 3
    for (int pointY = 0; pointY < PointsPerAxis; pointY++) {
#endif
      for (int pointX = 0; pointX < PointsPerAxis; pointX++) {
#if DIMENSIONS == 2
        const int pointIndex = pointX;
#else
        const int pointIndex = pointX + pointY * PointsPerAxis;
#endif
        const int mappingOffset = pointIndex * (DIMENSIONS - 1);
        position(firstTangentialDimension) =
          faceOffset(firstTangentialDimension)
          + faceSize(firstTangentialDimension) * Mapping[mappingOffset];
#if DIMENSIONS == 3
        position(secondTangentialDimension) =
          faceOffset(secondTangentialDimension)
          + faceSize(secondTangentialDimension) * Mapping[mappingOffset + 1];
#endif

        vertexIndices[pointIndex] = _vertexPlotter->plotVertex(position);

        const int unknownOffset = pointIndex * {{UNKNOWNS}};

{% if DESCRIPTION %}
        int dataOffset = 0;
{% for name, count in DESCRIPTION.items() %}
        double {{name}}Values[{{count}}];
        for (int unknown = 0; unknown < {{count}}; unknown++) {
          {{name}}Values[unknown] = (double) leftData[unknownOffset + dataOffset + unknown];
        }
        _{{name}}DataPlotterL->plotVertexData(vertexIndices[pointIndex], {{name}}Values, {{count}});
        for (int unknown = 0; unknown < {{count}}; unknown++) {
          {{name}}Values[unknown] = (double) rightData[unknownOffset + dataOffset + unknown];
        }
        _{{name}}DataPlotterR->plotVertexData(vertexIndices[pointIndex], {{name}}Values, {{count}});
        dataOffset += {{count}};
{% endfor %}
{% else %}
        double values[{{UNKNOWNS}}];
        for (int unknown = 0; unknown < {{UNKNOWNS}}; unknown++) {
          values[unknown] = (double) leftData[unknownOffset + unknown];
        }
        _vertexDataPlotterL->plotVertexData(vertexIndices[pointIndex], values, {{UNKNOWNS}});
        for (int unknown = 0; unknown < {{UNKNOWNS}}; unknown++) {
          values[unknown] = (double) rightData[unknownOffset + unknown];
        }
        _vertexDataPlotterR->plotVertexData(vertexIndices[pointIndex], values, {{UNKNOWNS}});
{% endif %}
      }

#if DIMENSIONS == 3
    }
#endif

#if DIMENSIONS == 3
    for (int cellY = 0; cellY < PointsPerAxis - 1; cellY++) {
#endif
      for (int cellX = 0; cellX < PointsPerAxis - 1; cellX++) {
#if DIMENSIONS == 2
        const int firstVertex = cellX;
#else
        const int firstVertex = cellX + cellY * PointsPerAxis;
#endif

        constexpr int XStride = 1;
#if DIMENSIONS == 3
        constexpr int YStride = PointsPerAxis;
#endif

#if DIMENSIONS == 2
        const int vertices[2] = {vertexIndices[firstVertex], vertexIndices[firstVertex + XStride]};
        const int cellIndex = _cellPlotter->plotLine(vertices);
#elif DIMENSIONS == 3
        const int vertices[4] = {
          vertexIndices[firstVertex],
          vertexIndices[firstVertex + XStride],
          vertexIndices[firstVertex + XStride + YStride],
          vertexIndices[firstVertex + YStride]
        };
        const int cellIndex = _cellPlotter->plotQuadrangle(vertices);
#endif

          _subdomainPlotter->plotCellData(cellIndex, tarch::mpi::Rank::getInstance().getRank());
          _subtreeTaskPlotter->plotCellData(cellIndex, getSubtreeTask());
          _refinementLevelPlotter->plotCellData(cellIndex, 0);
      }
#if DIMENSIONS == 3
    }
#endif
  }
"""
            ).render(**self.d)

        return result

    def get_attributes(self):
        return (
            super().get_attributes()
            + jinja2.Template(
                """
  std::unique_ptr<tarch::plotter::FileWriter::CellPlotter> _cellPlotter;
  std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter> _vertexPlotter;

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> _{{name}}DataPlotterL;
  std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> _{{name}}DataPlotterR;
{% endfor %}
{% else %}
  std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> _vertexDataPlotterL;
  std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> _vertexDataPlotterR;
{% endif %}
"""
            ).render(**self.d)
        )
