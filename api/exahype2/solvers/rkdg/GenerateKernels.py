# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output

from exahype2.solvers.ButcherTableau import ButcherTableau
from exahype2.solvers.LagrangeBasis import render_tensor_1


def _flatten(tensor):
    result = []
    for entry in tensor:
        if isinstance(entry, (list, tuple)):
            result += _flatten(entry)
        else:
            result.append(float(entry))
    return result


def _tensor_product(entries, dimensions):
    result = [1.0]
    for _ in range(dimensions):
        result = [left * right for left in result for right in entries]
    return result


def _add_operator_entries_to_dictionary(solver, d):
    """
    Adds the operator tables of the generated Quadrature, DGMatrices and
    ButcherTableau classes to the dictionary.

    A table with an expression in place of the entries is computed by the
    generated C++ code from the quadrature nodes and weights, as the
    subdivision factor of the grid is a build-time constant that the Python
    API does not know.
    """
    basis = solver._basis
    butcher_tableau = ButcherTableau(solver._rk_order)
    quadrature_weights = _flatten(basis.quadrature_weights)

    d["QUADRATURE_NODES_1D"] = render_tensor_1(_flatten(basis.quadrature_points))
    d["QUADRATURE_WEIGHTS_1D"] = render_tensor_1(quadrature_weights)

    d["QUADRATURE"] = [
        {
            "name": "QuadraturePoints1D",
            "entries": _flatten(basis.quadrature_points),
            "documentation": basis.QuadraturePoints1DDocumentation,
        },
        {
            "name": "QuadratureWeights1D",
            "entries": quadrature_weights,
            "documentation": basis.QuadratureWeights1DDocumentation,
        },
        {
            "name": "QuadratureWeightsOnFace",
            "entries": _tensor_product(quadrature_weights, d["DIMENSIONS"] - 1),
            "documentation": """
  Tensor product of the one-dimensional quadrature weights over a face.
""",
        },
        {
            "name": "InverseQuadratureWeightsInCell",
            "entries": [
                1.0 / weight
                for weight in _tensor_product(quadrature_weights, d["DIMENSIONS"])
            ],
            "documentation": """
  Reciprocals of the tensor-product quadrature weights in a cell.
""",
        },
    ]

    d["DG_MATRICES"] = [
        {
            "name": "MassMatrixDiagonal1D",
            "entries": _flatten(basis.mass_matrix_diagonal),
            "documentation": basis.MassMatrix1DDocumentation,
        },
        {
            "name": "StiffnessOperator1D",
            "entries": _flatten(basis.stiffness_operator),
            "documentation": basis.StiffnessOperator1DDocumentation,
        },
        {
            "name": "DerivativeOperator1D",
            "entries": _flatten(basis.derivative_operator),
            "documentation": basis.DerivativeOperator1DDocumentation,
        },
        {
            "name": "BasisFunctionValuesLeft1D",
            "entries": _flatten(basis.basis_function_values_left),
            "documentation": basis.BasisFunctionValuesLeft1DDocumentation,
        },
        {
            "name": "BasisFunctionValuesRight1D",
            "entries": list(reversed(_flatten(basis.basis_function_values_left))),
            "documentation": """
  Values of the one-dimensional basis functions at the right endpoint.
""",
        },
        {
            "name": "InterpolationMatrix1D",
            "expression": "::exahype2::dg::interpolationMatrix1D<NodesPerAxis>(QuadratureNodes1D)",
            "documentation": """
  Interpolation operator of the child intervals, packed as
  [interval][fine][coarse] with the coarse basis function evaluated at the
  fine node of that interval.
""",
        },
        {
            "name": "ProlongationMatrix1D",
            "expression": "::exahype2::dg::transposePerChild<NodesPerAxis>("
            "::exahype2::dg::interpolationMatrix1D<NodesPerAxis>(QuadratureNodes1D))",
            "documentation": """
  Interpolation operator of the child intervals, packed as
  [interval][coarse][fine] with the coarse basis function evaluated at the
  fine node, so a hanging face reads one matrix per tangential axis.
""",
        },
        {
            "name": "WeightedRestrictionMatrix1D",
            "expression": "::exahype2::dg::transposePerChild<NodesPerAxis>("
            "::exahype2::dg::weightedRestrictionMatrix1D<NodesPerAxis>(QuadratureNodes1D, QuadratureWeights1D))",
            "documentation": """
  Restriction operator of the child intervals, packed as
  [interval][fine][coarse] with the fine node's quadrature weight divided by
  the coarse node's weight, so one sum over the fine nodes of all children is
  the mass-lumped L2 projection.
""",
        },
    ]

    d["BUTCHER_TABLEAU"] = [
        {"name": "ButcherMatrix", "entries": _flatten(butcher_tableau.weight_matrix())},
        {
            "name": "FinalEstimateWeights",
            "entries": _flatten(butcher_tableau.final_estimate_weights()),
        },
        {
            "name": "RelativeTimeStepSizes",
            "entries": _flatten(butcher_tableau.time_step_sizes()),
        },
    ]


def _generate_operator_tables(
    solver, output, kernels_namespace, kernels_output_path, d
):
    template_directory = os.path.dirname(os.path.realpath(__file__)) + "/"
    for kernel, template_subdirectory in [
        ("Quadrature", "basis/"),
        ("DGMatrices", "basis/"),
        ("ButcherTableau", ""),
    ]:
        generated_tables = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_directory
            + template_subdirectory
            + kernel
            + ".template.h",
            cppfile_template=template_directory
            + template_subdirectory
            + kernel
            + ".template.cpp",
            classname=kernels_output_path + "/" + kernel,
            namespace=kernels_namespace,
            subdirectory=".",
            dictionary=d,
            apply_iteratively=True,
        )
        output.add(generated_tables)
        output.makefile.add_h_file(
            kernels_output_path + "/" + kernel + ".h", generated=True
        )
        output.makefile.add_cpp_file(
            kernels_output_path + "/" + kernel + ".cpp", generated=True
        )
        solver._user_action_set_includes += (
            f'#include "{kernels_output_path}/{kernel}.h"\n'
        )


def generate_kernels(solver, output):
    kernels_namespace = solver._namespace + ["kernels", solver._name]
    kernels_output_path = "kernels/" + solver._name
    d = output.makefile.dictionary()
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)

    _add_operator_entries_to_dictionary(solver, d)

    _generate_operator_tables(solver, output, kernels_namespace, kernels_output_path, d)

    solver._user_action_set_includes += """
#include "exahype2/dg/BoundaryConditions.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/FaceIntegral.h"
#include "exahype2/dg/Riemann.h"
#include "exahype2/dg/rk/LinearCombination.h"
#include "exahype2/dg/rk/VolumeIntegral.h"
"""
