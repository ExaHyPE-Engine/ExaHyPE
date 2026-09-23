# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.actionsets.action_set import ActionSet


class InsertParticlesFromFile(ActionSet):
    """Inserts one tracer particle per sample point of a coordinate file.

    Each cell asks the reader for the points inside it, and the reader hands
    every point out once, so a later traversal inserts no particle again.
    """

    def __init__(
        self,
        particle_set,
        filename,
        scale_factor=1.0,
        save_initialpos=False,
        initial_position_index=(1, 2, 3),
    ):
        """
        initial_position_index: tuple
            The three particle data indices that receive the x, y and z component
            of the initial position. A two-dimensional build uses the first two.
        """
        if len(initial_position_index) not in (2, 3):
            raise ValueError("The initial position needs one data index per axis.")

        self.d = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "STORE_INITIAL_POSITION": "true" if save_initialpos else "false",
            "INITIAL_POSITION_INDEX": ", ".join(
                str(index) for index in initial_position_index
            ),
            "INITIAL_POSITION_ARGUMENT": (
                "InitialPositionIndex" if save_initialpos else "nullptr"
            ),
            "SAVE_INITIALPOS": save_initialpos,
        }
        self._filename = filename
        self._scale = scale_factor

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = jinja2.Template("""
  if (not marker.willBeRefined()) {
{%- if SAVE_INITIALPOS %}
    static constexpr int InitialPositionIndex[] = { {{INITIAL_POSITION_INDEX}} };
    static_assert(sizeof(InitialPositionIndex) / sizeof(int) >= DIMENSIONS);
{%- endif %}
    // One file reader serves every tree of the rank and hands out each particle once, so two
    // trees that insert at the same time would read one particle twice.
    const std::scoped_lock lock(_mutex);

    ::exahype2::particles::insertParticlesFromFile<globaldata::{{PARTICLE}}, {{STORE_INITIAL_POSITION}}>(
      marker,
      _fileReader,
      fineGridVertices{{PARTICLES_CONTAINER}},
      _treeId,
      _particleNumberOnThisTree,
      {{INITIAL_POSITION_ARGUMENT}}
    );
  }
""")

    _TEMPLATE_END_TRAVERSAL = jinja2.Template("""
  if (_particleNumberOnThisTree > 0) {
    logInfo(
      "Inserted " << _particleNumberOnThisTree << " particle(s) on tree " << _treeId
                  << " (boundary vertices might host redundant particle data)"
    );
  }
""")

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self._TEMPLATE_TOUCH_CELL_FIRST_TIME.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self._TEMPLATE_END_TRAVERSAL.render(**self.d)
        return "\n"

    def includes(self):
        return [
            "exahype2/particles/InsertParticlesFromFile.h",
            "globaldata/{}.h".format(self.d["PARTICLE"]),
            "peano4/particles/FileReader.h",
            "tarch/instrumentation/Info.h",
            "vertexdata/{}.h".format(self.d["PARTICLES_CONTAINER"]),
        ]

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::mutex                    {full_qualified_classname}::_mutex;
peano4::particles::FileReader {full_qualified_classname}::_fileReader;
"""

    def get_constructor_body(self):
        return f"""
  _particleNumberOnThisTree = 0;
  _treeId                   = treeId;

  // One file reader serves every tree of the rank, so the first tree to arrive reads the file.
  const std::scoped_lock lock(_mutex);
  if (_fileReader.empty()) {{
    _fileReader.readDatFile("{self._filename}", {self._scale});
  }}
"""

    def attributes(self):
        return """
  static std::mutex                    _mutex;
  int                                  _particleNumberOnThisTree{0};
  int                                  _treeId{-1};
  static peano4::particles::FileReader _fileReader;
"""
