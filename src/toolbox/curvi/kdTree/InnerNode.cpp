#include "toolbox/curvi/kdTree/InnerNode.h"

#include "toolbox/curvi/kdTree/Leaf.h"

toolbox::curvi::InnerNode::InnerNode(const YAML::Node& node) {
  isInit = false;

  if (bool(node["coordinates"]) & bool(node["perturbation"]) & bool(node["position"])) {

    /*set up coordinates position and index of fault*/
    std::vector<std::string> coords_s = node["coordinates"].as<std::vector<std::string>>();

    coords[0] = stringToCoord(coords_s[0]);
    coords[1] = stringToCoord(coords_s[1]);

    if (coords[0] == coords[1]) {
      throw std::invalid_argument("Face must be defined on two different coordinates");
    }

    std::set<std::string> in;
    in.insert(coordToLowercaseString(coords[0]));
    in.insert(coordToLowercaseString(coords[1]));

    position = node["position"].as<double>();

    if (bool(node["index"])) {
      index = node["index"].as<int>();
    } else {
      index = -1; // only works because we assume index >= 0
    }
    /*-------------------------------------*/

    /*Construct splitting face*/
    easi::YAMLParser parser(2);
    model = parser.parse(node["perturbation"], in);

    /*define perturbation*/
    perturbation = [&](double y, double x) {
      static double result[1];

      easi::ArraysAdapter<double> adapter;
      easi::Query                 query(1, 2);

      // Easi binding point for topography
      std::string c_norm = coordToLowercaseString(getNormal(coords[0], coords[1]));
      adapter.addBindingPoint(c_norm, result);

      query.x(0, 0) = x;
      query.x(0, 1) = y;

      model->evaluate(query, adapter);
      return result[0];
    };
    /*----------------------------------------------*/

    /*Initialize children*/
    if (node["left"]) {
      left = new InnerNode(node["left"]);
    } else {
      left = new Leaf();
    }

    if (node["right"]) {
      right = new InnerNode(node["right"]);
    } else {
      right = new Leaf();
    }
  } else {
    std::stringstream ss;
    ss << "Splitting face not defined the right way: " << std::endl;
    std::cout << node << std::endl;
    throw std::invalid_argument(ss.str());
  }
}

toolbox::curvi::InnerNode::~InnerNode() {
  delete left;
  delete right;
  delete model;
}

std::string toolbox::curvi::InnerNode::toString(int depth) {
  std::stringstream ss;
  for (int i = 0; i < depth; i++) {
    ss << "\t";
  }
  ss << "Inner Node Coords: " << coordToString(coords[0]) << " " << coordToString(coords[1]) << " position " << position
     << " index " << index << std::endl;
  ss << left->toString(depth + 1);
  ss << right->toString(depth + 1);
  return ss.str();
}

void toolbox::curvi::InnerNode::init(BlockBoundary& boundary, uint modes[3]) {
  int indexOffset_fault[3];

  if (index == -1) {
    Coordinate n = getNormal(coords[0], coords[1]);
    double     box_offset[3];
    double     box_size[3];
    boundary.getBoundingBox(box_offset, box_size);
    index = int(std::round((position - box_offset[n]) / box_size[n] * boundary.getIndexSize(n)));
    index = int(std::round(index / (modes[n] - 1)) * (modes[n] - 1));
  }

  FaceBoundary fault_boundary = boundary.setupSplittingFace(coords, index, position, indexOffset_fault);

  Face fault(indexOffset_fault, fault_boundary, perturbation);
  /*----------------------------------------------*/

  /*Split an initialize children*/
  Coordinate boundary_coords[3];
  boundary.getCoord(boundary_coords);
  BlockBoundary block_boundary_left(boundary_coords);
  BlockBoundary block_boundary_right(boundary_coords);

  boundary.split(fault, block_boundary_left, block_boundary_right);
  left->init(block_boundary_left, modes);
  right->init(block_boundary_right, modes);

  isInit = true;
}

toolbox::curvi::BlockBoundary toolbox::curvi::InnerNode::traverse(int a_index[3]) {
  checkInit();

  Coordinate normal = getNormal(coords[0], coords[1]);

  if (a_index[normal] < index) {
    return left->traverse(a_index);
  } else {
    return right->traverse(a_index);
  }
}

toolbox::curvi::BlockBoundary toolbox::curvi::InnerNode::traverseProjection(double coordinate[3]) {
  checkInit();

  Coordinate normal = getNormal(coords[0], coords[1]);

  if (coordinate[normal] < position) {
    return left->traverseProjection(coordinate);
  } else {
    return right->traverseProjection(coordinate);
  }
}

toolbox::curvi::Block toolbox::curvi::InnerNode::traverse() {
  checkInit();

  return left->traverse() + right->traverse();
}

void toolbox::curvi::InnerNode::setPosition(double a_position) { position = a_position; }

double toolbox::curvi::InnerNode::getPosition() { return position; }

void toolbox::curvi::InnerNode::getCoordinates(Coordinate a_coords[2]) {
  a_coords[0] = coords[0];
  a_coords[1] = coords[1];
}

toolbox::curvi::Coordinate toolbox::curvi::InnerNode::getFaceNormal() { return getNormal(coords[0], coords[1]); }

double toolbox::curvi::InnerNode::evalPerturbation(double y, double x) { return perturbation(y, x); }

void toolbox::curvi::InnerNode::checkInit() {
  if (!isInit) {
    throw std::logic_error("Node not initialized yet");
  }
}