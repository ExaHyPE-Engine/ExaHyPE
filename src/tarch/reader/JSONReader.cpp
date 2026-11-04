// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "JSONReader.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"

namespace {
  struct JSONValue;
  using JSONObject = std::unordered_map<std::string, JSONValue>;
  using JSONArray  = std::vector<JSONValue>;

  struct JSONValue final {
    std::variant<std::nullptr_t, bool, int, double, std::string, JSONArray, JSONObject> data{};
  };

  class Parser final {
  public:
    Parser(std::string input, std::string filepath) :
      _input(std::move(input)),
      _filepath(std::move(filepath)) {}

    /**
     * Returns the object at the top level of the input. parseDocument() stops the run with an error
     * for any other top-level value.
     */
    JSONObject parseDocument() {
      skipWhitespace();
      if (current() != '{') {
        tarch::terminateWithError(
          "The JSON reader expects an object at the top level of the file '" + _filepath + "'."
        );
      }
      JSONObject result = parseObject();
      skipWhitespace();
      if (not isAtEnd()) {
        error("The file must end after its top-level value");
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
      message << "The JSON reader cannot parse the file '" << _filepath << "' at position " << _position << ". "
              << reason << ".";
      tarch::terminateWithError(message.str());
    }

    void expect(const char expected) {
      if (advance() != expected) {
        std::stringstream message;
        message << "The JSON syntax expects '" << expected << "'";
        error(message.str());
      }
    }

    void consumeKeyword(const std::string& keyword) {
      for (const char ch : keyword) {
        if (advance() != ch) {
          std::stringstream message;
          message << "The JSON syntax expects \"" << keyword << "\"";
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
        error("The JSON syntax expects a value");
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
          error("The JSON syntax expects a quoted key");
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
        error("A unicode escape needs four hexadecimal digits");
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
      error("A unicode escape needs four hexadecimal digits");
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
            error("A backslash needs an escaped character after it");
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
            error("A backslash escapes the characters \", \\, /, b, f, n, r, t and u only");
          }
        } else {
          result.push_back(ch);
        }
      }

      error("A string must end with a quote");
    }

    JSONValue parseNumber() {
      const std::size_t start = _position;

      if (current() == '-') {
        advance();
      }

      if (std::isdigit(static_cast<unsigned char>(current())) == 0) {
        error("A number must start with a digit");
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
          error("A decimal point needs a digit after it");
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
          error("An exponent needs at least one digit");
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
          error("A floating-point number must fit into a double-precision value");
        }
        return JSONValue{result};
      }

      auto       result = 0;
      const auto parsed = std::from_chars(token.data(), end, result);
      if (parsed.ec != std::errc{} or parsed.ptr != end) {
        error("An integer must fit into 32 bits");
      }
      return JSONValue{result};
    }
  };

  template <typename T>
  const T& requireType(const JSONValue& value, const std::string& key, const char* expectedTypeName) {
    if (not std::holds_alternative<T>(value.data)) {
      std::stringstream message;
      message << "The JSON key \"" << key << "\" expects a value of type " << expectedTypeName << ".";
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
   * Returns the value at the dot-separated key. Returns nullptr when a component is missing or when
   * a component before the last one is not an object.
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
      message << "The JSON reader cannot find the key \"" << key << "\".";
      tarch::terminateWithError(message.str());
    }
    return *value;
  }

  bool hasValue(const JSONObject& root, const std::string& key) { return findValue(root, key) != nullptr; }
} // namespace

struct tarch::reader::JSONReader::Impl final {
  JSONObject root{};
  // Every getter records the top-level key of the value it reads.
  std::unordered_set<std::string> used{};
};

tarch::reader::JSONReader::JSONReader(const std::string& filepath) :
  _impl(std::make_unique<Impl>()) {
  std::ifstream input(filepath.c_str());
  if (not input.is_open()) {
    std::stringstream message;
    message << "The JSON reader cannot open the file '" << filepath << "'.";
    tarch::terminateWithError(message.str());
  }

  std::stringstream buffer;
  buffer << input.rdbuf();
  _impl->root = Parser(buffer.str(), filepath).parseDocument();
}

tarch::reader::JSONReader::~JSONReader() = default;

/**
 * Returns whether the key exists without marking it read.
 */
bool
tarch::reader::JSONReader::hasKey(const std::string& key) const {
  return hasValue(_impl->root, key);
}

void
tarch::reader::JSONReader::markRead(const std::string& key) const {
  // getUnusedKeys() lists top-level keys. The insert records the top-level component only.
  _impl->used.insert(key.substr(0, key.find('.')));
}

/**
 * Returns the sorted top-level keys that no getter read.
 */
std::vector<std::string>
tarch::reader::JSONReader::getUnusedKeys() const {
  std::vector<std::string> result;
  for (const auto& entry : _impl->root) {
    if (not _impl->used.contains(entry.first)) {
      result.push_back(entry.first);
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

std::string
tarch::reader::JSONReader::getString(const std::string& key) const {
  markRead(key);
  return requireType<std::string>(getValue(_impl->root, key), key, "string");
}

int
tarch::reader::JSONReader::getInteger(const std::string& key) const {
  markRead(key);
  return requireType<int>(getValue(_impl->root, key), key, "integer");
}

double
tarch::reader::JSONReader::getDouble(const std::string& key) const {
  markRead(key);
  const JSONValue& value = getValue(_impl->root, key);
  if (std::holds_alternative<int>(value.data)) {
    return static_cast<double>(std::get<int>(value.data));
  }
  return requireType<double>(value, key, "number");
}

bool
tarch::reader::JSONReader::getBoolean(const std::string& key) const {
  markRead(key);
  return requireType<bool>(getValue(_impl->root, key), key, "boolean");
}

std::string
tarch::reader::JSONReader::getString(const std::string& key, const std::string& fallback) const {
  return hasKey(key) ? getString(key) : fallback;
}

int
tarch::reader::JSONReader::getInteger(const std::string& key, const int fallback) const {
  return hasKey(key) ? getInteger(key) : fallback;
}

double
tarch::reader::JSONReader::getDouble(const std::string& key, const double fallback) const {
  return hasKey(key) ? getDouble(key) : fallback;
}

bool
tarch::reader::JSONReader::getBoolean(const std::string& key, const bool fallback) const {
  return hasKey(key) ? getBoolean(key) : fallback;
}

std::vector<std::string>
tarch::reader::JSONReader::getStringArray(const std::string& key) const {
  markRead(key);
  const auto&              source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<std::string> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<std::string>(entry, key, "string array element"));
  }
  return result;
}

std::vector<int>
tarch::reader::JSONReader::getIntegerArray(const std::string& key) const {
  markRead(key);
  const auto&      source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<int> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<int>(entry, key, "integer array element"));
  }
  return result;
}

std::vector<double>
tarch::reader::JSONReader::getDoubleArray(const std::string& key) const {
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
      message << "The JSON key \"" << key << "\" expects an array of numbers.";
      tarch::terminateWithError(message.str());
    }
  }
  return result;
}

std::vector<bool>
tarch::reader::JSONReader::getBooleanArray(const std::string& key) const {
  markRead(key);
  const auto&       source = requireType<JSONArray>(getValue(_impl->root, key), key, "array");
  std::vector<bool> result;
  result.reserve(source.size());
  for (const JSONValue& entry : source) {
    result.push_back(requireType<bool>(entry, key, "boolean array element"));
  }
  return result;
}
