// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
{% set project_namespace = FULL_QUALIFIED_NAMESPACE -%}
{% set generated_default_arguments = [
  DEFAULT_MIN_TERMINAL_TIME,
  DEFAULT_MAX_TERMINAL_TIME,
  DEFAULT_GRID_LEVELS,
  DEFAULT_MAX_GRID_LEVELS,
  '"' ~ PROJECT_NAME ~ '"'
] -%}

#include "{{ MAIN_NAME }}.h"

#include "Constants.h"
#include "RuntimeParameters.h"

{% for step in MAIN_STEPS | sort(attribute="name") -%}
#include "observers/{{ step.name }}.h"
{% endfor -%}
#include "peano4/parallel/SpacetreeSet.h"
#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"
#include "runner/Engine.h"
#include "runner/Launch.h"
#include "runner/RuntimeParameters.h"
{% if ADDITIONAL_INCLUDES -%}
// Application headers can depend on the generated repositories and runner API.
{% for include in ADDITIONAL_INCLUDES -%}
{{ include }}
{% endfor -%}
{% endif %}

namespace {
  std::vector<::runner::Step> createSteps() {
    std::vector<::runner::Step> result;
    result.reserve({{ MAIN_STEPS | length }});
{% for step in MAIN_STEPS %}
    result.emplace_back(
      ::{{ project_namespace }}::repositories::StepRepository::toProgramStep(
        ::{{ project_namespace }}::repositories::StepRepository::Steps::{{ step.name }}
      ),
      "{{ step.name }}",
      []() {
{%- if step.start_hooks %}
{{ step.start_hooks | join("\n") | indent(8, true) }}
{%- endif %}
        ::{{ project_namespace }}::observers::{{ step.name }}::prepareTraversal();
        ::{{ project_namespace }}::observers::{{ step.name }} observer;
        ::peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        ::{{ project_namespace }}::observers::{{ step.name }}::unprepareTraversal();
{%- if step.finish_hooks %}
{{ step.finish_hooks | join("\n") | indent(8, true) }}
{%- endif %}
      }
    );
{% endfor %}
    return result;
  }

  ::runner::RepositoryCallbacks createRepositoryCallbacks() {
    ::runner::RepositoryCallbacks result;

    // The runner updates the load balancer before the application refreshes repository memory statistics.
    result.afterStep = ::{{ project_namespace }}::repositories::DataRepository::updateMemoryUsage;

    result.shutdownDataRepository = ::{{ project_namespace }}::repositories::DataRepository::shutdown;

    result.getGlobalCellDataMemoryUsageInBytes = []() {
      return ::{{ project_namespace }}::repositories::DataRepository::getGlobalCellDataMemoryUsageInBytes();
    };
    result.getGlobalFaceDataMemoryUsageInBytes = []() {
      return ::{{ project_namespace }}::repositories::DataRepository::getGlobalFaceDataMemoryUsageInBytes();
    };
    result.getGlobalVertexDataMemoryUsageInBytes = []() {
      return ::{{ project_namespace }}::repositories::DataRepository::getGlobalVertexDataMemoryUsageInBytes();
    };

    result.printApplicationHelp      = ::{{ project_namespace }}::RuntimeParameters::printUsage;
    result.loadApplicationParameters = [](::tarch::reader::JSONReader* const reader, int& argc, char** const argv) {
      ::{{ project_namespace }}::RuntimeParameters::load(reader, argc, argv);
    };
{% if ADDITIONAL_REPOSITORY_CALLBACKS %}
{{ ADDITIONAL_REPOSITORY_CALLBACKS | indent(2, true) }}
{% endif %}
    return result;
  }
} // namespace

int main(int argc, char** argv) {
  ::runner::RuntimeParameters::getInstance().setGeneratedDefaults({{ generated_default_arguments | join(", ") }});

  return ::runner::launch(
    ::{{ project_namespace }}::DomainOffset,
    ::{{ project_namespace }}::DomainSize,
    ::{{ project_namespace }}::PeriodicBC,
    createRepositoryCallbacks(),
    createSteps(),
    argc,
    argv
  );
}
