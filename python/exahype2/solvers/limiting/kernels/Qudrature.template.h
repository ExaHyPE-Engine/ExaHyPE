template <typename T>
class Quadrature{
  public:
    static void initQuadratureNodesAndWeights();
    static void freeQuadratureNodesAndWeights();

    {% if kernelType=="limiter" %}
    // limiter projection matrices
    static T* uh2lob;
    static T* dg2fv;
    static T* fv2dg;
    {% endif %}
};
