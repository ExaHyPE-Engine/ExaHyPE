class Enumerator:
    def __init__(self,start,end,iterators=["i","j","k"]):
        self.start = start
        self.end = end
        self.iterators = iterators
        self.size = []
        for i in range(len(start)):
            self.size.append(end[i]-start[i])
        self.depth = 0
        self.max_depth = len(start)

        self.i = iterators[0]
        self.s = self.size[0]
        self.S = start[0]
        self.E = end[0]
    
    def lower(self):
        if self.depth == self.max_depth-1:
            self.depth = -1
        self.depth += 1
        self.i = self.iterators[self.depth]
        self.s = self.size[self.depth]
        self.S = self.start[self.depth]
        self.E = self.end[self.depth]
        return self

    def index(self):
        string = "["
        string += self.iterators[0]
        for i in range(len(self.iterators)-1):
            string += "," + self.iterators[i+1]
        string += "]"
        return string

class impl:
    def __init__(self,upper):
        self.file = upper.file
        self.debug = upper.debug
        self.code = upper.code
        
        self.INDENT = 0

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

    def loop(self,name,expression,enum,below):
        if self.debug: print("Hello from impl.loop")
        self.indent()
        self.code += f"for (int {enum.i} = {enum.S}; {enum.i} < {enum.E}; {enum.i}++)" + " {\n"
        if below > 0:
            self.indent(1)
            self.loop(name,expression,enum.lower(),below-1)
            self.indent(-1)
        else:
            self.indent(1,True)
            self.code += name + enum.index() + " = " + self.express(expression,enum) + ";\n"
            self.indent(-1)
        self.indent()
        self.code += "}\n"

    def express(self,expression,enum):
        string = ""
        for char in expression:
            if char == "[":
                string += enum.index()
            elif char == "]":
                pass
            else:
                string += char
        return string

    def stencil(self,out_name,names,stencil,scales,enum,below):
        if self.debug: print("Hello from impl.stencil")
        self.indent()
        self.code += f"for (int {enum.i} = {enum.S}; {enum.i} < {enum.E}; {enum.i}++)" + " {\n"
        if below > 0:
            self.indent(1)
            self.stencil(out_name,names,stencil,scales,enum.lower(),below-1)
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
                    scale = scales[scale_i]
                    if scale >= 0:
                        self.code += " += "
                    else:
                        self.code += " -= "
                    self.code += f"{abs(scales[scale_i])} * " + names[int(temp)] + "["
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
    