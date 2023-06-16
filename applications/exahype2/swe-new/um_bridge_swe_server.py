import umbridge
from subprocess import run # run subprocesses to compile exahype and start simulation
from os.path import isdir, realpath
from glob import glob
class TsunamiModel(umbridge.Model):
    def __init__(self):
        super().__init__("forward") # Give a name to the model

    def get_input_sizes(self, config):
        return [2] # coordinates of the earthquake origin

    def get_output_sizes(self, config):
        # normally 2x max water height at time t for two DART buoys 21418 and 21419
        # but can in theory be any probe point one is interested in
        return [config["output_size"]] 

    def __call__(self, parameters, config):
        # check whether this script is being called from the build directory (is there a build folder on the path between peano and cwd)
        # or if build directory exists
        operating_directory = realpath(__file__)
        print("Calling script on path: " + operating_directory)
        
        print("Checking whether build directory is on path...")
        if("build" not in operating_directory.rsplit("Peano/",maxsplit=1)[1]):  
            print("Not in build directory.")
            operating_directory = operating_directory.rsplit("Peano/",maxsplit=1)[0]+"Peano/"

            print("Checking whether build directory exist...")
            if(not isdir(operating_directory + "build/")):
                print("Build directory does not exist, attempt to create it.")
                run(["mkdir", "build"], cwd=operating_directory)
                print("Created build directory at " + operating_directory+"build/")

            print("Changed into build directory.")
            operating_directory = operating_directory + "build/"

            print("Checking whether ExaHyPE is already compiled in release mode...")
            if(len(glob(operating_directory+"*release*")) == 0):
                print("ExaHyPE not compiled in release mode, will now attempt to compile.")
            
            print("Changed into build/applications/exahype2/swe-new/")
            operating_directory = operating_directory + "applications/exahype2/swe-new/"
        else:
            print("Build directory already on path.")

        print("Compiling swe.py now.") 
        run(["python3", 
            "swe.py", 
            "-s", 
            config["solver"], 
            "-sc", 
            config["scenario"], 
            "-amr", str(config["amr"]), 
            "-et", str(config["end_time"])
            ])
        # run exe 
        application_name = "swe-application-2d-solver-name-SWESolver%s-scenario-%s-patch-size-%d-amr-%d-cell-size-%f-end-time-%f" % (config["solver"], config["scenario"], config["patch_size"], config["amr"], config["max_cell_size"], config["end_time"])
        run("./%s" % application_name)
        #   read out values from csv file
        #   return output vector
        output = [config["output_size"]]
        
        return [output]

    def supports_evaluate(self):
        return True
    

model = TsunamiModel()
config = {
    "output_size": 4,
    "scenario": "Artificial_Tsunami",
    "bathymetry_file_path": "abc",
    "displacement_file_path": "jkl",
    "end_time": 3,
    "amr": 0,
    "solver": "FVRusanovGlobalAdaptive",
    "patch_size": 32,
    "max_cell_size": 0.1
}
model.__call__([],config)
#umbridge.serve_models([model], 4242)
