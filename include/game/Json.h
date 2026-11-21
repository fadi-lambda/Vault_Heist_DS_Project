#pragma once
#include <string>

namespace VaultHeist {

// Minimal hand-rolled JSON reader (no third-party dependency, no STL containers).
// Supports exactly what the level schema needs: objects, arrays, strings, integers,
// true/false/null. Not a general-purpose parser (no floats/exponents, no \u escapes).

static const int JSON_NULL   = 0;
static const int JSON_BOOL   = 1;
static const int JSON_INT    = 2;
static const int JSON_STRING = 3;
static const int JSON_ARRAY  = 4;
static const int JSON_OBJECT = 5;

class JsonValue {
public:
    JsonValue();
    ~JsonValue();
    JsonValue(const JsonValue&) = delete;
    JsonValue& operator=(const JsonValue&) = delete;

    int  type() const { return kind; }
    bool isNull() const   { return kind == JSON_NULL; }
    bool isInt() const    { return kind == JSON_INT; }
    bool isString() const { return kind == JSON_STRING; }
    bool isArray() const  { return kind == JSON_ARRAY; }
    bool isObject() const { return kind == JSON_OBJECT; }

    int asInt(int fallback = 0) const { return kind == JSON_INT ? intVal : (kind == JSON_BOOL ? intVal : fallback); }
    const std::string& asString() const { return strVal; }

    // Array / object element count
    int size() const { return count; }
    // Array element or object value by position (nullptr when out of range)
    const JsonValue* at(int index) const;
    // Object member lookup by key (nullptr when missing / not an object)
    const JsonValue* get(const char* key) const;
    const std::string& keyAt(int index) const;

    // Convenience accessors with fallbacks
    int         getInt(const char* key, int fallback) const;
    std::string getString(const char* key, const std::string& fallback) const;

    // Parser-facing mutators
    void setKind(int k) { kind = k; }
    void setInt(int v)  { intVal = v; }
    void setString(const std::string& s) { strVal = s; }
    void append(JsonValue* child, const std::string& key);

private:
    int kind;
    int intVal;
    std::string strVal;
    JsonValue** items;
    std::string* keys;
    int count;
    int capacity;
};

// Parses text; returns an owned root (caller deletes) or nullptr with errOut filled.
JsonValue* parseJson(const std::string& text, std::string& errOut);
// Reads a whole file then parses it.
JsonValue* parseJsonFile(const std::string& path, std::string& errOut);

} // namespace VaultHeist
