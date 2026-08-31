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
        attributes=None,
    ):
        """Each tuple of attributes supplies a field name, a component count and a
        generated C++ expression that returns a scalar, a vector or a component
        pointer for particle ``p``.
        """
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
        self.d["ATTRIBUTES"] = [] if attributes is None else list(attributes)

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super().get_includes()
            + """
#include "peano4/topology/TopologyManager.h"
#include "tarch/la/ScalarOperations.h"
"""
        )

    def get_body_of_operation(self, operation_name):
        result = "\n"

        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = (
                super().get_body_of_operation(operation_name)
                + jinja2.Template(
                    """
  _cellPlotter.reset();
  _cellPlotter = _fileWriter->createCellPlotter(_pieceId);

  _vertexPlotter.reset();
  _vertexPlotter = _fileWriter->createVertexPlotter(_pieceId);
{% for name, components, expression in ATTRIBUTES %}
  _attributePlotter{{loop.index0}}.reset();
  _attributePlotter{{loop.index0}} = _fileWriter->createCellDataPlotter(_pieceId, "{{name}}", {{components}});
{% endfor %}
"""
                ).render(**self.d)
            )

        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = jinja2.Template(
                """
  assertion(_cellPlotter);
  assertion(_vertexPlotter);
  assertion(_subdomainPlotter);
  assertion(_subtreeTaskPlotter);
  assertion(_refinementLevelPlotter);
{% for name, components, expression in ATTRIBUTES %}
  assertion(_attributePlotter{{loop.index0}});
{% endfor %}

  // One sweep visits a vertex at a periodic wall once per wall. Only the visit at a
  // lower wall writes the vertex's particles, so every particle appears once.
  const auto periodicBC = peano4::topology::TopologyManager::getInstance().getPeriodicBC();
  if (periodicBC.any()) {
    const auto domainOffset = peano4::topology::TopologyManager::getInstance().getDomainOffset();
    const auto domainWidth  = peano4::topology::TopologyManager::getInstance().getDomainWidth();
    for (int d = 0; d < DIMENSIONS; d++) {
      if (periodicBC[d] and tarch::la::equals(marker.getX()(d), domainOffset(d) + domainWidth(d))) {
        return;
      }
    }
  }

  for (auto* const p: fineGridVertex{{PARTICLES_CONTAINER}}) {
    if (p->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local) {
      const int vertexIndex = _vertexPlotter->plotVertex(p->getX());
      const int cellIndex = _cellPlotter->plotPoint(vertexIndex);

      _subdomainPlotter->plotCellData(cellIndex, tarch::mpi::Rank::getInstance().getRank());
      _subtreeTaskPlotter->plotCellData(cellIndex, getSubtreeTask());
      _refinementLevelPlotter->plotCellData(cellIndex, 1);
{% for name, components, expression in ATTRIBUTES %}
      _attributePlotter{{loop.index0}}->plotCellData(cellIndex, {{expression}});
{% endfor %}
    }
  }
"""
            ).render(**self.d)

        return result

    def get_attributes(self):
        return (
            super().get_attributes()
            + """
  std::unique_ptr<tarch::plotter::FileWriter::CellPlotter> _cellPlotter;
  std::unique_ptr<tarch::plotter::FileWriter::VertexPlotter> _vertexPlotter;
"""
            + jinja2.Template(
                """
{% for name, components, expression in ATTRIBUTES %}
  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> _attributePlotter{{loop.index0}}{};
{% endfor %}
"""
            ).render(**self.d)
        )
