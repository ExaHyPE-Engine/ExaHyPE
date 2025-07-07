#include "toolbox/curvi/kdTree/Root.h"

toolbox::curvi::Root::Root(const YAML::Node& node, double a_offset[3], double a_size[3], uint a_grid[3]) {
  isInit = false;

  std::copy_n(a_size, 3, size);
  std::copy_n(a_offset, 3, offset);
  std::copy_n(a_grid, 3, grid);
  // check if required parameters exist
  if (bool(node) && bool(node["top"]) & bool(node["coordinates"]) & bool(node["perturbation"])) {

    /*set up coordinates and position of topography*/
    top = node["top"].as<bool>();

    std::vector<std::string> coords_s = node["coordinates"].as<std::vector<std::string>>();
    coords[0]                         = stringToCoord(coords_s[0]);
    coords[1]                         = stringToCoord(coords_s[1]);
    if (coords[0] == coords[1]) {
      throw std::invalid_argument("Topography must be defined on two different coordinates");
    }
    std::set<std::string> in;
    if (node["perturbation"].Tag() == "!ASAGI") {
      in.insert("x");
      in.insert("y");
    } else {
      in.insert(coordToLowercaseString(coords[0]));
      in.insert(coordToLowercaseString(coords[1]));
    }
    /*--------------------------*/

    /* Construct domain_boundary from topography */
    easi::YAMLParser parser(2);
    model = parser.parse(node["perturbation"], in);

    topo = std::function<double(double, double)>([&](double y, double x) {
      static double               result[1];
      easi::ArraysAdapter<double> adapter;
      easi::Query                 query(1, 2);

      // Easi binding point for topography
      std::string c_norm = coordToLowercaseString(getNormal(coords[0], coords[1]));
      adapter.addBindingPoint(c_norm, result);

      query.x(0, 0) = x;
      query.x(0, 1) = y;

      model->evaluate(query, adapter);
      return result[0];
    });

    final_topo = topo;

    /*init next node*/
    if (bool(node["split"]))
      child = new InnerNode(node["split"]);
    else {
      child = new Leaf();
    }
    /*--------------------------*/

  } else {
    std::stringstream ss;
    ss << "Missing tag in topography:" << std::endl;
    ss << node << std::endl;
    ss << "instead of:" << std::endl;
    ss << "coordinates: [{\"X\"|\"Y\"|\"Z\"},{\"X\"|\"Y\"|\"Z\"}]" << std::endl;
    ss << "top true|false" << std::endl;
    ss << "perturbation: EASI_MAP" << std::endl;
    throw std::invalid_argument(ss.str());
  }
}

toolbox::curvi::Root::~Root() {
  delete model;
  delete child;
}

void toolbox::curvi::Root::init(uint modes[3]) {
  Coordinate c_depth        = getNormal(coords[0], coords[1]);
  int        indexOffset[3] = {0, 0, 0}; // for now we start at index offset 000 for the whole domain
  int        indexSize[3];

  if (top) {
    indexOffset[c_depth] += grid[c_depth] - 1;
    offset[c_depth] += size[c_depth];
  }

  std::copy_n(grid, 3, indexSize);
  indexSize[c_depth] = 1;

  FaceBoundary  topography_face_boundary(offset, size, indexSize, coords);
  Face          topography_face(indexOffset, topography_face_boundary, final_topo);
  BlockBoundary domain_boundary(topography_face, top, grid[c_depth], size[c_depth]);

  child->init(domain_boundary, modes);
  isInit = true;
}

toolbox::curvi::BlockBoundary toolbox::curvi::Root::traverse(int a_index[3]) {
  checkInit();

  return child->traverse(a_index);
}


toolbox::curvi::BlockBoundary toolbox::curvi::Root::traverseProjection(double coords[3]) {
  checkInit();

  return child->traverseProjection(coords);
}

toolbox::curvi::Block toolbox::curvi::Root::traverse() {
  checkInit();

  return child->traverse();
}

void toolbox::curvi::Root::checkInit() {
  if (!isInit) {
    throw std::logic_error("Root not initialized yet");
  }
}

std::string toolbox::curvi::Root::toString(int depth) {
  checkInit();

  std::stringstream ss;
  ss << "Root Coords: " << coordToString(coords[0]) << " " << coordToString(coords[1]) << " top: " << top << std::endl;
  ss << child->toString(depth + 1);
  return ss.str();
}

void toolbox::curvi::Root::multiplyToTopography(std::function<double(double, double)> factor) {
  final_topo = [&](double y, double x) { return factor(y, x) * topo(y, x); };
  //    topo = [&](double y, double x) {return (*topo_ptr)(y,x) * (*factor)(y,x);}; */
}