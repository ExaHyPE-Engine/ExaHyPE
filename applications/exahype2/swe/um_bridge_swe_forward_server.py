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
        self.output_size = len(config["-probes"])*2
        return [self.output_size] 

    def __call__(self, parameters, config):
        # if get_output_sizes() has not been called yet
        if(not hasattr(self, "output_size")):
            # calculate output size based on number of probes
            self.output_size = len(config["-probes"])*2

        print("Trying to compile swe.py now.") 

        compile_instructions = ["python3", "swe.py"]
        for k in config.keys():
            compile_instructions.append(k)
            if (k == "-probes"):
                for coordinates in config[k]:
                    compile_instructions.append(coordinates)
                continue
            compile_instructions.append(str(config[k]))

        compile_instructions.append("--uq-displacement-origin")
        compile_instructions.append("[%f,%f]" % (parameters[0][0], parameters[0][1]))
        
        run(compile_instructions).check_returncode()
        # run exe 
        application_name = "swe-application-2d-solver-name-SWESolver%s-scenario-%s-patch-size-%d-amr-%d-cell-size-%f-end-time-%f" % (config["-s"], config["-sc"], config["-ps"], config["-amr"], config["-maxh"], config["-et"])
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
    
model = TsunamiModel()
umbridge.serve_models([model], 4242)
