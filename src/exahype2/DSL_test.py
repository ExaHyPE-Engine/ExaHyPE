from exahypype import kernel as xhpp

test = xhpp(dim=2,patch_size=4)

Qcopy   =   test.Evaluate("Qcopy", "Q[]")

xnum    =   test.halo([1,0,0])
ynum    =   test.halo([0,1,0])

XFlux   =   test.Evaluate("flux_x", "Flux_x(Qcopy[],kwargs)", xnum)
YFlux   =   test.Evaluate("flux_y", "Flux_y(Qcopy[],kwargs)", ynum)

XEig   =   test.Evaluate("tmp_x_eigen", "X_max_eigenvalues(Qcopy[],kwargs)", xnum)
YEig   =   test.Evaluate("tmp_y_eigen", "Y_max_eigenvalues(Qcopy[],kwargs)", ynum)

test.Stencil(Qcopy,[XFlux,YFlux],"0[010],0[0-10],1[001],1[00-1]",[1,-1,1,-1])
test.Stencil(Qcopy,[XEig,YEig],"0[010],0[0-10],1[001],1[00-1]",[0.5,0.5,0.5,0.5])

