import numpy as np
from typing import List
from math import floor
from netcdf_writer import NetCDF_Writer
import netCDF4
from update_scheme import compute_numerical_fluxes, set_up_update_variables

class SWE_Adjoint_Solver():

    def __init__(self, output_file_name:str, bathymetry_file_path:str, width:List[float], cell_size:float, end_time:float, checkpoints:int = 0):

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
            
            """
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
            """

        self.h[1:self.nx+1, 1:self.ny+1] += np.array([[(self.hump_criterion(x,y) and self.h[x, y] > 0) * 1 for y in range(1, self.ny+1)] for x in range(1, self.nx+1)])

        # initialize ghost layer
        self.b[0] = self.b[1]
        self.b[-1] = self.b[-2]
        self.b.T[0] = self.b.T[1]
        self.b.T[-1] = self.b.T[-2]
        self.set_ghost_layer()

        # generate checkpoints
        self.checkpoints = [] # unlimited checkpoints
        if checkpoints > 0:
            self.checkpoints = np.linspace(0, end_time, num=checkpoints+2)
            self.number_of_checkpoints = self.number_of_checkpoints + 2 # for netcdf file (one output at t=0 and one at t=end_time)

        self.netcdf_writer = NetCDF_Writer(self.nx, self.ny, self.b)
        # set up netCDF file for output
        if not output_file_name.endswith(".nc"):
            output_file_name = output_file_name + ".nc"
        self.netcdf_writer.set_up_netcdf("%s" % output_file_name, self.number_of_checkpoints)

        set_up_update_variables(self.nx, self.ny, self.cell_size)
        
    def run_simulation(self):
        # run simulation from t=0 to t=end_time
        t = 0.0
        checkpoint = 0
        # write initial state to file
        self.netcdf_writer.write_eta_to_file(checkpoint, t, self.h)
        checkpoint = checkpoint + 1
        # simulation loop
        while t < self.end_time:
            print("Simulating time: %f" % t)
            self.set_ghost_layer()
            t = t + compute_numerical_fluxes(self.h, self.b, self.hu, self.hv)
            # checkpointing
            if len(self.checkpoints) == 0 or self.checkpoints[checkpoint] <= t:
                self.netcdf_writer.write_eta_to_file(checkpoint, t, self.h)
                checkpoint = checkpoint + 1

        # close netCDF file after simulation
        self.netcdf_writer.close_netcdf()
    
    def set_ghost_layer(self):
        # set ghost layer to boundary conditions (outflow)
        self.h[0] = self.h[1]
        self.h[-1] = self.h[-2]
        self.h.T[0] = self.h.T[1]
        self.h.T[-1] = self.h.T[-2]
        
        # TODO correct boundary conditions?
        self.hu[0] = self.hu[1]
        self.hu[-1] = self.hu[-2]
        self.hu.T[0] = self.hu.T[1]
        self.hu.T[-1] = self.hu.T[-2]

        self.hv[0] = self.hv[1]
        self.hv[-1] = self.hv[-2]
        self.hv.T[0] = self.hv.T[1]
        self.hv.T[-1] = self.hv.T[-2]

    def hump_criterion(self, x, y) -> bool:
        x_trans = self.transform_index(x, self.nx, 0, 6.5e6, -5e5)
        y_trans = self.transform_index(y, self.ny, 0, 1.5e6, -2.5e6)
        return -100e3 < x_trans and 100e3 > x_trans and 100e3 > y_trans and -100e3 < y_trans

    def transform_index(self, z, max_z, min_z, max_z_cdf, min_z_cdf):
        m = ((max_z_cdf - min_z_cdf) / (max_z - min_z))
        b = min_z_cdf - m * min_z
        ret = m * z + b
        return ret
        

# TODO: add t_s as parameter to constructor
solver = SWE_Adjoint_Solver(output_file_name="test.nc", bathymetry_file_path="../netCDFInputs/tohoku_gebco_ucsb3_2000m_hawaii_bath.nc", width=[10,10], cell_size=0.1, end_time=0.1)
solver.run_simulation()
