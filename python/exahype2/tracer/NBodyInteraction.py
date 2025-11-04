# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2


class NBodyInteraction(ActionSet):
    def __init__(
        self,
        particle_set,
        solver,
        project,
        update_kernel_arguments="""
*p,
marker,
fineGridCell{{SOLVER_NAME}}Q.value
""",
        interpolation_scheme=0,
        integration_scheme=0,
        gravity_model=0,
        fields_to_interpolate="0,3,4",
        mu="10.0",
    ):
        self.tracerDict = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "SOLVER_NAME": solver._name,
            "SOLVER_INSTANCE": solver.get_name_of_global_instance(),
            "PATCH_SIZE": solver._patch_size,
            "NUMBER_OF_UNKNOWNS": solver._unknowns,
            "NUMBER_OF_AUXILIARY_VARIABLES": solver._auxiliary_variables,
            "INTERPOLATION_SCHEME": interpolation_scheme,
            "INTEGRATION_SCHEME": integration_scheme,
            "GRAVITY_MODEL": gravity_model,
            "UPDATE_PARTICLE_KERNEL": "::exahype2::fv::particles::updateParticleState",
            "DOMAIN_SIZE": "{"
            + ",".join(map(str, project._domain_size[: project._dimensions]))
            + "}",
            "DOMAIN_OFFSET": "{"
            + ",".join(map(str, project._domain_offset[: project._dimensions]))
            + "}",
            "PERIODIC_BC": "{"
            + ",".join(
                ["1" if b else "0" for b in project._periodic_BC[: project._dimensions]]
            )
            + "}",
            "FIELDS_TO_INTERPOLATE": fields_to_interpolate,
            "MU": mu,
        }
        self.tracerDict["UPDATE_KERNEL_ARGUMENTS"] = jinja2.Template(
            update_kernel_arguments
        ).render(**self.tracerDict)

        # Fix: Add self.d as an alias to self.tracerDict for compatibility
        self.d = self.tracerDict

    __Template_PrepareTraversal = jinja2.Template(
        """
  {{PREPARE_TRAVERSAL_KERNEL}}
"""
    )

    __Template_UnprepareTraversal = jinja2.Template(
        """
  {{UNPREPARE_TRAVERSAL_KERNEL}}
"""
    )

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  _timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
  
  std::vector< globaldata::{{PARTICLE}}* > localParticles;
  std::vector< int > numberOfCoalescedLocalParticlesPerVertex( TWO_POWER_D );
  
  _numberOfActiveParticlesAdded.push_back(0);
  for (int i=0; i<TWO_POWER_D; i++) {
    // Keep track of number of particles
    _numberOfActiveParticlesPerVertex.push_back( fineGridVertices{{PARTICLES_CONTAINER}}(i).size() );
    _numberOfActiveParticlesAdded.back() += fineGridVertices{{PARTICLES_CONTAINER}}(i).size();
    numberOfCoalescedLocalParticlesPerVertex[i] = fineGridVertices{{PARTICLES_CONTAINER}}(i).size();
    
    // Add particles to active particle set, checking for duplicates
    for (auto* p: fineGridVertices{{PARTICLES_CONTAINER}}(i)) {
      // Check if this particle is already in the active particles list
      bool isDuplicate = false;
      for (auto* existingP: _activeParticles) {
        if (p->getNumber(0) == existingP->getNumber(0) && p->getNumber(1) == existingP->getNumber(1)) {
          isDuplicate = true;
          break;
        }
      }
      
      if (!isDuplicate && marker.isContained(p->getX())) {
        _activeParticles.push_back(p);
      }
    }
    
    // Construct the local particle sets
    for (auto* p: fineGridVertices{{PARTICLES_CONTAINER}}(i)) {
      localParticles.push_back(p);
    }
  }
  
  logDebug("touchCellFirstTime(...)", "size of local/active particles=" << localParticles.size() << "/" << _activeParticles.size() << " at " << marker.toString());

  std::vector< globaldata::{{PARTICLE}}* >& activeParticles = _activeParticles;
  auto& numberOfCoalescedActiveParticlesPerVertex = _numberOfActiveParticlesPerVertex;
  assertion1(
    not numberOfCoalescedActiveParticlesPerVertex.empty()
    and
    (numberOfCoalescedActiveParticlesPerVertex.size() % TwoPowerD == 0),
    numberOfCoalescedActiveParticlesPerVertex.size()
  );
  {{PARTICLE_INTERACTION_KERNEL}}
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
    ::exahype2::fv::particles::integration::setGlobalIntegrationParameters(
    _timeStepSize,
    {{INTEGRATION_SCHEME}}
);
    // Update particle position and velocity using the time integration kernel
    ::exahype2::fv::particles::ppintegration::applyIntegration(
      _activeParticles
    );

  for (auto* p: _activeParticles) {
    p->setMoveState(globaldata::{{PARTICLE}}::MoveState::Moved);
  }
  
  // Clear the global active particles list at the end of traversal
  _activeParticles.clear();
  _numberOfActiveParticlesAdded.clear();
  _numberOfActiveParticlesPerVertex.clear();
  _processedParticlePairs.clear();
  

"""
    )

    __Template_TouchCellLastTime = jinja2.Template(
        """
  assertion(not _numberOfActiveParticlesAdded.empty());
  assertion(_activeParticles.size() >= _numberOfActiveParticlesAdded.back());
  assertion(_numberOfActiveParticlesPerVertex.size() >= TWO_POWER_D);
  
  // We don't clear the active particles list after each cell
  // as we want to maintain the global list for the entire traversal
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_TouchCellLastTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self.__Template_EndTraversal.render(**self.d)
        return result

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

    def get_body_of_prepareTraversal(self):
        return self.__Template_PrepareTraversal.render(**self.d)

    def get_body_of_unprepareTraversal(self):
        return self.__Template_UnprepareTraversal.render(**self.d)

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "toolbox/particles/particles.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"

#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"
#include "exahype2/fv/particles/ParticleIntegration.h"
#include "exahype2/fv/particles/MultiParticleIntegration.h"
#include <utility>  
#include <set>      
#include <list>
#include <vector>
#include <algorithm>
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        return """
  int  _spacetreeId;
  double _timeStepSize;
  std::vector< globaldata::{{PARTICLE}}* > _activeParticles;
  std::vector< int > _numberOfActiveParticlesPerVertex;
  std::vector< int > _numberOfActiveParticlesAdded;
  std::set<std::pair<std::pair<int,int>, std::pair<int,int>>> _processedParticlePairs;
""".replace(
            "{{PARTICLE}}", self.d["PARTICLE"]
        )

    def get_constructor_body(self):
        return """
  _spacetreeId = treeNumber;
"""
