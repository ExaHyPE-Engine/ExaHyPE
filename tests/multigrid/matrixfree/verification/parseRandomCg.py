import numpy as np

def parse_patch_file(file_path):
    """
    Transform data from Peano patch file into numpy array
    in the format [x_vertex, y_vertex, value]
    """
    
    with open(file_path, 'r') as file:
        lines = file.readlines()
    
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
    assert np.sqrt(np.shape(data)[0]) == (np.sqrt(np.shape(data_unique)[0]) - 1) * 2
    
    return np.array(data_unique)

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

if __name__ == "__main__":
    v_solution = parse_patch_file("CollocatedPoisson.value-1.peano-patch-file") 
    v_random   = parse_patch_file("CollocatedPoisson.rand-1.peano-patch-file")
    v_random = reorder_array(v_solution, v_random)
    print(f'{np.max(np.abs(v_solution - v_random)) = }')