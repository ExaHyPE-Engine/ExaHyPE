import netCDF4
import numpy as np
class NetCDF_Writer():
    def __init__(self, nx, ny, b) -> None:
        self.nx = nx
        self.ny = ny
        self.b = b
    
    def set_up_netcdf(self, filename:str, source_filename:str, number_of_checkpoints:int, end_time:float, start_time:float = 0):
        self.nc_file = netCDF4.Dataset(filename, "w")
        self.nc_file.conventions = "CF-1.5"
        self.nc_file.title = "Computed Adjoint Solution"
        self.nc_file.history = "ExaHyPE SWE Application"
        self.nc_file.institution = "Technische Universitaet Muenchen, School of Computation, Information and Technology, Chair of Scientific Computing"
        self.nc_file.source = "%s" % source_filename
        self.nc_file.references = "exahype.org"
        self.nc_file.boundary_positions = [self.nx, self.ny, 0.0, 0.0]
        self.nc_file.start_time = start_time
        self.nc_file.end_time = end_time

        #TODO Write t_f and t_s to netCDF

        self.nc_dim_time = self.nc_file.createDimension("time", number_of_checkpoints)
        self.nc_dim_x = self.nc_file.createDimension("x", self.nx)
        self.nc_dim_y = self.nc_file.createDimension("y", self.ny)

        self.nc_var_time = self.nc_file.createVariable("time", np.float64, ("time",))
        self.nc_var_time.long_name = "Time"
        self.nc_var_time.units = "seconds since simulation start"

        self.nc_var_x = self.nc_file.createVariable("x", np.float32, ("x",))
        self.nc_var_y = self.nc_file.createVariable("y", np.float32, ("y",))
        self.nc_var_eta = self.nc_file.createVariable("eta", np.float64, ("time", "y", "x"))

        self.nc_var_x[:] = np.arange(self.nx)
        self.nc_var_y[:] = np.arange(self.ny)

    def close_netcdf(self):
        self.nc_file.close()

    def write_eta_to_file(self, checkpoint, t, h):
        self.nc_var_time[checkpoint] = t
        self.nc_var_eta[checkpoint, :, :] = h.T[1:self.ny+1, 1:self.nx+1] + self.b.T[1:self.ny+1, 1:self.nx+1]