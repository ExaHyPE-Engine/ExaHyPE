#ifndef __INITIAL_DATA_ADAPTER_CPP_FORTRAN_MHD__
#define __INITIAL_DATA_ADAPTER_CPP_FORTRAN_MHD__

extern "C" {
    // FORTRAN functions called by C
    void initialalfenwave_(double* x, double* Q);
    void initialblast_(double* x, double* Q);
    void initialorsagtang_(double* x, double* Q);
    void initialrotor_(double* x, double* Q);
    void alfenwave_(double* x, double* Q, double* /* scalar */ t);
}/* extern "C" */
#endif /* __INITIAL_DATA_ADAPTER_CPP_FORTRAN_MHD__ */
