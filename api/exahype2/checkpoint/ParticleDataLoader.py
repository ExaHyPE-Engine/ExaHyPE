# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.solversteps.ActionSet import ActionSet


class ParticleDataLoader(ActionSet):
    """!

    Fill one particle set of a resumed run from the checkpoint.

    The action set reads every particle row of the checkpoint file once per
    rank and inserts each particle whose position falls into a local
    unrefined cell, with its stored numbers, move state and data values. It
    replaces the initialisation-time particle insertion, which stays off
    while a checkpoint load path is set.
    """

    def __init__(self, particle_set):
        super().__init__(descend_invocation_order=1)
        self.d = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
        }

    def get_action_set_name(self):
        return (
            __name__.replace(".py", "").replace(".", "_")
            + "_"
            + self.d["PARTICLES_CONTAINER"]
        )

    def user_should_modify_template(self):
        return False

    def get_body_of_prepareTraversal(self):
        return jinja2.Template(
            """
  if (::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath()) {
    const tarch::reader::HDF5FileReader reader(::runner::RuntimeParameters::getInstance().getCheckpointLoadPath());
    const std::string particlePath = "/particles/{{PARTICLES_CONTAINER}}/particles";
    _particleRows = reader.readDoubleRows(particlePath, 0, reader.getDatasetDimensions(particlePath).first);
  }
"""
        ).render(**self.d)

    def get_body_of_unprepareTraversal(self):
        return """
  _particleRows.clear();
  _particleRows.shrink_to_fit();
"""

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = jinja2.Template(
                """
  if (::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath() and not marker.willBeRefined()) {
    const int valuesPerParticle = 3 + DIMENSIONS
                                  + static_cast<int>(globaldata::{{PARTICLE}}().getData().size());
    for (std::size_t row = 0; row < _particleRows.size() / valuesPerParticle; row++) {
      const double* const record = _particleRows.data() + row * valuesPerParticle;
      tarch::la::Vector<DIMENSIONS, double> position;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        position(axis) = record[3 + axis];
      }
      if (marker.isContained(position)) {
        auto* const particle = new globaldata::{{PARTICLE}}();
        peano4::particles::init(*particle, position, 0.0);
        particle->setNumber(0, static_cast<int>(record[0]));
        particle->setNumber(1, static_cast<int>(record[1]));
        particle->setMoveState(static_cast<globaldata::{{PARTICLE}}::MoveState>(static_cast<int>(record[2])));
        for (std::size_t value = 0; value < particle->getData().size(); value++) {
          particle->setData(static_cast<int>(value), record[3 + DIMENSIONS + value]);
        }
        peano4::particles::insertParticleIntoCell(marker, particle, fineGridVertices{{PARTICLES_CONTAINER}}, _treeId);
      }
    }
  }
"""
            ).render(**self.d)
        return result

    def get_constructor_body(self):
        return """
  _treeId = treeNumber;
"""

    def get_attributes(self):
        return """
  int _treeId{-1};

  static std::vector<double> _particleRows;
"""

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
std::vector<double> {full_qualified_classname}::_particleRows;
"""

    def get_includes(self):
        return jinja2.Template(
            """
#include "globaldata/{{PARTICLE}}.h"
#include "peano4/particles/ParticleFactory.h"
#include "peano4/particles/particles.h"
#include "runner/RuntimeParameters.h"
#include "tarch/reader/HDF5FileReader.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
"""
        ).render(**self.d)
