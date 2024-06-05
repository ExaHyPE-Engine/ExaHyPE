from kernel_impl import impl, Enumerator

class kernel:
    def __init__(self, dim, patch_size, n_patches=1, halo_size=1, file="test.cpp",debug=False):
        self.d = dim                    #number of dimensions
        self.np = n_patches
        self.p = patch_size             #size of patch
        self.h = halo_size              #size of halo
        
        self.file = file                #output file destination
        self.debug = debug              #debug mode
        self.code = ""                  #to output to file

        self.names = []                 #list of variable names

    def halo(self,which=[1,1,1]):
        start = [0]
        end = [self.np]
        for i in range(self.d):
            if which[i] > 0:
                end.append(self.p + self.h)
                start.append(- self.h)
            else:
                end.append(self.p)
                start.append(0)
        if self.d == 2:
            return Enumerator(start,end,["patch","i","j"])
        return Enumerator(start,end,["patch","i","j","k"])

    def Evaluate(self, name, expression, enum=None):
        if type(enum) == type(None):
            enum = self.halo()
        k = impl(self)
        if name not in self.names:
            k.alloc(name,enum)
            self.names.append(name)
        k.loop(name,expression,enum,self.d)
        self.code = k.code
        return name

    def Stencil(self, out_name, names, stencil, scale=1, enum=None):
        if type(enum) == type(None):
            enum = self.halo([1,1,1])
        k = impl(self)
        if out_name not in self.names:
            k.alloc(out_name,enum)
            self.names.append(name)
        k.stencil(out_name,names,stencil,scale,enum,self.d)
        self.code = k.code
        return out_name

    def __del__(self):
        self.code += '\n'
        with open(self.file,'w') as FOUT:
            FOUT.write(self.code)


