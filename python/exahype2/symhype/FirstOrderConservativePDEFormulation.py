# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import sympy
import numbers

from .PDE import PDE


class FirstOrderConservativePDEFormulation(PDE):
    """
    Helper class to model a hyperbolic PDE in first-order conservative
    formulation.

    To model your PDE, you typically run through a couple of single steps.
    First, include this package plus sympy. Next, create a new instance
    of this class to which you pass the number of equations in your PDE
    system, i.e., the number of unknowns you want to develop, plus the
    dimension.

    As a consequence, the solver holds an equation pde which describes the
    PDE's right-hand side in the representation

    \partial Q(t) + div_x F(Q)

    Most people prefer not to work with a vector Q but with some symbolic
    names. Feel free to derive them from Q via constructs similar to

    rho = euler.Q[0]
    j = sympy.Array(euler.Q[1:4])
    E = euler.Q[4]
    """

    def __init__(self, unknowns, auxiliary_variables, dimensions):
        PDE.__init__(self, unknowns, auxiliary_variables, dimensions)

        self.F = sympy.symarray("F", (unknowns, dimensions))
        self.ncp = sympy.symarray("ncp", (unknowns, dimensions))
        self.eigenvalues = sympy.symarray("alpha", (unknowns, dimensions))
        self.sources = sympy.symarray("S", (unknowns))

    def __str__(self):
        result = ""
        for i in range(0, self.unknowns):
            result += "d_t(" + str(self.Q[i]) + "(x)) + div("
            result += str(self.F[i])
            if isinstance(self.ncp[i], numbers.Number):
              result += " + "
              result += str(self.ncp[i])
            result += ") = "
            if isinstance(self.sources[i], numbers.Number) or isinstance(self.sources[i], sympy.Expr):
              result += str(self.sources[i]) + "\n"
            else:
              result += "0\n"
        result += "\n"
        for i in range(0, self.unknowns):
            result += "\lambda_{max," + str(i) + "}(x) = "
            result += str(self.eigenvalues[i])
            result += "\n"
        return result

    def LaTeX(self):
        result = "\\begin{eqnarray*}"
        for i in range(0, self.unknowns):
            result += "\partial _t " + str(self.Q[i]) + "(x) + div("
            for j in range(0, len(self.F[i])):
                if j != 0:
                    result += ", "
                result += str(self.F[i][j])
                if isinstance(self.ncp[i], numbers.Number):
                  result += " + "
                  result += str(self.ncp[i][j])
            result += ") & = & "
            if isinstance(self.sources[i], numbers.Number) or isinstance(self.sources[i], sympy.Expr):
              result += str(self.sources[i]) + " \\\\"
            else:
              result += "0 \\\\"
        for i in range(0, self.unknowns):
            result += "\\lambda _{max," + str(i) + "}(x) & = & ["
            for j in range(0, len(self.eigenvalues[i])):
                if j != 0:
                    result += ","
                result += str(self.eigenvalues[i][j])
            result += "] \\\\"
        result += "\\end{eqnarray*}"
        result = result.replace("**", "^")
        result = result.replace("sqrt", "\\sqrt")
        return result

    def substitute_expression(self, expression, new_expression):
        """
        Usually used to set a symbolic variable (expression) to a value
        (new_expression). We run over all internal expressions and set
        them accordingly.
        """
        for i in range(0, self.unknowns):
            if not isinstance(self.sources[i], numbers.Number):
                self.sources[i] = self.sources[i].subs(expression, new_expression)
            for d in range(0, self.dimensions):
                if not isinstance(self.F[i, d], numbers.Number):
                    self.F[i, d] = self.F[i, d].subs(expression, new_expression)
                if not isinstance(self.ncp[i, d], numbers.Number):
                    self.ncp[i, d] = self.ncp[i, d].subs(expression, new_expression)
                if not isinstance(self.eigenvalues[i, d], numbers.Number):
                    self.eigenvalues[i, d] = self.eigenvalues[i, d].subs(
                        expression, new_expression
                    )

    def implementation_of_flux(self, invoke_evalf_before_output=False):
        """
        Return implementation for flux along one coordinate axis (d) as C code.
        invoke_evalf_before_output: boolean
          If your expression is a symbolic expression (default) then we use evalf
          before we pipe it into the output. If your expression is something numeric,
          then evalf will fail (as it is not defined for scalar quantities).
        """
        result = "assertion(normal >= 0);\n"
        result += "assertion(normal < Dimensions);\n"
        result += self._implementation_of_mapping_onto_named_quantities(
            is_cell_mapping=False
        )
        result += "switch( normal ) {\n"
        for d in range(0, self.dimensions):
            result += "  case " + str(d) + ":\n"
            for i in range(0, self.unknowns):
                if invoke_evalf_before_output:
                    result += sympy.printing.cxxcode(
                        self.F[i, d].evalf(), assign_to="F[" + str(i) + "]"
                    )
                else:
                    result += (
                        "F["
                        + str(i)
                        + "] = "
                        + sympy.printing.cxxcode(self.F[i, d])
                        + ";"
                    )
                result += "\n"
                result += "assertion(!std::isnan(F[" + str(i) + "]));\n"
                result += "nonCriticalAssertion(F[" + str(i) + "] == F[" + str(i) + "]);\n"
            result += "  break;\n"
        result += "}\n"
        return result

    def implementation_of_ncp(self, invoke_evalf_before_output=False):
        """
        Return implementation for nonconservative product (ncp) along one coordinate axis (d) as C code.
        invoke_evalf_before_output: boolean
          If your expression is a symbolic expression (default) then we use evalf
          before we pipe it into the output. If your expression is something numeric,
          then evalf will fail (as it is not defined for scalar quantities).
        """
        result = "assertion(normal >= 0);\n"
        result += "assertion(normal < Dimensions);\n"
        result += self._implementation_of_mapping_onto_named_quantities(
            is_cell_mapping=False, has_delta=True
        )
        result += "switch( normal ) {\n"
        for d in range(0, self.dimensions):
            result += "  case " + str(d) + ":\n"
            for i in range(0, self.unknowns):
                if invoke_evalf_before_output:
                    result += sympy.printing.cxxcode(
                        self.ncp[i, d].evalf(), assign_to="BTimesDeltaQ[" + str(i) + "]"
                    )
                else:
                    result += (
                        "BTimesDeltaQ["
                        + str(i)
                        + "] = "
                        + sympy.printing.cxxcode(self.ncp[i, d])
                        + ";"
                    )
                result += "\n"
            result += "  break;\n"
        result += "}\n"
        return result

    def implementation_of_eigenvalues(self, invoke_evalf_before_output=False):
        """
        Return eigenvalues

        This yields a set of eigenvalues, i.e., one per unknown. For many solvers such as
        Rusanov, you need only one.

        d: int
          The axis along which we wanna have the eigenvalues

        invoke_evalf_before_output: boolean
          If your expression is a symbolic expression (default) then we use evalf
          before we pipe it into the output. If your expression is something numeric,
          then evalf will fail (as it is not defined for scalar quantities).
        """
        result = "assertion(normal >= 0);\n"
        result += "assertion(normal < Dimensions);\n"
        result += self._implementation_of_mapping_onto_named_quantities(
            is_cell_mapping=False
        )
        result += "switch (normal) {\n"
        for d in range(0, self.dimensions):
            result += "  case " + str(d) + ":\n"
            for i in range(0, self.unknowns):
                if invoke_evalf_before_output:
                    result += sympy.printing.cxxcode(
                        self.eigenvalues[i, d].evalf(),
                        assign_to="lambda[" + str(i) + "]",
                    )
                else:
                    result += (
                        "lambda["
                        + str(i)
                        + "] = "
                        + sympy.printing.cxxcode(self.eigenvalues[i, d])
                        + ";"
                    )
                result += "\n"
            result += "  break;\n"
        result += "}\n"
        return result

    def implementation_of_max_eigenvalue(
        self, invoke_evalf_before_output=False, use_absolute_values=True
    ):
        """
        Return maximum eigenvalue
        """
        result = "assertion(normal >= 0);\n"
        result += "assertion(normal < Dimensions);\n"
        result += self._implementation_of_mapping_onto_named_quantities(
            is_cell_mapping=False
        )
        result += "double lambda[" + str(self.unknowns) + "];\n"
        result += "switch (normal) {\n"
        for d in range(0, self.dimensions):
            result += "  case " + str(d) + ":\n"
            for i in range(0, self.unknowns):
                if invoke_evalf_before_output:
                    result += sympy.printing.cxxcode(
                        self.eigenvalues[i, d].evalf(),
                        assign_to="lambda[" + str(i) + "]",
                    )
                else:
                    result += (
                        "lambda["
                        + str(i)
                        + "] = "
                        + sympy.printing.cxxcode(self.eigenvalues[i, d])
                        + ";"
                    )
                result += "\n"
                result += "assertion(!std::isnan(lambda[" + str(i) + "]));\n"
            result += "  break;\n"
        result += "}\n"
        result += "double result = 0.0;\n"
        for i in range(0, self.unknowns):
            if use_absolute_values:
                result += (
                    "result = std::max( result, std::abs(lambda[" + str(i) + "]) );\n"
                )
            else:
                result += "result = std::max( result, lambda[" + str(i) + "] );\n"
        result += "return result;\n"
        return result

    def implementation_of_sources(self, invoke_evalf_before_output=False):
        """
        Return implementation for sources as C code.
        """
        result = "assertion(normal >= 0);\n"
        result += "assertion(normal < Dimensions);\n"
        result = self._implementation_of_mapping_onto_named_quantities(
            is_cell_mapping=True
        )
        for i in range(0, self.unknowns):
            if invoke_evalf_before_output:
                result += sympy.printing.cxxcode(
                    self.sources[i].evalf(), assign_to="S[" + str(i) + "]"
                )
            else:
                result += (
                    "S["
                    + str(i)
                    + "] = "
                    + sympy.printing.cxxcode(self.sources[i])
                    + ";"
                )
            result += "\n"
        return result
