# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet
import jinja2


class InsertParticlesByCoordinatesAndData(ActionSet):
    """
    Insert particles into the domain based on provided initial coordinates and optional per-particle data.

    This action set reads a flat array containing particle positions and additional properties
    and inserts each particle into the correct cell at the beginning of the simulation.

    The design ensures that each particle is inserted exactly once and only into a leaf cell,
    avoiding redundant or misplaced insertion. The insertion is thread-safe using a semaphore lock.

    Each particle is associated with:
    - A spatial coordinate (x, y, [z] depending on dimension)
    - Optional data attributes (e.g., velocity, mass, temperature)

    The initial data must be structured as a flat array, where **each particle** occupies
    (Dimensions + Number of Unknowns) consecutive entries.

    Example for a 2D setup with 3 unknowns per particle:
    ```
    {x0, y0, u0, v0, w0, x1, y1, u1, v1, w1, ...}
    ```

    Recommendation:
    - This method is ideal when the **number of initial particles is relatively small**.
    - It is particularly useful when **each particle requires different initial values**.
    - For large datasets or uniform initialization, prefer file-based loading or programmatic generation.
    """

    def __init__(
        self, particle_set, initial_data, unknowns_per_particle, additional_includes=""
    ):
        super(InsertParticlesByCoordinatesAndData, self).__init__(
            descend_invocation_order=1
        )

        self.d = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "UNKNOWNS_PER_PARTICLE": unknowns_per_particle,
            "INITIAL_DATA": jinja2.Template(initial_data).render(),
            "ADDITIONAL_INCLUDES": additional_includes
            + """
#include "Constants.h"
""",
        }

    __Template_BeginTraversal = jinja2.Template(
        """
_particleData = {{INITIAL_DATA}};
_particleAlreadyInserted = std::vector<bool>(_particleData.size() / ({{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS), false);
"""
    )

    __Template_TouchCellFirstTime = jinja2.Template(
        """
if (not marker.willBeRefined()) {
    tarch::multicore::Lock lock(_semaphore);
    int numberOfParticles = _particleData.size() / ({{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS);

    for (int i = 0; i < numberOfParticles; i++) {
        if (_particleAlreadyInserted[i])
            continue;

        tarch::la::Vector<DIMENSIONS, double> pos;
        for (int d = 0; d < DIMENSIONS; d++) {
            pos(d) = _particleData[i * ({{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS) + d];
        }

        if (
            marker.isContained(pos)
            &&
            pos(0) > marker.getX()(0) - marker.getH()(0) / 2.0
            &&
            pos(1) > marker.getX()(1) - marker.getH()(1) / 2.0
        ) {
            std::vector<double> dataPerParticle;
            for (int j = DIMENSIONS; j < {{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS; j++) {
                dataPerParticle.push_back(_particleData[i * ({{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS) + j]);
            }

            globaldata::{{PARTICLE}}* newParticle = new globaldata::{{PARTICLE}}();
            toolbox::particles::initWithData(*newParticle, pos, 0.0, dataPerParticle);

            // Safely get and update the per-spacetree particle ID
            newParticle->setNumber(0, _spacetreeId);
            newParticle->setNumber(1, _particleNumberOnThisTree);

            _particleNumberOnThisTree++;

            toolbox::particles::insertParticleIntoCell(
                marker,
                newParticle,
                fineGridVertices{{PARTICLES_CONTAINER}},
                _spacetreeId
            );

            _particleAlreadyInserted[i] = true;
        }
    }
}
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
_particleData.clear();
_particleAlreadyInserted.clear();
"""
    )

    def get_body_of_operation(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return self.__Template_BeginTraversal.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self.__Template_TouchCellFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self.__Template_EndTraversal.render(**self.d)
        return ""

    def get_body_of_getGridControlEvents(self):
        return "return std::vector< peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/particles.h"
#include "toolbox/particles/ParticleFactory.h"
"""
        ).render(**self.d)

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
tarch::multicore::BooleanSemaphore {full_qualified_classname}::_semaphore;
"""

    def get_constructor_body(self):
        return """
_particleNumberOnThisTree = 0;
_spacetreeId = treeNumber;
tarch::multicore::Lock lock(_semaphore);
"""

    def get_attributes(self):
        return """
static tarch::multicore::BooleanSemaphore _semaphore;
int _particleNumberOnThisTree;
int _spacetreeId;
std::vector<double> _particleData;
std::vector<bool> _particleAlreadyInserted;
"""
