from kernel_impl import impl, Enumerator

class kernel:
    def __init__(self, patch, dim, patch_size, n_patches=1, halo_size=1, file="test.cpp",debug=False,namespace=None):
        self.d = dim                    #number of dimensions
        self.np = n_patches
        self.p = patch_size             #size of patch
        self.h = halo_size              #size of halo
        
        self.file = file                #output file destination
        self.debug = debug              #debug mode
        self.patch = patch              #target to update
        self.code = ""                  #to output to file
        self.scope = 0 
        self.pre = ""

        self.names = []                 #list of variable names
        self.external_var = []          #list of external variables to be included
        self.enums = {}                 #dictionary of enumerator objects
        if namespace != None:
            k = impl(self)
            k.namespace(namespace)
            self.pre = k.code
            self.scope = k.SCOPE
    
    def halo(self,which=[1,1,1],name=None):
        its = [[_ for _ in range(0,self.np)]]
        for dim in range(self.d):
            start = self.h - which[dim]
            if start < 0:
                raise Exception("halo requested too large, increase halo_size")
            end = self.p + self.h + which[dim]
            its.append([_ for _ in range(start,end)])
        iterators = ["patch","i","j","k"]
        enum = Enumerator(its,iterators[:self.d+1])
        # if name != None:
        #     k = impl(self)
        #     k.alloc(name,enum)
        #     self.code = k.code
        #     self.names.append(name)
        #     self.enums[name] = enum.RETURN()
        return enum

    def Evaluate(self, name, expression, enum=None):
        if type(enum) == type(None):
            enum = self.halo([self.h for _ in range(self.d)])
        k = impl(self)
        if name not in self.names:
            k.alloc(name,enum)
            self.names.append(name)
            self.enums[name] = enum.RETURN()
        k.loop(name,expression,self.enums,self.d)
        self.code = k.code
        return name

    def Stencil(self, out_name, names, stencil, scale=1,var_list=[],enum=None,special=None):
        if type(enum) == type(None):
            enum = self.halo([0,0,0])
        k = impl(self)
        if out_name not in self.names:
            k.alloc(out_name,enum)
            self.names.append(out_name)
            self.enums[out_name] = enum
        k.stencil(out_name,names,stencil,scale,var_list,self.enums,self.d,special,enum)
        self.code = k.code
        return out_name

    def include(self,file_list):
        # string = ""
        for item in file_list:
            self.pre = '#include "' + item + '"\n' +self.pre
        # self.pre = string + "\n" + self.code

    def external(self,var_list):
        for var in var_list:
            self.external_var.append(var)

    def __del__(self):
        self.pre += f"\nvoid time_step(double* {self.patch}"
        self.scope += 1
        if len(self.external_var) > 0:
            for name in self.external_var:
                self.pre += ", " + name 
        self.pre += ")"+ " {\n"

        self.code = self.pre + self.code
        self.code += '\n'
        for _ in range(self.scope):
            self.code += "}"
            self.code += "\n"

        with open(self.file,'w') as FOUT:
            FOUT.write(self.code)


