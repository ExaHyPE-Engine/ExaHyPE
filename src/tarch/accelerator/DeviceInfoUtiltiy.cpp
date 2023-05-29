#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "DeviceInfoUtility.h"

namespace tarch::accelerator::info::utility
{
  size_t getPropertyOffset(const char name[])
  {
    auto it = std::find(properties.begin(), properties.end(), std::string(name));
    if (it == properties.end())
    {
      throw std::runtime_error("Property input name not a part of the properties list!");
    }
    return std::distance(properties.begin(), it);
  }

  std::string parseDeviceInformation(const std::vector<std::vector<std::string>>& propertyMatrix, int numberOfDevices)
  {
    unsigned int propertyCount = properties.size();

    std::stringstream ss;

    // This size is the padding we need to add the porpety-name column
    unsigned int maxPropertyLengthWithPadding
      = std::max_element(
          properties.begin(),
          properties.end(),
          [](const std::string& lhs, const std::string& rhs) { return lhs.size() < rhs.size(); }
        )->size()
        + 1;

    // Get the string entry of a columng with the highest length
    std::vector<unsigned int> columnMaxStringSizes(numberOfDevices, 0);
    for (unsigned int col = 0; col < numberOfDevices; col++)
    {
      std::vector<unsigned int> col_sizes;
      for (unsigned int row = 0; row < properties.size(); row++)
      {
        col_sizes.push_back(propertyMatrix[row][col].size());
      }

      // Device Id is also a part of the column
      col_sizes.push_back(std::to_string(col).size());

      unsigned int maxLength    = *std::max_element(col_sizes.begin(), col_sizes.end());
      columnMaxStringSizes[col] = maxLength;
    }

    // Generate the first 2 lines with the device ids and dash seperator
    unsigned int lineLength = 0;
    std::string  propPadding(maxPropertyLengthWithPadding, ' ');
    ss << propPadding << "|";
    lineLength += maxPropertyLengthWithPadding + 1;
    for (unsigned int i = 0; i < numberOfDevices; i++)
    {
      std::string paddingNeeded = std::string((columnMaxStringSizes[i] + 1) - std::to_string(i).size(), ' ');
      ss << paddingNeeded << i << "  ";
      lineLength += paddingNeeded.size() + std::to_string(i).size() + 2;
    }
    lineLength -= 2;
    ss.seekp(-2, ss.cur);
    ss << "\n";
    std::string dashLine(lineLength, '-');
    ss << dashLine << "\n";
    // Generation of first 2 lines ends here

    // Iterate the matrix and write per row, also while accessing the padding we need to add
    unsigned int x = 0;
    unsigned int y = 0;
    for (const auto& row : propertyMatrix)
    {
      std::string propPadding(maxPropertyLengthWithPadding - properties[x].size(), ' ');
      ss << properties[x] << propPadding << "|";
      y = 0;
      for (const auto& el : row)
      {
        unsigned int paddingLength = columnMaxStringSizes[y] + 1 - el.size();
        std::string  s(paddingLength, ' ');
        ss << s << el << "  ";
        y += 1;
      }
      ss.seekp(-1, ss.cur);
      ss << "\n";
      x += 1;
    }
    return ss.str();
  }

} // namespace tarch::accelerator::info::utility