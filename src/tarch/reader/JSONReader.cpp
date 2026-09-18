// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "JSONReader.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"

namespace {
  struct JSONValue;
  using JSONObject = std::unordered_map<std::string, JSONValue>;
  using JSONArray  = std::vector<JSONValue>;

  struct JSONValue {
    std::variant<std::nullptr_t, bool, int, double, std::string, JSONArray, JSONObject> data;
  };

  class Parser final {
  public:
    Parser(std::string input, std::string filepath) :
      _input(std::move(input)),
      _filepath(std::move(filepath)) {}

    JSONValue parse() {
      skipWhitespace();
      JSONValue result = parseValue();
      skipWhitespace();
      if (not isAtEnd()) {
        error("Unexpected trailing content");
      }
      return result;
    }

  private:
    const std::string _input;
    const std::string _filepath;
    std::size_t       _position{0};

    [[nodiscard]] bool isAtEnd() const { return _position >= _input.size(); }

    [[nodiscard]] char current() const { return isAtEnd() ? '\0' : _input[_position]; }

    char advance() { return isAtEnd() ? '\0' : _input[_position++]; }

    void skipWhitespace() {
      while (not isAtEnd() and std::isspace(static_cast<unsigned char>(current())) != 0) {
        ++_position;
      }
    }

    [[noreturn]] void error(const std::string& reason) const {
      std::stringstream message;
      message << "Failed to parse JSON file \"" << _filepath << "\": " << reason << " at position " << _position << ".";
      tarch::terminateWithError(message.str());
    }

    void expect(const char expected) {
      if (advance() != expected) {
        std::stringstream message;
        message << "Expected '" << expected << "'";
        error(message.str());
      }
    }

    void consumeKeyword(const std::string& keyword) {
      for (const char ch : keyword) {
        if (advance() != ch) {
          std::stringstream message;
          message << "Expected \"" << keyword << "\"";
          error(message.str());
        }
      }
    }

    JSONValue parseValue() {
      skipWhitespace();
      switch (current()) {
      case '{':
        return JSONValue{parseObject()};
      case '[':
        return JSONValue{parseArray()};
      case '"':
        return JSONValue{parseString()};
      case 't':
        consumeKeyword("true");
        return JSONValue{true};
      case 'f':
        consumeKeyword("false");
        return JSONValue{false};
      case 'n':
        consumeKeyword("null");
        return JSONValue{nullptr};
      default:
        if (current() == '-' or std::isdigit(static_cast<unsigned char>(current())) != 0) {
          return parseNumber();
        }
        error("Unexpected token");
      }
    }

    JSONObject parseObject() {
      expect('{');
      skipWhitespace();

      JSONObject result;
      if (current() == '}') {
        advance();
        return result;
      }

      while (true) {
        skipWhitespace();
        if (current() != '"') {
          error("Expected string key");
        }
        std::string key = parseString();

        skipWhitespace();
        expect(':');
        skipWhitespace();
        result.emplace(std::move(key), parseValue());
        skipWhitespace();

        if (current() == '}') {
          advance();
          return result;
        }
        expect(',');
      }
    }

    JSONArray parseArray() {
      expect('[');
      skipWhitespace();

      JSONArray result;
      if (current() == ']') {
        advance();
        return result;
      }

      while (true) {
        result.push_back(parseValue());
        skipWhitespace();

        if (current() == ']') {
          advance();
          return result;
        }
        expect(',');
        skipWhitespace();
      }
    }

    static void appendUtf8(std::string& target, const unsigned int codePoint) {
      if (codePoint <= 0x7F) {
        target.push_back(static_cast<char>(codePoint));
      } else if (codePoint <= 0x7FF) {
        target.push_back(static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F)));
        target.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
      } else if (codePoint <= 0xFFFF) {
        target.push_back(static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F)));
        target.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        target.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
      } else {
        target.push_back(static_cast<char>(0xF0 | ((codePoint >> 18) & 0x07)));
        target.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        target.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        target.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
      }
    }

    unsigned int parseHexDigit() {
      if (isAtEnd()) {
        error("Incomplete unicode escape");
      }

      const char ch = advance();
      if (ch >= '0' and ch <= '9') {
        return static_cast<unsigned int>(ch - '0');
      }
      if (ch >= 'a' and ch <= 'f') {
        return 10U + static_cast<unsigned int>(ch - 'a');
      }
      if (ch >= 'A' and ch <= 'F') {
        return 10U + static_cast<unsigned int>(ch - 'A');
      }
      error("Invalid unicode escape");
    }

    std::string parseString() {
      expect('"');

      std::string result;
      while (not isAtEnd()) {
        const char ch = advance();
        if (ch == '"') {
          return result;
        }
        if (ch == '\\') {
          if (isAtEnd()) {
            error("Incomplete escape sequence");
          }
          switch (const char escaped = advance()) {
          case '"':
          case '\\':
          case '/':
            result.push_back(escaped);
            break;
          case 'b':
            result.push_back('\b');
            break;
          case 'f':
            result.push_back('\f');
            break;
          case 'n':
            result.push_back('\n');
            break;
          case 'r':
            result.push_back('\r');
            break;
          case 't':
            result.push_back('\t');
            break;
          case 'u': {
            unsigned int codePoint = 0;
            for (int digit = 0; digit < 4; ++digit) {
              codePoint = (codePoint << 4) | parseHexDigit();
            }
            appendUtf8(result, codePoint);
            break;
          }
          default:
            error("Invalid escape sequence");
          }
        } else {
          result.push_back(ch);
        }
      }

      error("Unterminated string");
    }

    JSONValue parseNumber() {
      const std::size_t start = _position;

      if (current() == '-') {
        advance();
      }

      if (std::isdigit(static_cast<unsigned char>(current())) == 0) {
        error("Invalid number");
      }

      if (current() == '0') {
        advance();
      } else {
        while (std::isdigit(static_cast<unsigned char>(current())) != 0) {
          advance();
        }
      }

      bool isFloatingPoint = false;
      if (current() == '.') {
        isFloatingPoint = true;
        advance();
        if (std::isdigit(static_cast<unsigned char>(current())) == 0) {
          error("Invalid number");
        }
        while (std::isdigit(static_cast<unsigned char>(current())) != 0) {
          advance();
        }
      }

      if (current() == 'e' or current() == 'E') {
        isFloatingPoint = true;
        advance();
        if (current() == '+' or current() == '-') {
          advance();
        }
        if (std::isdigit(static_cast<unsigned char>(current())) == 0) {
          error("Invalid exponent");
        }
        while (std::isdigit(static_cast<unsigned char>(current())) != 0) {
          advance();
        }
      }

      const std::string token = _input.substr(start, _position - start);
      const char* const end   = token.data() + token.size();

      if (isFloatingPoint) {
        auto       result = 0.0;
        const auto parsed = std::from_chars(token.data(), end, result);
        if (parsed.ec != std::errc{} or parsed.ptr != end) {
          error("Failed to parse number");
        }
        return JSONValue{result};
      }

      auto       result = 0;
      const auto parsed = std::from_chars(token.data(), end, result);
      if (parsed.ec != std::errc{} or parsed.ptr != end) {
        error("Failed to parse number");
      }
      return JSONValue{result};
    }
  };

  template <typename T>
  const T& requireType(const JSONValue& value, const std::string& key, const char* expectedTypeName) {
    if (not std::holds_alternative<T>(value.data)) {
      std::stringstream message;
      message << "JSON key \"" << key << "\" is not of type " << expectedTypeName << ".";
      tarch::terminateWithError(message.str());
    }
    return std::get<T>(value.data);
  }

  std::vector<std::string> splitKey(const std::string& key) {
    std::vector<std::string> result;
    std::size_t              start = 0;

    while (start <= key.size()) {
      const std::size_t separator = key.find('.', start);
      const std::string part      = key.substr(start, separator - start);
      assertion(not part.empty(), key);
      result.push_back(part);

      if (separator == std::string::npos) {
        return result;
      }
      start = separator + 1;
    }

    return result;
  }

  /**
   * Returns the value the dot-separated key names, or nullptr when a component is absent or when a
   * component other than the last one holds no object.
   */
  const JSONValue* findValue(const JSONObject& root, const std::string& key) {
    const std::vector<std::string> path          = splitKey(key);
    const JSONObject*              currentObject = &root;
    const JSONValue*               currentValue  = nullptr;

    for (std::size_t index = 0; index < path.size(); ++index) {
      const auto entry = currentObject->find(path[index]);
      if (entry == currentObject->end()) {
        return nullptr;
      }
      currentValue = &entry->second;

      if (index + 1 < path.size()) {
        if (not std::holds_alternative<JSONObject>(currentValue->data)) {
          return nullptr;
        }
        currentObject = &std::get<JSONObject>(currentValue->data);
      }
    }

    return currentValue;
  }

  const JSONValue& getValue(const JSONObject& root, const std::string& key) {
    const JSONValue* const value = findValue(root, key);
    if (value == nullptr) {
      std::stringstream message;
      message << "JSON key \"" << key << "\" does not exist.";
      tarch::terminateWithError(message.str());
    }
    return *value;
  }

  bool hasValue(const JSONObject& root, const std::string& key) { return findValue(root, key) != nullptr; }
} // namespace

struct tarch::reader::JSONReader::Impl {
  JSONObject root;
  // Top-level keys whose value was read.
  std::unordered_set<std::string> used;
};

tarch::reader::JSONReader::JSONReader(const std::string& filepath) :
  _impl(std::make_unique<Impl>()) {
  std::ifstream input(filepath.c_str());
  if (not input.is_open()) {
    std::stringstream message;
    message << "Could not open JSON file at \"" << filepath << "\".";
    tarch::terminateWithError(message.str());
  }

  std::stringstream buffer;
  buffer << input.rdbuf();

  JSONValue document = Parser(buffer.str(), filepath).parse();
  if (not std::holds_alternative<JSONObject>(document.data)) {
    std::stringstream message;
    message << "JSON file \"" << filepath << "\" must contain a top-level object.";
    tarch::terminateWithError(message.str());
  }
  _impl->root = std::get<JSONObject>(std::move(document.data));
}

tarch::reader::JSONReader::~JSONReader() = default;

bool tarch::reader::JSONReader::hasKey(const std::string& key) const { return hasValue(_impl->root, key); }

void tarch::reader::JSONReader::markRead(const std::string& key) const {
  // getUnusedKeys() lists top-level keys, so the insert records the top-level component only.
  _impl->used.insert(key.substr(0, key.find('.')));
}

std::vector<std::string> tarch::reader::JSONReader::getUnusedKeys() const {
  std::vector<std::string> result;
  for (const auto& entry : _impl->root) {
    if (not _impl->used.contains(entry.first)) {
      result.push_back(entry.first);
    }
  }
  // The root map iterates in an unspecified order, so the sort makes the result reproducible.
  std::sort(result.begin(), result.end());
  return result;
}

std::string tarch::reader::JSONReader::getString(const std::string& key) const {
  markRead(key);
  return requireType<std::string>(getValue(_impl->root, key), key, "string");
}

int tarch::reader::JSONReader::getInteger(const std::string& key) const {
  markRead(key);
  return requireType<int>(getValue(_impl->root, key), key, "integer");
}

double tarch::reader::JSONReader::getDouble(const std::string& key) const {
  markRead(key);
  const JSONValue& value = getValue(_impl->root, key);
  if (std::holds_alternative<int>(value.data)) {
    return static_cast<double>(std::get<int>(value.data));
  }
  return requireType<double>(value, key, "number");
}

bool tarch::reader::JSONReader::getBoolean(const std::string& key) const {
  markRead(key);
  return requireType<bool>(getValue(_impl->root, key), key, "boolean");
}

std::string tarch::reader::JSONReader::getString(const std::string& key, const std::string& fallback) const {
  return hasKey(key) ? getString(key) : fallback;
}

int tarch::reader::JSONReader::getInteger(const std::string& key, const int fallback) const {
  return hasKey(key) ? getInteger(key) : fallback;
}

double tarch::reader::JSONReader::getDouble(const std::string& key, const double fallback) const {
  return hasKey(key) ? getDouble(key) : fallback;
}

bool tarch::reader::JSONReader::getBoolean(const std::string& key, const bool fallback) const {
  return hasKey(key) ? getBoolean(key) : fallback;
}

std::vector<std::string> tarch::reader::JSONReader::getStringArray(const std::string& key) const {
  markRead(key);
  const auto&              source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<std::string> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<std::string>(entry, key, "string array element"));
  }
  return result;
}

std::vector<int> tarch::reader::JSONReader::getIntegerArray(const std::string& key) const {
  markRead(key);
  const auto&      source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<int> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<int>(entry, key, "integer array element"));
  }
  return result;
}

std::vector<double> tarch::reader::JSONReader::getDoubleArray(const std::string& key) const {
  markRead(key);
  const auto&         source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<double> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    if (std::holds_alternative<int>(entry.data)) {
      result.push_back(static_cast<double>(std::get<int>(entry.data)));
    } else if (std::holds_alternative<double>(entry.data)) {
      result.push_back(std::get<double>(entry.data));
    } else {
      std::stringstream message;
      message << "JSON key \"" << key << "\" contains a non-numeric array element.";
      tarch::terminateWithError(message.str());
    }
  }
  return result;
}

std::vector<bool> tarch::reader::JSONReader::getBooleanArray(const std::string& key) const {
  markRead(key);
  const auto&       source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<bool> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<bool>(entry, key, "boolean array element"));
  }
  return result;
}
