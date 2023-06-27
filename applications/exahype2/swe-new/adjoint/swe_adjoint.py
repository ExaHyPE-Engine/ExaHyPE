import numpy as np
from typing import List
from math import floor
import netCDF4
from adjoint_solver import compute_net_updates

class SWE_Adjoint_Solver():

    def __init__(self, output_file_name:str, bathymetry_file_path:str, displacement_file_path:str, width:List[float], cell_size:float, end_time:float, checkpoints:int = 0):

        self.cell_size = cell_size
        self.end_time = end_time
        self.number_of_checkpoints = checkpoints

        # dimensions of simulation plane without ghost cells
        self.nx = floor(width[0] / cell_size)
        self.ny = floor(width[1] / cell_size)

        # initialize arrays
        self.h = np.zeros((self.nx+2, self.ny+2))
        self.hu = np.zeros((self.nx+2, self.ny+2))
        self.hv = np.zeros((self.nx+2, self.ny+2))
        self.b = np.zeros((self.nx+2, self.ny+2)) 

        with netCDF4.Dataset(bathymetry_file_path, "r") as bathymetry_nc:
            # create lists of indices that index the bathymetry entries to be used in the simulation
            index_list_bathymetry_x, mesh_width_x = np.linspace(0, len(bathymetry_nc['z'][0]), num=self.nx, endpoint=False, dtype=int, retstep=True)
            index_list_bathymetry_y, mesh_width_y = np.linspace(0, len(bathymetry_nc['z']), num=self.ny, endpoint=False, dtype=int, retstep=True)
            # convert (data) mesh width to integer
            mesh_width_x = int(mesh_width_x)
            mesh_width_y = int(mesh_width_y)

            # Define indices for splicing. Why splicing and not fancy indexing?
            # Because fancy indexing doesn't work in the displacement at the latest because of differing mesh widths.
            # Splicing works equally as good and also works for the displacement. 
            right_x_start, right_x_end = index_list_bathymetry_x[[0, -1]]
            right_x_end = right_x_end + 1
            right_y_start, right_y_end = index_list_bathymetry_y[[0, -1]]
            right_y_end = right_y_end + 1

            # Put values of bathymetry file into simulation arrays
            self.b.T[1:self.ny+1, 1:self.nx+1] = bathymetry_nc['z'][right_y_start:right_y_end:mesh_width_y, right_x_start:right_x_end:mesh_width_x]
            self.h[1:self.nx+1, 1:self.ny+1] = -self.b[1:self.nx+1, 1:self.ny+1]

            with netCDF4.Dataset(displacement_file_path, "r") as displacement_nc:
                # read out coordinate variables of the different files
                bath_x = bathymetry_nc['x']
                bath_y = bathymetry_nc['y']
                displ_x = displacement_nc['x']
                displ_y = displacement_nc['y']

                # Define indices for splicing (again). 
                # These lines describe the section of the bathymetry, that the displacement shall be added to.
                left_x_start, left_x_end = np.arange(1, self.nx+1)[np.isin(bath_x[index_list_bathymetry_x], displ_x)][[0, -1]]
                left_x_end = left_x_end + 1
                left_y_start, left_y_end = np.arange(1, self.ny+1)[np.isin(bath_y[index_list_bathymetry_y], displ_y)][[0, -1]]
                left_y_end = left_y_end + 1
                # Include all displacement values, whose x and y coordinate are also in our reduced bathymetry array
                right_x_start, right_x_end = np.arange(0, len(displ_x))[np.isin(displ_x, bath_x[index_list_bathymetry_x])][[0, -1]]
                right_x_end = right_x_end + 1
                right_y_start, right_y_end = np.arange(0, len(displ_y))[np.isin(displ_y, bath_y[index_list_bathymetry_y])][[0, -1]]
                right_y_end = right_y_end + 1
                # add displacement to bathymetry array
                self.b.T[left_y_start:left_y_end, left_x_start:left_x_end] += displacement_nc['z'][right_y_start:right_y_end:mesh_width_y, right_x_start:right_x_end:mesh_width_x]

        # initialize ghost layer
        self.b[0] = self.b[1]
        self.b[-1] = self.b[-2]
        self.b.T[0] = self.b.T[1]
        self.b.T[-1] = self.b.T[-2]
        self.set_ghost_layer()

        # initialize update arrays
        self.h_net_updates_left = np.zeros((self.nx+2, self.ny+2))
        self.h_net_updates_right = np.zeros((self.nx+2, self.ny+2))
        self.h_net_updates_above = np.zeros((self.nx+2, self.ny+2))
        self.h_net_updates_below = np.zeros((self.nx+2, self.ny+2))

        self.hu_net_updates_left = np.zeros((self.nx+2, self.ny+2))
        self.hu_net_updates_right = np.zeros((self.nx+2, self.ny+2))

        self.hv_net_updates_below = np.zeros((self.nx+2, self.ny+2))
        self.hv_net_updates_above = np.zeros((self.nx+2, self.ny+2))

        # generate checkpoints
        self.checkpoints = [] # unlimited checkpoints
        if checkpoints > 0:
            self.checkpoints = np.linspace(0, end_time, num=checkpoints+2)
            self.number_of_checkpoints = self.number_of_checkpoints + 2 # for netcdf file (one output at t=0 and one at t=end_time)

        # set up netCDF file for output
        if not output_file_name.endswith(".nc"):
            output_file_name = output_file_name + ".nc"
        self.set_up_netcdf("%s" % output_file_name)
        
    def run_simulation(self):
        ############# DEF HELPER FUNCTIONS #############
        def update_unknowns_x_sweep(dt):
            # update unknowns in x-dimension
            for i in range(1, self.nx+1):
                for j in range(0, self.ny+2):
                    self.h[i][j] = self.h[i][j] - (dt / self.cell_size * (self.h_net_updates_right[i-1][j] + self.h_net_updates_left[i][j]))
                    self.hu[i][j] = self.hu[i][j] - (dt / self.cell_size * (self.hu_net_updates_right[i-1][j] + self.hu_net_updates_left[i][j]))

        def update_unknowns_y_sweep(dt):
            # update unknowns in y-dimension
            for j in range(1, self.ny+1):
                for i in range(1, self.ny+1):
                    self.h[i][j] = self.h[i][j] - (dt / self.cell_size * (self.h_net_updates_above[i][j-1] + self.h_net_updates_below[i][j]))
                    self.hv[i][j] = self.hv[i][j] - (dt / self.cell_size * (self.hv_net_updates_above[i][j-1] + self.hv_net_updates_below[i][j]))
    
        def compute_numerical_fluxes() -> float:
            max_wave_speed = 0.0

            max_wave_speed_x = 0.0
            for i in range(1, self.nx+2):
                for j in range(0, self.ny+2):
                    max_edge_speed = 0.0
                    self.h_net_updates_left[i-1][j], self.hu_net_updates_left[i-1][j], self.h_net_updates_right[i-1][j], self.hu_net_updates_right[i-1][j], max_edge_speed = compute_net_updates(self.h[i-1][j], self.h[i][j], self.hu[i-1][j], self.hu[i][j], self.b[i-1][j], self.b[i][j])
                    max_wave_speed_x = max(max_edge_speed, max_wave_speed_x)

            if max_wave_speed_x > 0:
                update_unknowns_x_sweep(0.4 * self.cell_size / max_wave_speed_x)
            
            max_wave_speed_y = 0.0
            for j in range(1, self.ny+2):
                for i in range(1, self.nx+2):
                    max_edge_speed = 0.0
                    self.h_net_updates_below[i][j-1], self.hv_net_updates_below[i][j-1], self.h_net_updates_above[i][j-1], self.hv_net_updates_above[i][j-1], max_edge_speed = compute_net_updates(self.h[i][j-1], self.h[i][j], self.hu[i][j-1], self.hu[i][j], self.b[i][j-1], self.b[i][j])
                    max_wave_speed_y = max(max_edge_speed, max_wave_speed_y)

            max_wave_speed = max(max_wave_speed_x, max_wave_speed_y)
            # by here, we kind of assume a wave_speed greater 0
            print("max wave speed: %f" % max_wave_speed)
            max_time_step = 0.4 * self.cell_size / max_wave_speed
            update_unknowns_y_sweep(max_time_step)
            return max_time_step

        def write_eta_to_file(checkpoint, t):
            self.nc_var_time[checkpoint] = t
            self.nc_var_eta[checkpoint, :, :] = self.h.T[1:self.ny+1, 1:self.nx+1] + self.b.T[1:self.ny+1, 1:self.nx+1]

        ############# END DEF HELPER FUNCTIONS #############

        # run simulation from t=0 to t=end_time
        t = 0.0
        checkpoint = 0
        # write initial state to file
        write_eta_to_file(checkpoint, t)
        checkpoint = checkpoint + 1
        # simulation loop
        while t < self.end_time:
            print("Simulating time: %f" % t)
            self.set_ghost_layer()
            t = t + compute_numerical_fluxes()
            # checkpointing
            if len(self.checkpoints) == 0 or self.checkpoints[checkpoint] <= t:
                write_eta_to_file(checkpoint, t)
                checkpoint = checkpoint + 1

        # close netCDF file after simulation
        self.close_netcdf()
    
    def set_ghost_layer(self):
        # set ghost layer to boundary conditions (outflow)
        self.h[0] = self.h[1]
        self.h[-1] = self.h[-2]
        self.h.T[0] = self.h.T[1]
        self.h.T[-1] = self.h.T[-2]

        self.hu[0] = 0.0 # self.hu[1]
        self.hu[-1] = 0.0 # self.hu[-2]
        self.hu.T[0] = 0.0 # self.hu.T[1]
        self.hu.T[-1] = 0.0 # self.hu.T[-2]

        self.hv[0] = 0.0 # self.hv[1]
        self.hv[-1] = 0.0 # self.hv[-2]
        self.hv.T[0] = 0.0 # self.hv.T[1]
        self.hv.T[-1] = 0.0 # self.hv.T[-2]

    def set_up_netcdf(self, filename:str):
        self.nc_file = netCDF4.Dataset(filename, "w")

        self.nc_dim_time = self.nc_file.createDimension("time", self.number_of_checkpoints)
        self.nc_dim_x = self.nc_file.createDimension("x", self.nx)
        self.nc_dim_y = self.nc_file.createDimension("y", self.ny)

        self.nc_var_x = self.nc_file.createVariable("x", np.float64, ("x",))
        self.nc_var_y = self.nc_file.createVariable("y", np.float64, ("y",))
        self.nc_var_time = self.nc_file.createVariable("time", np.float64, ("time",))
        self.nc_var_eta = self.nc_file.createVariable("eta", np.float64, ("time", "y", "x"))

        self.nc_var_x[:] = np.arange(self.nx)
        self.nc_var_y[:] = np.arange(self.ny)

    def close_netcdf(self):
        self.nc_file.close()
    
solver = SWE_Adjoint_Solver(output_file_name="abc.nc", bathymetry_file_path="../netCDFInputs/artificialtsunami_bathymetry_1000.nc", displacement_file_path="../netCDFInputs/artificialtsunami_displ_1000.nc", width=[10,10], cell_size=0.1, end_time=0.1)
solver.run_simulation()
