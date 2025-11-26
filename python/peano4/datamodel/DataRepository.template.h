// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/TreeToStacks.h"

{{DATA_CONTAINER_INCLUDES}}

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

class {{FULL_QUALIFIED_CLASSNAME}} {
private:
  static std::uint64_t _localMemoryUsageInBytes;
  static std::uint64_t _globalMemoryUsageInBytes;

public:
  using DataKey = ::peano4::stacks::StackKey; // TODO: Remove and use the stacks::StackKey

  {{DATA_CONTAINER_DECLARATION}}

  static void init();
  static void shutdown();

  static void updateMemoryUsage();
  static std::size_t getLocalMemoryUsageInBytes();
  static std::size_t getGlobalMemoryUsageInBytes();
};
