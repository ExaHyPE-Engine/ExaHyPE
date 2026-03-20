# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .BasePlotter import BasePlotter

from peano4.solversteps.ActionSet import ActionSet

import jinja2


# Analogous to PatchNodalPlotter but for faces instead of cells
class FaceNodalPlotter(BasePlotter):
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

        # for i in patch.dim:
        #     assert i == patch.dim[1]

        self.d["DOFS_PER_AXIS"] = str(patch.dim[1])
        self.d["UNKNOWNS"] = str(patch.no_of_unknowns)

        assert len(mapping) > 0

        counter = 0
        self.d["MAPPING_2D"] = (
            "constexpr double mapping[" + str(patch.dim[1]) + "] = {"
        )
        self.d["MAPPING_3D"] = (
            "constexpr double mapping[" + str(2 * patch.dim[1] * patch.dim[1]) + "] = {"
        )
        for i in mapping:
            if counter > 0:
                separator = ", "
            else:
                separator = " "
            if counter < patch.dim[1]:
                self.d["MAPPING_2D"] += separator + str(i[0]) + "\n"
            if counter < patch.dim[1] * patch.dim[1]:
                self.d["MAPPING_3D"] += separator + str(i[0])
                self.d["MAPPING_3D"] += ", " + str(i[1])
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
  _{{name}}DataPlotterL.reset();
  _{{name}}DataPlotterL = _fileWriter->createVertexDataPlotter(_localTreeNumber, "{{name}}L", {{count}});
  _{{name}}DataPlotterR.reset();
  _{{name}}DataPlotterR = _fileWriter->createVertexDataPlotter(_localTreeNumber, "{{name}}R", {{count}});
{% endfor %}
{% else %}
  _vertexDataPlotterL.reset();
  _vertexDataPlotterL = _fileWriter->createVertexDataPlotter(_localTreeNumber, "{{NAME}}L", {{UNKNOWNS}});
  _vertexDataPlotterR.reset();
  _vertexDataPlotterR = _fileWriter->createVertexDataPlotter(_localTreeNumber, "{{NAME}}R", {{UNKNOWNS}});
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
    assertion(_refinementLevelPlotter);

{% if DESCRIPTION %}
{% for name, count in DESCRIPTION.items() %}
    assertion(_{{name}}DataPlotterL);
    assertion(_{{name}}DataPlotterR);
{% endfor %}
{% else %}
    assertion(_vertexDataPlotterL)
    assertion(_vertexDataPlotterR)
{% endif %}

    #if DIMENSIONS == 2
    {{MAPPING_2D}}
    #else
    {{MAPPING_3D}}
    #endif

    const int normal = marker.getSelectedFaceNumber() % DIMENSIONS;

    auto faceSize    = marker.getH();
    const auto& faceOffset     = marker.getX() - faceSize * 0.5;
    faceSize[normal] = 0.;

    // face directions, a face with normal 0 for example is a y-z (1,2)
    const int fD_1 = (normal==0);
#if DIMENSIONS == 3
    const int fD_2 = 2 - (normal==2);
#endif

    constexpr int PointsPerAxis = {{DOFS_PER_AXIS}};
    constexpr int TotalPoints   = (DIMENSIONS==2) ? PointsPerAxis*PointsPerAxis : PointsPerAxis*PointsPerAxis*PointsPerAxis;

    int vertexIndices[TotalPoints];
    tarch::la::Vector<DIMENSIONS, double> p;
    p(normal) = faceOffset(normal);

#if DIMENSIONS == 3
    constexpr int elementsPerFace = {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{UNKNOWNS}};
#else
    constexpr int elementsPerFace = {{DOFS_PER_AXIS}} * {{UNKNOWNS}};
#endif

    const auto* dataL = fineGridFace{{NAME}}.data();
    const auto* dataR = fineGridFace{{NAME}}.data() + elementsPerFace;

    int counter = 0;

    #if DIMENSIONS == 3
    for (int j=0; j<PointsPerAxis; j++) {
    #endif
      for (int i=0; i<PointsPerAxis; i++) {
        // Apply Mapping
        int baseIndex = counter * (DIMENSIONS-1);
        p(fD_1) = faceOffset(fD_1) + faceSize(fD_1) * mapping[baseIndex + 0];
        #if DIMENSIONS == 3
        p(fD_2) = faceOffset(fD_2) + faceSize(fD_2) * mapping[baseIndex + 1];
        #endif

        // Plot Vertex
        vertexIndices[counter] = _vertexPlotter->plotVertex(p);

        const int dofIndex = counter * {{UNKNOWNS}};

{% if DESCRIPTION %}
        int dataOffset = 0;
{% for name, count in DESCRIPTION.items() %}
        double {{name}}Values[{{count}}];
        for (int u=0; u<{{count}}; u++) {
          {{name}}Values[u] = dataL[dofIndex + dataOffset + u];
        }
        _{{name}}DataPlotterL->plotVertexData(vertexIndices[counter], {{name}}Values, {{count}});
        for (int u=0; u<{{count}}; u++) {
          {{name}}Values[u] = dataR[dofIndex + dataOffset + u];
        }
        _{{name}}DataPlotterR->plotVertexData(vertexIndices[counter], {{name}}Values, {{count}});
        dataOffset += {{count}};
{% endfor %}
{% else %}
        double values[2*{{UNKNOWNS}}];
        for (int u=0; u<{{UNKNOWNS}}; u++) {
          values[u] = dataL[dofIndex + u];
        }
        _vertexDataPlotterL->plotVertexData(vertexIndices[counter], values, {{UNKNOWNS}});
        for (int u=0; u<{{UNKNOWNS}}; u++) {
          values[u] = dataR[dofIndex + u];
        }
        _vertexDataPlotterR->plotVertexData(vertexIndices[counter], values, {{UNKNOWNS}});
{% endif %}
        counter++;
      }

    #if DIMENSIONS == 3
    }
    #endif

    #if DIMENSIONS == 3
    for (int j=0; j<PointsPerAxis - 1; j++) {
    #endif
      for (int i=0; i<PointsPerAxis - 1; i++) {
          int idx = i 
          #if DIMENSIONS == 3
            + j * PointsPerAxis
          #endif
          ;

          int s_x = 1;
          int s_y = PointsPerAxis;

          #if DIMENSIONS == 2
          int v[2];
          v[0] = vertexIndices[idx];
          v[1] = vertexIndices[idx + s_x];
          int cellIndex = _cellPlotter->plotLine(v);
          #elif DIMENSIONS == 3
          int v[4];
          v[0] = vertexIndices[idx];
          v[1] = vertexIndices[idx + s_x];
          v[2] = vertexIndices[idx + s_x + s_y];
          v[3] = vertexIndices[idx + s_y];
          int cellIndex = _cellPlotter->plotQuadrangle(v);
          #endif

          _subdomainPlotter->plotCellData(cellIndex, _treeNumber);
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
