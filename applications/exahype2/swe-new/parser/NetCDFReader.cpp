// we need a .cpp for scons and testing
#include "NetCDFReader.hpp"


    int applications::exahype2::swe::parser::NetCDFReader::openCdf(String filename)
    {
        //assert that file exists (nc_open does not do that apparently)
        struct stat buffer;
        if (stat(filename.c_str(), &buffer) != 0)
            return -1;
        int ncid;
        if (nc_open(filename.c_str(), NC_NOWRITE, &ncid) != NC_NOERR)
            return -1;
        return ncid;
    }

    int applications::exahype2::swe::parser::NetCDFReader::closeCdf(int ncid)
    {
        if (nc_close(ncid) != NC_NOERR)
            return -1;
        return 0;
    }

    size_t applications::exahype2::swe::parser::NetCDFReader::getDimension(int ncid, const char *key)
    {
        int dimidkey;
        size_t dim;
        if (nc_inq_dimid(ncid, key, &dimidkey) != NC_NOERR)
            return 0;
        if (nc_inq_dimlen(ncid, dimidkey, &dim) != NC_NOERR)
            return 0;
        return dim;
    }

    int* applications::exahype2::swe::parser::NetCDFReader::getGlobalAttInt(int ncid, const char *name)
    {
        size_t len;
        if (nc_inq_attlen(ncid, NC_GLOBAL, name, &len) != NC_NOERR || len == 0)
            return nullptr;

        int *tmp = new int[len];
        if (nc_get_att_int(ncid, NC_GLOBAL, name, tmp) != NC_NOERR)
            return nullptr;

        return tmp;
    }

    float* applications::exahype2::swe::parser::NetCDFReader::getGlobalAttFloat(int ncid, const char *name)
    {
        size_t len;
        if (nc_inq_attlen(ncid, NC_GLOBAL, name, &len) != NC_NOERR || len == 0)
            return nullptr;

        float *tmp = new float[len];
        if (nc_get_att_float(ncid, NC_GLOBAL, name, tmp) != NC_NOERR)
            return nullptr;

        return tmp;
    }

    char* applications::exahype2::swe::parser::NetCDFReader::getGlobalAttText(int ncid, const char *name)
    {
        size_t len;
        if (nc_inq_attlen(ncid, NC_GLOBAL, name, &len) != NC_NOERR || len == 0)
            return nullptr;

        char *tmp = new char[len];
        if (nc_get_att_text(ncid, NC_GLOBAL, name, tmp) != NC_NOERR)
            return nullptr;

        return tmp;
    }

    int applications::exahype2::swe::parser::NetCDFReader::readVariable1D(int ncid, const char *key, double *variable)
    {
        int varidkey;
        if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR)
            return -1;
        if (nc_get_var_double(ncid, varidkey, variable) != NC_NOERR)
            return -1;

        return 0;
    }

    int applications::exahype2::swe::parser::NetCDFReader::readVariable2D(int ncid, const char *key, double **variable, size_t rows, size_t cols)
    {
        int varidkey;
        if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR)
            return -1;
        double *tmp = new double[rows * cols];
        if (nc_get_var_double(ncid, varidkey, tmp) != NC_NOERR)
            return -1;
        size_t j = 0;
        for (size_t i = 0; i < rows * cols; i++)
        {
            if(i % rows == 0 && i > 0)
                j++;
            variable[i%rows][j] = tmp[i];
        }
        delete [] tmp;

        return 0;
    }

    int applications::exahype2::swe::parser::NetCDFReader::readVariable2D(int ncid, const char *key, double *variable, size_t rows, size_t cols)
    {
        int varidkey;
        if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR)
            return -1;
        if (nc_get_var_double(ncid, varidkey, variable) != NC_NOERR)
            return -1;

        return 0;
    }

    int applications::exahype2::swe::parser::NetCDFReader::readVariable2DHyperslab(int ncid, const char *key, double **variable, size_t rows, size_t cols, size_t start)
    {
        int varidkey;
        if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR)
            return -1;
        size_t startp[]{start, 0, 0};
        size_t countp[]{1, cols, rows};
        double *tmp = new double[cols * rows];
        if (nc_get_vara_double(ncid, varidkey, startp, countp, tmp) != NC_NOERR)
            return -1;
        // STOP LOOKING FOR ERRORS HERE
        size_t j = 0;
        for (size_t i = 0; i < rows * cols; i++)
        {
            if(i % rows == 0 && i > 0)
                j++;
            variable[i%rows][j] = tmp[i];
        }
        delete [] tmp;

        return 0;
    }

    int applications::exahype2::swe::parser::NetCDFReader::readVariable3DHyperslab(int ncid, const char *key, double *variable, size_t rows, size_t cols, size_t start, size_t end)
    {
        int varidkey;
        if(nc_inq_varid(ncid, key, &varidkey) != NC_NOERR)
            return -1;
        size_t startp[]{start, 0, 0};
        size_t countp[]{end, cols, rows};
        if(nc_get_vara_double(ncid, varidkey, startp, countp, variable) != NC_NOERR)
            return -1;
        
        return 0;
    }
