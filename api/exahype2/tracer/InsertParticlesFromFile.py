# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet
import jinja2


class InsertParticlesFromFile(ActionSet):
    def __init__(
        self,
        particle_set,
        filename,
        scale_factor=1.0,
        save_initialpos=False,
        initial_position_index=(1, 2, 3),
    ):
        """
        particle_set: ParticleSet
            The tracer particles which are inserted

        filename: String
            Name of the sample point file

        scale_factor: float
            Scaling factor for all coordinates from the sample file

        save_initialpos: bool
            Whether to store initial positions into particle->setData

        initial_position_index: tuple
            Indices for particle->setData corresponding to x,y[,z]
        """
        self.d = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "SAVE_INITIALPOS": save_initialpos,
            "INITIAL_POSITION_INDEX": initial_position_index,
        }
        self._filename = filename
        self._scale = scale_factor

    __Template_TouchCellFirstTime = jinja2.Template(
        """
if (not marker.willBeRefined()) {
    const std::scoped_lock lock(_mutex);

    const std::list< tarch::la::Vector<DIMENSIONS,double> > coordinates = _fileReader.getParticlesWithinVoxel(marker.getX(), marker.getH(), true);

    for (const auto& position : coordinates) {
        auto* const newParticle = new globaldata::{{PARTICLE}}();

        newParticle->setNumber(0, _treeId);
        newParticle->setNumber(1, _particleNumberOnThisTree);
        _particleNumberOnThisTree++;

        toolbox::particles::init(*newParticle, position, 0.0);

{% if SAVE_INITIALPOS %}
        newParticle->setData({{ INITIAL_POSITION_INDEX[0] }}, position(0));
        newParticle->setData({{ INITIAL_POSITION_INDEX[1] }}, position(1));
#if DIMENSIONS==3
        newParticle->setData({{ INITIAL_POSITION_INDEX[2] }}, position(2));
#endif
{% endif %}

        toolbox::particles::insertParticleIntoCell(
            marker,
            newParticle,
            fineGridVertices{{PARTICLES_CONTAINER}},
            _treeId
        );
    }
}
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  if (_particleNumberOnThisTree > 0) {
    logInfo("Inserted " << _particleNumberOnThisTree << " particle(s) on tree " << _treeId << " (boundary vertices might host redundant particle data)");
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self.__Template_TouchCellFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self.__Template_EndTraversal.render(**self.d)
        return "\n"

    def get_body_of_getGridControlEvents(self):
        return "return std::vector< peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return jinja2.Template(
            """
#include "tarch/instrumentation/Info.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/FileReader.h"
#include "toolbox/particles/particles.h"
#include "toolbox/particles/ParticleFactory.h"
#include "exahype2/fv/particles/ParticleHelper.h"
"""
        ).render(**self.d)

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::mutex {full_qualified_classname}::_mutex;
toolbox::particles::FileReader {full_qualified_classname}::_fileReader;
"""

    def get_constructor_body(self):
        return f"""
_particleNumberOnThisTree = 0;
_treeId = treeNumber;

const std::scoped_lock lock(_mutex);
if (_fileReader.empty()) {{
    _fileReader.readDatFile("{self._filename}", {self._scale});
}}
"""

    def get_attributes(self):
        return """
static std::mutex _mutex;
int _particleNumberOnThisTree{0};
int _treeId{-1};
static toolbox::particles::FileReader _fileReader;
"""
