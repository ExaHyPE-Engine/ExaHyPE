import Wrapper
import numpy as np
from pyproj import Transformer
import MyAssert

# 1. Gitter in lon/lat erzeugen
lons = np.linspace(-98, -70, 1000)
lats = np.linspace(15, 35, 1000)
LON, LAT = np.meshgrid(lons, lats)

# 2. Albers-Projektion (WGS84 → AEA)
proj_str = (
    "+proj=aea "
    "+lon_0=-84 +lat_1=18.3333333 +lat_2=31.6666667 +lat_0=25 "
    "+a=6371000 +b=6371000 "
    "+units=m +no_defs"
)
transformer = Transformer.from_crs(proj_str, "EPSG:4326", always_xy=True)

# 3. Inverse Projektion mit deiner Funktion `Z(x, y)` → gibt vermutlich (lon, lat)
# Wenn Z nur `lat` zurückgibt, dann:
X = np.vectorize(lambda x, y: Wrapper.sphereTransform(x, y).first)(LON, LAT)
Y = np.vectorize(lambda x, y: Wrapper.sphereTransform(x, y).second)(LON, LAT)

lon_reconstructed, lat_reconstructed = transformer.transform(X, Y)

lon_diff = lon_reconstructed - LON
MyAssert.test_array_within_threshold(lon_diff)

lat_diff = lat_reconstructed - LAT
MyAssert.test_array_within_threshold(lat_diff)
