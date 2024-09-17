import yaml

try:
    with open('/root/.spack/linux/compilers.yaml', 'r') as f:
        y = yaml.safe_load(f)
except FileNotFoundError:
    print("YAML file not found.")
    exit(1)
except yaml.YAMLError as e:
    print(f"Error loading YAML file: {e}")
    exit(1)

try:
    for c in y['compilers']:
        compiler = c.get('compiler', {})
        if compiler.get('spec') == 'clang@=18.1.8':
            compiler['paths']['f77'] = '/usr/bin/gfortran'
            compiler['paths']['fc'] = '/usr/bin/gfortran'
except KeyError as e:
    print(f"Key error: {e}")
    exit(1)

try:
    with open('/root/.spack/linux/compilers.yaml', 'w') as f:
        yaml.safe_dump(y, f, default_flow_style=False)
    print("YAML file updated successfully.")
except IOError:
    print("Error writing to YAML file.")
