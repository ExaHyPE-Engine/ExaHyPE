import ast_comments as ast, inspect
from exahype2.kerneldsl import SyntaxTree
import copy

class Parser:
    def _parse_constant(self, constant: ast.Constant, type_name = None):
        if type(constant.value) is int:
            return SyntaxTree.Integer(constant.value)
        if type(constant.value) is str:
            return SyntaxTree.String(constant.value)
        if type(constant.value) is float:
            return SyntaxTree.Float(constant.value)

    def _parse_call(self, call: ast.Call):
        function = call.func
        if type(function) is ast.Name:
            if function.id == "DataBlock":
                return self._parse_patch_constructor(call)
            elif function.id == "Integer":
                return SyntaxTree.Integer(0, call.args[0].value)
            elif function.id == "FloatReference":
                return SyntaxTree.Float(0.0, call.args[0].value, reference = True)
            elif function.id == "logDataBlock":
                return SyntaxTree.LogDataBlock(self._parse_expression(call.args[0]), self._parse_constant(call.args[1]))
            elif function.id == "max":
                if len(call.args) > 1:
                    return SyntaxTree.DataBlockMax(self._parse_expression(call.args[0]), self._parse_expression(call.args[1]))
                else:
                    return SyntaxTree.DataBlockUnaryMax(self._parse_expression(call.args[0]))
            else:
                return SyntaxTree.DataBlockIndex(SyntaxTree.Name(function.id), self._parse_expression(call.args[0]))

    def _parse_patch_constructor(self, constructor: ast.Call):
        iteration_range = []
        for interval in constructor.args[0].elts:
            iteration_range.append([self._parse_expression(interval.elts[0]), self._parse_expression(interval.elts[1])])
        
        if type(constructor.args[1]) is ast.Call:
            internal = SyntaxTree.String(constructor.args[1].args[0].value)
            function_arguments = []
            for arg in constructor.args[1].args[1:]:
                function_arguments.append(self._parse_expression(arg))
            if SyntaxTree.FunctionDefinition._stateless:
                internal = SyntaxTree.FunctionCall("SolverType::" + constructor.args[1].args[0].value, function_arguments, True)
            else:
                internal = SyntaxTree.FunctionCall(constructor.args[1].args[0].value, function_arguments)
            return SyntaxTree.DataBlock(iteration_range, internal, True) 
        elif type(constructor.args[1]) is ast.Constant:
            if type(constructor.args[1].value) is str:
                internal = SyntaxTree.String(constructor.args[1].value)
                if len(constructor.args) > 2:
                    return SyntaxTree.DataBlock(iteration_range, internal, False, underlying_type=SyntaxTree.String(constructor.args[2].value))
                else:
                    return SyntaxTree.DataBlock(iteration_range, internal, False)
            else:
                internal = self._parse_constant(constructor.args[1])
                return SyntaxTree.DataBlock(iteration_range, internal, True)

    def _parse_unary_operation(self, unary_operation: ast.UnaryOp):
        operand = self._parse_expression(unary_operation.operand)

        if type(unary_operation.op) is ast.USub:
            if type(operand.get_type()) is SyntaxTree.TDataBlock:
                return SyntaxTree.DataBlockUnaryOperation("-", operand)
            else:
                return SyntaxTree.UnaryOperation("-", self._parse_expression(unary_operation.operand))
                         

    def _parse_binary_operation(self, binary_operation: ast.BinOp):
        lhs = self._parse_expression(binary_operation.left)
        rhs = self._parse_expression(binary_operation.right)
        if type(binary_operation.op) is ast.Add:
            operation = "+"
        elif type(binary_operation.op) is ast.Mult:
            operation = "*"
        elif type(binary_operation.op) is ast.Sub:
            operation = "-"
        elif type(binary_operation.op) is ast.Div:
            operation = "/"

        if type(lhs.get_type()) is SyntaxTree.TDataBlock or type(rhs.get_type()) is SyntaxTree.TDataBlock:
            return SyntaxTree.DataBlockBinaryOperation(operation, lhs, rhs)
        else:
            return SyntaxTree.BinaryOperation(operation, lhs, rhs)

    def _parse_subscript(self, subscript: ast.Subscript):
        value = self._parse_expression(subscript.value)
        if type(value.get_type()) is SyntaxTree.TDataBlock:
            offset = []
            if type(subscript.slice) is ast.Tuple:
                for axis_offset in subscript.slice.elts:
                    if type(axis_offset) is ast.Slice:
                        offset.append([self._parse_expression(axis_offset.lower), self._parse_expression(axis_offset.upper)])
                    else:
                        offset.append([self._parse_expression(axis_offset), self._parse_expression(axis_offset) + SyntaxTree.Integer(1)])

            elif type(axis_offset) is ast.Slice:
                offset.append([self._parse_expression(axis_offset.lower), self._parse_expression(axis_offset.upper)])
            else:
                offset.append([self._parse_expression(axis_offset), self._parse_expression(axis_offset) + SyntaxTree.Integer(1)])

            return SyntaxTree.Name.variables[value.id].offset(offset)

    def _parse_comparison(self, comparison: ast.Compare):
        lhs = self._parse_expression(comparison.left)
        rhs = self._parse_expression(comparison.comparators[0])
        if type(comparison.ops[0]) is ast.Gt:
            return SyntaxTree.DataBlockComparison(">", lhs, rhs)
        if type(comparison.ops[0]) is ast.Lt:
            return SyntaxTree.DataBlockComparison(">", lhs, rhs)
        if type(comparison.ops[0]) is ast.Eq:
            return SyntaxTree.DataBlockComparison("==", lhs, rhs)

    def _parse_expression(self, expression):
        if type(expression) is ast.Constant:
            return self._parse_constant(expression)
        if type(expression) is ast.UnaryOp:
            return self._parse_unary_operation(expression)
        if type(expression) is ast.Compare:
            return self._parse_comparison(expression)
        if type(expression) is ast.BinOp:
            return self._parse_binary_operation(expression)
        if type(expression) is ast.Name:
            return SyntaxTree.Name(expression.id)
        if type(expression) is ast.Call:
            return self._parse_call(expression)
        if type(expression) is ast.Subscript:
            return self._parse_subscript(expression)
        if expression is None:
            return None

    def _parse_assignment_with_declaration(self, assignment_with_declaration: ast.Assign):
        value = self._parse_expression(assignment_with_declaration.value)
        id = assignment_with_declaration.targets[0].id
        if type(value.get_type()) is SyntaxTree.TDataBlock:
            if type(value) is SyntaxTree.DataBlockUnaryMax:
                value.set_output_variable(SyntaxTree.Name(id, value.get_type()))
                return value
            if value.requires_memory_allocation == False:
                return SyntaxTree.DataBlockConstructionFromExisting(SyntaxTree.Name(id, value.get_type()), value)
            else:
                if type(value) is SyntaxTree.DataBlockBinaryOperation or type(value) is SyntaxTree.DataBlockMax or type(value) is SyntaxTree.DataBlockComparison or type(value._internal) is not SyntaxTree.FunctionCall:
                    return SyntaxTree.DataBlockConstructionFromOperation(SyntaxTree.Name(id, value.get_type()), value)
                else:
                    return SyntaxTree.DataBlockConstructionFromFunction(SyntaxTree.Name(id, value.get_type()), value)
        else:
            return SyntaxTree.Construction(SyntaxTree.Name(id, value.get_type()), value)
    
    def _parse_assignment_without_declaration(self, assignment_with_declaration: ast.Assign):
        value = self._parse_expression(assignment_with_declaration.value)
        target = self._parse_expression(assignment_with_declaration.targets[0])

        if type(value.get_type()) is SyntaxTree.TDataBlock or type(target.get_type()) is SyntaxTree.TDataBlock:
            if type(value) is SyntaxTree.DataBlockUnaryMax:
                value.set_output_variable(target)
                return value
            return SyntaxTree.DataBlockAssignment(target, value)
        else:
            return SyntaxTree.Assignment(target, value)

    def _parse_assignment(self, assignment: ast.Assign):
        target = assignment.targets[0]
        if type(target) is ast.Subscript:
            return self._parse_assignment_without_declaration(assignment)
        elif target.id not in SyntaxTree.Name.variables:
            return self._parse_assignment_with_declaration(assignment)
        else:
            return self._parse_assignment_without_declaration(assignment)

    def _parse_if(self, statement: ast.If):
        if_node = SyntaxTree.If(self._parse_expression(statement.test))
        SyntaxTree.MemoryAllocation.memoryAllocated.append([])

        for s in statement.body:
            if_node.add_statement(self._parse_statement(s))
        for allocation in SyntaxTree.MemoryAllocation.memoryAllocated[-1]:
                if_node.add_statement(SyntaxTree.MemoryDeallocation(allocation))

        SyntaxTree.MemoryAllocation.memoryAllocated.pop()
        return if_node

    def _parse_statement(self, statement: ast.Expr):  
        if type(statement) is ast.Assign:
            return self._parse_assignment(statement)
        if type(statement) is ast.Comment:
            return SyntaxTree.Comment(statement.value)
        if type(statement) is ast.If:
            return self._parse_if(statement)
        if type(statement) is ast.Expr:
            return self._parse_expression(statement.value)
    
    def _parse_function(self, function: ast.FunctionDef, template, functor_arguments, namespaces, stateless, use_accelerator):
        id = function.name
        return_type = SyntaxTree.TCustom(function.returns.value) if function.returns is not None else SyntaxTree.TCustom(function.returns)
        parsed_function = SyntaxTree.FunctionDefinition(id, return_type, template, namespaces, stateless)

        if use_accelerator:
            parsed_function.add_argument(SyntaxTree.Argument("targetDevice", SyntaxTree.TInteger()))
            SyntaxTree.Node.use_accelerator = True
        for argument in function.args.args:
            parsed_function.add_argument(SyntaxTree.Argument(argument.arg, SyntaxTree.TCustom(argument.annotation.value) if argument.annotation is not None else SyntaxTree.TCustom(argument.annotation)))
            
        if stateless:
            parsed_function.id = parsed_function.id + "Stateless"
            parsed_function._template.insert(0, SyntaxTree.Argument("SolverType", SyntaxTree.TCustom("class")))
        else:
            for functor in functor_arguments:
                parsed_function.add_argument(functor)
        
        SyntaxTree.MemoryAllocation.memoryAllocated.append([])
        for statement in function.body:
            parsed_function.add_statement(self._parse_statement(statement))
        for allocation in SyntaxTree.MemoryAllocation.memoryAllocated[-1]:
            parsed_function.add_statement(SyntaxTree.MemoryDeallocation(allocation))
        SyntaxTree.MemoryAllocation.memoryAllocated.pop()

        return parsed_function

    def parse(self, function, template_arguments, functor_arguments, namespace = [], stateless = False, use_accelerator = False):
        SyntaxTree.Name.variables.clear()
        SyntaxTree.For._inuse_iteration_variables.clear()
        SyntaxTree.Node.use_accelerator = False
        SyntaxTree.FunctionDefinition._syclDataToCopy.clear()

        module = ast.parse(inspect.getsource(function))
        function_definition = module.body[0]
        assert isinstance(function_definition, ast.FunctionDef)
        for argument in template_arguments:
            if type(argument._type) is SyntaxTree.TInteger:
                SyntaxTree.Name.variables.update({argument.id: SyntaxTree.Integer(99)})
            if type(argument._type) is SyntaxTree.TBoolean:
                SyntaxTree.Name.variables.update({argument.id: SyntaxTree.Boolean(False)})
            if type(argument._type) is SyntaxTree.TFloat:
                SyntaxTree.Name.variables.update({argument.id: SyntaxTree.Float(99.0)})
            
        template_arguments_copy = copy.deepcopy(template_arguments)
        root = self._parse_function(function_definition, template_arguments_copy, functor_arguments, namespace, stateless, use_accelerator)
        return root