#include "Json.h"
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace EscapeNetwork {

JsonValue::JsonValue()
    : kind(JSON_NULL),
      intVal(0),
      doubleVal(0.0),
      items(nullptr),
      keys(nullptr),
      count(0),
      capacity(0) {}

JsonValue::~JsonValue() {
    for (int i = 0; i < count; ++i) {
        delete items[i];
    }
    delete[] items;
    delete[] keys;
}

const JsonValue* JsonValue::at(int index) const {
    if (index < 0 || index >= count) {
        return nullptr;
    }
    return items[index];
}

const std::string& JsonValue::keyAt(int index) const {
    static const std::string empty;
    if (index < 0 || index >= count || !keys) {
        return empty;
    }
    return keys[index];
}

const JsonValue* JsonValue::get(const char* key) const {
    if (kind != JSON_OBJECT) {
        return nullptr;
    }
    for (int i = 0; i < count; ++i) {
        if (keys[i] == key) {
            return items[i];
        }
    }
    return nullptr;
}

bool JsonValue::getBool(const char* key, bool fallback) const {
    const JsonValue* v = get(key);
    return v ? v->asBool(fallback) : fallback;
}

int JsonValue::getInt(const char* key, int fallback) const {
    const JsonValue* v = get(key);
    return (v && v->isNumber()) ? v->asInt(fallback) : fallback;
}

double JsonValue::getDouble(const char* key, double fallback) const {
    const JsonValue* v = get(key);
    return (v && v->isNumber()) ? v->asDouble(fallback) : fallback;
}

std::string JsonValue::getString(const char* key, const std::string& fallback) const {
    const JsonValue* v = get(key);
    return (v && v->isString()) ? v->strVal : fallback;
}

void JsonValue::append(JsonValue* child, const std::string& key) {
    if (count == capacity) {
        int newCap = (capacity == 0) ? 4 : capacity * 2;
        JsonValue** newItems = new JsonValue*[newCap];
        std::string* newKeys = new std::string[newCap];
        for (int i = 0; i < count; ++i) {
            newItems[i] = items[i];
            newKeys[i] = keys[i];
        }
        delete[] items;
        delete[] keys;
        items = newItems;
        keys = newKeys;
        capacity = newCap;
    }
    items[count] = child;
    keys[count] = key;
    ++count;
}

namespace {

struct Cursor {
    const std::string& s;
    int pos;
    std::string err;
    explicit Cursor(const std::string& text) : s(text), pos(0) {}

    bool atEnd() const { return pos >= static_cast<int>(s.size()); }
    char peek() const { return atEnd() ? '\0' : s[pos]; }

    void skipWs() {
        while (!atEnd()) {
            char c = s[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos;
            } else {
                break;
            }
        }
    }

    void fail(const char* msg) {
        if (err.empty()) {
            err = std::string(msg) + " at offset " + std::to_string(pos);
        }
    }
};

JsonValue* parseValue(Cursor& c);

bool parseStringLiteral(Cursor& c, std::string& out) {
    if (c.peek() != '"') {
        c.fail("expected string");
        return false;
    }
    ++c.pos;
    out.clear();
    while (!c.atEnd()) {
        char ch = c.s[c.pos++];
        if (ch == '"') {
            return true;
        }
        if (ch == '\\') {
            if (c.atEnd()) {
                break;
            }
            char e = c.s[c.pos++];
            switch (e) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                default: out += e; break;
            }
        } else {
            out += ch;
        }
    }
    c.fail("unterminated string");
    return false;
}

bool matchWord(Cursor& c, const char* word) {
    int i = 0;
    while (word[i]) {
        if (c.pos + i >= static_cast<int>(c.s.size()) || c.s[c.pos + i] != word[i]) {
            return false;
        }
        ++i;
    }
    c.pos += i;
    return true;
}

JsonValue* parseValue(Cursor& c) {
    c.skipWs();
    char ch = c.peek();
    JsonValue* v = new JsonValue();

    if (ch == '{') {
        v->setKind(JSON_OBJECT);
        ++c.pos;
        c.skipWs();
        if (c.peek() == '}') {
            ++c.pos;
            return v;
        }
        while (true) {
            c.skipWs();
            std::string key;
            if (!parseStringLiteral(c, key)) {
                delete v;
                return nullptr;
            }
            c.skipWs();
            if (c.peek() != ':') {
                c.fail("expected ':'");
                delete v;
                return nullptr;
            }
            ++c.pos;
            JsonValue* child = parseValue(c);
            if (!child) {
                delete v;
                return nullptr;
            }
            v->append(child, key);
            c.skipWs();
            if (c.peek() == ',') {
                ++c.pos;
                continue;
            }
            if (c.peek() == '}') {
                ++c.pos;
                return v;
            }
            c.fail("expected ',' or '}'");
            delete v;
            return nullptr;
        }
    }
    if (ch == '[') {
        v->setKind(JSON_ARRAY);
        ++c.pos;
        c.skipWs();
        if (c.peek() == ']') {
            ++c.pos;
            return v;
        }
        while (true) {
            JsonValue* child = parseValue(c);
            if (!child) {
                delete v;
                return nullptr;
            }
            v->append(child, "");
            c.skipWs();
            if (c.peek() == ',') {
                ++c.pos;
                continue;
            }
            if (c.peek() == ']') {
                ++c.pos;
                return v;
            }
            c.fail("expected ',' or ']'");
            delete v;
            return nullptr;
        }
    }
    if (ch == '"') {
        std::string s;
        if (!parseStringLiteral(c, s)) {
            delete v;
            return nullptr;
        }
        v->setKind(JSON_STRING);
        v->setString(s);
        return v;
    }
    if (ch == '-' || (ch >= '0' && ch <= '9')) {
        const int startPos = c.pos;
        if (ch == '-') {
            ++c.pos;
        }
        if (c.peek() < '0' || c.peek() > '9') {
            c.fail("invalid number");
            delete v;
            return nullptr;
        }
        while (c.peek() >= '0' && c.peek() <= '9') {
            ++c.pos;
        }
        bool isFloat = false;
        if (c.peek() == '.') {
            isFloat = true;
            ++c.pos;
            if (c.peek() < '0' || c.peek() > '9') {
                c.fail("invalid decimal number");
                delete v;
                return nullptr;
            }
            while (c.peek() >= '0' && c.peek() <= '9') {
                ++c.pos;
            }
        }
        std::string numStr = c.s.substr(startPos, static_cast<std::size_t>(c.pos - startPos));
        if (isFloat) {
            v->setKind(JSON_DOUBLE);
            v->setDouble(std::strtod(numStr.c_str(), nullptr));
        } else {
            v->setKind(JSON_INT);
            v->setInt(static_cast<int>(std::strtoll(numStr.c_str(), nullptr, 10)));
        }
        return v;
    }
    if (matchWord(c, "true")) {
        v->setKind(JSON_BOOL);
        v->setInt(1);
        return v;
    }
    if (matchWord(c, "false")) {
        v->setKind(JSON_BOOL);
        v->setInt(0);
        return v;
    }
    if (matchWord(c, "null")) {
        v->setKind(JSON_NULL);
        return v;
    }

    c.fail("unexpected character");
    delete v;
    return nullptr;
}

} // namespace

JsonValue* parseJson(const std::string& text, std::string& errOut) {
    Cursor c(text);
    JsonValue* root = parseValue(c);
    if (!root) {
        errOut = c.err.empty() ? "parse error" : c.err;
        return nullptr;
    }
    c.skipWs();
    if (!c.atEnd()) {
        delete root;
        errOut = "trailing characters after JSON value";
        return nullptr;
    }
    return root;
}

JsonValue* parseJsonFile(const std::string& path, std::string& errOut) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        errOut = "cannot open " + path;
        return nullptr;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text = text.substr(3);
    }
    return parseJson(text, errOut);
}

} // namespace EscapeNetwork
