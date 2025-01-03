import yaml

# Path to the compilers.yaml file
compilers_file = '/root/.spack/linux/compilers.yaml'

try:
    # Load the YAML file
    with open(compilers_file, 'r') as f:
        y = yaml.safe_load(f)
except FileNotFoundError:
    print("YAML file not found.")
    exit(1)
except yaml.YAMLError as e:
    print(f"Error loading YAML file: {e}")
    exit(1)

try:
    # Iterate through all compilers and update clang compilers
    for compiler_entry in y.get('compilers', []):
        compiler = compiler_entry.get('compiler', {})
        if compiler.get('spec', '').startswith('clang@'):
            compiler['paths']['f77'] = '/usr/bin/gfortran'
            compiler['paths']['fc'] = '/usr/bin/gfortran'

    # Write the updated YAML back to the file
    with open(compilers_file, 'w') as f:
        yaml.safe_dump(y, f, default_flow_style=False)
    print("Fortran compilers added to all clang compilers successfully.")
except KeyError as e:
    print(f"Key error: {e}")
    exit(1)
except IOError as e:
    print(f"Error writing to YAML file: {e}")
    exit(1)
