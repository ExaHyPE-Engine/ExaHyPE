## Used Commands for the Projection

# 1. Transform to tif

```bash
gdal_translate \
  -a_srs EPSG:4326 \
  NETCDF:"gebco_2024_n35.0_s15.0_w-98.0_e-70.0.nc":elevation \
  gebco_tmp.tif
```

# 2. Project to AEA

```bash
gdalwarp \
  -t_srs "+proj=aea +lon_0=-84 +lat_1=18.3333333 +lat_2=31.6666667 +lat_0=25 +datum=WGS84 +units=m +no_defs" \
  -r bilinear \
  -of netCDF \
  gebco_tmp.tif \
  o_gebco.nc
```
