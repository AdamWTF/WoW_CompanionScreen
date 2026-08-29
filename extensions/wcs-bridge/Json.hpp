// Small, dependency-free JSON value/parser used by the companion wire protocol.
// Copyright (C) 2026 WarcraftXL contributors. GPL-3.0-or-later.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace wcs_bridge::json
{
    struct Value
    {
        using Array = std::vector<Value>;
        using Object = std::map<std::string, Value, std::less<>>;
        using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

        Storage value = nullptr;

        Value() = default;
        Value(std::nullptr_t) : value(nullptr) {}
        Value(bool v) : value(v) {}
        Value(int v) : value(double(v)) {}
        Value(uint32_t v) : value(double(v)) {}
        Value(int64_t v) : value(double(v)) {}
        Value(double v) : value(v) {}
        Value(const char* v) : value(std::string(v ? v : "")) {}
        Value(std::string v) : value(std::move(v)) {}
        Value(Array v) : value(std::move(v)) {}
        Value(Object v) : value(std::move(v)) {}

        bool IsNull() const { return std::holds_alternative<std::nullptr_t>(value); }
        bool IsBool() const { return std::holds_alternative<bool>(value); }
        bool IsNumber() const { return std::holds_alternative<double>(value); }
        bool IsString() const { return std::holds_alternative<std::string>(value); }
        bool IsArray() const { return std::holds_alternative<Array>(value); }
        bool IsObject() const { return std::holds_alternative<Object>(value); }

        const std::string* String() const { return std::get_if<std::string>(&value); }
        const Array* ArrayValue() const { return std::get_if<Array>(&value); }
        Array* ArrayValue() { return std::get_if<Array>(&value); }
        const Object* ObjectValue() const { return std::get_if<Object>(&value); }
        Object* ObjectValue() { return std::get_if<Object>(&value); }
        const Value* Find(std::string_view key) const;
        bool Integer(int64_t& out) const;
        bool Boolean(bool& out) const;
    };

    bool Parse(std::string_view source, Value& out, std::string& error, size_t maxDepth = 32);
    std::string Dump(const Value& value);
    std::string Escape(std::string_view value);
}
