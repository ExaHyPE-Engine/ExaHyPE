// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGMatrices.h"

#include "exahype2/dg/AMRMatrices.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  constexpr int NodesPerAxis               = {{DG_ORDER}} + 1;
  constexpr int EntriesPerTransferOperator = peano4::sfc::ActiveCurve::SubdivisionFactor * NodesPerAxis * NodesPerAxis;

  constexpr double QuadratureNodes1D[NodesPerAxis]   = {{QUADRATURE_NODES_1D}};
  constexpr double QuadratureWeights1D[NodesPerAxis] = {{QUADRATURE_WEIGHTS_1D}};
{% for matrix in DG_MATRICES if matrix.expression is defined %}
  constexpr auto {{matrix.name}}Table = {{matrix.expression}};
{%- endfor %}
} // namespace

void {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices::initDGMatrices() {
{%- for matrix in DG_MATRICES %}
{%- set length = matrix.entries|length if matrix.entries is defined else "EntriesPerTransferOperator" %}
  {{matrix.name}} = tarch::allocateMemory<double>({{length}}, tarch::MemoryLocation::Heap);
{%- endfor %}
{% for matrix in DG_MATRICES %}
{%- if matrix.entries is defined %}
{%- for value in matrix.entries %}
  {{matrix.name}}[{{loop.index0}}] = {{value}};
{%- endfor %}
{%- else %}
  for (int entry = 0; entry < EntriesPerTransferOperator; entry++) {
    {{matrix.name}}[entry] = {{matrix.name}}Table[entry];
  }
{%- endif %}
{% endfor -%}
}

void {{FULL_QUALIFIED_NAMESPACE}}::DGMatrices::freeDGMatrices() {
{%- for matrix in DG_MATRICES %}
  tarch::freeMemory({{matrix.name}}, tarch::MemoryLocation::Heap);
{%- endfor %}
}
