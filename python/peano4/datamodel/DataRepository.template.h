// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/StackContainer.h"

#include "peano4/stacks/Stack.h"
#include "peano4/stacks/StackWithSerialisation.h"
#include "peano4/stacks/StackWithCustomSerialisation.h"

{{VERTEX_DATA_CONTAINER_INCLUDES}}
{{FACE_DATA_CONTAINER_INCLUDES}}
{{CELL_DATA_CONTAINER_INCLUDES}}

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

class {{FULL_QUALIFIED_CLASSNAME}} {
private:
  static std::uint64_t _localCellDataMemoryUsageInBytes;
  static std::uint64_t _globalCellDataMemoryUsageInBytes;
  static std::uint64_t _localFaceDataMemoryUsageInBytes;
  static std::uint64_t _globalFaceDataMemoryUsageInBytes;
  static std::uint64_t _localVertexDataMemoryUsageInBytes;
  static std::uint64_t _globalVertexDataMemoryUsageInBytes;

public:
{% for container in VERTEX_DATA_CONTAINERS -%}
  static ::peano4::grid::StackContainer<{{container.stack}}> {{container.name}}StackContainer;
  static ::peano4::grid::StackContainer<{{container.stack}}> {{container.name}}HangingStackContainer;
{% endfor -%}

{% for container in FACE_DATA_CONTAINERS -%}
  static ::peano4::grid::StackContainer<{{container.stack}}> {{container.name}}StackContainer;
  static ::peano4::grid::StackContainer<{{container.stack}}> {{container.name}}HangingStackContainer;
{% endfor -%}

{% for container in CELL_DATA_CONTAINERS -%}
  static ::peano4::grid::StackContainer<{{container.stack}}> {{container.name}}StackContainer;
{% endfor %}

  static void init();
  static void shutdown();

  static void updateMemoryUsage();
  static std::size_t getLocalCellDataMemoryUsageInBytes();
  static std::size_t getGlobalCellDataMemoryUsageInBytes();
  static std::size_t getLocalFaceDataMemoryUsageInBytes();
  static std::size_t getGlobalFaceDataMemoryUsageInBytes();
  static std::size_t getLocalVertexDataMemoryUsageInBytes();
  static std::size_t getGlobalVertexDataMemoryUsageInBytes();
};
