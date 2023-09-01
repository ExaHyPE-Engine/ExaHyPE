import umbridge

print(umbridge.supported_models("http://0.0.0.0:4242"))

model = umbridge.HTTPModel("http://0.0.0.0:4242", "forward")

# config should only contain options for swe
config = {
    "-sc": "Tsunami",
    "-bf": "./netCDFInputs/artificialtsunami_bathymetry_1000.nc",
    "-df": "./netCDFInputs/artificialtsunami_displ_1000.nc",
    "-et": 360,
    "-amr": 0,
    "-s": "FVRusanovGlobalAdaptive",
    "-ps": 4,
    "-maxh": 100,
    "-probes": ["[5e3,5e3]"],
    "-wd": "[10e3,10e3]",
    "-pi": 1
}

print(model.get_input_sizes())
print(model.get_output_sizes(config))

print(model([[-25e2, -25e2]], config))