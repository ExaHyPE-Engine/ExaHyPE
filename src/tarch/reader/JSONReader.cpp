// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "JSONReader.h"

namespace {
  struct JSONValue;
  using JSONObject = std::unordered_map<std::string, JSONValue>;
  using JSONArray  = std::vector<JSONValue>;

  struct JSONValue {
    std::variant<std::nullptr_t, bool, int, double, std::string, JSONArray, JSONObject> data;
  };

  class Parser {
  public:
    explicit Parser(std::string input) :
      _input(std::move(input)) {}

    JSONValue parse() {
      skipWhitespace();
      JSONValue result = parseValue();
      skipWhitespace();
      if (!isAtEnd()) {
        error("Unexpected trailing content");
      }
      return result;
    }

  private:
    const std::string _input;
    std::size_t       _position = 0;

    bool isAtEnd() const { return _position >= _input.size(); }

    char current() const { return isAtEnd() ? '\0' : _input[_position]; }

    char advance() { return isAtEnd() ? '\0' : _input[_position++]; }

    void skipWhitespace() {
      while (!isAtEnd() && std::isspace(static_cast<unsigned char>(current()))) {
        ++_position;
      }
    }

    [[noreturn]] void error(const std::string& message) const {
      std::stringstream ss;
      ss << message << " at position " << _position << ".";
      throw std::runtime_error(ss.str());
    }

    void expect(char expected) {
      if (advance() != expected) {
        std::stringstream ss;
        ss << "Expected '" << expected << "'";
        error(ss.str());
      }
    }

    void consumeKeyword(const std::string& keyword) {
      for (char ch : keyword) {
        if (advance() != ch) {
          std::stringstream ss;
          ss << "Expected \"" << keyword << "\"";
          error(ss.str());
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
        if (current() == '-' || std::isdigit(static_cast<unsigned char>(current()))) {
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

    static void appendUtf8(std::string& target, unsigned int codePoint) {
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
      if (ch >= '0' && ch <= '9') {
        return static_cast<unsigned int>(ch - '0');
      }
      if (ch >= 'a' && ch <= 'f') {
        return 10u + static_cast<unsigned int>(ch - 'a');
      }
      if (ch >= 'A' && ch <= 'F') {
        return 10u + static_cast<unsigned int>(ch - 'A');
      }
      error("Invalid unicode escape");
    }

    std::string parseString() {
      expect('"');

      std::string result;
      while (!isAtEnd()) {
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
            for (int i = 0; i < 4; ++i) {
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

      if (!std::isdigit(static_cast<unsigned char>(current()))) {
        error("Invalid number");
      }

      if (current() == '0') {
        advance();
      } else {
        while (std::isdigit(static_cast<unsigned char>(current()))) {
          advance();
        }
      }

      bool isFloatingPoint = false;
      if (current() == '.') {
        isFloatingPoint = true;
        advance();
        if (!std::isdigit(static_cast<unsigned char>(current()))) {
          error("Invalid number");
        }
        while (std::isdigit(static_cast<unsigned char>(current()))) {
          advance();
        }
      }

      if (current() == 'e' || current() == 'E') {
        isFloatingPoint = true;
        advance();
        if (current() == '+' || current() == '-') {
          advance();
        }
        if (!std::isdigit(static_cast<unsigned char>(current()))) {
          error("Invalid exponent");
        }
        while (std::isdigit(static_cast<unsigned char>(current()))) {
          advance();
        }
      }

      const std::string token = _input.substr(start, _position - start);

      try {
        if (isFloatingPoint) {
          return JSONValue{std::stod(token)};
        }
        return JSONValue{std::stoi(token)};
      } catch (const std::exception&) {
        error("Failed to parse number");
      }
    }
  };

  const JSONValue& getObjectValue(const JSONObject& object, const std::string& key) {
    const auto entry = object.find(key);
    if (entry == object.end()) {
      std::stringstream ss;
      ss << "JSON key \"" << key << "\" does not exist.";
      throw std::runtime_error(ss.str());
    }
    return entry->second;
  }

  template <typename T>
  const T& requireType(const JSONValue& value, const std::string& key, const char* expectedTypeName) {
    if (!std::holds_alternative<T>(value.data)) {
      std::stringstream ss;
      ss << "JSON key \"" << key << "\" is not of type " << expectedTypeName << ".";
      throw std::runtime_error(ss.str());
    }
    return std::get<T>(value.data);
  }

  std::vector<std::string> splitKey(const std::string& key) {
    std::vector<std::string> result;
    std::size_t              start = 0;

    while (start <= key.size()) {
      const std::size_t separator = key.find('.', start);
      const std::string part      = key.substr(start, separator - start);
      if (part.empty()) {
        std::stringstream ss;
        ss << "JSON key \"" << key << "\" contains an empty path component.";
        throw std::runtime_error(ss.str());
      }
      result.push_back(part);

      if (separator == std::string::npos) {
        return result;
      }
      start = separator + 1;
    }

    return result;
  }

  const JSONValue& getValue(const JSONObject& root, const std::string& key) {
    const std::vector<std::string> path          = splitKey(key);
    const JSONObject*              currentObject = &root;
    const JSONValue*               currentValue  = nullptr;

    for (std::size_t i = 0; i < path.size(); ++i) {
      currentValue = &getObjectValue(*currentObject, path[i]);

      if (i + 1 < path.size()) {
        if (!std::holds_alternative<JSONObject>(currentValue->data)) {
          std::stringstream ss;
          ss << "JSON key \"" << key << "\" does not resolve to an object at component \"" << path[i] << "\".";
          throw std::runtime_error(ss.str());
        }
        currentObject = &std::get<JSONObject>(currentValue->data);
      }
    }

    return *currentValue;
  }

  bool hasValue(const JSONObject& root, const std::string& key) {
    try {
      getValue(root, key);
      return true;
    } catch (const std::runtime_error&) {
      return false;
    }
  }
} // namespace

struct tarch::reader::JSONReader::Impl {
  JSONObject root;
  // Top-level keys whose value was read.
  std::unordered_set<std::string> used;
};

tarch::reader::JSONReader::JSONReader(const std::string& filepath) :
  _impl(std::make_unique<Impl>()) {
  std::ifstream input(filepath.c_str());
  if (!input.is_open()) {
    std::stringstream ss;
    ss << "Could not open JSON file at \"" << filepath << "\".";
    throw std::runtime_error(ss.str());
  }

  std::stringstream buffer;
  buffer << input.rdbuf();

  try {
    JSONValue document = Parser(buffer.str()).parse();
    if (!std::holds_alternative<JSONObject>(document.data)) {
      std::stringstream ss;
      ss << "JSON file \"" << filepath << "\" must contain a top-level object.";
      throw std::runtime_error(ss.str());
    }
    _impl->root = std::get<JSONObject>(std::move(document.data));
  } catch (const std::runtime_error& exception) {
    std::stringstream ss;
    ss << "Failed to parse JSON file \"" << filepath << "\": " << exception.what();
    throw std::runtime_error(ss.str());
  }
}

tarch::reader::JSONReader::~JSONReader() {}

bool tarch::reader::JSONReader::hasKey(const std::string& key) const { return hasValue(_impl->root, key); }

void tarch::reader::JSONReader::markRead(const std::string& key) const {
  // Top-level component only, to match getUnusedKeys().
  _impl->used.insert(key.substr(0, key.find('.')));
}

std::vector<std::string> tarch::reader::JSONReader::getUnusedKeys() const {
  std::vector<std::string> result;
  for (const auto& entry : _impl->root) {
    if (_impl->used.count(entry.first) == 0) {
      result.push_back(entry.first);
    }
  }
  // sort for a stable diagnostic order
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

int tarch::reader::JSONReader::getInteger(const std::string& key, int fallback) const {
  return hasKey(key) ? getInteger(key) : fallback;
}

double tarch::reader::JSONReader::getDouble(const std::string& key, double fallback) const {
  return hasKey(key) ? getDouble(key) : fallback;
}

bool tarch::reader::JSONReader::getBoolean(const std::string& key, bool fallback) const {
  return hasKey(key) ? getBoolean(key) : fallback;
}

std::vector<std::string> tarch::reader::JSONReader::getStringArray(const std::string& key) const {
  markRead(key);
  const JSONArray&         source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<std::string> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<std::string>(entry, key, "string array element"));
  }
  return result;
}

std::vector<int> tarch::reader::JSONReader::getIntegerArray(const std::string& key) const {
  markRead(key);
  const JSONArray& source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<int> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<int>(entry, key, "integer array element"));
  }
  return result;
}

std::vector<double> tarch::reader::JSONReader::getDoubleArray(const std::string& key) const {
  markRead(key);
  const JSONArray&    source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<double> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    if (std::holds_alternative<int>(entry.data)) {
      result.push_back(static_cast<double>(std::get<int>(entry.data)));
    } else if (std::holds_alternative<double>(entry.data)) {
      result.push_back(std::get<double>(entry.data));
    } else {
      std::stringstream ss;
      ss << "JSON key \"" << key << "\" contains a non-numeric array element.";
      throw std::runtime_error(ss.str());
    }
  }
  return result;
}

std::vector<bool> tarch::reader::JSONReader::getBooleanArray(const std::string& key) const {
  markRead(key);
  const JSONArray&  source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<bool> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<bool>(entry, key, "boolean array element"));
  }
  return result;
}
