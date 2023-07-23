import umbridge
from subprocess import run
from os.path import isdir, realpath, basename
from glob import glob
import pandas as pd

class TsunamiModel(umbridge.Model):
    def __init__(self):
        super().__init__("forward") # Give a name to the model

    def get_input_sizes(self, config):
        # coordinates of the earthquake origin
        return [2] 

    def get_output_sizes(self, config):
        # normally 2x max water height at time t for two DART buoys 21418 and 21419
        # but can in theory be any probe point one is interested in
        self.output_size = self.calculate_output_size(config["probe_coordinates"])
        return [self.output_size] 

    def __call__(self, parameters, config):
        # can be adapted for docker image
        root_directory = "work/"

        # if get_output_sizes() has not been called yet
        if(not hasattr(self, "output_size")):
            # calculate output size based on number of probes
            self.output_size = self.calculate_output_size(config["probe_coordinates"])

        print("Trying to compile swe.py now.") 

        flag_map = {
            "solver":                   "s",
            "end_time":                 "et", 
            "amr":                      "amr",
            "scenario":                 "sc",
            "patch_size":               "ps",
            "max_cell_size":            "maxh",
            "bathymetry_file_path":     "bf",
            "displacement_file_path":   "df",
            "plot_interval":            "pi", # this also defines the schedule for the probes being dumped into the database
            "output_path":              "o",
            "rk_order":                 "rk-order",
            "dg_order":                 "dg-order",
            "probe_coordinates":        "probes"
        }

        compile_instructions = ["python3", "swe.py"]
        for k in config.keys():
            if(k not in flag_map):
                continue
            compile_instructions.append("-%s" % flag_map[k])
            if (k == "probe_coordinates"):
                for coordinates in config[k]:
                    compile_instructions.append(coordinates)
                continue
            compile_instructions.append(str(config[k]))

        compile_instructions.append("--uq-displacement-origin")
        compile_instructions.append("[%f,%f]" % (parameters[0][0], parameters[0][1]))
        
        run(compile_instructions).check_returncode()
        # run exe 
        application_name = "swe-application-2d-solver-name-SWESolver%s-scenario-%s-patch-size-%d-amr-%d-cell-size-%f-end-time-%f" % (config["solver"], config["scenario"], config["patch_size"], config["amr"], config["max_cell_size"], config["end_time"])
        run("./%s" % application_name).check_returncode()

        operating_directory = "./output-%s/" % application_name
        # read out values from csv file
        if(not isdir(operating_directory)):
            print("Output directory not found! Aborting!")
            return

        print("Now reading in csv files.")
        csv_frame = pd.concat([pd.read_csv(csv_file, skipinitialspace=True, header=0, names=["n1", "n2", "t", "x1", "x2", "h", "hu", "hv", "b"]) for csv_file in glob(operating_directory+"*.csv")], ignore_index=True)

        print("Reading finished, now formatting dataframe.")
        csv_frame['coordinates'] = csv_frame.apply(lambda row: (row['x1'], row['x2']), axis=1)
        csv_frame = csv_frame.drop(["x1", "x2", "n1", "n2"], axis=1)
        csv_frame = csv_frame.iloc[csv_frame.groupby(['coordinates'])['h'].idxmax()]

        print("Maximum water height for probes:")
        print(csv_frame)

        # return output vector
        output = []
        for _, row in csv_frame.iterrows():
            output.append(row['t'])
            output.append(row['h'])
        print("Output: " + str(output))
        return [output] 

    def supports_evaluate(self):
        return True
    
    def calculate_output_size(self, probe_coordinates, number_of_variables=2):
        return len(probe_coordinates)*number_of_variables
    
# config should only contain options for swe
config = {
    "scenario": "Artificial_Tsunami",
    "bathymetry_file_path": "abc",
    "displacement_file_path": "jkl",
    "end_time": 0.1,
    "amr": 0,
    "solver": "FVRusanovGlobalAdaptive",
    "patch_size": 32,
    "max_cell_size": 0.1,
    "probe_coordinates": ["[5,5]", "[7,3]"] # don't question it, has to be this way
}
model = TsunamiModel()
model.__call__([[0, 0]],config)
#umbridge.serve_models([model], 4242)
