from exahypype import kernel as xhpp
import numpy as np

test = xhpp(dim=3,patch_size=4,namespace="exahype2::fv::rusanov::internal")
dt = 0.01

Qcopy   =   test.Evaluate("Qcopy", "Q[]")

xnum    =   test.halo([1,0,0])
ynum    =   test.halo([0,1,0])
znum    =   test.halo([0,0,1])

XFlux   =   test.Evaluate("flux_x", "Flux_x(Qcopy[],kwargs)", xnum)
YFlux   =   test.Evaluate("flux_y", "Flux_y(Qcopy[],kwargs)", ynum)
ZFlux   =   test.Evaluate("flux_z", "Flux_z(Qcopy[],kwargs)", ynum)

XEig   =   test.Evaluate("tmp_x_eigen", "X_max_eigenvalues(Qcopy[],kwargs)", xnum)
YEig   =   test.Evaluate("tmp_y_eigen", "Y_max_eigenvalues(Qcopy[],kwargs)", ynum)
ZEig   =   test.Evaluate("tmp_z_eigen", "Z_max_eigenvalues(Qcopy[],kwargs)", ynum)

test.Stencil(Qcopy,[XFlux,YFlux,ZFlux],"0[0100],0[0-100],1[0010],1[00-10],2[0001],2[000-1]",dt*np.array([1,-1,1,-1,1,-1]))
test.Stencil(Qcopy,[XEig,YEig,ZEig],"0[0100],0[0-100],1[0010],1[00-10],2[0001],2[000-1]",[0.5,0.5,0.5,0.5,0.5,0.5])

