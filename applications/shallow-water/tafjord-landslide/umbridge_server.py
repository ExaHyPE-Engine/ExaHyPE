
import os
import time
import umbridge

class TafjordLandslide(umbridge.Model):
    def __init__(self):
        super().__init__("TafjordLandslide")

    def get_input_sizes(self, config):
        return [1]  # Input is friction parameter

    def get_output_sizes(self, config):
        return [1]  # Max. eigenvalue

    def __call__(self, parameters, config):
        os.system(f"python3 tafjord-landslide.py --friction {parameters[0][0]} -ns 0")
        start = time.time()
        os.system(f"./TafjordLandslide.Release")
        end = time.time() - start
        return [[end]]

    def supports_evaluate(self):
        return True

model = TafjordLandslide()
umbridge.serve_models([model], 4242)
