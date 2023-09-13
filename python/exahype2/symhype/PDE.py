# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import sympy
import numpy


class PDE(object):
    def __init__(self, unknowns, auxiliary_variables, dimensions):
        self.unknowns = unknowns
        self.auxiliary_variables = auxiliary_variables
        self.dimensions = dimensions
        self.Q = sympy.symarray("Q", (unknowns + auxiliary_variables))
        self.delta_Q = sympy.symarray("deltaQ", (unknowns + auxiliary_variables))
        self.initial_values = sympy.symarray("Q0", (unknowns + auxiliary_variables))
        self.boundary_values = sympy.symarray("Qb", (unknowns + auxiliary_variables))
        self.x = sympy.symarray("x", (dimensions))
        self.h = sympy.symarray("h", (dimensions))
        pass

    def unknown_identifier_for_plotter(self):
        """
        Returns identifier for the unknowns.
        Use this one to feed set_plot_description of your solver.
        """
        result = ""
        for i in range(0, self.unknowns + self.auxiliary_variables):
            if i != 0:
                result += ","
            result += str(self.Q[i])
        return result

    def _implementation_of_mapping_onto_named_quantities(
        self, is_cell_mapping=True, is_boundary=False, has_delta=False
    ):
        """
        Return the C code that maps the quantities from Q onto
        properly labelled quantities
        """
        result = ""
        for i in range(0, self.unknowns + self.auxiliary_variables):
            result += "[[maybe_unused]] const "
            result += "double "
            result += sympy.printing.cxxcode(self.Q[i])
            if is_boundary:
                result += " = Qinside[" + str(i) + "];\n"
                result += "nonCriticalAssertion(Qinside[" + str(i) + "] == Qinside[" + str(i) + "]);\n"
            else:
                result += " = Q[" + str(i) + "];\n"
                result += "nonCriticalAssertion(Q[" + str(i) + "] == Q[" + str(i) + "]);\n"
            result += "assertion(!std::isnan(" + sympy.printing.cxxcode(self.Q[i]) + "));\n"

        if has_delta:
            for i in range(0, self.unknowns + self.auxiliary_variables):
                result += "nonCriticalAssertion(deltaQ[" + str(i) + "] == deltaQ[" + str(i) + "]);\n"
                result += "[[maybe_unused]] const "
                result += "double "
                result += sympy.printing.cxxcode(self.delta_Q[i])
                result += " = deltaQ[" + str(i) + "];\n"
                result += "assertion(!std::isnan(" + sympy.printing.cxxcode(self.delta_Q[i]) + "));\n"

        for i in range(0, self.dimensions):
            result += "const double x_"
            result += str(i)
            result += " = "
            if is_cell_mapping:
                result += "volumeCentre"
            else:
                result += "faceCentre"
            result += "(" + str(i) + ");\n"
        for i in range(0, self.dimensions):
            result += "const double h_"
            result += str(i)
            result += " = "
            result += "volumeH"
            result += "(" + str(i) + ");\n"

        return result

    def name_Q_entry(self, offset_in_Q, name):
        """
        Q covers both unknowns plus auxiliary variables. They simply are concatenated.
        So if you have 10 unknowns and 2 auxiliary variables, the name you assign the
        10s Q entry is the one for the first auxiliary variable.
        There's also a dedicated routine for auxiliary variables if you prefer this one.
        @see name_auxiliary_variable
        """
        self.Q[offset_in_Q] = sympy.symbols(name)
        return self.Q[offset_in_Q]

    def name_auxiliary_variable(self,number,name):
        self.Q[number + self.unknowns] = sympy.symbols(name)
        return self.Q[number + self.unknowns]

    def name_Q_entries(self, offset_in_Q, cardinality, name):
        new_entry = sympy.symarray(name, cardinality)
        for i in range(0, cardinality):
            self.Q[offset_in_Q + i] = new_entry[i]
        return new_entry

    def grad(self, Q):
        if isinstance(Q, numpy.ndarray):  # Q is vector
            offset_in_delta_Q = numpy.where(self.Q == Q[0])[0][0]
            cardinality = len(Q)
            name = Q[0].name.split("_")[0]
            new_entry = sympy.symarray("delta_" + name, cardinality)
            for i in range(0, cardinality):
                self.delta_Q[offset_in_delta_Q + i] = new_entry[i]
            return new_entry
        else:  # Q is scalar
            offset_in_delta_Q = numpy.where(self.Q == Q)[0][0]
            cardinality = 1
            name = Q.name
            self.delta_Q[offset_in_delta_Q] = sympy.symbols("delta_" + name)
            return self.delta_Q[offset_in_delta_Q]

    def implementation_of_homogeneous_Neumann_BC(self):
        result = "assertion(normal >= 0);\n"
        result += "//assertion(normal < Dimensions);\n"
        for i in range(0, self.unknowns + self.auxiliary_variables):
            result += "assertion(!std::isnan(Qoutside[" + str(i) + "]));\n"
            result += "assertion(!std::isnan(Qinside[" + str(i) + "]));\n"
            result += "nonCriticalAssertion(Qinside[" + str(i) + "] == Qinside[" + str(i) + "]);\n"
            result += "Qoutside[" + str(i) + "] = Qinside[" + str(i) + "];\n"
            result += "nonCriticalAssertion(Qoutside[" + str(i) + "] == Qoutside[" + str(i) + "]);\n"
        return result

    def implementation_of_boundary_conditions(self, invoke_evalf_before_output=False):
        """
        invoke_evalf_before_output: boolean
          If your expression is a symbolic expression (default) then we use evalf
          before we pipe it into the output. If your expression is something numeric,
          then evalf will fail (as it is not defined for scalar quantities).
        """
        result = "assertion(normal >= 0);\n"
        result += "//assertion(normal < Dimensions);\n"
        result += self._implementation_of_mapping_onto_named_quantities(
            is_cell_mapping=False, is_boundary=True
        )
        for i in range(0, self.unknowns + self.auxiliary_variables):
            if invoke_evalf_before_output:
                result += sympy.printing.cxxcode(
                    self.boundary_values[i].evalf(), assign_to="Q[" + str(i) + "]"
                )
            else:
                result += (
                    "Qoutside["
                    + str(i)
                    + "] = "
                    + sympy.printing.cxxcode(self.boundary_values[i])
                    + ";"
                )
            result += "\n"
        return result

    def implementation_of_initial_conditions(self, invoke_evalf_before_output=False):
        """
        invoke_evalf_before_output: boolean
          If your expression is a symbolic expression (default) then we use evalf
          before we pipe it into the output. If your expression is something numeric,
          then evalf will fail (as it is not defined for scalar quantities).
        """
        result = self._implementation_of_mapping_onto_named_quantities(
            is_cell_mapping=True
        )
        for i in range(0, self.unknowns + self.auxiliary_variables):
            if invoke_evalf_before_output:
                result += sympy.printing.cxxcode(
                    self.initial_values[i].evalf(), assign_to="Q[" + str(i) + "]"
                )
            else:
                result += (
                    "Q["
                    + str(i)
                    + "] = "
                    + sympy.printing.cxxcode(self.initial_values[i])
                    + ";"
                )
            result += "\n"
            result += "assertion(!std::isnan(Q[" + str(i) + "]));\n"
        return result
