#ifndef NETCDFREADER_H
#define NETCDFREADER_H
#include <string>
#include <sys/stat.h>
#include <netcdf.h>

typedef std::string String;

namespace applications::exahype2::swe::parser::NetCDFReader
{

    /**
     * @brief Opens a netCDF file, returns ncid.
     * 
     * @param filename path of file to be opened.
     * @return id of netCDF file for further operations.
     */
    int openCdf(String filename);

    /**
     * @brief Closes netCDF file
     * 
     * @param ncid Pointer to location where returned netCDF ID is to be stored.
     * @return 0 iff successfull, -1 otherwise.
     */
    int closeCdf(int ncid);

    /**
     * @brief Get the dimension of a variable.
     * 
     * It is assumed that the dimension inquired is greater 0.
     * 
     * @param ncid Pointer to location where returned netCDF ID is to be stored.
     * @param key name of the variable.
     * @return size_t dimension or 0 if an error occured.
     */
    size_t getDimension(int ncid, const char *key);

    /**
     * @brief reads the global attribute name of type int and returns an array containing all the associated values.
     * 
     * @param ncid id of the netCDF file.
     * @param name name of the attribute.
     * @return int* array with values stored in the attribute or nullptr if unsuccessfull.
     */
    int *getGlobalAttInt(int ncid, const char *name);

    /**
     * @brief reads the global attribute name of type float and returns an array containing all the associated values.
     * 
     * @param ncid id of the netCDF file.
     * @param name name of the attribute.
     * @return float* array with the values stored in the attribute or nullptr if unsuccessfull.
     */
    float *getGlobalAttFloat(int ncid, const char *name);

    /**
     * @brief reads the global attribute name of type text and returns an array containing all the associated values.
     * 
     * @param ncid id of the netCDF file.
     * @param name name of the attribute.
     * @return char* array with the values stored in the attribute or nullptr if unsuccessfull.
     */
    char *getGlobalAttText(int ncid, const char *name);

    /**
     * @brief Read contents of a 1D double variable from netCDF file.
     * 
     * @param ncid Pointer to location where returned netCDF ID is to be stored.
     * @param key variable name for which data is loaded.
     * @param variable Array where data is stored.
     * @return 0 iff successfull, -1 otherwise.
     */
    int readVariable1D(int ncid, const char *key, double *variable);

    /**
     * @brief Read contents of a 2D double variable from netCDF file.
     * 
     * @param ncid Pointer to location where returned netCDF ID is to be stored.
     * @param key variable name for which data is loaded.
     * @param variable Array where data is stored.
     * @param rows count of rows of the array.
     * @param cols count of columns of the array.
     * @return 0 iff successfull, -1 otherwise.
     */
    int readVariable2D(int ncid, const char *key, double **variable, size_t rows, size_t cols);

    /**
     * @brief Read contents of a 2D double variable from netCDF file into 1D array.
     * 
     * @param ncid Pointer to location where returned netCDF ID is to be stored.
     * @param key variable name for which data is loaded.
     * @param variable Array where data is stored.
     * @param rows count of rows of the array.
     * @param cols count of columns of the array.
     * @return 0 iff successfull, -1 otherwise.
     */
    int readVariable2D(int ncid, const char *key, double *variable, size_t rows, size_t cols);

    /**
     * @brief Read contents of a 2D double variable from netCDF file starting at a specified first-dimension value.
     * 
     * THIS FUNCTION WORKS AS INTENDED!!
     * 
     * @param ncid Pointer to location where returned netCDF ID is to be stored.
     * @param key variable name for which data is loaded.
     * @param variable Array where data is stored.
     * @param rows count of rows of the array.
     * @param cols count of columns of the array.
     * @param start value of the first dimension from which to start reading.
     * @return 0 iff successfull, -1 otherwise.
     */
    int readVariable2DHyperslab(int ncid, const char *key, double **variable, size_t rows, size_t cols, size_t start);
}
#endif