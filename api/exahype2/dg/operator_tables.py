# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import itertools
import os

import exahype2.codegen

from .matrices import render_vector


def flatten(tensor):
    result = []
    for entry in tensor:
        if isinstance(entry, (list, tuple)):
            result += flatten(entry)
        else:
            result.append(float(entry))
    return result


def tensor_product(entries, dimensions):
    """Returns the product of every combination of ``dimensions`` entries, the last index fastest.

    The factors of a product multiply in the order of ascending magnitude. Every order of the axes
    then gives the same bits.
    """
    result = []
    for indices in itertools.product(range(len(entries)), repeat=dimensions):
        product = 1.0
        for factor in sorted((entries[index] for index in indices), key=abs):
            product *= factor
        result.append(product)
    return result


def transpose(matrix, rows):
    return [
        matrix[column * rows + row] for row in range(rows) for column in range(rows)
    ]


def documentation_lines(text):
    """Returns the lines of ``text`` without indentation."""
    return [line.strip() for line in text.strip().split("\n")]


def table(name, entries, documentation):
    return {
        "name": name,
        "entries": entries,
        "length": len(entries),
        "rendered": render_vector(entries),
        "documentation": documentation_lines(documentation),
    }


def computed_table(name, expression, documentation, nodes_per_axis, length=None):
    """
    Returns a table that the generated C++ code computes from the quadrature nodes and
    weights. Without length, the table holds one matrix per child interval and depends on the
    subdivision factor, a build-time constant of the C++ code.
    """
    return {
        "name": name,
        "expression": expression,
        "length": length
        or "static_cast<std::size_t>(peano4::SubdivisionFactor) * {0} * {0}".format(
            nodes_per_axis
        ),
        "documentation": documentation_lines(documentation),
    }


def quadrature_tables(basis, dimensions):
    quadrature_weights = flatten(basis.quadrature_weights)
    return [
        table(
            "QuadraturePoints1D",
            basis.rounded_quadrature_points,
            basis.QuadraturePoints1DDocumentation,
        ),
        table(
            "QuadratureWeights1D",
            quadrature_weights,
            basis.QuadratureWeights1DDocumentation,
        ),
        table(
            "InverseQuadratureWeights1D",
            [1.0 / weight for weight in quadrature_weights],
            """
  Reciprocals of the one-dimensional quadrature weights.
""",
        ),
        table(
            "QuadratureWeightsOnFace",
            tensor_product(quadrature_weights, dimensions - 1),
            """
  Tensor product of the one-dimensional quadrature weights over a face.
""",
        ),
        table(
            "QuadratureWeightsInCell",
            tensor_product(quadrature_weights, dimensions),
            """
  Tensor product of the one-dimensional quadrature weights over a cell.
""",
        ),
        table(
            "InverseQuadratureWeightsInCell",
            [1.0 / weight for weight in tensor_product(quadrature_weights, dimensions)],
            """
  Reciprocals of the tensor-product quadrature weights in a cell.
""",
        ),
    ]


def dg_matrix_tables(basis):
    stiffness_operator = flatten(basis.stiffness_operator)
    return [
        table(
            "MassMatrixDiagonal1D",
            flatten(basis.mass_matrix_diagonal),
            basis.MassMatrix1DDocumentation,
        ),
        table(
            "StiffnessOperator1D",
            stiffness_operator,
            basis.StiffnessOperator1DDocumentation,
        ),
        table(
            "StiffnessOperatorTransposed1D",
            transpose(stiffness_operator, basis.dofs_per_axis),
            """
  Transpose of StiffnessOperator1D.
""",
        ),
        table(
            "DerivativeOperator1D",
            flatten(basis.derivative_operator),
            basis.DerivativeOperator1DDocumentation,
        ),
        table(
            "BasisFunctionValuesLeft1D",
            flatten(basis.basis_function_values_left),
            basis.BasisFunctionValuesLeft1DDocumentation,
        ),
        table(
            "BasisFunctionValuesRight1D",
            list(reversed(flatten(basis.basis_function_values_left))),
            """
  Values of the one-dimensional basis functions at the right endpoint.
""",
        ),
        computed_table(
            "InterpolationMatrix1D",
            "::exahype2::dg::getInterpolationMatrix1D<NodesPerAxis>(QuadratureNodes1D)",
            """
  Interpolation operator of the child intervals, packed as
  [interval][fine][coarse] with the coarse basis function evaluated at the
  fine node of that interval.
""",
            basis.dofs_per_axis,
        ),
        computed_table(
            "PlotMatrix1D",
            "::exahype2::dg::getEquidistantInterpolationMatrix1D<NodesPerAxis, {}>("
            "QuadratureNodes1D)".format(max(2 * basis.dofs_per_axis - 1, 2)),
            """
  Evaluation operator of the plot points, packed as [point][node] with the basis
  function evaluated at max(2 NodesPerAxis - 1, 2) equidistant points from face to face.
""",
            basis.dofs_per_axis,
            max(2 * basis.dofs_per_axis - 1, 2) * basis.dofs_per_axis,
        ),
        computed_table(
            "ProlongationMatrix1D",
            "::exahype2::dg::transposePerChild<NodesPerAxis>("
            "::exahype2::dg::getInterpolationMatrix1D<NodesPerAxis>(QuadratureNodes1D))",
            """
  Interpolation operator of the child intervals, packed as
  [interval][coarse][fine] with the coarse basis function evaluated at the
  fine node. A hanging face reads one matrix per tangential axis.
""",
            basis.dofs_per_axis,
        ),
        computed_table(
            "WeightedRestrictionMatrix1D",
            "::exahype2::dg::transposePerChild<NodesPerAxis>("
            "::exahype2::dg::getWeightedRestrictionMatrix1D<NodesPerAxis>(QuadratureNodes1D, QuadratureWeights1D))",
            """
  Restriction operator of the child intervals, packed as
  [interval][fine][coarse] with the fine node's quadrature weight divided by
  the coarse node's weight. One sum over the fine nodes of all children is the
  mass-lumped L2 projection.
""",
            basis.dofs_per_axis,
        ),
    ]


def add_operator_tables_to_dictionary(basis, dimensions, d):
    """
    Adds the quadrature and the reference operators of the basis to the
    dictionary as the table classes Quadrature and DGMatrices.
    """
    d["QUADRATURE_NODES_1D"] = render_vector(basis.rounded_quadrature_points)
    d["QUADRATURE_WEIGHTS_1D"] = render_vector(flatten(basis.quadrature_weights))
    d["OPERATOR_TABLES"] = [
        {
            "name": "Quadrature",
            "documentation": "1D quadrature of the solver's Lagrange basis.",
            "tables": quadrature_tables(basis, dimensions),
        },
        {
            "name": "DGMatrices",
            "documentation": (
                "1D reference operators of the solver's Lagrange basis, flattened row major,\n"
                "the inter-grid transfer operators as one consecutive block per child interval."
            ),
            "tables": dg_matrix_tables(basis),
        },
    ]


def generate_operator_tables(solver, output, d, precisions):
    """
    Generates the class template of every entry of OPERATOR_TABLES and instantiates it for
    every precision. The class name is the solver name followed by the entry name.
    """
    template_directory = os.path.dirname(os.path.realpath(__file__)) + "/"
    for table_class in d["OPERATOR_TABLES"]:
        class_name = solver.name + table_class["name"]
        dictionary = dict(d)
        dictionary["TABLE_CLASS_DOCUMENTATION"] = documentation_lines(
            table_class["documentation"]
        )
        dictionary["TABLES"] = table_class["tables"]
        dictionary["TABLE_PRECISIONS"] = precisions
        for header_template, implementation_template in (
            ("OperatorTables.h.jinja2", "OperatorTables.cpp.jinja2"),
            ("OperatorTables.cpph.jinja2", None),
        ):
            output.add(
                exahype2.codegen.TemplatedFile(
                    template_directory + header_template,
                    (
                        template_directory + implementation_template
                        if implementation_template
                        else None
                    ),
                    class_name,
                    solver._namespace,
                    "",
                    dictionary,
                    apply_iteratively=True,
                )
            )
        output.makefile.add_h_file(class_name + ".h", generated=True)
        output.makefile.add_h_file(class_name + ".cpph", generated=True)
        output.makefile.add_cpp_file(class_name + ".cpp", generated=True)
