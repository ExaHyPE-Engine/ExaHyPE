#include "AdjointParser.h"
#include <iostream>
using namespace applications::exahype2::swe::parser::NetCDFHelper;

applications::exahype2::swe::parser::AdjointParser::AdjointParser(const char *adjoint_file_path, double start_time){
    this->parse_adjoint_file(adjoint_file_path, start_time);

}

applications::exahype2::swe::parser::AdjointParser::~AdjointParser(){
    delete this->adjoint_h_;
    delete this->adjoint_hu_;
    delete this->adjoint_hv_;
    delete this->adjoint_b_;
    delete this->adjoint_timestamps_; 
}

void applications::exahype2::swe::parser::AdjointParser::interpolate_values(size_t i_index, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b, double *interpolated_values){
    double x_cdf = NetCDFHelper::transformIndexSimulationToCDFRange(i_x, size_x_adjoint_, min_x_adjoint_, max_x_adjoint_);
    double y_cdf = NetCDFHelper::transformIndexSimulationToCDFRange(i_y, size_y_adjoint_, min_y_adjoint_, max_y_adjoint_);
    if(x_cdf < min_x_adjoint_ || x_cdf > max_x_adjoint_ || y_cdf < min_y_adjoint_ || y_cdf > max_y_adjoint_)
        return;
    int array_index = NetCDFHelper::transformIndexCDFRangeToArray(x_cdf, y_cdf, min_x_adjoint_, min_y_adjoint_, max_x_adjoint_, max_y_adjoint_, size_x_adjoint_, size_y_adjoint_);
    int array_index_shift = (number_timestamps_ - i_index - 1) * array_index; // reverese time as adjoint simulation has to be backwards in time but is computed forwards in time
    interpolated_values[0] = adjoint_h_[array_index_shift]; // get h of adjoint
    interpolated_values[1] = adjoint_hu_[array_index_shift]; // get hu of adjoint
    interpolated_values[2] = adjoint_hv_[array_index_shift]; // get hv of adjoint
    interpolated_values[3] = adjoint_b_[array_index]; // get bathymetry of adjoint
    return;
}

double applications::exahype2::swe::parser::AdjointParser::calculate_inner_product(size_t i_index, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b){
    if(i_b >= 0.f)
        return 0.f;

    double interpolated_values[4]{0};
    interpolate_values(i_index, i_x, i_y, i_eta, i_hu, i_hv, i_b, interpolated_values);
    // interpolated_values[0] is h
    // interpolated_values[1] is hu
    // interpolated_values[2] is hv
    // interpolated_values[3] is bathymetry

    if(interpolated_values[3] >= 0.f)
        return 0.f;
    
    return std::abs(i_eta * (interpolated_values[0] + interpolated_values[3])
                    + i_hu * interpolated_values[1]
                    + i_hv * interpolated_values[2]);
}

double applications::exahype2::swe::parser::AdjointParser::max_inner_product(double i_t, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b){

    if(i_t != t_last_call_)
    {
        t_last_call_ = i_t;
        set_start_index(i_t);
    }

    double t_nm = adjoint_timestamps_[inner_product_start_index_-1];

    double t_current;
    double inner_product_current;
    double t_comp = i_t + (t_f_ - t_s_);
    double max_inner_product = calculate_inner_product(inner_product_start_index_-1, i_x, i_y, i_eta, i_hu, i_hv, i_b);

    for(size_t i = inner_product_start_index_; i < number_timestamps_; i++)
    {
        t_current = adjoint_timestamps_[i];
        if(i_t < t_current && t_nm <= t_comp)
        {
            inner_product_current = calculate_inner_product(i, i_x, i_y, i_eta, i_hu, i_hv, i_b);
            max_inner_product = std::max(inner_product_current, max_inner_product);
            t_nm = t_current;
        }
        else
            break;
    }
    if(max_inner_product > 0 && false)
        std::cout << "max_inner_product: " << max_inner_product << std::endl;
    return max_inner_product;
}

void applications::exahype2::swe::parser::AdjointParser::set_start_index(double i_t){
    double t_nm = adjoint_timestamps_[inner_product_start_index_ - 1];
    for(size_t i = inner_product_start_index_; i < number_timestamps_; i++)
    {
        if(t_nm <= i_t + (t_f_ - t_s_) && adjoint_timestamps_[i] > i_t)
        {
            inner_product_start_index_ = i;
            return;
        }
        t_nm = adjoint_timestamps_[i];
    }
}

void applications::exahype2::swe::parser::AdjointParser::parse_adjoint_file(const char *adjoint_file_path, double start_time){
    int ncid;
    // Open adjoint netCDF file
    if ((ncid = NetCDFReader::openCdf(adjoint_file_path)) == -1)
    {
    std::cout << "Guided AMR could not be initialized: Adjoint file could not be opened at: "<< adjoint_file_path << std::endl;
    return;
    }

    // Read number of checkpoints completed by adjoint simulation.
    // There needs to be at least one record in the netCDF file!
    if ((number_timestamps_ = NetCDFReader::getDimension(ncid, "time")) == 0)
    {
        std::cout << "Guided AMR could not be initialized: No timestamps written so far." << std::endl;
        return;
    }

    adjoint_timestamps_ = new double[number_timestamps_];

    // Read time of the last checkpoint written in netCDF.
    {
        if (NetCDFReader::readVariable1D(ncid, "time", adjoint_timestamps_) != 0)
        {
            std::cout << "Guided AMR could not be initialized: Timestamps not found." << std::endl;
            return;
        }
    }

    // Read x-dimension of adjoint data
    if ((size_x_adjoint_ = NetCDFReader::getDimension(ncid, "x")) == 0)
    {
    std::cout << "Guided AMR could not be initialized: Dimension of the x-Adjoint could not be read." << std::endl;
    return;
    }

    // Read y-dimension of adjoint data
    if ((size_y_adjoint_ = NetCDFReader::getDimension(ncid, "y")) == 0)
    {
    std::cout << "Guided AMR could not be initialized: Dimension of the y-Adjoint could not be read." << std::endl;
    return;
    }

    adjoint_h_ = new double[number_timestamps_*size_x_adjoint_*size_y_adjoint_];
    // Read h
    if (NetCDFReader::readVariable3DHyperslab(ncid, "h", adjoint_h_, size_x_adjoint_, size_y_adjoint_, 0, number_timestamps_) == -1)
    {
    std::cout << "Guided AMR could not be initialized: Adjoint h could not be read." << std::endl;
    return;
    }

    adjoint_hu_ = new double[number_timestamps_*size_x_adjoint_*size_y_adjoint_];
    // Read hu
    if (NetCDFReader::readVariable3DHyperslab(ncid, "hu", adjoint_hu_, size_x_adjoint_, size_y_adjoint_, 0, number_timestamps_) == -1)
    {
    std::cout << "Guided AMR could not be initialized: Adjoint hu could not be read." << std::endl;
    return;
    }

    adjoint_hv_ = new double[number_timestamps_*size_x_adjoint_*size_y_adjoint_];
    // Read hv
    if (NetCDFReader::readVariable3DHyperslab(ncid, "hv", adjoint_hv_, size_y_adjoint_, size_x_adjoint_, 0, number_timestamps_) == -1)
    {
    std::cout << "Guided AMR could not be initialized: Adjoint hv could not be read." << std::endl;
    return;
    }
    
    adjoint_b_ = new double[size_x_adjoint_*size_y_adjoint_];
    // Read b
    if (NetCDFReader::readVariable2D(ncid, "b", adjoint_b_) == -1)
    {
    std::cout << "Guided AMR could not be initialized: Adjoint bathymetry could not be read." << std::endl;
    return;
    }

    // Get max and min x
    {
    double tmp[size_x_adjoint_]{0};
    if (NetCDFReader::readVariable1D(ncid, "x", tmp) == -1)
    {
        std::cout << "Guided AMR could not be initialized: Adjoint x indices could not be read." << std::endl;
        return;
    }
    min_x_adjoint_ = tmp[0];
    max_x_adjoint_ = tmp[size_x_adjoint_ - 1];
    }

    // Get max and min y
    {
    double tmp[size_y_adjoint_]{0};
    if (NetCDFReader::readVariable1D(ncid, "y", tmp) == -1)
    {
        std::cout << "Guided AMR could not be initialized: Adjoint y indices could not be read." << std::endl;
        return;
    }
    min_y_adjoint_ = tmp[0];
    max_y_adjoint_ = tmp[size_y_adjoint_ - 1];
    }

    // Read end time
    {
        double *tmp = nullptr;
        if((tmp = NetCDFReader::getGlobalAttDouble(ncid, "end_time")) == nullptr)
        {
            std::cout << "Guided AMR could not be initialized: Adjoint end_time could not be read." << std::endl;
            return;
        }
        t_f_ = tmp[0];
    }

    // Read start time
    if(isinf(start_time))
    {
        double *tmp = nullptr;
        if((tmp = NetCDFReader::getGlobalAttDouble(ncid, "start_time")) == nullptr)
        {
            std::cout << "Guided AMR could not be initialized: Adjoint start_time could not be read." << std::endl;
            return;
        }
        t_s_ = tmp[0];
    }
    else
    {
        t_s_ = start_time;
    }

    // Close adjoint netCDF file
    if (NetCDFReader::closeCdf(ncid) == -1)
    {
    std::cout << "Guided AMR could not be initialized: Adjoint file could not be closed." << std::endl;
    return;
    }
}