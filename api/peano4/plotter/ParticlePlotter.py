# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .BasePlotter import BasePlotter

from peano4.solversteps.ActionSet import ActionSet

import jinja2


class ParticlePlotter(BasePlotter):
    def __init__(
        self,
        filename,
        particle_set,
        description,
        time_stamp_evaluation,
        file_format=None,
    ):
        super().__init__(
            filename,
            particle_set.name,
            description,
            time_stamp_evaluation,
            "true",
            file_format=file_format,
        )

        self.d["OUTPUT_SUBFOLDER"] = "particle/"
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_body_of_operation(self, operation_name):
        result = "\n"

        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = (
                super().get_body_of_operation(operation_name)
                + jinja2.Template(
                    """
  _cellPlotter.reset();
  _cellPlotter = _fileWriter->createCellPlotter(_localTreeNumber);

  _vertexPlotter.reset();
  _vertexPlotter = _fileWriter->createVertexPlotter(_localTreeNumber);
"""
                ).render(**self.d)
            )

        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = jinja2.Template(
                """
  assertion(_cellPlotter);
  assertion(_vertexPlotter);
  assertion(_subdomainPlotter);
  assertion(_refinementLevelPlotter);

  for (auto* p: fineGridVertex{{PARTICLES_CONTAINER}}) {
    if (p->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local) {
      // Plot geometry
      int vertexIndex = _vertexPlotter->plotVertex(p->getX());

      // Plot topology
      int cellIndex = _cellPlotter->plotPoint(vertexIndex);

      // Plot attributes
      _subdomainPlotter->plotCellData(cellIndex, _treeNumber);
      _refinementLevelPlotter->plotCellData(cellIndex, 1);
    }
  }
"""
            ).render(**self.d)

        return result

    def get_attributes(self):
        return (
            super(ParticlePlotter, self).get_attributes()
            + f"""
  std::unique_ptr<tarch::plotter::FileWriter::CellPlotter> _cellPlotter;
  std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter> _vertexPlotter;
"""
        )
