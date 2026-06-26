// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{MAIN_NAME}}.h"

#include "Constants.h"
#include "RuntimeParameters.h"
#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"

{% for step in MAIN_STEPS -%}
#include "observers/{{step.name}}.h"
{% endfor %}

#include "runner/Runner.h"
#include "runner/RuntimeParameters.h"

{{EXTRA_INCLUDES}}

#include "peano4/parallel/SpacetreeSet.h"

#include "tarch/instrumentation/Log.h"

#include <string>
#include <vector>

using namespace {{NAMESPACE | join("::")}};

namespace {
  std::vector<::runner::Step> createSteps() {
    return {
{% for step in MAIN_STEPS %}
      {
        repositories::StepRepository::toProgramStep(repositories::StepRepository::Steps::{{step.name}}),
        "{{step.name}}",
        []() {
          {{step.start_hook}}observers::{{step.name}}::prepareTraversal();
          observers::{{step.name}} observer;
          ::peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
          observers::{{step.name}}::unprepareTraversal();
          {{step.finish_hook}}
        }
      }{{ "," if not loop.last else "" }}
{% endfor %}
    };
  }

  ::runner::RepositoryCallbacks createRepositoryCallbacks() {
    ::runner::RepositoryCallbacks result;

    // The runtime updates the load balancer itself after every step; the project
    // only has to refresh its own memory statistics.
    result.afterStep = []() { repositories::DataRepository::updateMemoryUsage(); };

    result.initDataRepository     = []() { repositories::DataRepository::init(); };
    result.shutdownDataRepository = []() { repositories::DataRepository::shutdown(); };
    result.getGlobalCellDataMemoryUsageInBytes   = []() { return repositories::DataRepository::getGlobalCellDataMemoryUsageInBytes(); };
    result.getGlobalFaceDataMemoryUsageInBytes   = []() { return repositories::DataRepository::getGlobalFaceDataMemoryUsageInBytes(); };
    result.getGlobalVertexDataMemoryUsageInBytes = []() { return repositories::DataRepository::getGlobalVertexDataMemoryUsageInBytes(); };
    result.getConfigurationInformation           = []() { return std::string(ConfigurationInformation); };

    result.printApplicationHelp = []() { RuntimeParameters::printUsage(); };
    result.loadApplicationParameters = [](const std::string& configFilePath, int& argc, char** argv) {
      RuntimeParameters::load(configFilePath, argc, argv);
    };
{{EXTRA_REPOSITORY_CALLBACKS}}
    return result;
  }
} // namespace

int main(int argc, char** argv) {
  ::runner::RuntimeParameters::getInstance().setGeneratedDefaults(
    {{DEFAULT_MIN_TERMINAL_TIME}},
    {{DEFAULT_MAX_TERMINAL_TIME}},
    {{DEFAULT_GRID_LEVELS}},
    {{DEFAULT_MAX_GRID_LEVELS}}
  );

  tarch::instrumentation::Log::setOutputFile(std::string({{EXECUTABLE_NAME}}) + ".log");

  return ::runner::run(
    DomainOffset,
    DomainSize,
    PeriodicBC,
    createRepositoryCallbacks(),
    createSteps(),
    argc,
    argv
  );
}
