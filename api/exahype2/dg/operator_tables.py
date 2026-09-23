# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
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
    result = [1.0]
    for _ in range(dimensions):
        result = [left * right for left in result for right in entries]
    return result


def transpose(matrix, rows):
    return [
        matrix[column * rows + row] for row in range(rows) for column in range(rows)
    ]


def table(name, entries, documentation):
    return {
        "name": name,
        "entries": entries,
        "length": len(entries),
        "rendered": render_vector(entries),
        "documentation": documentation,
    }


def computed_table(name, expression, documentation):
    """
    A table the generated C++ code computes from the quadrature nodes and
    weights, because it depends on the subdivision factor of the grid, which
    is a build-time constant the Python API does not know.
    """
    return {"name": name, "expression": expression, "documentation": documentation}


def quadrature_tables(basis, dimensions):
    quadrature_weights = flatten(basis.quadrature_weights)
    return [
        table(
            "QuadraturePoints1D",
            flatten(basis.quadrature_points),
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
            "::exahype2::dg::interpolationMatrix1D<NodesPerAxis>(QuadratureNodes1D)",
            """
  Interpolation operator of the child intervals, packed as
  [interval][fine][coarse] with the coarse basis function evaluated at the
  fine node of that interval.
""",
        ),
        computed_table(
            "ProlongationMatrix1D",
            "::exahype2::dg::transposePerChild<NodesPerAxis>("
            "::exahype2::dg::interpolationMatrix1D<NodesPerAxis>(QuadratureNodes1D))",
            """
  Interpolation operator of the child intervals, packed as
  [interval][coarse][fine] with the coarse basis function evaluated at the
  fine node, so a hanging face reads one matrix per tangential axis.
""",
        ),
        computed_table(
            "WeightedRestrictionMatrix1D",
            "::exahype2::dg::transposePerChild<NodesPerAxis>("
            "::exahype2::dg::weightedRestrictionMatrix1D<NodesPerAxis>(QuadratureNodes1D, QuadratureWeights1D))",
            """
  Restriction operator of the child intervals, packed as
  [interval][fine][coarse] with the fine node's quadrature weight divided by
  the coarse node's weight, so one sum over the fine nodes of all children is
  the mass-lumped L2 projection.
""",
        ),
    ]


def add_operator_tables_to_dictionary(basis, dimensions, d):
    """
    Adds the quadrature and the reference operators of the basis to the
    dictionary as the table classes Quadrature and DGMatrices.
    """
    d["QUADRATURE_NODES_1D"] = render_vector(flatten(basis.quadrature_points))
    d["QUADRATURE_WEIGHTS_1D"] = render_vector(flatten(basis.quadrature_weights))
    d["OPERATOR_TABLES"] = [
        {
            "name": "Quadrature",
            "documentation": " * 1D quadrature of the solver's Lagrange basis.\n",
            "tables": quadrature_tables(basis, dimensions),
        },
        {
            "name": "DGMatrices",
            "documentation": (
                " * 1D reference operators of the solver's Lagrange basis, flattened row major,\n"
                " * the inter-grid transfer operators as one consecutive block per child interval.\n"
            ),
            "tables": dg_matrix_tables(basis),
        },
    ]


def generate_operator_tables(solver, output, d, precisions):
    """
    Renders one class per entry of OPERATOR_TABLES into the solver's kernels
    directory, instantiated for every precision.
    """
    kernels_namespace = solver._namespace + ["kernels", solver._name]
    kernels_output_path = "kernels/" + solver._name
    template_directory = os.path.dirname(os.path.realpath(__file__)) + "/"
    for table_class in d["OPERATOR_TABLES"]:
        dictionary = dict(d)
        dictionary["TABLE_CLASS"] = table_class["name"]
        dictionary["TABLE_CLASS_DOCUMENTATION"] = table_class["documentation"]
        dictionary["TABLES"] = table_class["tables"]
        dictionary["TABLE_PRECISIONS"] = precisions
        generated_tables = exahype2.codegen.TemplatedFile(
            template_directory + "OperatorTables.h.jinja2",
            template_directory + "OperatorTables.cpp.jinja2",
            kernels_output_path + "/" + table_class["name"],
            kernels_namespace,
            ".",
            dictionary,
            apply_iteratively=True,
        )
        output.add(generated_tables)
        output.makefile.add_h_file(
            kernels_output_path + "/" + table_class["name"] + ".h", generated=True
        )
        output.makefile.add_cpp_file(
            kernels_output_path + "/" + table_class["name"] + ".cpp", generated=True
        )
