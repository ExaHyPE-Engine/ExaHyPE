# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from .codegen.helper import KeepUndefined
from .codegen.helper import render_jinja2_template


class Layout:
    """How a family lays out the degrees of freedom of one cell.

    ``loop`` is a Jinja2 template of the library loop call with ``{{ACCESSOR}}`` for the data
    object and ``{{BODY}}`` for the lambda body, which reads ``value``, ``x``, ``h``, ``weight``
    and ``index``.
    """

    def __init__(
        self,
        dofs_per_axis,
        representation,
        loop,
        solutions,
        includes,
        tracer_include,
        tracer_arguments,
        plot_mapping=None,
        initialises_patches=False,
    ):
        self.dofs_per_axis = dofs_per_axis
        self.representation = representation
        self.plot_mapping = plot_mapping
        self.includes = list(includes)
        self.tracer_include = tracer_include
        self.tracer_arguments = tracer_arguments
        self.initialises_patches = initialises_patches
        self._loop = loop
        self._solutions = dict(solutions)

    def solution(self, when="current"):
        """Returns the accessor of the current or the previous solution of a cell."""
        return "fineGridCell" + self._solutions[when]

    def for_each_dof(self, body, when="current", accessor=None):
        """Returns the loop call that runs ``body`` on every degree of freedom of ``accessor``,
        which defaults to the current or previous solution."""
        target = self.solution(when) if accessor is None else accessor
        return render_jinja2_template(
            self._loop,
            {"ACCESSOR": target, "BODY": textwrap.dedent(body).strip()},
            KeepUndefined,
        )


_TEMPLATE_LAMBDA = """[&](
      [[maybe_unused]] auto* const  value,
      [[maybe_unused]] const auto&  x,
      [[maybe_unused]] const auto&  h,
      [[maybe_unused]] const double weight,
      [[maybe_unused]] const int    index
    ) {
      {{BODY | indent(6)}}
    }"""


def volume_loop():
    return (
        """
  ::exahype2::fv::forEachVolume<{{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_DATA}}>(
    marker.getX(),
    marker.getH(),
    {{ACCESSOR}}.data(),
    """
        + _TEMPLATE_LAMBDA
        + """
  );
"""
    )


def subcell_loop():
    return (
        """
  ::exahype2::dg::limiting::forEachSubcell<2 * {{DG_ORDER}} + 1, {{NUMBER_OF_DATA}}>(
    marker.getX(),
    marker.getH(),
    {{ACCESSOR}}.data(),
    """
        + _TEMPLATE_LAMBDA
        + """
  );
"""
    )


def node_loop():
    return (
        """
  ::exahype2::dg::forEachNode<{{DG_ORDER}} + 1, {{NUMBER_OF_DATA}}>(
    marker.getX(),
    marker.getH(),
    {{QUADRATURE_TABLE}}::QuadraturePoints1D,
    {{QUADRATURE_TABLE}}::QuadratureWeights1D,
    {{ACCESSOR}}.data(),
    """
        + _TEMPLATE_LAMBDA
        + """
  );
"""
    )
