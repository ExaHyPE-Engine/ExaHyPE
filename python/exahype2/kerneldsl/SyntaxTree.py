from abc import ABC, abstractmethod
import os
import copy
from enum import Enum

class Node(ABC):
    output_format = None
    spaces_per_tab = 4
    
    @abstractmethod
    def print_cpp(self, indent_level = 0):
        pass

    @abstractmethod
    def print_mlir(self, indent_level = 0):
        pass

    @abstractmethod
    def print_omp(self, indent_level = 0):
        pass

    @abstractmethod
    def print_sycl(self, indent_level = 0):
        pass
    
    @abstractmethod
    def print_tree(self, indent_level = 0):
        pass

class Statement(Node):
    pass


class Expression(Node):
    def multidimensional_index(self, index_list, start_index=0):
        return self
    
    @abstractmethod
    def get_type(self):
        pass


class Type(Node):
    pass


class Argument(Node):
    def __init__(self, id, type: Type):
        self.id = id
        self._type = type
    
    def print_cpp(self, indent_level = 0):
        return self._type.print_cpp() + " " + self.id
    
    def print_omp(self, indent_level = 0):
        return self._type.print_omp() + " " + self.id
    
    def print_sycl(self, indent_level = 0):
        return self._type.print_sycl() + " " + self.id

    def print_mlir(self, indent_level = 0):
        return "%" + self.id + ": " + self._type.print_mlir()
    
    def print_tree(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"Argument: {self.id}"


class FunctionDefinition(Node):
    _stateless = False

    def __init__(self, id, return_type: Type, template = None, namespaces = [], stateless = False):
        self.id = id
        self._return_type = return_type
        self._template = template
        self._arguments = []
        self._body = []
        self._namespaces = namespaces
        FunctionDefinition._stateless = stateless

    def add_statement(self, statement: Statement):
        self._body.append(statement)

    def add_argument(self, argument: Argument):
        self._arguments.append(argument)

    def print_declaration(self, indent_level = 0):
        namespace_header = f"""namespace {"::".join(self._namespaces)} {{"""
        namespace_footer = "}"
        template_string = ""
        if self._template is not None:
            template_strings = [argument.print_cpp() for argument in self._template]
            template_string = "template <" + ",".join(template_strings) + ">\n"

        current_indent = indent_level + 1  # Indentation increases inside namespace
        argument_prints = [argument.print_cpp() for argument in self._arguments]

        return f"""#include <fstream>\n{namespace_header}
{template_string}{' ' * current_indent * Node.spaces_per_tab}{self._return_type.print_cpp()} {self.id}({', '.join(argument_prints)});
{namespace_footer}
"""

    def print_cpp(self, indent_level=0):
        namespace_header = f"""namespace {"::".join(self._namespaces)} {{"""
        namespace_footer = "}"
        template_string = ""
        if self._template is not None:
            template_strings = [argument.print_cpp() for argument in self._template]
            template_string = "template <" + ",".join(template_strings) + ">\n"

        current_indent = indent_level + 1  # Indentation increases inside namespace
        statement_prints = [statement.print_cpp(current_indent + 1) for statement in self._body]
        argument_prints = [argument.print_cpp() for argument in self._arguments]

        return f"""{namespace_header}
{template_string}{' ' * current_indent * Node.spaces_per_tab}{self._return_type.print_cpp()} {self.id}({', '.join(argument_prints)}) {{
std::ofstream log;
{os.linesep.join(statement_prints)}
{' ' * current_indent * Node.spaces_per_tab}}}
{namespace_footer}
"""
    
    def print_omp(self, indent_level=0):
        namespace_header = f"""namespace {"::".join(self._namespaces)} {{"""
        namespace_footer = "}"
        template_string = ""
        if self._template is not None:
            template_strings = [argument.print_omp() for argument in self._template]
            template_string = "template <" + ",".join(template_strings) + ">\n"

        current_indent = indent_level + 1  # Indentation increases inside namespace
        statement_prints = [statement.print_omp(current_indent + 1) for statement in self._body]
        argument_prints = [argument.print_omp() for argument in self._arguments]

        return f"""{namespace_header}
{template_string}{' ' * current_indent * Node.spaces_per_tab}{self._return_type.print_omp()} {self.id}({', '.join(argument_prints)}) {{
{os.linesep.join(statement_prints)}
{' ' * current_indent * Node.spaces_per_tab}}}
{namespace_footer}
"""
    
    def print_sycl(self, indent_level=0):
        namespace_header = f"""namespace {"::".join(self._namespaces)} {{"""
        namespace_footer = "}"
        template_string = ""
        if self._template is not None:
            template_strings = [argument.print_sycl() for argument in self._template]
            template_string = "template <" + ",".join(template_strings) + ">\n"

        current_indent = indent_level + 1  # Indentation increases inside namespace
        statement_prints = [statement.print_sycl(current_indent + 1) for statement in self._body]
        argument_prints = [argument.print_sycl() for argument in self._arguments]
        #statement_prints.append("::sycl::queue Queue;")

        return f"""{namespace_header}
{template_string}{' ' * current_indent * Node.spaces_per_tab}{self._return_type.print_sycl()} {self.id}({', '.join(argument_prints)}) {{
{os.linesep.join(statement_prints)}
{' ' * current_indent * Node.spaces_per_tab}}}
{namespace_footer}
"""
    
    def print_mlir(self, indent_level = 0):
        statement_prints = [statement.print_mlir(indent_level + 1) for statement in self._body]
        argument_prints = [argument.print_mlir() for argument in self._arguments]
        return f"""
func.func @{self.id}({', '.join(argument_prints)}) -> ({self._return_type.print_mlir()}) {{
{os.linesep.join(statement_prints)}
}}
"""

    def print_tree(self, indent_level = 0):
        statement_prints = [statement.print_tree(indent_level + 1) for statement in self._body]
        argument_prints = [argument.print_tree(indent_level + 1) for argument in self._arguments]
        return f"""
FunctionDefinition: {self.id}:
{os.linesep.join(argument_prints)}
{os.linesep.join(statement_prints)}"""
    
    def print_definition_with_timer(self, indent_level = 0):
        namespace_header = f"""namespace {"::".join(self._namespaces)} {{"""
        namespace_footer = "}"
        template_string = ""
        function_call_string = self.id
        if self._template is not None:
            template_strings = [argument.print_cpp() for argument in self._template]
            template_string = "template <" + ",".join(template_strings) + ">\n"
            function_call_string += "<" + ",".join([template.split(' ')[1] for template in template_strings]) + ">"

        arguments = copy.deepcopy(self._arguments)
        arguments.append(Argument("measurement", TCustom("tarch::timing::Measurement&")))

        current_indent = indent_level + 1  # Indentation increases inside namespace
        argument_prints = [argument.print_cpp() for argument in arguments]

        return f"""{namespace_header}
{template_string}{' ' * current_indent * Node.spaces_per_tab}{self._return_type.print_cpp()} {self.id}({', '.join(argument_prints)}) {{
tarch::timing::Watch watch("{"::".join(self._namespaces)}", "{self.id}", false, true);
{function_call_string}({",".join([argument.print_cpp().split(' ')[-1] for argument in self._arguments])});
watch.stop();
measurement.setValue(watch.getCalendarTime());
{' ' * current_indent * Node.spaces_per_tab}}}
{namespace_footer}
"""
    
    def print_declaration_with_timer(self, indent_level = 0):
        namespace_header = f"""namespace {"::".join(self._namespaces)} {{"""
        namespace_footer = "}"
        template_string = ""
        if self._template is not None:
            template_strings = [argument.print_cpp() for argument in self._template]
            template_string = "template <" + ",".join(template_strings) + ">\n"

        arguments = copy.deepcopy(self._arguments)
        arguments.append(Argument("measurement", TCustom("tarch::timing::Measurement&")))

        current_indent = indent_level + 1  # Indentation increases inside namespace
        argument_prints = [argument.print_cpp() for argument in arguments]

        return f"""{namespace_header}
{template_string}{' ' * current_indent * Node.spaces_per_tab}{self._return_type.print_cpp()} {self.id}({', '.join(argument_prints)});
{namespace_footer}
"""

class LogToFile(Statement):
    def __init__(self, statementToLog, outputStream):
        self._statement = statementToLog
        self._output = outputStream

    def print_cpp(self, indent_level=0):
        return f"""{self._output.print_cpp()} << {self._statement.print_cpp()};"""
    
    def print_omp(self, indent_level=0):
        return f"""{self._output.print_omp()} << {self._statement.print_omp()};"""
    
    def print_sycl(self, indent_level=0):
        return f"""{self._output.print_sycl()} << {self._statement.print_sycl()};"""
    
    def print_mlir(self, indent_level=0):
        pass

    def print_tree(self, indent_level=0):
        return "LogToFile"


# Expressions
class Name(Expression):
    variables = dict()

    def __init__(self, id, type = None):
        self.id = id
        self.type = type
        if self.type is None:
            self.type = Name.variables[self.id].get_type()

    def multidimensional_index(self, index_list, start_index=0):
        if type(Name.variables[self.id].get_type()) is TDataBlock:
            return Name.variables[self.id].multidimensional_index(index_list, start_index)
        else:
            return self

    def __add__(self, rhs):
        return BinaryOperation("+", self, rhs)

    def __sub__(self, rhs):
        return BinaryOperation("-", self, rhs)
    
    def __mul__(self, rhs):
        return BinaryOperation("*", self, rhs)
    
    def __div__(self, rhs):
        return BinaryOperation("/", self, rhs)
    
    def __neg__(self):
        return UnaryOperation("-", self)

    def __gt__(self, rhs):
        if type(rhs) is Name:
            return (Name.variables[self.id] > Name.variables[rhs.id])
        return (Name.variables[self.id] > rhs)

    def __eq__(self, rhs):
        if type(rhs) is Name:
            if rhs.id in Name.variables and self.id in Name.variables:
                return (self._value == Name.variables[rhs.id])
            else:
                return False
        else:
            if self.id in Name.variables:
                return (Name.variables[self.id] == rhs)
            else:
                return False

    def get_type(self):
        return self.type
    
    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self.id
    
    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self.id
    
    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self.id
    
    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "%" + self.id
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "Name:" + self.id
    
class Integer(Expression):
    def __init__(self, value, string = None):
        self._value = int(value)
        self._string = string
    
    def __add__(self, rhs):
        if self._value == 0:
            return rhs
        if type(rhs) is Integer:
            return Integer(self._value + rhs._value)
        return BinaryOperation("+", self, rhs)

    def __sub__(self, rhs):
        if self._value == 0:
            if type(rhs) is Integer:
                return Integer(-rhs._value)
            if type(rhs) is UnaryOperation and rhs._operation == "-":
                return rhs._operand
            return -rhs
        if type(rhs) is Integer:
            return Integer(self._value - rhs._value)
        return BinaryOperation("-", self, rhs)
    
    def __mul__(self, rhs):
        return BinaryOperation("*", self, rhs)
    
    def __div__(self, rhs):
        return BinaryOperation("/", self, rhs)

    def __neg__(self):
        if self._value == 0:
            return self
        else:
            return UnaryOperation("-", self)

    def __lt__(self, rhs):
        if type(rhs) is Name:
            if rhs.id in Name.variables:
                return (self._value < Name.variables[rhs.id])
            else:
                return False
        return (self._value < rhs)
    
    def __gt__(self, rhs):
        if type(rhs) is Name:
            if rhs.id in Name.variables:
                return (self._value > Name.variables[rhs.id])
            else:
                return False
        return (self._value > rhs)

    def __eq__(self, rhs):
        if type(rhs) is Integer:
            return (self._value == rhs._value)

    def __int__(self):
        return self._value

    def get_type(self):
        return TInteger()
    
    def print_cpp(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._string)
        
    def print_omp(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._string)
    
    def print_sycl(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._string)
    
    def print_mlir(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._string)
        
    def print_tree(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + "Integer: " + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + "Integer: " + str(self._string)


class Boolean(Expression):
    def __init__(self, value):
        self._value = value
    
    def __str__(self):
        return str(self._value)
    
    def get_type(self):
        return TBoolean()

    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
    
    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + str(self._value)

    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + str(self._value)

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "Boolean: " + str(self._value)

class String(Expression):
    def __init__(self, value):
        self._value = value
    
    def __str__(self):
        return self._value

    def get_type(self):
        return TString()
    
    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._value
    
    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._value

    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._value

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._value
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "String: " + self._value


class Float(Expression):
    def __init__(self, value,  string = None, reference = False):
        self._value = value
        self._string = string
        self._reference = reference

    
    def __float__(self):
        return self._value

    def get_type(self):
        return TFloat(self._reference)
    
    def print_cpp(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._string)
    
    def print_omp(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._string)
    
    def print_sycl(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._string)
    
    def print_mlir(self, indent_level=0):
        if self._string is None:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._value)
        else:
            return ' ' * indent_level * Node.spaces_per_tab + str(self._string)
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "Float: " + str(self._value)
    

class DataBlock(Expression):
    def __init__(self, iteration_range, internal, requires_memory_allocation, id = None, underlying_type = String("double")):
        self._iteration_range = iteration_range
        self._internal = internal
        self.requires_memory_allocation = requires_memory_allocation
        self._underlying_type = underlying_type
        self.id = id
        self._offset = []
        self._memory_range = []
        for [x, y] in self._iteration_range:
            self._offset.append(Integer(0))
            self._memory_range.append([Integer(0), y - x])
    
    def offset(self, offset):
        output = DataBlock(copy.deepcopy(self._iteration_range), copy.deepcopy(self._internal), True)
        output.id = self.id

        for i in range(len(offset)):
            if type(offset[i]) is list:
                if offset[i][0] is None:
                    offset[i][0] = self._iteration_range[i][0]
                if offset[i][1] is None:
                    offset[i][1] = self._iteration_range[i][1]

                output._offset[i] = offset[i][0] - self._iteration_range[i][0]
                output._iteration_range[i][1] = offset[i][1]
                output._iteration_range[i][0] = offset[i][0]
            else:
                raise Exception("Invalid index")
        
        for i in range(len(output._offset), len(output._iteration_range)):
            output._offset.append(Integer(0))

        return output

    def linearise_index(self, indices, dimensions, offset_start_index = 0):
        factors = [Integer(1)]
        index_list = indices[0:-1]
        for i in range(0, len(dimensions) - 1):
            factors.append(factors[-1] * (self._memory_range[i][1]))
        if type(dimensions[-len(index_list)][1] - dimensions[-len(index_list)][0]) is Integer and (dimensions[-len(index_list)][1] - dimensions[-len(index_list)][0])._value == 1:
            index = self._offset[offset_start_index]
        else:
            if type(self._offset[offset_start_index + 0].get_type()) is TDataBlock:
                index = index_list[0] + self._offset[offset_start_index + 0].multidimensional_index(index_list, offset_start_index)
            else:
                index = index_list[0] + self._offset[offset_start_index + 0]

        index = factors[-len(index_list)] * index
        for i in range(1, len(index_list)):
            if type(self._offset[offset_start_index + i].get_type()) is TDataBlock:
                temp = index_list[i] + self._offset[offset_start_index + i].multidimensional_index(indices, offset_start_index)
            else:
                temp = index_list[i] + self._offset[offset_start_index + i]
            index = index + factors[-len(index_list) + i] * temp
        return index
    
    def multidimensional_index(self, index_list, start_index = 0):
        if len(self._iteration_range) == 1:
            return Subscript(self, index_list[-1])
        else:
            return Subscript(Subscript(self, index_list[-1]), self.linearise_index(index_list, self._iteration_range[0:-1], start_index))

    def get_type(self):
        return TDataBlock(len(self._iteration_range), self._underlying_type)
    
    def print_cpp(self, indent_level=0):
        if self.id is None:
            return ' ' * indent_level * Node.spaces_per_tab + self._internal.print_cpp()
        else:
            return ' ' * indent_level * Node.spaces_per_tab + self.id
        
    def print_omp(self, indent_level=0):
        if self.id is None:
            return ' ' * indent_level * Node.spaces_per_tab + self._internal.print_omp()
        else:
            return ' ' * indent_level * Node.spaces_per_tab + self.id
    
    def print_sycl(self, indent_level=0):
        if self.id is None:
            return ' ' * indent_level * Node.spaces_per_tab + self._internal.print_sycl()
        else:
            return ' ' * indent_level * Node.spaces_per_tab + self.id
    

    def print_mlir(self, indent_level=0):
        if self.id is None:
            return ' ' * indent_level * Node.spaces_per_tab + self._internal.print_mlir()
        else:
            return ' ' * indent_level * Node.spaces_per_tab + "%" + self.id
        
    def print_tree(self, indent_level=0):
        if self.id is None:
            return ' ' * indent_level * Node.spaces_per_tab + f"""DataBlock:
{self._internal.print_tree(indent_level + 1)}"""
        else:
            return ' ' * indent_level * Node.spaces_per_tab + "DataBlock: " + self.id

class LogDataBlock(Statement):
    def __init__(self, dataBlock, filename):
        self._filename = filename

        self._dataBlock = dataBlock

        loops = []
        index = []

        loops.append(For([Integer(0), self._dataBlock._iteration_range[-1][1], - self._dataBlock._iteration_range[-1][0]]))
        for i in range(len(self._dataBlock._memory_range) - 2, -1, -1):
            loops.append(For([Integer(0), self._dataBlock._iteration_range[i][1] - self._dataBlock._iteration_range[i][0]]))
       
        for i in range(len(self._dataBlock._memory_range) - 1, -1, -1):
            index.append(loops[i].get_iteration_variable())                            

        loops[-1].add_statement(LogToFile(self._dataBlock.multidimensional_index(index), String("log")))
        loops[-1].add_statement(LogToFile(String("\" \""), String("log")))
        for i in range(0, len(loops) - 1):
            loops[i].add_statement(loops[i + 1])

        loops[-2].add_statement(LogToFile(String("\", \""), String("log")))
        for i in range(0, len(self._dataBlock._memory_range) - 2):
            loops[i].add_statement(LogToFile(String("\"\\n\""), String("log")))

        for loop in loops[::-1]:
            loop.close_scope()
        self._loop = loops[0]
    
    def print_cpp(self, indent_level=0):
        return f"""log.open("{self._filename.print_cpp()}");
{self._loop.print_cpp()}
log.close();
"""
    
    def print_omp(self, indent_level=0):
        return ""

    def print_sycl(self, indent_level=0):
        return ""

    def print_mlir(self, indent_level=0):
        return ""

    def print_tree(self, indent_level=0):
        return ""

class Subscript(Expression):
    def __init__(self, value: Expression, index: Expression):
        self._value = value
        self._index = index
    
    def __neg__(self):
        return UnaryOperation("-", self)

    def get_type(self):
        return self._value.get_type()

    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._value.print_cpp()}[{self._index.print_cpp()}]"
    
    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._value.print_omp()}[{self._index.print_omp()}]"

    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._value.print_sycl()}[{self._index.print_sycl()}]"

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._value.print_mlir()}[{self._index.print_mlir()}]"

    def print_tree(self, indent_level=0):
        return f"""Subscript:
{self._value.print_tree(indent_level + 1)}
{self._index.print_tree(indent_level + 1)}"""


class Reference(Expression):
    def __init__(self, expression: Expression):
        self._expression = expression
    
    def get_type(self):
        return self._expression.get_type()
    
    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "&" + self._expression.print_cpp()
    
    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "&" + self._expression.print_omp()

    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "&" + self._expression.print_sycl()

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "llvm.mlir.addressof" + self._expression.print_mlir()
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""Reference:
{self._expression.print_tree(indent_level + 1)}"""


class VectorIndex(Expression):
    def __init__(self, value: Expression, index: Expression):
        self._value = value
        self._index = index
    
    def get_type(self):
        return self._value.get_type()
    
    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._value.print_cpp()}({self._index.print_cpp()})"

    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._value.print_omp()}({self._index.print_omp()})"

    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._value.print_sycl()}({self._index.print_sycl()})"

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._value.print_mlir()}({self._index.print_mlir()})"

    def print_tree(self, indent_level=0):
        return f"""
VectorIndex: 
{self.id}
{self._value.print_tree(indent_level + 1)}
{self._index.print_tree(indent_level + 1)}"""
    

class Comparison(Expression):
    def __init__(self, operation, lhs, rhs):
        self._operation = operation
        self._lhs = lhs
        self._rhs = rhs

    def get_type(self):
        return self._lhs.get_type()

    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"({self._lhs.print_cpp()} {self._operation} {self._rhs.print_cpp()})"
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"({self._lhs.print_omp()} {self._operation} {self._rhs.print_omp()})"
    
    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"({self._lhs.print_sycl()} {self._operation} {self._rhs.print_sycl()})"
    
    def print_mlir(self, indent_level = 0):
        if type(self._lhs.get_type()) is TFloat or type(self._rhs.get_type()) is TFloat:
            if self._operation == "*":
                mlir_operation = "arith.mulf"
            elif self._operation == "/":
                mlir_operation = "arith.divf"
            elif self._operation == "+":
                mlir_operation = "arith.addf"
            elif self._operation == "-":
                mlir_operation = "arith.subf"
        else:
            if self._operation == "*":
                mlir_operation = "arith.muli"
            elif self._operation == "/":
                mlir_operation = "arith.divi"
            elif self._operation == "+":
                mlir_operation = "arith.addi"
            elif self._operation == "-":
                mlir_operation = "arith.subi"
        
        return ' ' * indent_level * Node.spaces_per_tab + f"""{mlir_operation} {self._lhs.print_mlir()}, {self._rhs.print_mlir()} : {self._lhs.get_type().print_mlir()}"""

    def print_tree(self, indent_level=0):
        return f"""
Comparison {self._operation}:
{self._lhs.print_tree(indent_level + 1)}
{self._rhs.print_tree(indent_level + 1)}"""

class BinaryOperation(Expression):
    def __init__(self, operation, lhs, rhs):
        self._operation = operation
        self._lhs = lhs
        self._rhs = rhs 

        if type(self._lhs) is BinaryOperation:
            if self._lhs._operation == "+":
                if self._lhs._rhs == Integer(0):
                    self._lhs = self._lhs._lhs
                elif self._lhs._lhs == Integer(0):
                    self._lhs = self._lhs._rhs
            elif self._lhs._operation == "-":
                if self._lhs._rhs == Integer(0):
                    self._lhs = self._lhs._lhs
            elif self._lhs._operation == "*":
                if self._lhs._rhs == Integer(1):
                    self._lhs = self._lhs._lhs
                elif self._lhs._lhs == Integer(1):
                    self._lhs = self._lhs._rhs

        if type(self._rhs) is BinaryOperation:
            if self._rhs._operation == "+":
                if self._rhs._rhs == Integer(0):
                    self._rhs = self._rhs._lhs
                elif self._rhs._lhs == Integer(0):
                    self._rhs = self._rhs._rhs
            elif self._rhs._operation == "-":
                if self._rhs._rhs == Integer(0):
                    self._rhs = self._rhs._lhs
            elif self._rhs._operation == "*":
                if self._rhs._rhs == Integer(1):
                    self._rhs = self._rhs._lhs
                elif self._rhs._lhs == Integer(1):
                    self._rhs = self._rhs._rhs

        if self._operation == "-" and type(self._rhs) is UnaryOperation and self._rhs._operation == "-":
            self._operation = "+"
            self._rhs = self._rhs._operand
        if self._operation == "+" and type(self._rhs) is UnaryOperation and self._rhs._operation == "-":
            self._operation = "-"
            self._rhs = self._rhs._operand

    def index(self, index):
        return BinaryOperation(self._operation, self._lhs.index(index), self._rhs.index(index))

    def multidimensional_index(self, index, start_index = 0):
        return BinaryOperation(self._operation, self._lhs.multidimensional_index(index, start_index), self._rhs.multidimensional_index(index, start_index))


    def __add__(self, rhs):
        return BinaryOperation("+", self, rhs)

    def __sub__(self, rhs):
        return BinaryOperation("-", self, rhs)
    
    def __mul__(self, rhs):
        return BinaryOperation("*", self, rhs)

    def __neg__(self):
        return UnaryOperation("-", self)

    def get_type(self):
        return self._lhs.get_type()

    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"({self._lhs.print_cpp()} {self._operation} {self._rhs.print_cpp()})"
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"({self._lhs.print_omp()} {self._operation} {self._rhs.print_omp()})"
    
    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"({self._lhs.print_sycl()} {self._operation} {self._rhs.print_sycl()})"
    
    def print_mlir(self, indent_level = 0):
        if type(self._lhs.get_type()) is TFloat or type(self._rhs.get_type()) is TFloat:
            if self._operation == "*":
                mlir_operation = "arith.mulf"
            elif self._operation == "/":
                mlir_operation = "arith.divf"
            elif self._operation == "+":
                mlir_operation = "arith.addf"
            elif self._operation == "-":
                mlir_operation = "arith.subf"
        else:
            if self._operation == "*":
                mlir_operation = "arith.muli"
            elif self._operation == "/":
                mlir_operation = "arith.divi"
            elif self._operation == "+":
                mlir_operation = "arith.addi"
            elif self._operation == "-":
                mlir_operation = "arith.subi"
        
        return ' ' * indent_level * Node.spaces_per_tab + f"""{mlir_operation} {self._lhs.print_mlir()}, {self._rhs.print_mlir()} : {self._lhs.get_type().print_mlir()}"""

    def print_tree(self, indent_level=0):
        return f"""
BinaryOperation {self._operation}:
{self._lhs.print_tree(indent_level + 1)}
{self._rhs.print_tree(indent_level + 1)}"""
    

class UnaryOperation(Expression):
    def __init__(self, operation, operand):
        self._operation = operation
        self._operand = operand
    
    def __add__(self, rhs):
        return BinaryOperation("+", self, rhs)

    def __sub__(self, rhs):
        return BinaryOperation("-", self, rhs)
    
    def __mul__(self, rhs):
        return BinaryOperation("*", self, rhs)

    def __neg__(self):
        if self._operation == "-":
            return self._operand
        else:
            return UnaryOperation("-", self._operand)

    def get_type(self):
        return self._operand.get_type()

    def print_cpp(self, indent_level = 0):
        return f"({self._operation} {self._operand.print_cpp()})"
    
    def print_omp(self, indent_level = 0):
        return f"({self._operation} {self._operand.print_omp()})"
    
    def print_sycl(self, indent_level = 0):
        return f"({self._operation} {self._operand.print_sycl()})"
    
    def print_mlir(self, indent_level = 0):
        if type(self._operand.get_type()) is TFloat:
            if self._operation == "+":
                return self._operand.print_mlir(indent_level)
            elif self._operation == "-":
                return ' ' * indent_level * Node.spaces_per_tab + f"""arith.negf {self._operand.print_mlir()} : {self._operand.get_type().print_mlir()}"""
        else:
            if self._operation == "+":
                return self._operand.print_mlir(indent_level)
            elif self._operation == "-":
                return ' ' * indent_level * Node.spaces_per_tab + f"""arith.negf {self._operand.print_mlir()} : {self._operand.get_type().print_mlir()}"""

    def print_tree(self, indent_level=0):
        return f"""
UnaryOperation {self._operation}:
{self._operand.print_tree(indent_level + 1)}"""

class DataBlockIndex(Expression):
    def __init__(self, dataBlock, index):
        if type(dataBlock) is Name:
            self._dataBlock = Name.variables[dataBlock.id]
        else:
            self._dataBlock = dataBlock
        
        self._index = index
        self._iteration_range = self._dataBlock._iteration_range
        self._memory_range = self._dataBlock._memory_range
    
    def index(self, index):
        return VectorIndex(self._dataBlock.index(index), self._index)

    def multidimensional_index(self, index, start_index = 0):
        return VectorIndex(self._dataBlock.multidimensional_index(index, start_index), self._index)

    def get_type(self):
        return TDataBlock(self._iteration_range, String("double"))

    def print_cpp(self, indent_level = 0):
        pass

    def print_omp(self, indent_level = 0):
        pass

    def print_sycl(self, indent_level = 0):
        pass

    def print_mlir(self, indent_level = 0):
        pass

    def print_tree(self, indent_level=0):
        pass


class DataBlockComparison(Expression):
    def __init__(self, operation, lhs, rhs):
        if type(lhs) is Name and type(lhs.get_type()) is TDataBlock:
            self._lhs = Name.variables[lhs.id]
        else:
            self._lhs = lhs
        
        if type(rhs) is Name and type(rhs.get_type()) is TDataBlock:
            self._rhs = Name.variables[rhs.id]
        else:
            self._rhs = rhs
        
        self._operation = operation
        self.requires_memory_allocation = True
        self._offset = None

        if type(self._lhs.get_type()) is TDataBlock and type(self._rhs.get_type()) is TDataBlock:
            self._iteration_range = copy.deepcopy(self._lhs._iteration_range)
            self._memory_range = copy.deepcopy(self._lhs._memory_range)
            if type(self._rhs._iteration_range[0][1]) is not BinaryOperation and  type(self._iteration_range[0][1]) is not BinaryOperation and self._rhs._iteration_range[0][1] > self._iteration_range[0][1]:
                self._iteration_range[0][1] = self._rhs._iteration_range[0][1]
            elif type(self._iteration_range[0][1]) is not BinaryOperation and self._iteration_range[0][1] == Integer(1):
                self._iteration_range[0][1] = self._rhs._iteration_range[0][1]
            if type(self._rhs._memory_range[0]) is not BinaryOperation and  type(self._memory_range[0]) is not BinaryOperation and self._rhs._memory_range[0][1] > self._memory_range[0][1]:
                self._memory_range[0] = self._rhs._memory_range[0]
            elif type(self._memory_range[0]) is not BinaryOperation and self._memory_range[0] == Integer(1):
                self._memory_range[0] = self._rhs._memory_range[0]

        elif type(self._lhs.get_type()) is TDataBlock:
            self._iteration_range = copy.deepcopy(self._lhs._iteration_range)
            self._memory_range = copy.deepcopy(self._lhs._memory_range) 
        else:
            self._iteration_range = copy.deepcopy(self._rhs._iteration_range)
            self._memory_range = copy.deepcopy(self._rhs._memory_range)
    
    def index(self, index):
        return Comparison(self._operation, self._lhs.index(index), self._rhs.index(index))

    def multidimensional_index(self, index, start_index = 0):
        return Comparison(self._operation, self._lhs.multidimensional_index(index, start_index), self._rhs.multidimensional_index(index, start_index))

    def get_type(self):
        return TDataBlock(self._iteration_range, String("int"))

    def print_cpp(self, indent_level = 0):
        pass

    def print_omp(self, indent_level = 0):
        pass

    def print_sycl(self, indent_level = 0):
        pass

    def print_mlir(self, indent_level = 0):
        pass

    def print_tree(self, indent_level=0):
        pass

class DataBlockBinaryOperation(Expression):
    def __init__(self, operation, lhs, rhs, useFunctionSyntax=False):
        if type(lhs) is Name and type(lhs.get_type()) is TDataBlock:
            self._lhs = Name.variables[lhs.id]
        else:
            self._lhs = lhs
        
        if type(rhs) is Name and type(rhs.get_type()) is TDataBlock:
            self._rhs = Name.variables[rhs.id]
        else:
            self._rhs = rhs
        
        self._operation = operation
        self.requires_memory_allocation = True
        self._useFunctionSyntax = useFunctionSyntax

        if type(self._lhs.get_type()) is TDataBlock and type(self._rhs.get_type()) is TDataBlock:
            self._iteration_range = copy.deepcopy(self._lhs._iteration_range)
            self._memory_range = copy.deepcopy(self._lhs._memory_range)
            if len(self._iteration_range) == 1:
                self._iteration_range = copy.deepcopy(self._rhs._iteration_range)
            elif type(self._rhs._iteration_range[0][1]) is not BinaryOperation and  type(self._iteration_range[0][1]) is not BinaryOperation and self._rhs._iteration_range[0][1] > self._iteration_range[0][1]:
                self._iteration_range[0][1] = self._rhs._iteration_range[0][1]
            elif type(self._iteration_range[0][1]) is not BinaryOperation and self._iteration_range[0][1] == Integer(1):
                self._iteration_range[0][1] = self._rhs._iteration_range[0][1]

            self._memory_range[0] = [Integer(0), self._iteration_range[0][1] - self._iteration_range[0][0]]

        elif type(self._lhs.get_type()) is TDataBlock:
            self._iteration_range = copy.deepcopy(self._lhs._iteration_range)
            self._memory_range = copy.deepcopy(self._lhs._memory_range) 
        else:
            self._iteration_range = copy.deepcopy(self._rhs._iteration_range)
            self._memory_range = copy.deepcopy(self._rhs._memory_range)

        self._offset = []
        for [x, y] in self._iteration_range:
            self._offset.append(x)

    
    def index(self, index):
        if self._useFunctionSyntax:
            return FunctionCall(self._operation, [self._lhs.index(index), self._rhs.index(index)])
        else:
            return BinaryOperation(self._operation, self._lhs.index(index), self._rhs.index(index))

    def multidimensional_index(self, index, start_index = 0):
        if self._useFunctionSyntax:
            return FunctionCall(self._operation, [self._lhs.multidimensional_index(index, start_index), self._rhs.multidimensional_index(index, start_index)])
        else:
            return BinaryOperation(self._operation, self._lhs.multidimensional_index(index, start_index), self._rhs.multidimensional_index(index, start_index))

    def get_type(self):
        lhs_is_double = False
        rhs_is_double = False
        if type(self._lhs.get_type()) is TDataBlock:
            lhs_is_double = (str(self._lhs.get_type().get_single()) == "double")
        else:
            lhs_is_double = (str(self._lhs.get_type()) == "double")

        if type(self._rhs.get_type()) is TDataBlock:
            rhs_is_double = (str(self._rhs.get_type().get_single()) == "double")
        else:
            rhs_is_double = (str(self._rhs.get_type()) == "double")

        if (lhs_is_double or rhs_is_double):
            return TDataBlock(self._iteration_range, String("double"))
        else:
            return TDataBlock(self._iteration_range, String("int"))

    def __sub__(self, rhs):
        return DataBlockBinaryOperation("-", self, rhs)
    
    def __add__(self, rhs):
        return DataBlockBinaryOperation("+", self, rhs)

    def print_cpp(self, indent_level = 0):
        pass

    def print_omp(self, indent_level = 0):
        pass

    def print_sycl(self, indent_level = 0):
        pass

    def print_mlir(self, indent_level = 0):
        pass

    def print_tree(self, indent_level=0):
        pass


class DataBlockUnaryOperation(Expression):
    def __init__(self, operation, dataBlock):
        if type(dataBlock) is Name and type(dataBlock.get_type()) is TDataBlock:
            self._dataBlock = Name.variables[dataBlock.id]
        else:
            self._dataBlock = dataBlock
        
        self._operation = operation
        self.requires_memory_allocation = True
        self._offset = None

        self._iteration_range = copy.deepcopy(self._dataBlock._iteration_range)
        self._memory_range = copy.deepcopy(self._dataBlock._memory_range)
    
    def index(self, index):
        return UnaryOperation(self._operation, self._dataBlock.index(index))

    def multidimensional_index(self, index, start_index = 0):
        return UnaryOperation(self._operation, self._dataBlock.multidimensional_index(index, start_index))

    def get_type(self):
        return TDataBlock(self._iteration_range, String("double"))

    def print_cpp(self, indent_level = 0):
        pass

    def print_omp(self, indent_level = 0):
        pass

    def print_sycl(self, indent_level = 0):
        pass

    def print_mlir(self, indent_level = 0):
        pass

    def print_tree(self, indent_level=0):
        pass


class DataBlockUnaryMax:
    def __init__(self, dataBlock):
        if type(dataBlock) is Name:
            self._dataBlock = Name.variables[dataBlock.id]
        else:
            self._dataBlock = dataBlock

    def set_output_variable(self, outputVariable):
        self._outputVariable = Name.variables[outputVariable.id]
        self._loop = For(self._dataBlock._iteration_range[-1])
        loops = [self._loop]
        self._loop.add_statement(Assignment(self._outputVariable.multidimensional_index([self._loop.get_iteration_variable()]), Float(0.0)))
        for i in range(len(self._dataBlock._iteration_range) - 2, -1, -1):
            loops.append(For(self._dataBlock._iteration_range[i]))
            loops[-2].add_statement(loops[-1])
        index = []
        for loop in loops[::-1]:
            index.append(loop.get_iteration_variable())
        loops[-1].add_statement(Assignment(self._outputVariable.multidimensional_index(index), Max(self._outputVariable.multidimensional_index(index), self._dataBlock.multidimensional_index(index))))
        for loop in loops:
            loop.close_scope()
    
    def get_type(self):
        return TDataBlock(self._dataBlock._iteration_range, TFloat())

    def print_cpp(self, indent_level = 0):
        return self._loop.print_cpp(indent_level)
    
    def print_omp(self, indent_level = 0):
        return self._loop.print_cpp(indent_level)
    
    def print_mlir(self, indent_level = 0):
        return self._loop.print_cpp(indent_level)
    
    def print_tree(self, indent_level=0):
        return f"""DataBlockUnaryMax:
{self._dataBlock.print_tree(indent_level + 1)}"""
        

class DataBlockMax(DataBlockBinaryOperation):
    def __init__(self, lhs, rhs):
        super().__init__("max", lhs, rhs)

    def index(self, index):
        return Max(self._lhs.index(index), self._rhs.index(index))
        
    def multidimensional_index(self, index, start_index = 0):
        return Max(self._lhs.multidimensional_index(index, start_index), self._rhs.multidimensional_index(index, start_index))
    
    def print_cpp(self, indent_level = 0):
        pass

    def print_omp(self, indent_level = 0):
        pass

    def print_sycl(self, indent_level = 0):
        pass

    def print_mlir(self, indent_level = 0):
        pass

    def print_tree(self, indent_level=0):
        pass

# Types
class TCustom(Type):
    def __init__(self, type_name):
        self._type = type_name
    
    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._type
    
    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._type

    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._type

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._type
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "TCustom: " + self._type


class TInteger(Type):
    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "int"
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "int"
    
    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "int"

    def print_mlir(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "i32"
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "TInteger"

class TBoolean(Type):
    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "bool"
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "bool"
    
    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "bool"

    def print_mlir(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "i1"
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "TBoolean"

class TFloat(Type):
    def __init__(self, reference = False):
        self._reference = reference
    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "double" + ("&" if self._reference else "")
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "double" + ("&" if self._reference else "")
    
    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "double" + ("&" if self._reference else "")

    def print_mlir(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "f64"
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "TFloat"


class TString(Type):
    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "const char*"
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "const char*"
    
    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "const char*"
    
    def print_mlir(self, indent_level=0):
        pass

    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "TString"


class TDataBlock(Type):
    def __init__(self, dimensions, underlying_type):
        self._dimensions = dimensions
        self._underlying_type = underlying_type
    
    def get_single(self):
        return self._underlying_type

    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._underlying_type.print_cpp() + "*"
    
    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._underlying_type.print_omp() + "*"

    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + self._underlying_type.print_sycl() + "*"

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""memref<?>"""
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "TDataBlock"


# Statements
class If(Statement):
    def __init__(self, boolean):
        self._boolean = boolean
        self._statements = []
    
    def add_statement(self, statement):
        self._statements.append(statement)

    def print_cpp(self, indent_level=0):
        statement_prints = [statement.print_cpp(indent_level + 1) for statement in self._statements]
        return ' ' * indent_level * Node.spaces_per_tab + f"""if constexpr ({self._boolean.print_cpp()}) {{
{os.linesep.join(statement_prints)}
""" + ' ' * indent_level * Node.spaces_per_tab + "}"
    
    def print_omp(self, indent_level=0):
        statement_prints = [statement.print_cpp(indent_level + 1) for statement in self._statements]
        return ' ' * indent_level * Node.spaces_per_tab + f"""if constexpr ({self._boolean.print_cpp()}) {{
{os.linesep.join(statement_prints)}
""" + ' ' * indent_level * Node.spaces_per_tab + "}"
    
    def print_sycl(self, indent_level=0):
        statement_prints = [statement.print_cpp(indent_level + 1) for statement in self._statements]
        return ' ' * indent_level * Node.spaces_per_tab + f"""if constexpr ({self._boolean.print_cpp()}) {{
{os.linesep.join(statement_prints)}
""" + ' ' * indent_level * Node.spaces_per_tab + "}"
    
    def print_mlir(self, indent_level=0):
        pass

    def print_tree(self, indent_level=0):
        statement_prints = [statement.print_tree(indent_level + 1) for statement in self._statements]
        return ' ' * indent_level * Node.spaces_per_tab + f"""If: 
{os.linesep.join(statement_prints)}"""

class For(Statement):
    _inuse_iteration_variables = set()
    _iteration_variable_names = ['i', 'j', 'k', 'l', 'n', 'm', 'a', 'b', 'c', 'd']

    def __init__(self, iteration_range, iteration_variable_name = None, use_scheduler = False):
        self._iteration_range = iteration_range
        self._statements = []
        self._use_scheduler = False# use_scheduler
        
        if iteration_variable_name == None:
            for variable_name in For._iteration_variable_names:
                if variable_name not in For._inuse_iteration_variables:
                    self._iteration_variable = Name(variable_name, TInteger())
                    For._inuse_iteration_variables.add(variable_name)
                    break
        else:
            self._iteration_variable = Name(iteration_variable_name, TInteger())
            For._inuse_iteration_variables.update({iteration_variable_name : self._iteration_variable})
        
    def get_interval_size(self):
        return self._iteration_range[1] - self._iteration_range[0]

    def get_iteration_variable(self):
        return self._iteration_variable

    def add_statement(self, statement):
        self._statements.append(statement)
    
    def close_scope(self):
        For._inuse_iteration_variables.remove(self._iteration_variable.id)
    
    def print_cpp(self, indent_level = 0):
        statement_prints = [statement.print_cpp(indent_level + 1) for statement in self._statements]
        if self._use_scheduler:
            return ' ' * indent_level * Node.spaces_per_tab + f"""parallelForWithSchedulerInstructions({self._iteration_variable.print_cpp()}, {self._iteration_range[1].print_cpp()}, loopParallelism) {{
{os.linesep.join(statement_prints)}
{' ' * indent_level * Node.spaces_per_tab}}}
endParallelFor"""
        else:
            return ' ' * indent_level * Node.spaces_per_tab + f"""for (int {self._iteration_variable.print_cpp()} = {self._iteration_range[0].print_cpp()}; {self._iteration_variable.print_cpp()} < {self._iteration_range[1].print_cpp()}; {self._iteration_variable.print_cpp()}++) {{
{os.linesep.join(statement_prints)}
""" + ' ' * indent_level * Node.spaces_per_tab + "}"
        
    
    def print_omp(self, indent_level = 0):
        statement_prints = [statement.print_omp(indent_level + 1) for statement in self._statements]
        return ' ' * indent_level * Node.spaces_per_tab + f"""for (int {self._iteration_variable.print_omp()} = {self._iteration_range[0].print_omp()}; {self._iteration_variable.print_omp()} < {self._iteration_range[1].print_omp()}; {self._iteration_variable.print_omp()}++) {{
{os.linesep.join(statement_prints)}
""" + ' ' * indent_level * Node.spaces_per_tab + "}"

    def print_sycl(self, indent_level = 0):
        statement_prints = [statement.print_sycl(indent_level + 1) for statement in self._statements]
        return ' ' * indent_level * Node.spaces_per_tab + f"""for (int {self._iteration_variable.print_sycl()} = {self._iteration_range[0].print_sycl()}; {self._iteration_variable.print_sycl()} < {self._iteration_range[1].print_sycl()}; {self._iteration_variable.print_sycl()}++) {{
{os.linesep.join(statement_prints)}
""" + ' ' * indent_level * Node.spaces_per_tab + "}"

    def print_mlir(self, indent_level = 0):
        statement_prints = [statement.print_mlir(indent_level + 1) for statement in self._statements]
        return ' ' * indent_level * Node.spaces_per_tab + f"""affine.for {self._iteration_variable.print_mlir()} = {self._iteration_range[0].print_mlir()} to {self._iteration_range[1].print_mlir()} {{
{os.linesep.join(statement_prints)}
""" + ' ' * indent_level * Node.spaces_per_tab + "}"
    
    def print_tree(self, indent_level = 0):
        statement_prints = [statement.print_tree(indent_level + 1) for statement in self._statements]
        return ' ' * indent_level * Node.spaces_per_tab + f"""For: 
{os.linesep.join(statement_prints)}"""

    

class Comment(Statement):
    def __init__(self, comment):
        self._comment = comment

    def get_type(self):
        return None

    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "//" + self._comment[1:]
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "//" + self._comment[1:]
    
    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "//" + self._comment[1:]

    def print_mlir(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + "//" + self._comment[1:]
    
    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + "Comment"


class FunctionCall(Statement):
    def __init__(self, id, arguments, is_offloadable = False):
        self.id = id
        self._arguments = arguments
        self._is_offloadable = is_offloadable
    
    def add_argument(self, argument: Expression):
        self._arguments.append(argument)

    def print_cpp(self, indent_level=0):
        argument_prints = [argument.print_cpp() for argument in self._arguments]
        return ' ' * indent_level * Node.spaces_per_tab + f"""{self.id}({", ".join(argument_prints)});"""
    
    def print_omp(self, indent_level=0):
        argument_prints = [argument.print_omp() for argument in self._arguments]
        return ' ' * indent_level * Node.spaces_per_tab + f"""{self.id}({", ".join(argument_prints)});"""

    def print_sycl(self, indent_level=0):
        argument_prints = [argument.print_sycl() for argument in self._arguments]
        return ' ' * indent_level * Node.spaces_per_tab + f"""{self.id}({", ".join(argument_prints)});"""

    def print_mlir(self, indent_level=0):
        argument_prints = [argument.print_cpp() for argument in self._arguments]
        return ' ' * indent_level * Node.spaces_per_tab + f"""func.call @{self.id}({", ".join(argument_prints)});"""

    def print_tree(self, indent_level=0):
        argument_prints = [argument.print_tree() for argument in self._arguments]
        return ' ' * indent_level * Node.spaces_per_tab + f"""FunctionCall:
{' ' * (indent_level + 1) * Node.spaces_per_tab + ",".join(argument_prints)}"""


class Max(Statement):
    def __init__(self, lhs, rhs):
        self._lhs = lhs
        self._rhs = rhs
    
    def print_cpp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""std::max({self._lhs.print_cpp()}, {self._rhs.print_cpp()});"""

    def print_omp(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""std::max({self._lhs.print_omp()}, {self._rhs.print_omp()});"""

    def print_sycl(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""std::max({self._lhs.print_sycl()}, {self._rhs.print_sycl()});"""

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""affine.max({self._lhs.print_mlir()}, {self._rhs.print_mlir()});"""

    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""Max:
{self._lhs.print_tree(indent_level + 1)}
{self._rhs.print_tree(indent_level + 1)}"""

class MemoryAllocation(Statement):
    memoryAllocated = []

    def __init__(self,  name: Name, object_type: Type, dimensions, specify_type = True, add_to_stack=True):
        self._type = object_type
        if type(dimensions[0]) is list:
            self._dimensions = [dimension[1] for dimension in dimensions]
        else:
            self._dimensions = dimensions
        self._name = name
        self._specify_type = specify_type

        if add_to_stack == True:
            MemoryAllocation.memoryAllocated[-1].append(self)

        if len(self._dimensions) > 1:
            size = self._dimensions[-2]
            for i in range(len(self._dimensions) - 3, -1, -1):
                size = size * self._dimensions[i]

            self._loop = For([Integer(0), self._dimensions[-1]])
            self._loop.add_statement(MemoryAllocation(Subscript(self._name, self._loop.get_iteration_variable()), self._type, [size], False, False))
            self._loop.close_scope()
    
    def print_cpp(self, indent_level=0):
        if len(self._dimensions) == 1:
            return ' ' * indent_level * Node.spaces_per_tab + f"""{self._type.print_cpp() + "* " if self._specify_type else ""}{self._name.print_cpp()} = new {self._type.print_cpp()}[{self._dimensions[0].print_cpp()}];"""
        else:
            size = self._dimensions[-2]
            for i in range(len(self._dimensions) - 3, -1, -1):
                size = size * self._dimensions[i]

            return ' ' * indent_level * Node.spaces_per_tab + f"""{self._type.print_cpp()}** {self._name.print_cpp()} = new {self._type.print_cpp()}*[{self._dimensions[-1].print_cpp()}];
{self._loop.print_cpp(indent_level)}"""
        
    def print_omp(self, indent_level=0):
        if len(self._dimensions) == 1:
            return ' ' * indent_level * Node.spaces_per_tab + f"""{self._type.print_omp() + "* " if self._specify_type else ""}{self._name.print_omp()} = new {self._type.print_omp()}[{self._dimensions[0].print_omp()}];"""
        else:
            return ' ' * indent_level * Node.spaces_per_tab + f"""{self._type.print_omp()}** {self._name.print_omp()} = new {self._type.print_omp()}*[{self._dimensions[-1].print_omp()}];
{self._loop.print_omp(indent_level)}"""
        
    def print_sycl(self, indent_level=0):
        if len(self._dimensions) == 1:
            return ' ' * indent_level * Node.spaces_per_tab + f"""{self._type.print_sycl() + "* " if self._specify_type else ""}{self._name.print_sycl()} = new {self._type.print_sycl()}[{self._dimensions[0].print_sycl()}];"""
        else:
            return ' ' * indent_level * Node.spaces_per_tab + f"""{self._type.print_sycl()}** {self._name.print_sycl()} = new {self._type.print_sycl()}*[{self._dimensions[-1].print_sycl()}];
{self._loop.print_sycl(indent_level)}"""
        
    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""{self._name.print_mlir()} = memref.alloc() : {self._name.get_type().print_mlir()}"""

    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""MemoryAllocation:
{self._name.print_tree(indent_level + 1)}""" 


class MemoryDeallocation(Statement):
    def __init__(self, allocation: MemoryAllocation):
        self._dimensions = allocation._dimensions
        self._name = allocation._name

        if len(self._dimensions) > 1:
            size = self._dimensions[-2]
            for i in range(len(self._dimensions) - 3, -1, -1):
                size = size * self._dimensions[i]

            self._loop = For([Integer(0), self._dimensions[-1]])
            self._loop.add_statement(MemoryDeallocation(MemoryAllocation(Subscript(self._name, self._loop.get_iteration_variable()), None, [size], False, False)))
            self._loop.close_scope()

    def print_cpp(self, indent_level=0):
        if len(self._dimensions) == 1:
            return ' ' * indent_level * Node.spaces_per_tab + f"""delete[] {self._name.print_cpp()};"""
        else:
            return f"""{self._loop.print_cpp(indent_level)}
{' ' * indent_level * Node.spaces_per_tab}delete[] {self._name.print_cpp()};"""

    def print_omp(self, indent_level=0):
        if len(self._dimensions) == 1:
            return ' ' * indent_level * Node.spaces_per_tab + f"""delete[] {self._name.print_omp()};"""
        else:
            return f"""{self._loop.print_omp(indent_level)}
{' ' * indent_level * Node.spaces_per_tab}delete[] {self._name.print_omp()};"""

    def print_sycl(self, indent_level=0):
        if len(self._dimensions) == 1:
            return ' ' * indent_level * Node.spaces_per_tab + f"""delete[] {self._name.print_sycl()};"""
        else:
            return f"""{self._loop.print_sycl(indent_level)}
{' ' * indent_level * Node.spaces_per_tab}delete[] {self._name.print_sycl()};"""

    def print_mlir(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""memref.dealloc {self._name.print_mlir()} : {self._name.get_type().print_mlir()}"""

    def print_tree(self, indent_level=0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""MemoryDeallocation:
{self._name.print_tree(indent_level + 1)}""" 
    

class Construction(Statement):
    def __init__(self, name: Name, expression: Expression):
        self._name = name
        self._expression = expression
        Name.variables.update({self._name.id: expression})
    
    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._name.get_type().print_cpp()} {self._name.print_cpp()} = {self._expression.print_cpp()};"

    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._name.get_type().print_omp()} {self._name.print_omp()} = {self._expression.print_omp()};"

    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._name.get_type().print_sycl()} {self._name.print_sycl()} = {self._expression.print_sycl()};"

    def print_mlir(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._name.print_mlir()} = {self._expression.print_mlir()} : {self._name.get_type().print_mlir()}"

    def print_tree(self, indent_level=0):
        return f"""
Construction: 
{self._name.print_tree(indent_level + 1)}
{self._expression.print_tree(indent_level + 1)}"""


class DataBlockConstructionFromExisting(Statement):
    def __init__(self, name: Name, dataBlock: DataBlock):
        self._name = name
        self._dataBlock = dataBlock
        self._dataBlock.id = name.id
        Name.variables.update({self._name.id: self._dataBlock})

    def print_cpp(self, indent_level = 0):
        type_print = self._name.get_type().print_cpp()
        if len(self._dataBlock._iteration_range) > 1:
            type_print += "*"
        return ' ' * indent_level * Node.spaces_per_tab + f"{type_print} {self._name.print_cpp()} = {self._dataBlock._internal.print_cpp()};"
    
    def print_omp(self, indent_level = 0):
        type_print = self._name.get_type().print_omp()
        if len(self._dataBlock._iteration_range) > 1:
            type_print += "*"
        return ' ' * indent_level * Node.spaces_per_tab + f"{type_print} {self._name.print_omp()} = {self._dataBlock._internal.print_omp()};"
    
    def print_sycl(self, indent_level = 0):
        type_print = self._name.get_type().print_sycl()
        if len(self._dataBlock._iteration_range) > 1:
            type_print += "*"
        return ' ' * indent_level * Node.spaces_per_tab + f"{type_print} {self._name.print_sycl()} = {self._dataBlock._internal.print_sycl()};"
    
    def print_mlir(self, indent_level = 0):
        type_print = self._name.get_type().print_cpp()
        if len(self._dataBlock._iteration_range) > 1:
            type_print = "!llvm.ptr"
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._name.print_cpp()} = {self._dataBlock._internal.print_mlir()} : {type_print}"

    def print_tree(self, indent_level=0):
        return f"""
DataBlockConstructionFromExisting: 
{self._name.print_tree(indent_level + 1)}
{self._dataBlock._internal.print_tree(indent_level + 1)}"""
    

class DataBlockConstructionFromOperation:
    def __init__(self, name: Name, dataBlockOperation):
        self._name = name
        self._dataBlock = DataBlock(dataBlockOperation._iteration_range, None, False, self._name.id, underlying_type=dataBlockOperation.get_type().get_single())
        Name.variables.update({self._name.id: self._dataBlock})
        if type(dataBlockOperation) is DataBlockBinaryOperation or type(dataBlockOperation) is DataBlockUnaryOperation or type(dataBlockOperation) is DataBlockMax or type(dataBlockOperation) is DataBlockComparison or type(dataBlockOperation._internal) is String:
            self._operation = dataBlockOperation
        else:
            self._operation = dataBlockOperation._internal

        self.memoryAllocation = MemoryAllocation(self._name, self._dataBlock.get_type().get_single(), self._dataBlock._memory_range)
        self.assignment = DataBlockAssignment(self._name, self._operation)
        
    def print_cpp(self, indent_level = 0):
        return f"""{self.memoryAllocation.print_cpp(indent_level)}
{self.assignment.print_cpp(indent_level)}"""
    
    def print_omp(self, indent_level = 0):
        return f"""{self.memoryAllocation.print_omp(indent_level)}
{self.assignment.print_omp(indent_level)}"""

    def print_sycl(self, indent_level = 0):
        return f"""{self.memoryAllocation.print_sycl(indent_level)}
{self.assignment.print_sycl(indent_level)}"""
    
    def print_mlir(self, indent_level = 0):
        return f"""{self.memoryAllocation.print_mlir(indent_level)}
{self.assignment.print_mlir(indent_level)}"""
    
    def print_tree(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""DataBlockConstructionFromOperation:
{self._name.print_tree(indent_level + 1)}
{self._dataBlock.print_tree(indent_level + 1)}"""


class DataBlockConstructionFromFunction(Statement):
    def __init__(self, name: Name, dataBlock: DataBlock):
        self._name = name
        self._dataBlock = dataBlock
        self._dataBlock.id = name.id
        Name.variables.update({self._name.id: self._dataBlock})

        self.memoryAllocation = MemoryAllocation(self._name, self._dataBlock.get_type().get_single(), self._dataBlock._memory_range)
        self.initialisation = For(self._dataBlock._memory_range[-1])
        

        inner_loops = []
        for i in range(len(self._dataBlock._iteration_range) - 2, 0, -1):
            inner_loop = For([Integer(0), self._dataBlock._memory_range[i][1] - self._dataBlock._memory_range[i][0]])
            inner_loops.append(inner_loop)
        if type(self._dataBlock._internal) is not FunctionCall:
            inner_loop = For(self._dataBlock._memory_range[0])
            inner_loops.append(inner_loop)

        index = []
        for i in range(len(inner_loops)):
            index.append(inner_loops[-(i + 1)].get_iteration_variable())
        index.append(self.initialisation.get_iteration_variable())
        
        output_offset = [Integer(0) for i in self._dataBlock._iteration_range[1:]]
        input_offset = [self._dataBlock._iteration_range[i][0] - Name.variables[self._dataBlock._internal._arguments[0].id]._iteration_range[i][0] for i in range(1, len(self._dataBlock._iteration_range))]

        if type(self._dataBlock._internal) is FunctionCall:
            functionCall = FunctionCall(self._dataBlock._internal.id, [])
            functionCall._is_offloadable = self._dataBlock._internal._is_offloadable
            
            functionCall.add_argument(Reference(self._dataBlock._internal._arguments[0].multidimensional_index(index, 1)))

            for argument in self._dataBlock._internal._arguments[1:]:
                if type(argument.get_type()) is TDataBlock:
                    offset = [i[0] for i in Name.variables[argument.id]._iteration_range[1:]]
                    if len(Name.variables[argument.id]._iteration_range) == 1:
                        functionCall.add_argument(Name.variables[argument.id].multidimensional_index(index, 1))
                    else:
                        functionCall.add_argument(Reference(Name.variables[argument.id].multidimensional_index(index, 1)))
                else:
                    functionCall.add_argument(argument)
        
            functionCall.add_argument(Reference(self._dataBlock.multidimensional_index(index, 1)))
            if functionCall._is_offloadable:
                functionCall.add_argument(String("Solver::Offloadable::Yes"))
        else:
            functionCall = Assignment(self._dataBlock.multidimensional_index(index), self._dataBlock._internal)
        
        self.initialisation.add_statement(inner_loops[0])

        for i in range (0, len(inner_loops) - 1):
            inner_loops[i].add_statement(inner_loops[i + 1])
        inner_loops[-1].add_statement(functionCall)

        for i in range(len(inner_loops) - 1, -1, -1):
            inner_loops[i].close_scope()
        self.initialisation.close_scope()

    def print_cpp(self, indent_level = 0):
        return f"""{self.memoryAllocation.print_cpp(indent_level)}
{self.initialisation.print_cpp(indent_level)}"""
    
    def print_omp(self, indent_level = 0):
        return f"""{self.memoryAllocation.print_omp(indent_level)}
{' ' * indent_level * Node.spaces_per_tab}#pragma omp parallel for simd collapse(Dimensions + 1) schedule(static, 1)
{self.initialisation.print_omp(indent_level)}"""
    
    def print_sycl(self, indent_level = 0):
        return f"""{self.memoryAllocation.print_sycl(indent_level)}
{self.initialisation.print_sycl(indent_level)}"""

    def print_mlir(self, indent_level = 0):
        return f"""{self.memoryAllocation.print_mlir(indent_level)}
{self.initialisation.print_mlir(indent_level)}"""

    def print_tree(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""DataBlockConstructionFromFunction:
{self._name.print_tree(indent_level + 1)}
{self._dataBlock.print_tree(indent_level + 1)}"""

class Assignment(Statement):
    def __init__(self, lhs: Expression, rhs: Expression):
        self._lhs = lhs
        self._rhs = rhs
    
    def print_cpp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._lhs.print_cpp()} = {self._rhs.print_cpp()};"
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._lhs.print_omp()} = {self._rhs.print_omp()};"

    def print_sycl(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._lhs.print_sycl()} = {self._rhs.print_sycl()};"

    def print_mlir(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"{self._lhs.print_mlir()} = {self._rhs.print_mlir()} : {self._lhs.get_type().print_mlir()}"
    
    def print_tree(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""Assignment:
{self._lhs.print_tree(indent_level + 1)}
{self._rhs.print_tree(indent_level + 1)}"""
    

class DataBlockAssignment(Statement):
    def __init__(self, lhs: Expression, rhs: Expression):
        if type(lhs) is Name:
            self._lhs = Name.variables[lhs.id]
            self._lhs._id = lhs.id
        else:
            self._lhs = lhs
        
        if type(rhs) is Name:
            self._rhs = Name.variables[rhs.id]
        else:
            self._rhs = rhs

        loops = []
        index = []

        loops.append(For([Integer(0), self._lhs._iteration_range[-1][1] - self._lhs._iteration_range[-1][0]]))
        for i in range(len(self._lhs._memory_range) - 2, -1, -1):
            loops.append(For([Integer(0), self._lhs._iteration_range[i][1] - self._lhs._iteration_range[i][0]]))
       
        for i in range(len(self._lhs._memory_range) - 1, -1, -1):
            index.append(loops[i].get_iteration_variable())              

        if type(self._rhs.get_type()) is TDataBlock:
            rhs_offset = []
            for i in range(len(self._lhs._iteration_range)):
                lhs_start = self._lhs._iteration_range[i][0]
                rhs_start = self._rhs._iteration_range[i][0]
                if type(rhs_start.get_type()) is TDataBlock:
                    rhs_start = Subscript(rhs_start, lhs_start)
                rhs_offset.append(lhs_start - rhs_start)
        else:
            rhs_offset = [Integer(0) for i in self._lhs._iteration_range]

        lhs_offset = [Integer(0) for i in self._lhs._iteration_range]

        loops[-1].add_statement(Assignment(self._lhs.multidimensional_index(index), self._rhs.multidimensional_index(index)))
        for i in range(0, len(loops) - 1):
            loops[i].add_statement(loops[i + 1])

        for loop in loops[::-1]:
            loop.close_scope()
        self._loop = loops[0]
        self._numDimensions = len(loops)
    
    def print_cpp(self, indent_level = 0):
        return self._loop.print_cpp(indent_level)
    
    def print_omp(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""#pragma omp parallel for simd collapse(Dimensions + 1) schedule(static, 1)
{self._loop.print_omp(indent_level)}"""
    
    def print_sycl(self, indent_level = 0):
        return self._loop.print_sycl(indent_level)

    def print_mlir(self, indent_level = 0):
        return self._loop.print_mlir(indent_level)
    
    def print_tree(self, indent_level = 0):
        return ' ' * indent_level * Node.spaces_per_tab + f"""DataBlockAssignment:
{self._lhs.print_tree(indent_level + 1)}
{self._rhs.print_tree(indent_level + 1)}"""