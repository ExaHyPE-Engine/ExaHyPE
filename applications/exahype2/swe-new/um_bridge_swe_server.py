import umbridge
from subprocess import run # run subprocesses to compile exahype and start simulation
from os.path import expandvars # expand env vars
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
        output = [config["output_size"]]
        
        return [output]

    def supports_evaluate(self):
        return True
    

model = TsunamiModel()
umbridge.serve_models([model], 4242)
