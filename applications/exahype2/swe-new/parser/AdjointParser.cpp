#include "AdjointParser.h"
#include <iostream>
using namespace applications::exahype2::swe::parser::NetCDFHelper;

applications::exahype2::swe::parser::AdjointParser::AdjointParser(const char *adjoint_file_path){

    l_nX_ = DomainSize(0);
    l_nY_ = DomainSize(1);
    this->parse_adjoint_file(adjoint_file_path)

}

applications::exahype2::swe::parser::AdjointParser::~AdjointParser(){
    delete this->eta_;
    delete this->timestamps_; 
}

void applications::exahype2::swe::parser::AdjointParser::interpolate_values(size_t i_index, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b, double *interpolated_values){
    return;
}

double applications::exahype2::swe::parser::AdjointParser::calculate_inner_product(size_t i_index, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b){
    if(i_b >= 0.f)
        return 0.f;

    double interpolated_values[4]{0};
    interpolate_values(i_index, i_x, i_y, i_eta, i_hu, i_hv, i_b, interpolated_values);

    if(interpolated_values[3] - interpolated_values[0] >= 0.f)
        return 0.f;
    
    return std::abs(i_eta * interpolated_values[3] 
                    + i_hu * interpolated_values[1]
                    + i_hv * interpolated_values[2]);
}

double applications::exahype2::swe::parser::AdjointParser::max_inner_product(double i_t, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b){
    double max_inner_product = -1.f;
    for(size_t i = 0; i < number_timestamps_; i++)
    {
        double t_nm = 0.f;
        if(i > 0)
            t_nm = timestamps_[i-1];
        
        // TODO: Parse t_f and t_s from netCDF file
        // Is i_t smaller than next timestamp in adjoint solution
        // AND is i_t in the time window for guided AMR?
        if(i_t < timestamps_[i] && t_nm <= i_t + (t_f - t_s))
        {
            double inner_product_previous = 0.f;
            double inner_product_current = calculate_inner_product(i, i_x, i_y, i_eta, i_hu, i_hv, i_b);

            if(i > 0)
                inner_product_previous = calculate_inner_product(i-1, i_x, i_y, i_eta, i_hu, i_hv, i_b);
            
            max_inner_product = std::max({inner_product_current, inner_product_previous, max_inner_product});
        }
    }
}

void applications::exahype2::swe::parser::AdjointParser::parse_adjoint_file(const char *adjoint_file_path){
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

    timestamps_ = new double[number_timestamps_]

    // Read time of the last checkpoint written in netCDF.
    {
        if (NetCDFReader::readVariable1D(ncid, "time", timestamps_) != 0)
        {
            std::cout << "Guided AMR could not be initialized: Timestamps not found." << std::endl;
            return;
        }
    }

    // TODO: Timestamps might have to be reversed? Since adjoint simulation goes from end to beginning.
    // I.e.: double tmp = timestamps_[i]; timestamps_[i] = t_f - timestamps_[len-1-i]; timestamps[len-1-i] = t_f - tmp;

    // Read x-dimension of adjoint data
    if ((x_eta_ = NetCDFReader::getDimension(ncid, "x")) == 0)
    {
    std::cout << "Guided AMR could not be initialized: Dimension of the x-Adjoint could not be read." << std::endl;
    return;
    }

    // Read y-dimension of adjoint data
    if ((y_eta_ = NetCDFReader::getDimension(ncid, "y")) == 0)
    {
    std::cout << "Guided AMR could not be initialized: Dimension of the y-Adjoint could not be read." << std::endl;
    return;
    }

    eta_ = new double[x_eta_*y_eta_];
    // Read eta
    if (NetCDFReader::readVariable2D(ncid, "eta", eta_, x_eta_, y_eta_) == -1)
    {
    std::cout << "Guided AMR could not be initialized: Eta could not be read." << std::endl;
    return;
    }

    // Get max and min x
    {
    double tmp[x_eta_]{0};
    if (NetCDFReader::readVariable1D(ncid, "x", tmp) == -1)
    {
        std::cout << "Guided AMR could not be initialized: Adjoint x indices could not be read." << std::endl;
        return;
    }
    min_x_eta_ = tmp[0];
    max_x_eta_ = tmp[x_eta_ - 1];
    }

    // Get max and min y
    {
    double tmp[y_eta_]{0};
    if (NetCDFReader::readVariable1D(ncid, "y", tmp) == -1)
    {
        std::cout << "Guided AMR could not be initialized: Adjoint y indices could not be read." << std::endl;
        return;
    }
    min_y_eta_ = tmp[0];
    max_y_eta_ = tmp[y_eta_ - 1];
    }

    // Close adjoint netCDF file
    if (NetCDFReader::closeCdf(ncid) == -1)
    {
    std::cout << "Guided AMR could not be initialized: Adjoint file could not be closed." << std::endl;
    return;
    }
}