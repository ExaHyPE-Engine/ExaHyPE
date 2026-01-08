# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .BasePlotter import BasePlotter

from peano4.solversteps.ActionSet import ActionSet

import jinja2


class PatchNodalPlotter(BasePlotter):
    def __init__(
        self,
        output_path,
        filename,
        patch,
        dataset_name,
        description,
        time_stamp_evaluation,
        mapping,
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

        assert len(mapping) > 0

        counter = 0
        self.d["MAPPING_2D"] = (
            "constexpr double mapping[" + str(2 * patch.dim[0] * patch.dim[0]) + "] = {"
        )
        self.d["MAPPING_3D"] = (
            "constexpr double mapping["
            + str(3 * patch.dim[0] * patch.dim[0] * patch.dim[0])
            + "] = {"
        )
        for i in mapping:
            if counter > 0:
                separator = ", "
            else:
                separator = " "
            if counter < patch.dim[0] * patch.dim[0]:
                self.d["MAPPING_2D"] += separator + str(i[0])
                self.d["MAPPING_2D"] += ", " + str(i[1])
                self.d["MAPPING_2D"] += "\n"
            self.d["MAPPING_3D"] += separator + str(i[0])
            self.d["MAPPING_3D"] += ", " + str(i[1])
            if len(i) > 2:
                self.d["MAPPING_3D"] += ", " + str(i[2])
            else:
                self.d["MAPPING_3D"] += ", 0.0"
            self.d["MAPPING_3D"] += "\n"
            counter += 1

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
  _vertexPlotter = _fileWriter->createVertexPlotter(_localTreeNumber);

  _cellPlotter.reset();
  _cellPlotter = _fileWriter->createCellPlotter(_localTreeNumber);

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
  _{{name}}DataPlotter.reset();
  _{{name}}DataPlotter = _fileWriter->createVertexDataPlotter(_localTreeNumber, "{{name}}", {{count}});
{% endfor %}
{% else %}
  _vertexDataPlotter.reset();
  _vertexDataPlotter = _fileWriter->createVertexDataPlotter(_localTreeNumber, "{{NAME}}", {{UNKNOWNS}});
{% endif %}
"""
                ).render(**self.d)
            )

        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = jinja2.Template(
                """
  if ({{GUARD_PREDICATE}}) {
    assertion(_vertexPlotter);
    assertion(_cellPlotter);
    assertion(_subdomainPlotter);
    assertion(_refinementLevelPlotter);

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
    assertion(_{{name}}DataPlotter);
{% endfor %}
{% else %}
    assertion(_vertexDataPlotter)
{% endif %}

    #if DIMENSIONS == 2
    {{MAPPING_2D}}
    #else
    {{MAPPING_3D}}
    #endif

    const auto& patchSize       = marker.getH();
    const auto& patchOffset     = marker.getX() - patchSize * 0.5;
    const auto& refinementLevel = marker.level;

    constexpr int PointsPerAxis = {{DOFS_PER_AXIS}};
    constexpr int TotalPoints   = (DIMENSIONS==2) ? PointsPerAxis*PointsPerAxis : PointsPerAxis*PointsPerAxis*PointsPerAxis;

    int vertexIndices[TotalPoints];
    tarch::la::Vector<DIMENSIONS, double> p;

    int counter = 0;
    const auto* data = fineGridCell{{NAME}}.data();

    #if DIMENSIONS == 3
    for (int k=0; k<PointsPerAxis; k++) {
    #endif
      for (int j=0; j<PointsPerAxis; j++) {
        for (int i=0; i<PointsPerAxis; i++) {
          // Apply Mapping
          int baseIndex = counter * DIMENSIONS;
          p(0) = patchOffset(0) + patchSize(0) * mapping[baseIndex + 0];
          p(1) = patchOffset(1) + patchSize(1) * mapping[baseIndex + 1];
          #if DIMENSIONS == 3
          p(2) = patchOffset(2) + patchSize(2) * mapping[baseIndex + 2];
          #endif

          // Plot Vertex
          vertexIndices[counter] = _vertexPlotter->plotVertex(p);

          const int dofIndex = counter * {{UNKNOWNS}};

{% if DESCRIPTION %}
          int dataOffset = 0;
{% for name, count in DESCRIPTION.items() %}
          double {{name}}Values[{{count}}];
          for (int u=0; u<{{count}}; u++) {
            {{name}}Values[u] = data[dofIndex + dataOffset + u];
          }
          _{{name}}DataPlotter->plotVertexData(vertexIndices[counter], {{name}}Values, {{count}});
          dataOffset += {{count}};
{% endfor %}
{% else %}
          double values[{{UNKNOWNS}}];
          for (int u=0; u<{{UNKNOWNS}}; u++) {
            values[u] = data[dofIndex + u];
          }
          _vertexDataPlotter->plotVertexData(vertexIndices[counter], values, {{UNKNOWNS}});
{% endif %}
          counter++;
        }
      }
    #if DIMENSIONS == 3
    }
    #endif

    #if DIMENSIONS == 3
    for (int k=0; k<PointsPerAxis - 1; k++) {
    #endif
      for (int j=0; j<PointsPerAxis - 1; j++) {
        for (int i=0; i<PointsPerAxis - 1; i++) {
            int idx = i + j * PointsPerAxis
            #if DIMENSIONS == 3
                    + k * PointsPerAxis * PointsPerAxis
            #endif
            ;

            int s_x = 1;
            int s_y = PointsPerAxis;

            #if DIMENSIONS == 2
            int v[4];
            v[0] = vertexIndices[idx];
            v[1] = vertexIndices[idx + s_x];
            v[2] = vertexIndices[idx + s_x + s_y];
            v[3] = vertexIndices[idx + s_y];
            int cellIndex = _cellPlotter->plotQuadrangle(v);
            #elif DIMENSIONS == 3
            int s_z = PointsPerAxis * PointsPerAxis;
            int v[8];
            v[0] = vertexIndices[idx];
            v[1] = vertexIndices[idx + s_x];
            v[2] = vertexIndices[idx + s_x + s_y];
            v[3] = vertexIndices[idx + s_y];
            v[4] = vertexIndices[idx + s_z];
            v[5] = vertexIndices[idx + s_x + s_z];
            v[6] = vertexIndices[idx + s_x + s_y + s_z];
            v[7] = vertexIndices[idx + s_y + s_z];
            int cellIndex = _cellPlotter->plotHexahedron(v);
            #endif

            _subdomainPlotter->plotCellData(cellIndex, _treeNumber);
            _refinementLevelPlotter->plotCellData(cellIndex, refinementLevel);
        }
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
  std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> _{{name}}DataPlotter;
{% endfor %}
{% else %}
  std::unique_ptr<tarch::plotter::FileWriter::VertexDataPlotter> _vertexDataPlotter;
{% endif %}
"""
            ).render(**self.d)
        )
