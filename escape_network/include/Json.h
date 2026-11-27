#pragma once

#include <string>

namespace EscapeNetwork {

// Minimal hand-rolled JSON reader (reusing Deliverable 1's Json.h pattern).
// Supports objects, arrays, strings, integers, floating-point numbers, booleans, and null.

static const int JSON_NULL   = 0;
static const int JSON_BOOL   = 1;
static const int JSON_INT    = 2;
static const int JSON_DOUBLE = 3;
static const int JSON_STRING = 4;
static const int JSON_ARRAY  = 5;
static const int JSON_OBJECT = 6;

class JsonValue {
public:
    JsonValue();
    ~JsonValue();
    JsonValue(const JsonValue&) = delete;
    JsonValue& operator=(const JsonValue&) = delete;

    int  type() const     { return kind; }
    bool isNull() const   { return kind == JSON_NULL; }
    bool isBool() const   { return kind == JSON_BOOL; }
    bool isInt() const    { return kind == JSON_INT; }
    bool isNumber() const { return kind == JSON_INT || kind == JSON_DOUBLE; }
    bool isString() const { return kind == JSON_STRING; }
    bool isArray() const  { return kind == JSON_ARRAY; }
    bool isObject() const { return kind == JSON_OBJECT; }

    bool asBool(bool fallback = false) const {
        if (kind == JSON_BOOL) return intVal != 0;
        if (kind == JSON_INT) return intVal != 0;
        return fallback;
    }
    int asInt(int fallback = 0) const {
        if (kind == JSON_INT || kind == JSON_BOOL) return intVal;
        if (kind == JSON_DOUBLE) return static_cast<int>(doubleVal);
        return fallback;
    }
    double asDouble(double fallback = 0.0) const {
        if (kind == JSON_DOUBLE) return doubleVal;
        if (kind == JSON_INT) return static_cast<double>(intVal);
        return fallback;
    }
    const std::string& asString() const { return strVal; }

    int size() const { return count; }
    const JsonValue* at(int index) const;
    const JsonValue* get(const char* key) const;
    const std::string& keyAt(int index) const;

    bool        getBool(const char* key, bool fallback) const;
    int         getInt(const char* key, int fallback) const;
    double      getDouble(const char* key, double fallback) const;
    std::string getString(const char* key, const std::string& fallback) const;

    void setKind(int k) { kind = k; }
    void setInt(int v) { intVal = v; }
    void setDouble(double v) { doubleVal = v; }
    void setString(const std::string& s) { strVal = s; }
    void append(JsonValue* child, const std::string& key);

private:
    int kind;
    int intVal;
    double doubleVal;
    std::string strVal;
    JsonValue** items;
    std::string* keys;
    int count;
    int capacity;
};

JsonValue* parseJson(const std::string& text, std::string& errOut);
JsonValue* parseJsonFile(const std::string& path, std::string& errOut);

} // namespace EscapeNetwork
