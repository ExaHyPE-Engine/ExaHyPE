# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet
import jinja2


class InsertParticlesByCoordinatesAndData(ActionSet):
    """
    Every particle occupies DIMENSIONS + unknowns_per_particle consecutive
    entries of initial_data, the coordinates first, and enters exactly one leaf
    cell.
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
    const std::scoped_lock lock(_mutex);
    const auto numberOfParticles = _particleData.size() / ({{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS);

    for (std::size_t particleIndex = 0; particleIndex < numberOfParticles; ++particleIndex) {
        if (_particleAlreadyInserted[particleIndex]) {
            continue;
        }

        tarch::la::Vector<DIMENSIONS, double> position;
        for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
            position(dimension) = _particleData[
                particleIndex * ({{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS) + dimension
            ];
        }

        // A coordinate on a cell face lies in both adjacent cells, so only the
        // cell whose lower face it exceeds inserts it. A lower face on the
        // domain wall has no neighbour and inserts as well.
        bool insertedByThisCell = marker.isContained(position);
        for (int dimension = 0; dimension < DIMENSIONS; ++dimension) {
            const double lowerFace = marker.getX()(dimension) - marker.getH()(dimension) / 2.0;
            insertedByThisCell = insertedByThisCell
                and (tarch::la::greater(position(dimension), lowerFace)
                     or not tarch::la::greater(lowerFace, DomainOffset(dimension)));
        }

        if (insertedByThisCell) {
            std::vector<double> particleData;
            particleData.reserve({{UNKNOWNS_PER_PARTICLE}});
            for (
                int attributeIndex = DIMENSIONS;
                attributeIndex < {{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS;
                ++attributeIndex
            ) {
                particleData.push_back(
                    _particleData[
                        particleIndex * ({{UNKNOWNS_PER_PARTICLE}} + DIMENSIONS) + attributeIndex
                    ]
                );
            }

            auto* const newParticle = new globaldata::{{PARTICLE}}();
            peano4::particles::initWithData(*newParticle, position, 0.0, particleData);

            newParticle->setNumber(0, _treeId);
            newParticle->setNumber(1, _particleNumberOnThisTree);

            ++_particleNumberOnThisTree;

            peano4::particles::insertParticleIntoCell(
                marker,
                newParticle,
                fineGridVertices{{PARTICLES_CONTAINER}},
                _treeId
            );

            _particleAlreadyInserted[particleIndex] = true;
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
        return "return std::vector<peano4::grid::GridControlEvent>();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return jinja2.Template(
            """
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "tarch/la/ScalarOperations.h"
#include "peano4/particles/ParticleFactory.h"
#include "peano4/particles/particles.h"
"""
        ).render(**self.d)

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::mutex {full_qualified_classname}::_mutex;
"""

    def get_constructor_body(self):
        return """
_treeId = treeNumber;
"""

    def get_attributes(self):
        return """
static std::mutex _mutex;
int _particleNumberOnThisTree{0};
int _treeId{-1};
std::vector<double> _particleData{};
std::vector<bool> _particleAlreadyInserted{};
"""
