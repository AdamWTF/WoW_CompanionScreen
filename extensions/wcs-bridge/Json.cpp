#include "Json.hpp"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace wcs_bridge::json
{
    const Value* Value::Find(std::string_view key) const
    {
        const auto* object = ObjectValue();
        if (!object) return nullptr;
        const auto it = object->find(key);
        return it == object->end() ? nullptr : &it->second;
    }

    bool Value::Integer(int64_t& out) const
    {
        const auto* number = std::get_if<double>(&value);
        if (!number || !std::isfinite(*number) || std::floor(*number) != *number ||
            *number < double(INT64_MIN) || *number > double(INT64_MAX)) return false;
        out = static_cast<int64_t>(*number);
        return true;
    }

    bool Value::Boolean(bool& out) const
    {
        const auto* boolean = std::get_if<bool>(&value);
        if (!boolean) return false;
        out = *boolean;
        return true;
    }

    namespace
    {
        void AppendUtf8(std::string& out, uint32_t cp)
        {
            if (cp <= 0x7f) out.push_back(char(cp));
            else if (cp <= 0x7ff)
            {
                out.push_back(char(0xc0 | (cp >> 6))); out.push_back(char(0x80 | (cp & 0x3f)));
            }
            else if (cp <= 0xffff)
            {
                out.push_back(char(0xe0 | (cp >> 12))); out.push_back(char(0x80 | ((cp >> 6) & 0x3f)));
                out.push_back(char(0x80 | (cp & 0x3f)));
            }
            else
            {
                out.push_back(char(0xf0 | (cp >> 18))); out.push_back(char(0x80 | ((cp >> 12) & 0x3f)));
                out.push_back(char(0x80 | ((cp >> 6) & 0x3f))); out.push_back(char(0x80 | (cp & 0x3f)));
            }
        }

        struct Parser
        {
            std::string_view text;
            size_t pos = 0, maxDepth;
            std::string error;

            void Space() { while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\r' || text[pos] == '\n')) ++pos; }
            bool Fail(const char* why) { error = std::string(why) + " at byte " + std::to_string(pos); return false; }

            bool Hex4(uint32_t& cp)
            {
                if (pos + 4 > text.size()) return Fail("short unicode escape");
                cp = 0;
                for (int i = 0; i < 4; ++i)
                {
                    const char c = text[pos++];
                    cp <<= 4;
                    if (c >= '0' && c <= '9') cp |= uint32_t(c - '0');
                    else if (c >= 'a' && c <= 'f') cp |= uint32_t(c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') cp |= uint32_t(c - 'A' + 10);
                    else return Fail("invalid unicode escape");
                }
                return true;
            }

            bool String(std::string& out)
            {
                if (pos >= text.size() || text[pos++] != '"') return Fail("expected string");
                out.clear();
                while (pos < text.size())
                {
                    const unsigned char c = static_cast<unsigned char>(text[pos++]);
                    if (c == '"') return true;
                    if (c < 0x20) return Fail("control character in string");
                    if (c != '\\') { out.push_back(char(c)); continue; }
                    if (pos >= text.size()) return Fail("short escape");
                    switch (text[pos++])
                    {
                    case '"': out.push_back('"'); break; case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break; case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break; case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break; case 't': out.push_back('\t'); break;
                    case 'u':
                    {
                        uint32_t cp = 0; if (!Hex4(cp)) return false;
                        if (cp >= 0xd800 && cp <= 0xdbff)
                        {
                            if (pos + 2 > text.size() || text[pos] != '\\' || text[pos + 1] != 'u') return Fail("unpaired surrogate");
                            pos += 2; uint32_t low = 0; if (!Hex4(low)) return false;
                            if (low < 0xdc00 || low > 0xdfff) return Fail("unpaired surrogate");
                            cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
                        }
                        else if (cp >= 0xdc00 && cp <= 0xdfff) return Fail("unpaired surrogate");
                        AppendUtf8(out, cp); break;
                    }
                    default: return Fail("invalid escape");
                    }
                }
                return Fail("unterminated string");
            }

            bool ValueAt(Value& out, size_t depth)
            {
                if (depth > maxDepth) return Fail("nesting limit exceeded");
                Space(); if (pos >= text.size()) return Fail("expected value");
                const char c = text[pos];
                if (c == '"') { std::string s; if (!String(s)) return false; out = Value(std::move(s)); return true; }
                if (c == '{')
                {
                    ++pos; Value::Object object; Space();
                    if (pos < text.size() && text[pos] == '}') { ++pos; out = Value(std::move(object)); return true; }
                    for (;;)
                    {
                        Space(); std::string key; if (!String(key)) return false; Space();
                        if (pos >= text.size() || text[pos++] != ':') return Fail("expected colon");
                        Value child; if (!ValueAt(child, depth + 1)) return false;
                        object.insert_or_assign(std::move(key), std::move(child)); Space();
                        if (pos < text.size() && text[pos] == '}') { ++pos; out = Value(std::move(object)); return true; }
                        if (pos >= text.size() || text[pos++] != ',') return Fail("expected comma");
                    }
                }
                if (c == '[')
                {
                    ++pos; Value::Array array; Space();
                    if (pos < text.size() && text[pos] == ']') { ++pos; out = Value(std::move(array)); return true; }
                    for (;;)
                    {
                        Value child; if (!ValueAt(child, depth + 1)) return false; array.push_back(std::move(child)); Space();
                        if (pos < text.size() && text[pos] == ']') { ++pos; out = Value(std::move(array)); return true; }
                        if (pos >= text.size() || text[pos++] != ',') return Fail("expected comma");
                    }
                }
                if (text.substr(pos, 4) == "true") { pos += 4; out = Value(true); return true; }
                if (text.substr(pos, 5) == "false") { pos += 5; out = Value(false); return true; }
                if (text.substr(pos, 4) == "null") { pos += 4; out = Value(nullptr); return true; }
                const size_t begin = pos;
                if (text[pos] == '-') ++pos;
                if (pos >= text.size()) return Fail("invalid number");
                if (text[pos] == '0') ++pos;
                else if (text[pos] >= '1' && text[pos] <= '9') while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') ++pos;
                else return Fail("invalid number");
                if (pos < text.size() && text[pos] == '.') { ++pos; const size_t digits = pos; while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') ++pos; if (digits == pos) return Fail("invalid number"); }
                if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E'))
                {
                    ++pos; if (pos < text.size() && (text[pos] == '+' || text[pos] == '-')) ++pos;
                    const size_t digits = pos; while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') ++pos; if (digits == pos) return Fail("invalid number");
                }
                std::string number(text.substr(begin, pos - begin)); char* end = nullptr;
                const double parsed = std::strtod(number.c_str(), &end);
                if (!end || *end || !std::isfinite(parsed)) return Fail("invalid number");
                out = Value(parsed); return true;
            }
        };

        void DumpInto(const Value& value, std::string& out)
        {
            if (value.IsNull()) { out += "null"; return; }
            if (const auto* b = std::get_if<bool>(&value.value)) { out += *b ? "true" : "false"; return; }
            if (const auto* n = std::get_if<double>(&value.value))
            {
                char buf[64]; const int count = std::snprintf(buf, sizeof buf, "%.17g", *n); out.append(buf, size_t(count)); return;
            }
            if (const auto* s = value.String()) { out.push_back('"'); out += Escape(*s); out.push_back('"'); return; }
            if (const auto* a = value.ArrayValue())
            {
                out.push_back('['); bool first = true; for (const auto& item : *a) { if (!first) out.push_back(','); first = false; DumpInto(item, out); } out.push_back(']'); return;
            }
            const auto* object = value.ObjectValue(); out.push_back('{'); bool first = true;
            for (const auto& [key, child] : *object) { if (!first) out.push_back(','); first = false; out.push_back('"'); out += Escape(key); out += "\":"; DumpInto(child, out); }
            out.push_back('}');
        }
    }

    bool Parse(std::string_view source, Value& out, std::string& error, size_t maxDepth)
    {
        Parser parser{source, 0, maxDepth, {}};
        if (!parser.ValueAt(out, 0)) { error = std::move(parser.error); return false; }
        parser.Space(); if (parser.pos != source.size()) { error = "trailing data at byte " + std::to_string(parser.pos); return false; }
        error.clear(); return true;
    }

    std::string Escape(std::string_view value)
    {
        std::string out; out.reserve(value.size());
        static constexpr char hex[] = "0123456789abcdef";
        for (const unsigned char c : value)
        {
            switch (c)
            {
            case '"': out += "\\\""; break; case '\\': out += "\\\\"; break; case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break; case '\n': out += "\\n"; break; case '\r': out += "\\r"; break; case '\t': out += "\\t"; break;
            default: if (c < 0x20) { out += "\\u00"; out.push_back(hex[c >> 4]); out.push_back(hex[c & 15]); } else out.push_back(char(c));
            }
        }
        return out;
    }

    std::string Dump(const Value& value) { std::string out; DumpInto(value, out); return out; }
}
