import numpy as np

def parse_patch_file(file_path, solver_type):
    """
    Transform data from Peano patch file into numpy array
    in the format [x_vertex, y_vertex, value]

    :arg file_path: path to Peano patch file
    :solver_type: "CG" or "DG"
    """

    if solver_type not in ["CG", "DG"]: raise ValueError("Invalid solver type. Expected 'CG' or 'DG'.")
    
    with open(file_path, 'r') as file:
        lines = file.readlines()

    if solver_type == 'CG':
        # For CG, convert the patch file format into a list:
        data = [] # the output format is [x, y, val], where (x,y) is vertex coordinates
        i = 0
        while i < len(lines):
            if lines[i].startswith("begin patch"):
                # Read 'offset x_left_bottom_node_coordinate y_left_bottom_node_coordinate'
                _, xlb, ylb = lines[i+1].split()
                xlb = float(xlb)
                ylb = float(ylb)

                # Read 'size x_cell_size y_cell_size'
                _, hx, hy = lines[i+2].split()
                hx = float(hx)
                hy = float(hy)

                # Read vertex values in the order: left-bottom, right-bottom, left-top, right-top
                values = [float(value) for value in lines[i+4].split()]

                # Write in the output
                data.extend([[xlb,    ylb,    values[0]],
                             [xlb+hx, ylb,    values[1]],
                             [xlb,    ylb+hy, values[2]],
                             [xlb+hx, ylb+hy, values[3]]])
                
                # jump to the next 'begin patch'
                i += 8

            else:
                i += 1
            
        # keep only unique rows (one vertex -- one entry)
        tolerance = 1e-9
        data_unique = []

        for row in data:
            # Check if row=[x,y,val] is close to any existing row
            if not any( np.all(np.isclose(row, unique_row, atol=tolerance)) for unique_row in data_unique ):
                data_unique.append(row)
        
        # assert:
        # size of data = 4*N^2
        # size of data_unique = (N+1)^2, where N is the number of cells
        assert np.sqrt(np.shape(data)[0]) == (np.sqrt(np.shape(data_unique)[0]) - 1) * 2, f'{np.shape(data) = }, {np.shape(data_unique) = }'
        result = np.array(data_unique)

    elif solver_type == 'DG':
        # For DG, convert the patch file format into a dictionary:
        # {(x*,y*): [val0, val1, ...], ...}, where
        #  (x*,y*) are the coordinates of the left bottom vertex of the cell and
        # val0, val1, ... are values assigned to the cell dofs

        result = {}

        i = 0
        while i < len(lines):
            if lines[i].startswith("begin patch"):
                # Read line 'offset x_left_bottom_node_coordinate y_left_bottom_node_coordinate'
                _, xlb, ylb = lines[i+1].split()
                # Read values
                values = [float(value) for value in lines[i+4].split()]
                result[( float(xlb), float(ylb) )] = values

                # jump to the next 'begin patch'
                i += 8
            else:
                # check next line
                i += 1
    
    return result

def reorder_array(array1, array2):
    """
    Given array1 and array2 of the same format [x,y,val],
    rearrange array2 in accordance with the order of coordinates
    in array1
    """

    array2_reorder = np.zeros_like(array2)

    for i in range(np.shape(array1)[0]):
        x = array1[i, 0]
        y = array1[i, 1]
        mask = np.isclose(array2[:, 0], x) & np.isclose(array2[:, 1], y)
        matching_row = array2[mask]
        array2_reorder[i,:] = matching_row

    #assert np.allclose( array2[:,0:2], array2_reorder[:,0:2] )

    return array2_reorder

def plot_field(data):
        """
        Visualise the parsed field as an interactive scatter plot.
        """

        import matplotlib.pyplot as plt
        from mpl_toolkits.mplot3d import Axes3D

        x = data[:, 0]
        y = data[:, 1]
        val = data[:, 2]

        # Plot
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')
        sc = ax.scatter(x, y, val, c=val, cmap='viridis')
        fig.colorbar(sc, ax=ax, label='Value')
        ax.set_xlabel('x')
        ax.set_ylabel('y')
        ax.set_zlabel('val')
        ax.set_title(' ')
        plt.show()

def plot_field_dg(data, H):
    """
    Visualise the parsed field as an interactive scatter plot.

    :arg data: parsed patch data in the form of a dictionary as output of parse_patch_file()
    :arg H: mesh size as list of cell sizes in each direction: [hx, hy]
    """
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D
    import mghype

    assert len(H) == 2, "Only works for 2D"

    # For plot
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    x = []
    y = []
    val = []
    
    # Local coordinates of cell nodes
    poly_degree = int(np.sqrt(len(data[(0.0, 0.0)]))) - 1
    matrices = mghype.api.matrixgenerators.blockmatrix.DGPoisson2dPenaltyBlockMatrix(poly_degree)
    local_node_coordinates = matrices._Xi_C
    num_of_nodes = len(local_node_coordinates[0])

    # Allocate list of global coordinates
    global_node_coordinates = [[np.zeros_like(arr) for arr in local_node_coordinates[0]]]

    for key in data.keys():
        # Coordinates of the left-bottom cell vertex
        xlb = key[0]
        ylb = key[1]

        # Local-to-global mapping:
        # x = xlb + hx/2*(xi  + 1)
        # y = ylb + hy/2*(eta + 1)
        for i in range(num_of_nodes):
            global_node_coordinates[0][i][0] = xlb + 0.5 * H[0] * (local_node_coordinates[0][i][0] + 1.0)
            global_node_coordinates[0][i][1] = ylb + 0.5 * H[1] * (local_node_coordinates[0][i][1] + 1.0)
            # global_node_coordinates[0][i][:] = [ key[coord_id] + 0.5 * H[coord_id] * (local_node_coordinates[0][i][coord_id] + 1.0) for coord_id in range(2) ]

        # Add to global arrays for plotting
        x.extend([ global_node_coordinates[0][i][0] for i in range(num_of_nodes) ])
        y.extend([ global_node_coordinates[0][i][1] for i in range(num_of_nodes) ])
        val.extend(data[key])
    
    # Plot
    sc = ax.scatter(x, y, val, c=val, cmap='viridis')
    fig.colorbar(sc, ax=ax, label='Value')
    ax.set_xlabel('x')
    ax.set_ylabel('y')
    ax.set_zlabel('val')
    ax.set_title(' ')
    #plt.show()

def construct_dg_function(data, H):
    """
    Construct exact solution with the same structure as data

    :arg data: parsed patch data in the form of a dictionary as output of parse_patch_file()
    :arg H: mesh size as list of cell sizes in each direction: [hx, hy]
    """
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D
    import mghype

    assert len(H) == 2, "Only works for 2D"

    # Create dictionary with the same keys as data and zero values
    result = {key: [0] * len(value) for key, value in data.items()}
    
    # Local coordinates of cell nodes
    poly_degree = int(np.sqrt(len(data[(0.0, 0.0)]))) - 1
    matrices = mghype.api.matrixgenerators.blockmatrix.DGPoisson2dPenaltyBlockMatrix(poly_degree)
    local_node_coordinates = matrices._Xi_C
    num_of_nodes = len(local_node_coordinates[0])

    for key in data.keys():
        # Coordinates of the left-bottom cell vertex
        xlb = key[0]
        ylb = key[1]

        # Local-to-global mapping:
        # x = xlb + hx/2*(xi  + 1)
        # y = ylb + hy/2*(eta + 1)
        for i in range(num_of_nodes):
            x_node = xlb + 0.5 * H[0] * (local_node_coordinates[0][i][0] + 1.0)
            y_node = ylb + 0.5 * H[1] * (local_node_coordinates[0][i][1] + 1.0)
            result[key][i] = np.sin(2.0*np.pi*x_node) * np.sin(2.0*np.pi*y_node)

    return result