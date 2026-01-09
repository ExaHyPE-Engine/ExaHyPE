#include "../EllipsoidProjection.h"
#include "../SphereProjection.h"

extern "C" {
struct Pair {
  double first;
  double second;
};

Pair ellipsoidTransform(const double lon, const double lat) {
  Pair p{};
  auto [x, y] = projection::albers_equal_area::ellipsoid::forward(lon, lat);
  p.first     = x;
  p.second    = y;
  return p;
}

Pair ellipsoidTransformInverse(const double x, const double y) {
  Pair p{};
  auto [lon, lat] = projection::albers_equal_area::ellipsoid::inverse(x, y);
  p.first         = lon;
  p.second        = lat;
  return p;
}

Pair sphereTransform(const double lon, const double lat) {
  Pair p{};
  auto [x, y] = projection::albers_equal_area::sphere::forward(lon, lat);
  p.first     = x;
  p.second    = y;
  return p;
}

Pair sphereTransformInverse(const double x, const double y) {
  Pair p{};
  auto [lon, lat] = projection::albers_equal_area::sphere::inverse(x, y);
  p.first         = lon;
  p.second        = lat;
  return p;
}
}
