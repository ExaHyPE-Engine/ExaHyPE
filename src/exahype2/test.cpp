#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "Functors.h"
#include "exahype2/Solver.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/enumerator/enumerator.h"

namespace exahype2::fv::rusanov::internal{

void time_step(double* Q, centre, size, double t, double dt) {
Q_copy = new double[1,6,6];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 6; j++) {
			Q_copy[patch,i,j] = Q[patch,i,j];
		}
	}
}
tmp_x_eigen = new double[1,6,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 4; j++) {
			tmp_x_eigen[patch,i,j] = maxEigenvalueFunctor(Q_copy,centre,size,t,dt,0);
		}
	}
}
tmp_y_eigen = new double[1,4,6];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			tmp_y_eigen[patch,i,j] = maxEigenvalueFunctor(Q_copy,centre,size,t,dt,1);
		}
	}
}
flux_x = new double[1,6,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 4; j++) {
			flux_x[patch,i,j] = fluxFunctor(Q_copy,centre,size,t,dt,0);
		}
	}
}
flux_y = new double[1,4,6];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			flux_y[patch,i,j] = fluxFunctor(Q_copy,centre,size,t,dt,0);
		}
	}
}
Xleft = new double[1,4,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 1; i < 5; i++) {
		for (int j = 1; j < 5; j++) {
			Xleft[patch,i,j] += 1 * tmp_x_eigen[patch,i+1,j]
			Xleft[patch,i,j] += 1 * tmp_x_eigen[patch,i,j]
		}
	}
}
for (int patch = 0; patch < 1; patch++) {
	for (int i = 1; i < 5; i++) {
		for (int j = 1; j < 5; j++) {
			Q_copy[patch,i,j] += 0.5 * tmp_x_eigen[patch,i+1,j]
			Q_copy[patch,i,j] += 0.5 * tmp_x_eigen[patch,i-1,j]
			Q_copy[patch,i,j] += 0.5 * tmp_y_eigen[patch,i,j+1]
			Q_copy[patch,i,j] += 0.5 * tmp_y_eigen[patch,i,j-1]
		}
	}
}

}
}
