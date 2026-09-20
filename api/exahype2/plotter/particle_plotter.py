# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet

from .base_plotter import BasePlotter
from .base_plotter import data_field_list


class ParticlePlotter(BasePlotter):
    """! Every local particle of a vertex becomes one plot point, and each
    attribute becomes one field of the plot.
    """

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
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["ATTRIBUTES"] = [] if attributes is None else list(attributes)
        self.d["PLOT_FIELDS"] = data_field_list(
            [(name, components) for name, components, _ in self.d["ATTRIBUTES"]]
        )

    def includes(self):
        return super().includes() + [
            "Constants.h",
            "exahype2/plotter/ParticlePlotter.h",
        ]

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return self.render("""
  _plotter.open(_snapshot, _treeId, {{PLOT_FIELDS}});
""") + super().operation_body(operation_name)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return self.render("""
  _plotter.plotParticles(
    marker,
    fineGridVertex{{PARTICLES_CONTAINER}},
    DomainOffset,
    DomainSize,
    PeriodicBoundaries,
    [&]([[maybe_unused]] const int cellIndex, [[maybe_unused]] const auto* const p) {
{% for name, components, expression in ATTRIBUTES %}
      _plotter.getAttributePlotter({{loop.index0}}).plotCellData(cellIndex, {{expression}});
{% endfor %}
    }
  );
""")
        return "\n"

    def attributes(self):
        return super().attributes() + """
  ::exahype2::plotter::ParticlePlotter _plotter{};
"""
