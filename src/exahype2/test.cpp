Qcopy = new double[1,6,6];
for (int patch = 0; patch < 1; patch++) {
	for (int i = -1; i < 5; i++) {
		for (int j = -1; j < 5; j++) {
			Qcopy[patch,i,j] = Q[patch,i,j];
		}
	}
}
flux_x = new double[1,6,4];
for (int patch = 0; patch < 1; patch++) {
	for (int i = -1; i < 5; i++) {
		for (int j = 0; j < 4; j++) {
			flux_x[patch,i,j] = Flux_x(Qcopy[patch,i,j],kwargs);
		}
	}
}
flux_y = new double[1,4,6];
for (int patch = 0; patch < 1; patch++) {
	for (int i = 0; i < 4; i++) {
		for (int j = -1; j < 5; j++) {
			flux_y[patch,i,j] = Flux_y(Qcopy[patch,i,j],kwargs);
		}
	}
}
tmp_x_eigen = new double[1,6,4];
for (int j = 0; j < 4; j++) {
	for (int patch = 0; patch < 1; patch++) {
		for (int i = -1; i < 5; i++) {
			tmp_x_eigen[patch,i,j] = X_max_eigenvalues(Qcopy[patch,i,j],kwargs);
		}
	}
}
tmp_y_eigen = new double[1,4,6];
for (int j = -1; j < 5; j++) {
	for (int patch = 0; patch < 1; patch++) {
		for (int i = 0; i < 4; i++) {
			tmp_y_eigen[patch,i,j] = Y_max_eigenvalues(Qcopy[patch,i,j],kwargs);
		}
	}
}
for (int patch = 0; patch < 1; patch++) {
	for (int i = -1; i < 5; i++) {
		for (int j = -1; j < 5; j++) {
			Qcopy[patch,i,j] += 1 * flux_x[patch,i+1,j]
			Qcopy[patch,i,j] -= 1 * flux_x[patch,i-1,j]
			Qcopy[patch,i,j] += 1 * flux_y[patch,i,j+1]
			Qcopy[patch,i,j] -= 1 * flux_y[patch,i,j-1]
		}
	}
}
for (int patch = 0; patch < 1; patch++) {
	for (int i = -1; i < 5; i++) {
		for (int j = -1; j < 5; j++) {
			Qcopy[patch,i,j] += 0.5 * tmp_x_eigen[patch,i+1,j]
			Qcopy[patch,i,j] += 0.5 * tmp_x_eigen[patch,i-1,j]
			Qcopy[patch,i,j] += 0.5 * tmp_y_eigen[patch,i,j+1]
			Qcopy[patch,i,j] += 0.5 * tmp_y_eigen[patch,i,j-1]
		}
	}
}

