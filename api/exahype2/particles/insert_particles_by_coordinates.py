# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

import jinja2

from exahype2.actionsets.action_set import ActionSet


class InsertParticlesByCoordinates(ActionSet):
    """Inserts particles at fixed coordinates into unrefined cells.

    The ``peano4::particles::init`` function sets the position and a zero search
    radius before initialisation_call runs with ``particle`` in scope. A caller
    runs this action only in an initialisation traversal; each later traversal
    inserts another copy of every particle.
    """

    def __init__(self, particle_set, coordinates, initialisation_call):
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
            "INITIALISATION_CALL": textwrap.dedent(initialisation_call).strip(),
        }

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = jinja2.Template("""
  ::exahype2::particles::insertParticlesByCoordinates<globaldata::{{PARTICLE}}>(
    marker,
    _coordinates,
    DomainOffset,
    fineGridVertices{{PARTICLES_CONTAINER}},
    _treeId,
    [&]([[maybe_unused]] auto* const particle) {
      {{INITIALISATION_CALL | indent(6)}}
    }
  );
""")

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self._TEMPLATE_TOUCH_CELL_FIRST_TIME.render(**self._dictionary)
        return ""

    def includes(self):
        return [
            "Constants.h",
            "exahype2/particles/InsertParticlesByCoordinates.h",
            "globaldata/{}.h".format(self._dictionary["PARTICLE"]),
            "vertexdata/{}.h".format(self._dictionary["PARTICLES_CONTAINER"]),
        ]

    def get_constructor_body(self):
        return jinja2.Template("""
  _treeId = treeId;
{%- if COORDINATE_DIMENSIONS == 2 %}
  static_assert(DIMENSIONS == 2, "Two-component coordinates require a two-dimensional build.");
{%- endif %}
""").render(**self._dictionary)

    def get_static_initialisations(self, full_qualified_classname):
        return jinja2.Template("""
#if DIMENSIONS == 2
const std::array<tarch::la::Vector<DIMENSIONS, double>, {{NUMBER_OF_PARTICLES}}>
  {{FULL_QUALIFIED_CLASSNAME}}::_coordinates{
{%- for coordinate in COORDINATES %}
    tarch::la::Vector<DIMENSIONS, double>{
      {{coordinate[0]}},
      {{coordinate[1]}}
    }{{"," if not loop.last}}
{%- endfor %}
  };
#else
const std::array<tarch::la::Vector<DIMENSIONS, double>, {{NUMBER_OF_PARTICLES}}>
  {{FULL_QUALIFIED_CLASSNAME}}::_coordinates{
{%- for coordinate in COORDINATES %}
    tarch::la::Vector<DIMENSIONS, double>{
      {{coordinate[0]}},
      {{coordinate[1]}},
      {{coordinate[2] if coordinate | length == 3 else "0.0"}}
    }{{"," if not loop.last}}
{%- endfor %}
  };
#endif
""").render(
            FULL_QUALIFIED_CLASSNAME=full_qualified_classname,
            **self._dictionary,
        )

    def attributes(self):
        return jinja2.Template("""
  static const std::array<tarch::la::Vector<DIMENSIONS, double>, {{NUMBER_OF_PARTICLES}}> _coordinates;

  int _treeId{-1};
""").render(**self._dictionary)
