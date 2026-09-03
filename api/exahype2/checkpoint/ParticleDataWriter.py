# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.solversteps.ActionSet import ActionSet


class ParticleDataWriter(ActionSet):
    """!

    Write one particle set's records into the current checkpoint.

    The action set buffers one row per local particle, holding the particle's
    two numbers, its move state, its position and its data values, and hands
    the rows to peano4::checkpoint::ParticleDataWriter, which writes them into
    <checkpoint output path>/checkpoint-<n>/<particle set name>.h5 with one
    collective write. n counts the checkpoints this run has written.
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
  _writer.reset();
  _writer = std::make_unique<::peano4::checkpoint::ParticleDataWriter>(
    ::runner::RuntimeParameters::getInstance().getCheckpointOutputPath()
      + "/checkpoint-" + std::to_string(_checkpointCounter) + "/{{PARTICLES_CONTAINER}}.h5",
    3 + DIMENSIONS + static_cast<int>(globaldata::{{PARTICLE}}().getData().size())
  );
"""
        ).render(**self.d)

    def get_body_of_unprepareTraversal(self):
        return """
  _writer->write();
  _writer.reset();
  _checkpointCounter++;
"""

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = """
  _rows.clear();
"""
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = """
  _writer->collect(std::move(_rows));
  _rows.clear();
"""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = jinja2.Template(
                """
  if (not marker.willBeRefined()) {
    for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
      for (const auto* const particle : fineGridVertices{{PARTICLES_CONTAINER}}(vertexNumber)) {
        if (particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local
            and marker.isContained(particle->getX())) {
          _rows.push_back(static_cast<double>(particle->getNumber(0)));
          _rows.push_back(static_cast<double>(particle->getNumber(1)));
          _rows.push_back(static_cast<double>(static_cast<int>(particle->getMoveState())));
          const auto& position = particle->getX();
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            _rows.push_back(position(axis));
          }
          const auto& data = particle->getData();
          for (int value = 0; value < static_cast<int>(data.size()); value++) {
            _rows.push_back(data(value));
          }
        }
      }
    }
  }
"""
            ).render(**self.d)
        return result

    def get_attributes(self):
        return """
  std::vector<double> _rows;

  static int _checkpointCounter;
  static std::unique_ptr<::peano4::checkpoint::ParticleDataWriter> _writer;
"""

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
int {full_qualified_classname}::_checkpointCounter = 0;
std::unique_ptr<::peano4::checkpoint::ParticleDataWriter> {full_qualified_classname}::_writer = nullptr;
"""

    def get_includes(self):
        return jinja2.Template(
            """
#include "globaldata/{{PARTICLE}}.h"
#include "peano4/checkpoint/ParticleDataWriter.h"
#include "runner/RuntimeParameters.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
"""
        ).render(**self.d)
