namespace exahype2::fv::rusanov::internal{
Qcopy = new double[1,6,6,6];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 6; j++) {
			for (int k = 0; k < 6; k++) {
				Qcopy[patch,i,j,k] = Q[patch,i,j,k];
			}
		}
	}
}
flux_x = new double[1,6,4,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				flux_x[patch,i,j,k] = Flux_x(Qcopy[patch,i,j+1,k+1],kwargs);
			}
		}
	}
}
flux_y = new double[1,4,6,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			for (int k = 0; k < 4; k++) {
				flux_y[patch,i,j,k] = Flux_y(Qcopy[patch,i+1,j,k+1],kwargs);
			}
		}
	}
}
flux_z = new double[1,4,6,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			for (int k = 0; k < 4; k++) {
				flux_z[patch,i,j,k] = Flux_z(Qcopy[patch,i+1,j,k+1],kwargs);
			}
		}
	}
}
tmp_x_eigen = new double[1,6,4,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				tmp_x_eigen[patch,i,j,k] = X_max_eigenvalues(Qcopy[patch,i,j+1,k+1],kwargs);
			}
		}
	}
}
tmp_y_eigen = new double[1,4,6,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			for (int k = 0; k < 4; k++) {
				tmp_y_eigen[patch,i,j,k] = Y_max_eigenvalues(Qcopy[patch,i+1,j,k+1],kwargs);
			}
		}
	}
}
tmp_z_eigen = new double[1,4,6,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			for (int k = 0; k < 4; k++) {
				tmp_z_eigen[patch,i,j,k] = Z_max_eigenvalues(Qcopy[patch,i+1,j,k+1],kwargs);
			}
		}
	}
}
for (int patch = 0; patch < 1; patch++) {
	for (int i = 1; i < 5; i++) {
		for (int j = 1; j < 5; j++) {
			for (int k = 1; k < 5; k++) {
				Qcopy[patch,i,j,k] += 0.01 * flux_x[patch,i+1,j,k]
				Qcopy[patch,i,j,k] -= 0.01 * flux_x[patch,i-1,j,k]
				Qcopy[patch,i,j,k] += 0.01 * flux_y[patch,i,j+1,k]
				Qcopy[patch,i,j,k] -= 0.01 * flux_y[patch,i,j-1,k]
				Qcopy[patch,i,j,k] += 0.01 * flux_z[patch,i,j,k+1]
				Qcopy[patch,i,j,k] -= 0.01 * flux_z[patch,i,j,k-1]
			}
		}
	}
}
for (int patch = 0; patch < 1; patch++) {
	for (int i = 1; i < 5; i++) {
		for (int j = 1; j < 5; j++) {
			for (int k = 1; k < 5; k++) {
				Qcopy[patch,i,j,k] += 0.5 * tmp_x_eigen[patch,i+1,j,k]
				Qcopy[patch,i,j,k] += 0.5 * tmp_x_eigen[patch,i-1,j,k]
				Qcopy[patch,i,j,k] += 0.5 * tmp_y_eigen[patch,i,j+1,k]
				Qcopy[patch,i,j,k] += 0.5 * tmp_y_eigen[patch,i,j-1,k]
				Qcopy[patch,i,j,k] += 0.5 * tmp_z_eigen[patch,i,j,k+1]
				Qcopy[patch,i,j,k] += 0.5 * tmp_z_eigen[patch,i,j,k-1]
			}
		}
	}
}

}
