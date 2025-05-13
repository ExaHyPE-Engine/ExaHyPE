#pragma once

// Template in python/swift2/output/SWIFTMain.template.h

#include <string>


/**
 * Main routine of the SPH code
 */
int main(int argc, char** argv);


{% for item in NAMESPACE -%}
  namespace {{ item }} {
{%- endfor %}
  /**
   * Only executed on global master, i.e. rank 0.
   */
  bool selectNextAlgorithmicStep();

  /**
   * Run one global step on rank. Called on each and every rank.
   */
  void step();
{% for item in NAMESPACE -%}
  }
{%- endfor %}

