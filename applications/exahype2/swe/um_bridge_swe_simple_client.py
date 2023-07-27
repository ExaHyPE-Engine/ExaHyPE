import umbridge

print(umbridge.supported_models("http://0.0.0.0:4242"))

model = umbridge.HTTPModel("http://0.0.0.0:4242", "forward")

# config should only contain options for swe
config = {
    "-sc": "Tsunami",
    "-bf": "./netCDFInputs/artificialtsunami_bathymetry_1000.nc",
    "-df": "./netCDFInputs/artificialtsunami_displ_1000.nc",
    "-et": 0.1,
    "-amr": 0,
    "-s": "FVRusanovGlobalAdaptive",
    "-ps": 32,
    "-maxh": 0.1,
    "-probes": ["[5,5]", "[7,3]"],
    "--no-dynamic-h": ""
}

print(model.get_input_sizes())
print(model.get_output_sizes(config))

print(model([[2000, 2000]], config))