# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.actionsets.action_set import ActionSet


class InsertParticlesByCoordinatesAndData(ActionSet):
    """
    Every particle occupies DIMENSIONS + unknowns_per_particle consecutive
    entries of initial_data, the coordinates first, and enters exactly one leaf
    cell.
    """

    def __init__(self, particle_set, initial_data, unknowns_per_particle):
        super().__init__(descend_invocation_order=1)

        self.d = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "UNKNOWNS_PER_PARTICLE": unknowns_per_particle,
            "INITIAL_DATA": jinja2.Template(initial_data).render(),
        }

    _TEMPLATE_BEGIN_TRAVERSAL = jinja2.Template("""
  _particleData            = {{INITIAL_DATA}};
  _particleAlreadyInserted = std::vector<bool>(_particleData.size() / ({{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS), false);
""")

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = jinja2.Template("""
  if (not marker.willBeRefined()) {
    // The particle data and the flag per particle belong to the rank, so two trees that insert
    // at the same time would create one particle twice.
    const std::scoped_lock lock(_mutex);

    ::exahype2::particles::insertParticlesByCoordinatesAndData<globaldata::{{PARTICLE}}, {{UNKNOWNS_PER_PARTICLE}}>(
      marker,
      _particleData,
      _particleAlreadyInserted,
      DomainOffset,
      fineGridVertices{{PARTICLES_CONTAINER}},
      _treeId,
      _particleNumberOnThisTree
    );
  }
""")

    _TEMPLATE_END_TRAVERSAL = jinja2.Template("""
  _particleData.clear();
  _particleAlreadyInserted.clear();
""")

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return self._TEMPLATE_BEGIN_TRAVERSAL.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self._TEMPLATE_TOUCH_CELL_FIRST_TIME.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self._TEMPLATE_END_TRAVERSAL.render(**self.d)
        return ""

    def includes(self):
        return [
            "Constants.h",
            "exahype2/particles/InsertParticlesByCoordinatesAndData.h",
            "globaldata/{}.h".format(self.d["PARTICLE"]),
            "vertexdata/{}.h".format(self.d["PARTICLES_CONTAINER"]),
        ]

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::mutex {full_qualified_classname}::_mutex;
"""

    def get_constructor_body(self):
        return """
  _treeId = treeId;
"""

    def attributes(self):
        return """
  static std::mutex   _mutex;
  int                 _particleNumberOnThisTree{0};
  int                 _treeId{-1};
  std::vector<double> _particleData{};
  std::vector<bool>   _particleAlreadyInserted{};
"""
