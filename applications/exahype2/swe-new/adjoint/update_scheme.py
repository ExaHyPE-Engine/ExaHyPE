import numpy as np
from adjoint_solver import compute_net_updates

def set_up_update_variables(nx, ny, cell_size) -> None:
    # initialize update arrays
    global h_net_updates_left 
    h_net_updates_left = np.zeros((nx+2, ny+2))
    global h_net_updates_right 
    h_net_updates_right = np.zeros((nx+2, ny+2))
    global h_net_updates_above 
    h_net_updates_above = np.zeros((nx+2, ny+2))
    global h_net_updates_below 
    h_net_updates_below = np.zeros((nx+2, ny+2))

    global hu_net_updates_left 
    hu_net_updates_left = np.zeros((nx+2, ny+2))
    global hu_net_updates_right 
    hu_net_updates_right = np.zeros((nx+2, ny+2))

    global hv_net_updates_below 
    hv_net_updates_below = np.zeros((nx+2, ny+2))
    global hv_net_updates_above 
    hv_net_updates_above = np.zeros((nx+2, ny+2))

    global dx
    dx = cell_size

    global size_x
    size_x = nx

    global size_y
    size_y = ny 


def _update_unknowns_x_sweep(dt, h , hu):
    # update unknowns in x-dimension
    for i in range(1, size_x+1):
        for j in range(0, size_y+2):
            h[i][j] = h[i][j] - (dt / dx * (h_net_updates_right[i-1][j] + h_net_updates_left[i][j]))
            hu[i][j] = hu[i][j] - (dt / dx * (hu_net_updates_right[i-1][j] + hu_net_updates_left[i][j]))

def _update_unknowns_y_sweep(dt, h, hv):
    # update unknowns in y-dimension
    for j in range(1, size_y+1):
        for i in range(1, size_x+1):
            h[i][j] = h[i][j] - (dt / dx * (h_net_updates_above[i][j-1] + h_net_updates_below[i][j]))
            hv[i][j] = hv[i][j] - (dt / dx * (hv_net_updates_above[i][j-1] + hv_net_updates_below[i][j]))
    
def compute_numerical_fluxes(h, b, hu, hv) -> float:
    max_wave_speed = 0.0

    max_wave_speed_x = 0.0
    for i in range(1, size_x + 2):
        for j in range(0, size_y + 2):
            max_edge_speed = 0.0
            h_net_updates_left[i-1][j], hu_net_updates_left[i-1][j], h_net_updates_right[i-1][j], hu_net_updates_right[i-1][j], max_edge_speed = compute_net_updates(h[i-1][j], h[i][j], hu[i-1][j], hu[i][j], b[i-1][j], b[i][j])
            max_wave_speed_x = max(max_edge_speed, max_wave_speed_x)

    if max_wave_speed_x > 0:
        _update_unknowns_x_sweep(0.4 * dx / max_wave_speed_x, h ,hu)
    
    max_wave_speed_y = 0.0
    for j in range(1, size_y+2):
        for i in range(1, size_x+2):
            max_edge_speed = 0.0
            h_net_updates_below[i][j-1], hv_net_updates_below[i][j-1], h_net_updates_above[i][j-1], hv_net_updates_above[i][j-1], max_edge_speed = compute_net_updates(h[i][j-1], h[i][j], hv[i][j-1], hv[i][j], b[i][j-1], b[i][j])
            max_wave_speed_y = max(max_edge_speed, max_wave_speed_y)

    max_wave_speed = max(max_wave_speed_x, max_wave_speed_y)
    # by here, we kind of assume a wave_speed greater 0
    print("max wave speed: %f" % max_wave_speed)
    if max_wave_speed > 0.00001:
        max_time_step = 0.4 * dx / max_wave_speed
        _update_unknowns_y_sweep(max_time_step, h, hv)
        return max_time_step
    return max_wave_speed