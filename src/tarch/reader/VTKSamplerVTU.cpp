// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
//
// The VTU back-end of tarch::reader::VTKSampler: the XML output of
// tarch::plotter::VTUFileWriter, i.e. a .pvd collection of one .pvtu per step,
// each of which references one .vtu per rank. Reading it needs no third-party
// library - the format is a plain XML skeleton whose data arrays sit in one
// appended binary blob, optionally ZLib compressed - which is what makes it the
// restart path of a build without HDF5.
#include "VTKSampler.h"

#ifdef PEANO_WITH_ZLIB
#include <zlib.h>
#endif

namespace {
  [[noreturn]] void fail(const std::string& filePath, const std::string& message) {
    throw std::runtime_error("Could not read VTU file \"" + filePath + "\": " + message + ".");
  }

  std::string toLowerCase(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
      return static_cast<char>(std::tolower(character));
    });
    return result;
  }

  bool isWhitespace(char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; }

  std::string readWholeFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (not in) {
      fail(path.string(), "the file does not exist or cannot be opened");
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
  }

  std::uint64_t toInteger(const std::string& text, const std::string& filePath, const std::string& what) {
    try {
      return static_cast<std::uint64_t>(std::stoull(text));
    } catch (const std::exception&) {
      fail(filePath, what + " is \"" + text + "\", which is no integer");
    }
  }

  double toReal(const std::string& text, const std::string& filePath, const std::string& what) {
    try {
      return std::stod(text);
    } catch (const std::exception&) {
      fail(filePath, what + " is \"" + text + "\", which is no number");
    }
  }

  // --------------------------------------------------------------------------
  // A minimal XML scanner
  //
  // The skeleton of a VTU file is flat and machine-written, so tag-by-tag
  // scanning is enough and spares Peano an XML dependency. The scan never
  // reaches into the appended binary section, which is not XML at all and may
  // hold any byte, including '<'.
  // --------------------------------------------------------------------------
  struct Tag {
    std::string                        name;
    std::map<std::string, std::string> attributes;
    bool                               isClosing     = false;
    bool                               isSelfClosing = false;
    /** First character behind the tag, i.e. where its text content starts. */
    std::size_t contentBegin = 0;
  };

  std::string attributeOr(const Tag& tag, const std::string& name, const std::string& fallback) {
    const auto entry = tag.attributes.find(name);
    return entry == tag.attributes.end() ? fallback : entry->second;
  }

  /**
   * @brief Scans for the next tag in [position,end) and advances position.
   *
   * Comments, processing instructions and doctypes are skipped. Returns false
   * once the range holds no further tag.
   */
  bool nextTag(const std::string& text, std::size_t end, std::size_t& position, Tag& tag) {
    while (true) {
      const std::size_t open = text.find('<', position);
      if (open == std::string::npos or open >= end) {
        position = end;
        return false;
      }

      if (text.compare(open, 4, "<!--") == 0) {
        const std::size_t close = text.find("-->", open);
        if (close == std::string::npos or close >= end) {
          position = end;
          return false;
        }
        position = close + 3;
        continue;
      }

      if (open + 1 < end and (text[open + 1] == '?' or text[open + 1] == '!')) {
        const std::size_t close = text.find('>', open);
        if (close == std::string::npos or close >= end) {
          position = end;
          return false;
        }
        position = close + 1;
        continue;
      }

      tag                = Tag{};
      std::size_t cursor = open + 1;
      if (cursor < end and text[cursor] == '/') {
        tag.isClosing = true;
        cursor++;
      }

      const std::size_t nameBegin = cursor;
      while (cursor < end and not isWhitespace(text[cursor]) and text[cursor] != '>' and text[cursor] != '/') {
        cursor++;
      }
      tag.name = text.substr(nameBegin, cursor - nameBegin);

      while (cursor < end) {
        while (cursor < end and isWhitespace(text[cursor])) {
          cursor++;
        }
        if (cursor >= end) {
          break;
        }
        if (text[cursor] == '/') {
          tag.isSelfClosing = true;
          cursor++;
          continue;
        }
        if (text[cursor] == '>') {
          cursor++;
          break;
        }

        const std::size_t keyBegin = cursor;
        while (cursor < end and text[cursor] != '=' and text[cursor] != '>' and not isWhitespace(text[cursor])) {
          cursor++;
        }
        const std::string key = text.substr(keyBegin, cursor - keyBegin);

        while (cursor < end and isWhitespace(text[cursor])) {
          cursor++;
        }
        if (cursor < end and text[cursor] == '=') {
          cursor++;
          while (cursor < end and isWhitespace(text[cursor])) {
            cursor++;
          }
          if (cursor < end and (text[cursor] == '"' or text[cursor] == '\'')) {
            const char        quote      = text[cursor++];
            const std::size_t valueBegin = cursor;
            while (cursor < end and text[cursor] != quote) {
              cursor++;
            }
            tag.attributes[key] = text.substr(valueBegin, cursor - valueBegin);
            if (cursor < end) {
              cursor++;
            }
          }
        }
      }

      tag.contentBegin = cursor;
      position         = cursor;
      return true;
    }
  }

  // --------------------------------------------------------------------------
  // The document
  // --------------------------------------------------------------------------
  struct DataArray {
    std::string   name;
    std::string   type       = "Float32";
    std::string   format     = "appended";
    int           components = 1;
    std::uint64_t offset     = 0;
    /** Payload of an inline array, i.e. one whose format is ascii or binary. */
    std::string inlineText;
    bool        isSet = false;
  };

  /**
   * One <Piece> of a .vtu. ExaHyPE writes one piece per subdomain, so a single
   * rank's file already holds dozens of them, and their connectivity indexes
   * their own points.
   */
  struct Piece {
    std::size_t                      numberOfPoints = 0;
    std::size_t                      numberOfCells  = 0;
    DataArray                        points;
    DataArray                        connectivity;
    DataArray                        offsets;
    std::map<std::string, DataArray> pointData;
    std::map<std::string, DataArray> cellData;
  };

  struct Document {
    std::string filePath;
    /** The whole file. Offsets of appended arrays index into it. */
    std::string content;
    /** First byte behind the underscore of the appended section. */
    std::size_t appendedBegin    = std::string::npos;
    bool        appendedIsBase64 = false;
    bool        isCompressed     = false;
    bool        isBigEndian      = false;
    /** Whether the file's byte order differs from this machine's. */
    bool swapsBytes = false;
    /** Width of the length prefixes, i.e. the file's header_type. */
    int                headerSize = 4;
    std::vector<Piece> pieces;
  };

  std::uint64_t readWord(const std::uint8_t* data, int headerSize, bool isBigEndian) {
    std::uint64_t result = 0;
    for (int byte = 0; byte < headerSize; byte++) {
      const std::uint8_t value = isBigEndian ? data[headerSize - 1 - byte] : data[byte];
      result |= static_cast<std::uint64_t>(value) << (8 * byte);
    }
    return result;
  }

  // --------------------------------------------------------------------------
  // Base64
  // --------------------------------------------------------------------------
  int base64Value(char character) {
    if (character >= 'A' and character <= 'Z') {
      return character - 'A';
    }
    if (character >= 'a' and character <= 'z') {
      return character - 'a' + 26;
    }
    if (character >= '0' and character <= '9') {
      return character - '0' + 52;
    }
    if (character == '+') {
      return 62;
    }
    if (character == '/') {
      return 63;
    }
    return -1;
  }

  /** Number of base64 characters a payload of that many bytes occupies. */
  std::size_t base64Length(std::uint64_t bytes) { return static_cast<std::size_t>((bytes + 2) / 3) * 4; }

  std::vector<std::uint8_t> decodeBase64(
    const std::string& text,
    std::size_t        begin,
    std::size_t        numberOfCharacters,
    const std::string& filePath
  ) {
    std::vector<std::uint8_t> result;
    result.reserve(numberOfCharacters / 4 * 3 + 3);

    std::uint32_t buffer = 0;
    int           bits   = 0;
    const std::size_t end = std::min(text.size(), begin + numberOfCharacters);

    for (std::size_t position = begin; position < end; position++) {
      const char character = text[position];
      if (isWhitespace(character)) {
        continue;
      }
      if (character == '=') {
        break;
      }

      const int value = base64Value(character);
      if (value < 0) {
        fail(filePath, "the base64 payload holds the invalid character '" + std::string(1, character) + "'");
      }

      buffer = (buffer << 6) | static_cast<std::uint32_t>(value);
      bits += 6;
      if (bits >= 8) {
        bits -= 8;
        result.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xFFu));
      }
    }

    return result;
  }

  // --------------------------------------------------------------------------
  // Compressed payloads
  //
  // A compressed array is prefixed by a header of [number of blocks, block
  // size, size of the last (partial) block, compressed size per block], all of
  // them written in the file's header_type.
  // --------------------------------------------------------------------------
  struct BlockHeader {
    std::uint64_t              numberOfBlocks = 0;
    std::uint64_t              blockSize      = 0;
    std::uint64_t              lastBlockSize  = 0;
    std::vector<std::uint64_t> compressedSizes;
    /** Size of the header itself, i.e. where the first block starts. */
    std::size_t byteSize = 0;
  };

  BlockHeader readBlockHeader(const Document& document, const std::uint8_t* data, std::size_t availableBytes) {
    const std::size_t headerSize = static_cast<std::size_t>(document.headerSize);
    if (availableBytes < 3 * headerSize) {
      fail(document.filePath, "a compressed array is truncated before its header");
    }

    BlockHeader header;
    header.numberOfBlocks = readWord(data, document.headerSize, document.isBigEndian);
    header.blockSize      = readWord(data + headerSize, document.headerSize, document.isBigEndian);
    header.lastBlockSize  = readWord(data + 2 * headerSize, document.headerSize, document.isBigEndian);

    if (header.numberOfBlocks > (availableBytes - 3 * headerSize) / headerSize) {
      fail(document.filePath, "a compressed array claims more blocks than the file can hold");
    }
    header.byteSize = (3 + static_cast<std::size_t>(header.numberOfBlocks)) * headerSize;

    for (std::uint64_t block = 0; block < header.numberOfBlocks; block++) {
      header.compressedSizes.push_back(
        readWord(data + (3 + block) * headerSize, document.headerSize, document.isBigEndian)
      );
    }

    return header;
  }

  std::vector<std::uint8_t> inflateBlocks(
    const Document&     document,
    const BlockHeader&  header,
    const std::uint8_t* data,
    std::size_t         availableBytes
  ) {
#ifdef PEANO_WITH_ZLIB
    std::vector<std::uint8_t> result;
    std::size_t               cursor = 0;

    for (std::uint64_t block = 0; block < header.numberOfBlocks; block++) {
      const std::uint64_t compressedSize = header.compressedSizes[block];
      if (cursor + compressedSize > availableBytes) {
        fail(document.filePath, "a compressed array is truncated");
      }

      const bool    isLastBlock = block + 1 == header.numberOfBlocks;
      std::uint64_t plainSize   = header.blockSize;
      if (isLastBlock and header.lastBlockSize != 0) {
        plainSize = header.lastBlockSize;
      }

      std::vector<std::uint8_t> plainBlock(static_cast<std::size_t>(plainSize));
      uLongf                    plainLength = static_cast<uLongf>(plainSize);
      const int                 status      = uncompress(
        reinterpret_cast<Bytef*>(plainBlock.data()),
        &plainLength,
        reinterpret_cast<const Bytef*>(data + cursor),
        static_cast<uLong>(compressedSize)
      );
      if (status != Z_OK) {
        fail(document.filePath, "a ZLib compressed array could not be inflated");
      }

      plainBlock.resize(static_cast<std::size_t>(plainLength));
      result.insert(result.end(), plainBlock.begin(), plainBlock.end());
      cursor += static_cast<std::size_t>(compressedSize);
    }

    return result;
#else
    static_cast<void>(header);
    static_cast<void>(data);
    static_cast<void>(availableBytes);
    fail(
      document.filePath,
      "the file is ZLib compressed, but Peano has been built without ZLib support. Reconfigure with "
      "-DPEANO_WITH_ZLIB=ON"
    );
#endif
  }

  /** Decodes one raw payload, i.e. one that sits in the file byte for byte. */
  std::vector<std::uint8_t> decodeRawPayload(
    const Document& document, const std::uint8_t* data, std::size_t availableBytes
  ) {
    const std::size_t headerSize = static_cast<std::size_t>(document.headerSize);

    if (not document.isCompressed) {
      if (availableBytes < headerSize) {
        fail(document.filePath, "an appended array is truncated before its length prefix");
      }
      const std::uint64_t numberOfBytes = readWord(data, document.headerSize, document.isBigEndian);
      if (numberOfBytes > availableBytes - headerSize) {
        fail(document.filePath, "an appended array claims more bytes than the file holds");
      }
      return std::vector<std::uint8_t>(
        data + headerSize, data + headerSize + static_cast<std::size_t>(numberOfBytes)
      );
    }

    const BlockHeader header = readBlockHeader(document, data, availableBytes);
    return inflateBlocks(document, header, data + header.byteSize, availableBytes - header.byteSize);
  }

  /**
   * Decodes one base64 payload. An uncompressed one is a single stream holding
   * the length prefix and the data, while a compressed one encodes its header
   * and its blocks as two independent streams - which is why only the
   * characters that are actually needed may be decoded here.
   */
  std::vector<std::uint8_t> decodeBase64Payload(const Document& document, const std::string& text, std::size_t begin) {
    const std::size_t headerSize = static_cast<std::size_t>(document.headerSize);

    if (not document.isCompressed) {
      const std::vector<std::uint8_t> prefix = decodeBase64(text, begin, base64Length(headerSize), document.filePath);
      if (prefix.size() < headerSize) {
        fail(document.filePath, "a base64 array is truncated before its length prefix");
      }
      const std::uint64_t numberOfBytes = readWord(prefix.data(), document.headerSize, document.isBigEndian);

      std::vector<std::uint8_t> payload = decodeBase64(
        text, begin, base64Length(headerSize + numberOfBytes), document.filePath
      );
      if (payload.size() < headerSize + numberOfBytes) {
        fail(document.filePath, "a base64 array is shorter than its length prefix announces");
      }
      return std::vector<std::uint8_t>(
        payload.begin() + static_cast<std::ptrdiff_t>(headerSize),
        payload.begin() + static_cast<std::ptrdiff_t>(headerSize + numberOfBytes)
      );
    }

    // Three characters carry the first nine bytes, which is enough for the
    // block count whatever the header type is.
    const std::vector<std::uint8_t> blockCount = decodeBase64(text, begin, 12, document.filePath);
    if (blockCount.size() < headerSize) {
      fail(document.filePath, "a compressed base64 array is truncated before its header");
    }
    const std::uint64_t numberOfBlocks = readWord(blockCount.data(), document.headerSize, document.isBigEndian);

    const std::size_t headerBytes      = (3 + static_cast<std::size_t>(numberOfBlocks)) * headerSize;
    const std::size_t headerCharacters = base64Length(headerBytes);

    const std::vector<std::uint8_t> rawHeader = decodeBase64(text, begin, headerCharacters, document.filePath);
    if (rawHeader.size() < headerBytes) {
      fail(document.filePath, "a compressed base64 array is truncated within its header");
    }
    const BlockHeader header = readBlockHeader(document, rawHeader.data(), rawHeader.size());

    std::uint64_t compressedBytes = 0;
    for (const std::uint64_t size : header.compressedSizes) {
      compressedBytes += size;
    }

    const std::vector<std::uint8_t> blocks = decodeBase64(
      text, begin + headerCharacters, base64Length(compressedBytes), document.filePath
    );
    return inflateBlocks(document, header, blocks.data(), blocks.size());
  }

  std::vector<std::uint8_t> decodeBinary(const Document& document, const DataArray& array) {
    if (array.format == "appended") {
      if (document.appendedBegin == std::string::npos) {
        fail(document.filePath, "array \"" + array.name + "\" is appended, but the file has no AppendedData section");
      }
      const std::size_t begin = document.appendedBegin + static_cast<std::size_t>(array.offset);
      if (begin >= document.content.size()) {
        fail(document.filePath, "the offset of array \"" + array.name + "\" points behind the end of the file");
      }

      if (document.appendedIsBase64) {
        return decodeBase64Payload(document, document.content, begin);
      }
      return decodeRawPayload(
        document,
        reinterpret_cast<const std::uint8_t*>(document.content.data()) + begin,
        document.content.size() - begin
      );
    }

    if (array.format == "binary") {
      return decodeBase64Payload(document, array.inlineText, 0);
    }

    fail(document.filePath, "array \"" + array.name + "\" uses the unsupported format \"" + array.format + "\"");
  }

  // --------------------------------------------------------------------------
  // Typed access
  // --------------------------------------------------------------------------
  template <class Source, class Target>
  std::vector<Target> convertTyped(
    const std::vector<std::uint8_t>& bytes, bool swapsBytes, const Document& document, const std::string& name
  ) {
    if (bytes.size() % sizeof(Source) != 0) {
      fail(document.filePath, "the payload of array \"" + name + "\" is no multiple of its element size");
    }

    const std::size_t   count = bytes.size() / sizeof(Source);
    std::vector<Target> result(count);

    for (std::size_t entry = 0; entry < count; entry++) {
      std::uint8_t raw[sizeof(Source)];
      std::memcpy(raw, bytes.data() + entry * sizeof(Source), sizeof(Source));
      if (swapsBytes) {
        std::reverse(raw, raw + sizeof(Source));
      }
      Source value;
      std::memcpy(&value, raw, sizeof(Source));
      result[entry] = static_cast<Target>(value);
    }

    return result;
  }

  template <class Target>
  std::vector<Target> convertBytes(
    const std::vector<std::uint8_t>& bytes, const std::string& type, const Document& document, const std::string& name
  ) {
    const bool swaps = document.swapsBytes;

    if (type == "Float32") {
      return convertTyped<float, Target>(bytes, swaps, document, name);
    }
    if (type == "Float64") {
      return convertTyped<double, Target>(bytes, swaps, document, name);
    }
    if (type == "Int8") {
      return convertTyped<std::int8_t, Target>(bytes, swaps, document, name);
    }
    if (type == "UInt8") {
      return convertTyped<std::uint8_t, Target>(bytes, swaps, document, name);
    }
    if (type == "Int16") {
      return convertTyped<std::int16_t, Target>(bytes, swaps, document, name);
    }
    if (type == "UInt16") {
      return convertTyped<std::uint16_t, Target>(bytes, swaps, document, name);
    }
    if (type == "Int32") {
      return convertTyped<std::int32_t, Target>(bytes, swaps, document, name);
    }
    if (type == "UInt32") {
      return convertTyped<std::uint32_t, Target>(bytes, swaps, document, name);
    }
    if (type == "Int64") {
      return convertTyped<std::int64_t, Target>(bytes, swaps, document, name);
    }
    if (type == "UInt64") {
      return convertTyped<std::uint64_t, Target>(bytes, swaps, document, name);
    }

    fail(document.filePath, "array \"" + name + "\" has the unsupported type \"" + type + "\"");
  }

  template <class Target>
  std::vector<Target> readArray(const Document& document, const DataArray& array, const std::string& what) {
    if (not array.isSet) {
      fail(document.filePath, "a piece has no " + what + " array");
    }

    if (array.format == "ascii") {
      std::istringstream  stream(array.inlineText);
      std::vector<Target> result;
      if constexpr (std::is_integral_v<Target>) {
        long long value = 0;
        while (stream >> value) {
          result.push_back(static_cast<Target>(value));
        }
      } else {
        double value = 0.0;
        while (stream >> value) {
          result.push_back(static_cast<Target>(value));
        }
      }
      return result;
    }

    return convertBytes<Target>(decodeBinary(document, array), array.type, document, array.name);
  }

  // --------------------------------------------------------------------------
  // Parsing the files
  // --------------------------------------------------------------------------
  Document parseDocument(const std::filesystem::path& path) {
    Document document;
    document.filePath = path.string();
    document.content  = readWholeFile(path);

    // Everything behind the AppendedData tag is binary and must not be scanned
    // as XML.
    std::size_t       xmlEnd      = document.content.size();
    const std::size_t appendedTag = document.content.find("<AppendedData");
    if (appendedTag != std::string::npos) {
      xmlEnd = appendedTag;

      const std::size_t tagEnd = document.content.find('>', appendedTag);
      if (tagEnd == std::string::npos) {
        fail(document.filePath, "the AppendedData tag is not terminated");
      }
      document.appendedIsBase64 = document.content.substr(appendedTag, tagEnd - appendedTag).find("base64")
                                  != std::string::npos;

      const std::size_t underscore = document.content.find('_', tagEnd);
      if (underscore == std::string::npos) {
        fail(document.filePath, "the AppendedData section does not open with an underscore");
      }
      document.appendedBegin = underscore + 1;
    }

    std::size_t position = 0;
    Tag         tag;
    Piece*      piece = nullptr;
    std::string section;

    while (nextTag(document.content, xmlEnd, position, tag)) {
      if (tag.name == "VTKFile" and not tag.isClosing) {
        const std::string type = attributeOr(tag, "type", "");
        if (type != "UnstructuredGrid") {
          fail(document.filePath, "the file holds a \"" + type + "\", while the sampler only reads UnstructuredGrids");
        }

        document.isBigEndian = attributeOr(tag, "byte_order", "LittleEndian") == "BigEndian";
        document.swapsBytes  = document.isBigEndian != (std::endian::native == std::endian::big);
        document.headerSize  = (attributeOr(tag, "header_type", "UInt32") == "UInt64") ? 8 : 4;

        const std::string compressor = attributeOr(tag, "compressor", "");
        document.isCompressed        = not compressor.empty();
        if (document.isCompressed and compressor != "vtkZLibDataCompressor") {
          fail(document.filePath, "the file uses the unsupported compressor \"" + compressor + "\"");
        }
      } else if (tag.name == "Piece" and not tag.isClosing) {
        document.pieces.emplace_back();
        piece                 = &document.pieces.back();
        piece->numberOfPoints = static_cast<std::size_t>(
          toInteger(attributeOr(tag, "NumberOfPoints", "0"), document.filePath, "the point count of a piece")
        );
        piece->numberOfCells = static_cast<std::size_t>(
          toInteger(attributeOr(tag, "NumberOfCells", "0"), document.filePath, "the cell count of a piece")
        );
        section.clear();
      } else if (tag.name == "PointData" or tag.name == "CellData" or tag.name == "Points" or tag.name == "Cells") {
        section = (tag.isClosing or tag.isSelfClosing) ? "" : tag.name;
      } else if (tag.name == "DataArray" and not tag.isClosing and piece != nullptr) {
        DataArray array;
        array.isSet      = true;
        array.name       = attributeOr(tag, "Name", "");
        array.type       = attributeOr(tag, "type", "Float32");
        array.format     = toLowerCase(attributeOr(tag, "format", "appended"));
        array.components = static_cast<int>(
          toInteger(attributeOr(tag, "NumberOfComponents", "1"), document.filePath, "the component count of an array")
        );
        array.offset = toInteger(attributeOr(tag, "offset", "0"), document.filePath, "the offset of an array");

        if (not tag.isSelfClosing) {
          const std::size_t textEnd = std::min(document.content.find('<', tag.contentBegin), xmlEnd);
          array.inlineText          = document.content.substr(tag.contentBegin, textEnd - tag.contentBegin);
        }

        if (section == "Points") {
          piece->points = array;
        } else if (section == "Cells") {
          if (array.name == "connectivity") {
            piece->connectivity = array;
          } else if (array.name == "offsets") {
            piece->offsets = array;
          }
        } else if (section == "PointData") {
          piece->pointData[array.name] = array;
        } else if (section == "CellData") {
          piece->cellData[array.name] = array;
        }
      }
    }

    return document;
  }

  /** One entry of a .pvd collection: a time stamp and the files that hold it. */
  struct Step {
    double                             time = 0.0;
    std::vector<std::filesystem::path> files;
  };

  std::vector<Step> readCollection(const std::filesystem::path& path) {
    const std::string content  = readWholeFile(path);
    const std::string filePath = path.string();

    std::vector<Step> steps;
    std::size_t       position = 0;
    Tag               tag;

    while (nextTag(content, content.size(), position, tag)) {
      if (tag.name != "DataSet" or tag.isClosing) {
        continue;
      }

      const std::string file = attributeOr(tag, "file", "");
      if (file.empty()) {
        fail(filePath, "a DataSet of the collection has no file attribute");
      }

      const std::string timeText = attributeOr(tag, "timestep", "");
      const double      time     = timeText.empty() ? static_cast<double>(steps.size())
                                                    : toReal(timeText, filePath, "the timestep of a DataSet");

      // A collection may split one step over several parts, which then share
      // one entry here.
      auto step = std::find_if(steps.begin(), steps.end(), [time](const Step& candidate) {
        return candidate.time == time;
      });
      if (step == steps.end()) {
        steps.push_back(Step{time, {}});
        step = steps.end() - 1;
      }
      step->files.push_back(path.parent_path() / file);
    }

    if (steps.empty()) {
      fail(filePath, "the collection holds no DataSet, i.e. no time step");
    }

    return steps;
  }

  std::vector<std::filesystem::path> readPieceSources(const std::filesystem::path& path) {
    const std::string content  = readWholeFile(path);
    const std::string filePath = path.string();

    std::vector<std::filesystem::path> sources;
    std::size_t                        position = 0;
    Tag                                tag;

    while (nextTag(content, content.size(), position, tag)) {
      if (tag.name != "Piece" or tag.isClosing) {
        continue;
      }
      const std::string source = attributeOr(tag, "Source", "");
      if (source.empty()) {
        fail(filePath, "a Piece of the parallel file has no Source attribute");
      }
      sources.push_back(path.parent_path() / source);
    }

    if (sources.empty()) {
      fail(filePath, "the parallel file references no piece");
    }

    return sources;
  }

  /**
   * A snapshot's time only shows up in its file name once the .pvd is bypassed,
   * where the plotter encodes it as <name>-<seconds>_<fraction>.
   */
  double timeFromFileName(const std::filesystem::path& path) {
    std::string stem = path.stem().string();

    // A per-rank file carries the rank behind the time stamp.
    const std::size_t rank = stem.rfind("-rank-");
    if (rank != std::string::npos) {
      stem = stem.substr(0, rank);
    }

    const std::size_t separator = stem.rfind('-');
    if (separator == std::string::npos) {
      return 0.0;
    }

    std::string time = stem.substr(separator + 1);
    std::replace(time.begin(), time.end(), '_', '.');

    try {
      return std::stod(time);
    } catch (const std::exception&) {
      return 0.0;
    }
  }
} // namespace

void tarch::reader::VTKSampler::readVTU(const std::vector<std::string>& fieldNames, int step) {
  if (fieldNames.empty()) {
    fail(_filePath, "no field name was requested");
  }

  // --------------------------------------------------------------------------
  // Pick the step
  // --------------------------------------------------------------------------
  const std::filesystem::path path(_filePath);
  const std::string           extension = toLowerCase(path.extension().string());

  std::vector<Step> steps;
  if (extension == ".pvd") {
    steps = readCollection(path);
  } else {
    steps.push_back(Step{timeFromFileName(path), {path}});
  }

  _numberOfSteps = static_cast<int>(steps.size());
  _step          = (step < 0) ? _numberOfSteps + step : step;
  if (_step < 0 or _step >= _numberOfSteps) {
    fail(
      _filePath,
      "step " + std::to_string(step) + " does not exist, the data set holds " + std::to_string(_numberOfSteps)
        + " steps"
    );
  }
  _timeStamp = steps[_step].time;

  // --------------------------------------------------------------------------
  // Resolve that step into the .vtu files of the individual ranks
  // --------------------------------------------------------------------------
  std::vector<std::filesystem::path> files;
  for (const std::filesystem::path& file : steps[_step].files) {
    if (toLowerCase(file.extension().string()) == ".pvtu") {
      const std::vector<std::filesystem::path> sources = readPieceSources(file);
      files.insert(files.end(), sources.begin(), sources.end());
    } else {
      files.push_back(file);
    }
  }

  // --------------------------------------------------------------------------
  // Read every piece of every file into one point cloud
  // --------------------------------------------------------------------------
  bool isFirstPiece = true;

  for (const std::filesystem::path& file : files) {
    const Document document = parseDocument(file);

    for (const Piece& piece : document.pieces) {
      if (piece.numberOfPoints == 0) {
        continue;
      }

      // The association follows from the first field, and all of them have to
      // share it: a point index has to identify the same sample point for each.
      const bool samplesCellData = piece.pointData.count(fieldNames.front()) == 0;

      for (const std::string& fieldName : fieldNames) {
        const bool isPointData = piece.pointData.count(fieldName) != 0;
        const bool isCellData  = piece.cellData.count(fieldName) != 0;

        if (not isPointData and not isCellData) {
          fail(document.filePath, "neither PointData nor CellData holds a field \"" + fieldName + "\"");
        }
        if ((not isPointData) != samplesCellData) {
          fail(
            document.filePath,
            "field \"" + fieldName + "\" is " + (isPointData ? "point" : "cell")
              + " data, while the preceding fields are " + (samplesCellData ? "cell" : "point")
              + " data. One sampler can only serve one of the two"
          );
        }
      }

      if (isFirstPiece) {
        _samplesCellData = samplesCellData;
      } else if (samplesCellData != _samplesCellData) {
        fail(document.filePath, "the pieces of one step disagree on whether a field is point or cell data");
      }

      if (_samplesCellData and piece.numberOfCells == 0) {
        continue;
      }

      // ----------------------------------------------------------------------
      // Sample point positions: the plotted vertices, or the cells' centroids
      // ----------------------------------------------------------------------
      const std::vector<double> points = readArray<double>(document, piece.points, "Points");
      if (points.size() != 3 * piece.numberOfPoints) {
        fail(
          document.filePath,
          "a piece announces " + std::to_string(piece.numberOfPoints) + " points, but its Points array holds "
            + std::to_string(points.size()) + " coordinates"
        );
      }

      std::size_t numberOfSamples = 0;

      if (not _samplesCellData) {
        _positions.insert(_positions.end(), points.begin(), points.end());
        numberOfSamples = piece.numberOfPoints;
      } else {
        const std::vector<long long> connectivity = readArray<long long>(document, piece.connectivity, "connectivity");
        const std::vector<long long> offsets      = readArray<long long>(document, piece.offsets, "offsets");

        // Unlike VTKHDF, a .vtu stores one offset per cell, namely the end of
        // its corner list, and no leading zero.
        if (offsets.size() != piece.numberOfCells) {
          fail(
            document.filePath,
            "a piece announces " + std::to_string(piece.numberOfCells) + " cells, but its offsets array holds "
              + std::to_string(offsets.size()) + " entries"
          );
        }

        for (std::size_t cell = 0; cell < piece.numberOfCells; cell++) {
          const long long first = (cell == 0) ? 0 : offsets[cell - 1];
          const long long last  = offsets[cell];
          if (last <= first or last > static_cast<long long>(connectivity.size())) {
            fail(document.filePath, "cell " + std::to_string(cell) + " of a piece has a broken corner list");
          }

          double centroid[3] = {0.0, 0.0, 0.0};
          for (long long entry = first; entry < last; entry++) {
            const long long point = connectivity[static_cast<std::size_t>(entry)];
            if (point < 0 or static_cast<std::size_t>(point) >= piece.numberOfPoints) {
              fail(document.filePath, "cell " + std::to_string(cell) + " of a piece refers to a point it has not got");
            }
            for (int d = 0; d < 3; d++) {
              centroid[d] += points[3 * static_cast<std::size_t>(point) + d];
            }
          }
          for (int d = 0; d < 3; d++) {
            _positions.push_back(centroid[d] / static_cast<double>(last - first));
          }
        }

        numberOfSamples = piece.numberOfCells;
      }

      // ----------------------------------------------------------------------
      // The data arrays themselves
      // ----------------------------------------------------------------------
      for (const std::string& fieldName : fieldNames) {
        const DataArray& array = _samplesCellData ? piece.cellData.at(fieldName) : piece.pointData.at(fieldName);

        const std::vector<double> values = readArray<double>(document, array, fieldName);
        if (values.size() != numberOfSamples * static_cast<std::size_t>(array.components)) {
          fail(
            document.filePath,
            "field \"" + fieldName + "\" holds " + std::to_string(values.size()) + " values, while its piece has "
              + std::to_string(numberOfSamples) + " sample points with " + std::to_string(array.components)
              + " components each"
          );
        }

        Field& field = _fields[fieldName];
        if (isFirstPiece) {
          field.numberOfComponents = array.components;
        } else if (array.components != field.numberOfComponents) {
          fail(
            document.filePath,
            "field \"" + fieldName + "\" holds " + std::to_string(array.components)
              + " components here and " + std::to_string(field.numberOfComponents) + " in a preceding piece"
          );
        }

        field.values.insert(field.values.end(), values.begin(), values.end());
      }

      isFirstPiece = false;
    }
  }

  _numberOfPoints = _positions.size() / 3;
  if (_numberOfPoints == 0) {
    fail(_filePath, "step " + std::to_string(_step) + " holds no data");
  }
}
