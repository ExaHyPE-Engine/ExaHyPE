# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.solversteps.ActionSet import ActionSet


class InsertParticlesByCoordinates(ActionSet):
    """Inserts particles at fixed coordinates into unrefined cells.

    Coordinate entries have two or three components and must use one dimension
    consistently. Three-component coordinates work in two- and three-dimensional
    builds; a two-component layout requires a two-dimensional build.

    The ``toolbox::particles::init`` function sets the position and a zero search
    radius before initialisation_call runs with ``particle`` in scope. A caller
    runs this action only in an initialisation traversal; each later traversal
    inserts another copy of every particle.
    """

    def __init__(
        self,
        particle_set,
        coordinates,
        initialisation_call,
        additional_includes="",
    ):
        super().__init__(descend_invocation_order=1)

        coordinate_dimensions = {len(coordinate) for coordinate in coordinates}
        if not coordinate_dimensions.issubset({2, 3}):
            raise ValueError(
                "Each particle coordinate requires two or three components."
            )
        if len(coordinate_dimensions) > 1:
            raise ValueError(
                "All particle coordinates require the same number of components."
            )

        self._dictionary = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "NUMBER_OF_PARTICLES": len(coordinates),
            "COORDINATES": coordinates,
            "COORDINATE_DIMENSIONS": next(iter(coordinate_dimensions), 0),
            "INITIALISATION_CALL": initialisation_call,
        }
        self._additional_includes = additional_includes

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  for (const auto& coordinate : _coordinates) {
    if (not marker.willBeRefined() and marker.isContained(coordinate)) {
      auto* const particle = new globaldata::{{PARTICLE}}();
      toolbox::particles::init(*particle, coordinate, 0.0);
      {{INITIALISATION_CALL}}
      toolbox::particles::insertParticleIntoCell(
        marker,
        particle,
        fineGridVertices{{PARTICLES_CONTAINER}},
        _spacetreeId
      );
    }
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self.__Template_TouchCellFirstTime.render(**self._dictionary)
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<peano4::grid::GridControlEvent>{};\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return (
            jinja2.Template(
                """
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/ParticleFactory.h"
#include "toolbox/particles/particles.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
"""
            ).render(**self._dictionary)
            + self._additional_includes
        )

    def get_constructor_body(self):
        return jinja2.Template(
            """
  _spacetreeId = treeNumber;
{% if COORDINATE_DIMENSIONS == 2 %}
  static_assert(DIMENSIONS == 2, "Two-component coordinates require a two-dimensional build.");
{% endif %}
"""
        ).render(**self._dictionary)

    def get_static_initialisations(self, full_qualified_classname):
        return jinja2.Template(
            """
#if DIMENSIONS == 2
const std::array<tarch::la::Vector<DIMENSIONS, double>, {{NUMBER_OF_PARTICLES}}>
  {{FULL_QUALIFIED_CLASSNAME}}::_coordinates{
{% for coordinate in COORDINATES %}
    tarch::la::Vector<DIMENSIONS, double>{
      {{coordinate[0]}},
      {{coordinate[1]}}
    }{% if not loop.last %},{% endif %}
{% endfor %}
  };
#else
const std::array<tarch::la::Vector<DIMENSIONS, double>, {{NUMBER_OF_PARTICLES}}>
  {{FULL_QUALIFIED_CLASSNAME}}::_coordinates{
{% for coordinate in COORDINATES %}
    tarch::la::Vector<DIMENSIONS, double>{
      {{coordinate[0]}},
      {{coordinate[1]}},
      {% if coordinate|length == 3 %}{{coordinate[2]}}{% else %}0.0{% endif %}
    }{% if not loop.last %},{% endif %}
{% endfor %}
  };
#endif
"""
        ).render(
            FULL_QUALIFIED_CLASSNAME=full_qualified_classname,
            **self._dictionary,
        )

    def get_attributes(self):
        return jinja2.Template(
            """
  static const std::array<
    tarch::la::Vector<DIMENSIONS, double>,
    {{NUMBER_OF_PARTICLES}}
  > _coordinates;
  int _spacetreeId;
"""
        ).render(**self._dictionary)
