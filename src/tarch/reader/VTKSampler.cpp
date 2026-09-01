// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTKSampler.h"

#include "tarch/instrumentation/Info.h"

namespace {
  std::string extensionOf(const std::string& filePath) {
    std::string extension = std::filesystem::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) {
      return static_cast<char>(std::tolower(character));
    });
    return extension;
  }
} // namespace

tarch::reader::VTKSampler::VTKSampler(const std::string& filePath, const std::vector<std::string>& fieldNames, int step):
  _filePath(filePath) {
  read(fieldNames, step);
  createLookupStructure();

  logInfo(
    "VTKSampler(...): read step " << _step << " of " << _numberOfSteps << " (t=" << _timeStamp << ", "
                                  << _numberOfPoints << (_samplesCellData ? " cells" : " points") << ") from \""
                                  << _filePath << "\""
  );
}

tarch::reader::VTKSampler::VTKSampler(
  const std::string& filePath, std::initializer_list<std::string> fieldNames, int step
):
  VTKSampler(filePath, std::vector<std::string>(fieldNames), step) {}

void tarch::reader::VTKSampler::read(const std::vector<std::string>& fieldNames, int step) {
  const std::string extension = extensionOf(_filePath);

  if (extension == ".vtkhdf" or extension == ".hdf5" or extension == ".h5" or extension == ".hdf") {
    readVTKHDF(fieldNames, step);
  } else if (extension == ".pvd" or extension == ".pvtu" or extension == ".vtu") {
    readVTU(fieldNames, step);
  } else {
    throw std::runtime_error(
      "Could not read \"" + _filePath + "\": the extension \"" + extension
      + "\" belongs to none of the supported formats (.vtkhdf, .h5, .hdf5, .hdf, .pvd, .pvtu, .vtu)."
    );
  }
}

void tarch::reader::VTKSampler::createLookupStructure() {
  // A uniform bucket grid over the point cloud's bounding box. The sample
  // points of a patch-based solver are close to uniform, so a few points per
  // bucket turn the nearest-neighbour search into a scan of a handful of
  // candidates.
  double boundingBoxMax[3];
  for (int d = 0; d < 3; d++) {
    _boundingBoxMin[d] = _positions[d];
    boundingBoxMax[d]  = _positions[d];
  }
  for (std::size_t point = 1; point < _numberOfPoints; point++) {
    for (int d = 0; d < 3; d++) {
      _boundingBoxMin[d] = std::min(_boundingBoxMin[d], _positions[3 * point + d]);
      boundingBoxMax[d]  = std::max(boundingBoxMax[d], _positions[3 * point + d]);
    }
  }

  // The extent is zero along the third axis of a 2d plot, which contributes
  // neither to the bucket volume nor to the bucket count.
  constexpr double TargetPointsPerBucket = 2.0;

  double extent[3];
  double volume            = 1.0;
  int    nonDegenerateAxes = 0;
  for (int d = 0; d < 3; d++) {
    extent[d] = boundingBoxMax[d] - _boundingBoxMin[d];
    if (extent[d] > 0.0) {
      volume *= extent[d];
      nonDegenerateAxes++;
    }
  }

  const double targetNumberOfBuckets = std::max(1.0, static_cast<double>(_numberOfPoints) / TargetPointsPerBucket);
  const double bucketEdge = (nonDegenerateAxes > 0) ? std::pow(volume / targetNumberOfBuckets, 1.0 / nonDegenerateAxes)
                                                    : 1.0;

  std::size_t totalNumberOfBuckets = 1;
  for (int d = 0; d < 3; d++) {
    if (extent[d] > 0.0 and bucketEdge > 0.0) {
      const double buckets = std::ceil(extent[d] / bucketEdge);
      _numberOfBuckets[d]  = static_cast<int>(std::min(buckets, static_cast<double>(_numberOfPoints)));
      _numberOfBuckets[d]  = std::max(_numberOfBuckets[d], 1);
      _bucketSize[d]       = extent[d] / _numberOfBuckets[d];
    } else {
      _numberOfBuckets[d] = 1;
      // A degenerate axis must not shrink the search radius, so its bucket is
      // as wide as the whole cloud.
      _bucketSize[d] = std::max(extent[d], 1.0);
    }
    totalNumberOfBuckets *= static_cast<std::size_t>(_numberOfBuckets[d]);
  }

  // Counting sort of the points into the buckets (CSR).
  _bucketStart.assign(totalNumberOfBuckets + 1, 0);
  std::vector<int> bucketOfPoint(_numberOfPoints);

  for (std::size_t point = 0; point < _numberOfPoints; point++) {
    int index[3];
    for (int d = 0; d < 3; d++) {
      const double relative = (_positions[3 * point + d] - _boundingBoxMin[d]) / _bucketSize[d];
      index[d]              = std::clamp(static_cast<int>(relative), 0, _numberOfBuckets[d] - 1);
    }
    const int bucket     = getBucketIndex(index[0], index[1], index[2]);
    bucketOfPoint[point] = bucket;
    _bucketStart[bucket + 1]++;
  }

  for (std::size_t bucket = 0; bucket < totalNumberOfBuckets; bucket++) {
    _bucketStart[bucket + 1] += _bucketStart[bucket];
  }

  std::vector<int> cursor(_bucketStart.begin(), _bucketStart.end() - 1);
  _pointsInBuckets.resize(_numberOfPoints);
  for (std::size_t point = 0; point < _numberOfPoints; point++) {
    _pointsInBuckets[cursor[bucketOfPoint[point]]++] = static_cast<int>(point);
  }
}

int tarch::reader::VTKSampler::getBucketIndex(int i, int j, int k) const {
  return (k * _numberOfBuckets[1] + j) * _numberOfBuckets[0] + i;
}

int tarch::reader::VTKSampler::getClosestPoint(double x, double y, double z) const {
  const double query[3] = {x, y, z};

  int centre[3];
  for (int d = 0; d < 3; d++) {
    const double relative = (query[d] - _boundingBoxMin[d]) / _bucketSize[d];
    centre[d]             = std::clamp(static_cast<int>(relative), 0, _numberOfBuckets[d] - 1);
  }

  // Grow a box of buckets around the query until the closest candidate found so
  // far cannot be beaten by anything outside that box. One ring is enough for a
  // query that sits on a sample point, which is the restart case.
  // Only an axis that actually holds several buckets narrows the search down,
  // so a degenerate one (the third axis of a 2d plot) must not enter either the
  // termination radius or the guaranteed distance.
  double smallestBucketSize = std::numeric_limits<double>::max();
  int    maximumRadius      = 0;
  for (int d = 0; d < 3; d++) {
    if (_numberOfBuckets[d] > 1) {
      smallestBucketSize = std::min(smallestBucketSize, _bucketSize[d]);
      maximumRadius      = std::max(maximumRadius, _numberOfBuckets[d]);
    }
  }

  int    closestPoint    = -1;
  double closestDistance = std::numeric_limits<double>::max();

  for (int radius = 0; radius <= maximumRadius; radius++) {
    for (int k = centre[2] - radius; k <= centre[2] + radius; k++) {
      if (k < 0 or k >= _numberOfBuckets[2]) {
        continue;
      }
      for (int j = centre[1] - radius; j <= centre[1] + radius; j++) {
        if (j < 0 or j >= _numberOfBuckets[1]) {
          continue;
        }
        for (int i = centre[0] - radius; i <= centre[0] + radius; i++) {
          if (i < 0 or i >= _numberOfBuckets[0]) {
            continue;
          }
          // Only the newly added shell has to be scanned.
          const bool isOnShell = radius == 0 or std::abs(i - centre[0]) == radius or std::abs(j - centre[1]) == radius
                                 or std::abs(k - centre[2]) == radius;
          if (not isOnShell) {
            continue;
          }

          const int bucket = getBucketIndex(i, j, k);
          for (int entry = _bucketStart[bucket]; entry < _bucketStart[bucket + 1]; entry++) {
            const int    point    = _pointsInBuckets[entry];
            const double dx       = _positions[3 * point + 0] - query[0];
            const double dy       = _positions[3 * point + 1] - query[1];
            const double dz       = _positions[3 * point + 2] - query[2];
            const double distance = dx * dx + dy * dy + dz * dz;
            if (distance < closestDistance) {
              closestDistance = distance;
              closestPoint    = point;
            }
          }
        }
      }
    }

    // Everything outside the scanned box is at least radius buckets away from
    // the query's own bucket, so a closer candidate can no longer show up.
    const double guaranteedDistance = radius * smallestBucketSize;
    if (closestPoint >= 0 and closestDistance <= guaranteedDistance * guaranteedDistance) {
      break;
    }
  }

  if (closestPoint < 0) {
    throw std::runtime_error(
      "Could not find a sample point of \"" + _filePath + "\" close to (" + std::to_string(x) + "," + std::to_string(y)
      + "," + std::to_string(z) + ")."
    );
  }

  return closestPoint;
}

double tarch::reader::VTKSampler::getValue(const std::string& fieldName, int component, int point) const {
  const auto field = _fields.find(fieldName);
  if (field == _fields.end()) {
    throw std::runtime_error("No field \"" + fieldName + "\" has been read from \"" + _filePath + "\".");
  }
  if (component < 0 or component >= field->second.numberOfComponents) {
    throw std::runtime_error(
      "Component " + std::to_string(component) + " does not exist for field \"" + fieldName + "\" of \"" + _filePath
      + "\", which holds " + std::to_string(field->second.numberOfComponents) + " components."
    );
  }
  if (point < 0 or static_cast<std::size_t>(point) >= _numberOfPoints) {
    throw std::runtime_error(
      "Point " + std::to_string(point) + " does not exist in \"" + _filePath + "\", which holds "
      + std::to_string(_numberOfPoints) + " sample points."
    );
  }

  return field->second.values[point * field->second.numberOfComponents + component];
}

double tarch::reader::VTKSampler::sample(const std::string& fieldName, int component, double x, double y, double z)
  const {
  return getValue(fieldName, component, getClosestPoint(x, y, z));
}

double tarch::reader::VTKSampler::getTimeStamp() const { return _timeStamp; }

int tarch::reader::VTKSampler::getStep() const { return _step; }

int tarch::reader::VTKSampler::getNumberOfSteps() const { return _numberOfSteps; }

int tarch::reader::VTKSampler::getNumberOfPoints() const { return static_cast<int>(_numberOfPoints); }

int tarch::reader::VTKSampler::getNumberOfComponents(const std::string& fieldName) const {
  const auto field = _fields.find(fieldName);
  if (field == _fields.end()) {
    throw std::runtime_error("No field \"" + fieldName + "\" has been read from \"" + _filePath + "\".");
  }
  return field->second.numberOfComponents;
}

bool tarch::reader::VTKSampler::samplesCellData() const { return _samplesCellData; }
