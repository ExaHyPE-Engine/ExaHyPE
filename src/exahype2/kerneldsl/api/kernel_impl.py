import warnings

class Enumerator:
    def __init__(self,its=[[0]],iterators=["patch","i","j","k"],depth=0):
        self.its = its
        self.start = [i[0] for i in its]
        self.end = [i[-1] for i in its]
        self.iterators = iterators
        self.size = []
        for i in range(len(self.start)):
            self.size.append(self.end[i]-self.start[i]+1)
        self.depth = depth
        self.max_depth = len(self.start)

        self.i = self.iterators[depth]
        self.s = self.size[depth]
        self.S = self.start[depth]
        self.E = self.end[depth] + 1

        self.shifted = False
        self.offset = ["" for _ in range(len(self.iterators))]
    
    def lower(self):
        if self.depth == self.max_depth-1:
            self.depth = -1
        self.depth += 1
        self.i = self.iterators[self.depth]
        self.s = self.size[self.depth]
        self.S = self.start[self.depth]
        self.E = self.end[self.depth] + 1
        return self

    def upper(self):
        self.depth -= 1
        self.i = self.iterators[self.depth]
        self.s = self.size[self.depth]
        self.S = self.start[self.depth]
        self.E = self.end[self.depth] + 1
        return self

    def index(self):
        string = "["
        string += self.iterators[0]+self.offset[0]
        for i in range(len(self.iterators)-1):
            string += "," + self.iterators[i+1] + self.offset[i+1]
        string += "]"
        return string

    def shift(self,enum):
        self.shifted=True
        # self.offset = ["" for _ in range(len(self.iterators))]
        for i in range(len(self.iterators)):
                temp = enum.start[i] - self.start[i]
                if temp == 0:
                    self.offset[i] = ""
                elif temp > 0:
                    self.offset[i] = f"+{temp}"
                elif temp < 0:
                    self.offset[i] = f"-{temp}"
        # return self.shifted, self.offset
            
    def fetch(self,enum):
        its,iterators,depth = enum[0],enum[1],enum[2]
        self.its = its
        self.start = [i[0] for i in its]
        self.end = [i[-1] for i in its]
        self.iterators = iterators
        self.size = []
        for i in range(len(self.start)):
            self.size.append(self.end[i]-self.start[i]+1)
        self.depth = depth
        self.max_depth = len(self.start)

        self.i = iterators[depth]
        self.s = self.size[depth]
        self.S = self.start[depth]
        self.E = self.end[depth] + 1
        return self

    def RETURN(self):
        return [self.its,self.iterators,self.depth]

class impl:
    def __init__(self,xhpp):
        self.file = xhpp.file
        self.debug = xhpp.debug
        self.code = xhpp.code
        
        self.INDENT = 0
        self.SCOPE = 0

    def namespace(self,location):
        self.code += f"\nnamespace {location}" + "{" + "\n"
        self.SCOPE += 1
        self.INDENT += 1

    def indent(self,val=0,force=False):
        self.INDENT += val
        if val == 0 or force:
            self.code += (self.INDENT * "\t")

    def alloc(self,name,enum):
        size = enum.size
        if self.debug: print("Hello from impl.alloc")
        self.indent()
        self.code += name
        self.code += f" = new double[{size[0]}"
        for _ in range(len(size)-1):
            self.code += f",{size[_+1]}"
        self.code += "];\n"

    def loop(self,name,expression,enums,below):
        if self.debug: print("Hello from impl.loop")
        enum = enums[name]
        enum = Enumerator().fetch(enum)
        self.indent()
        self.code += f"for (int {enum.i} = {0}; {enum.i} < {enum.E-enum.S}; {enum.i}++)" + " {\n"
        if below > 0:
            self.indent(1)
            enum.lower()
            enums[name] = enum.RETURN()
            self.loop(name,expression,enums,below-1)
            self.indent(-1)
        else:
            self.indent(1,True)
            self.code += name + enum.index() + " = " + self.express(expression,enum,enums) + ";\n"
            self.indent(-1)
        self.indent()
        self.code += "}\n"

    def express(self,expression,enum,enums):
        var_start = 0
        string = ""
        for i in range(len(expression)):
            char = expression[i]
            if char == "(":
                string += char
                var_start = i+1
            elif char == "[" and expression[i+1] == "]":
                temp_enum = expression[var_start:i]
                if temp_enum in enums:
                    temp_enum = Enumerator().fetch(enums[temp_enum])
                else:
                    temp_enum = enum
                temp_enum.shift(enum)
                string += temp_enum.index()
            elif char == "]" and expression[i-1] == "[":
                pass
            else:
                string += char
        return string

    def stencil(self,out_name,names,stencil,scales,var_list,enums,below,special=None,enum=None):
        if self.debug: print("Hello from impl.stencil")
        if enum==None:
            enum = Enumerator().fetch(enums[out_name])
        self.indent()
        self.code += f"for (int {enum.i} = {enum.S}; {enum.i} < {enum.E}; {enum.i}++)" + " {\n"
        if below > 0:
            self.indent(1)
            enum.lower()
            enums[out_name] = enum.RETURN()
            self.stencil(out_name,names,stencil,scales,var_list,enums,below-1)
            self.indent(-1)
        else:
            self.indent(1)
            lhs = out_name + enum.index()
            temp = ""
            inside = False
            dim = 0
            scale_i = 0
            for char in stencil:
                if char == "[":
                    inside = True
                    dim = 0
                    self.indent()
                    self.code += lhs
                    if type(scales) == list:
                        scale = scales[scale_i]
                    else: scale = scales
                    if scale >= 0:
                        self.code += " += "
                    else:
                        self.code += " -= "
                    self.code += f"{abs(scale)} * " + names[int(temp)] + "["
                    temp = ""
                elif char == "]":
                    inside = False
                    self.code += temp
                    self.code += "]\n"
                    temp = ""
                    scale_i += 1
                elif char == "-":
                    temp += char
                elif char.isnumeric():
                    if inside:
                        if dim > 0:
                                self.code += ","
                        if char == "0":
                            self.code += enum.iterators[dim]
                        else:
                            if len(temp) == 0:
                                temp += "+"
                            self.code += enum.iterators[dim] + temp + char
                        dim += 1
                        temp = ""
                    else:
                        temp += char
                    
            self.indent(-1)
        self.indent()
        self.code += "}\n"

    def __del__(self):
        self.code += '\n'
        # with open(self.file,'w') as FOUT:
        #     FOUT.write(self.code)
    