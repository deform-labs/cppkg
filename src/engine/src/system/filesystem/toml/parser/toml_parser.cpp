#include "toml_parser.h"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cppkg;

namespace {
    bool IsBareKeyChar(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
    }

    std::string Trim(const std::string& text) {
        const size_t first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return std::string();
        const size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    int HexValue(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return c - 'A' + 10;
    }

    /// Encodes one scalar value as UTF-8, as TOML's \u and \U escapes require.
    void AppendCodepoint(std::string& out, unsigned long codepoint) {
        if (codepoint < 0x80) {
            out += static_cast<char>(codepoint);
        } else if (codepoint < 0x800) {
            out += static_cast<char>(0xC0 | (codepoint >> 6));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            out += static_cast<char>(0xE0 | (codepoint >> 12));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (codepoint >> 18));
            out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }

    /// Cursor over a whole manifest. Every failure names the file and line instead of
    /// silently skipping the statement, which is what the old line-splitter did.
    class Reader {
        public:
            Reader(const std::string& path, const std::string& text) : path_(path), text_(text) {}

            Toml Parse() {
                Toml result;
                result.source = path_;
                std::string section;
                for (;;) {
                    SkipBlank();
                    if (AtEnd()) break;
                    if (Peek() == '[') ParseTable(result, section);
                    else               ParseKeyValue(result, section);
                }
                return result;
            }

        private:
            bool AtEnd() const { return pos_ >= text_.size(); }

            char Peek(size_t ahead = 0) const {
                return pos_ + ahead < text_.size() ? text_[pos_ + ahead] : '\0';
            }

            void Advance() {
                if (pos_ < text_.size() && text_[pos_] == '\n') ++line_;
                ++pos_;
            }

            [[noreturn]] void Fail(const std::string& message) const {
                throw std::runtime_error(path_ + ":" + std::to_string(line_) + ": " + message);
            }

            /// Spaces and tabs only; used where a newline is meaningful.
            void SkipSpaces() {
                while (!AtEnd() && (Peek() == ' ' || Peek() == '\t')) Advance();
            }

            /// Whitespace, blank lines and whole-line comments.
            void SkipBlank() {
                for (;;) {
                    SkipSpaces();
                    if (Peek() == '\r') { Advance(); continue; }
                    if (Peek() == '\n') { Advance(); continue; }
                    if (Peek() == '#') {
                        while (!AtEnd() && Peek() != '\n') Advance();
                        continue;
                    }
                    return;
                }
            }

            /// After a value or table header only a comment may follow on the line.
            void ExpectEndOfStatement() {
                SkipSpaces();
                if (Peek() == '#') {
                    while (!AtEnd() && Peek() != '\n') Advance();
                }
                if (Peek() == '\r') Advance();
                if (AtEnd() || Peek() == '\n') {
                    if (!AtEnd()) Advance();
                    return;
                }
                Fail(std::string("unexpected '") + Peek() + "' after value");
            }

            void ParseTable(Toml& result, std::string& section) {
                const int header_line = line_;
                Advance();  // '['
                const bool array_table = Peek() == '[';
                if (array_table) Advance();

                std::string name;
                for (;;) {
                    if (AtEnd() || Peek() == '\n') Fail("unterminated table header");
                    const char c = Peek();
                    if (c == ']') break;
                    if (c == '"' || c == '\'') { name += ReadQuotedString(); continue; }
                    if (!(IsBareKeyChar(c) || c == '.' || c == ' ')) {
                        Fail(std::string("invalid character '") + c + "' in table name");
                    }
                    name += c;
                    Advance();
                }
                Advance();  // ']'
                if (array_table) {
                    if (Peek() != ']') Fail("unterminated [[array table]] header");
                    Advance();
                }

                // "a . b", "a.b" and "a .b" all name the same table.
                std::string compact;
                for (char c : Trim(name))
                    if (c != ' ' && c != '\t') compact += c;
                if (compact.empty()) {
                    line_ = header_line;
                    Fail("empty table name");
                }
                if (result.sections.count(compact)) {
                    line_ = header_line;
                    Fail("duplicate table [" + compact + "]");
                }

                section = compact;
                result.sections[section];
                ExpectEndOfStatement();
            }

            void ParseKeyValue(Toml& result, const std::string& section) {
                const int key_line = line_;
                std::string key;

                if (Peek() == '"' || Peek() == '\'') {
                    key = ReadQuotedString();
                } else {
                    while (IsBareKeyChar(Peek())) {
                        key += Peek();
                        Advance();
                    }
                }
                if (key.empty()) Fail("expected a key");

                SkipSpaces();
                if (Peek() != '=') Fail("expected '=' after key \"" + key + "\"");
                Advance();
                SkipBlank();

                TomlValue value;
                ReadValue(value);

                auto& table = result.sections[section];
                if (table.keys.count(key)) {
                    line_ = key_line;
                    Fail("duplicate key \"" + key + "\" in [" + (section.empty() ? "root" : section) + "]");
                }
                table.keys[key] = value;
                ExpectEndOfStatement();
            }

            void ReadValue(TomlValue& value) {
                const char c = Peek();
                if (c == '"' || c == '\'') {
                    value.value = ReadQuotedString();
                    return;
                }
                if (c == '[') {
                    ReadArray(value);
                    return;
                }
                if (c == '{') {
                    value.value = ReadInlineTable();
                    return;
                }
                value.value = ReadBareValue();
                if (value.value.empty()) Fail("missing value");
            }

            /// Numbers, booleans and dates are kept verbatim; readers interpret them by type.
            std::string ReadBareValue() {
                std::string text;
                while (!AtEnd()) {
                    const char c = Peek();
                    if (c == '\n' || c == '#' || c == ',' || c == ']') break;
                    text += c;
                    Advance();
                }
                return Trim(text);
            }

            void ReadArray(TomlValue& value) {
                value.is_array = true;
                Advance();  // '['
                for (;;) {
                    SkipBlank();
                    if (AtEnd()) Fail("unterminated array");
                    if (Peek() == ']') {
                        Advance();
                        return;
                    }

                    TomlValue item;
                    ReadValue(item);
                    value.items.push_back(item.is_array ? JoinItems(item.items) : item.value);

                    SkipBlank();
                    if (Peek() == ',') {
                        Advance();
                        continue;
                    }
                    if (Peek() == ']') {
                        Advance();
                        return;
                    }
                    Fail("expected ',' or ']' in array");
                }
            }

            /// Inline tables are kept as text: no manifest field needs to read one back.
            std::string ReadInlineTable() {
                std::string text;
                int depth = 0;
                for (;;) {
                    if (AtEnd()) Fail("unterminated inline table");
                    const char c = Peek();
                    if (c == '\n') Fail("newline inside inline table");
                    if (c == '"' || c == '\'') {
                        const char quote = c;
                        text += c;
                        Advance();
                        while (!AtEnd() && Peek() != quote) {
                            if (quote == '"' && Peek() == '\\') {
                                text += Peek();
                                Advance();
                                if (AtEnd()) break;
                            }
                            text += Peek();
                            Advance();
                        }
                        if (AtEnd()) Fail("unterminated string in inline table");
                        text += Peek();
                        Advance();
                        continue;
                    }
                    if (c == '{') ++depth;
                    if (c == '}') {
                        --depth;
                        if (depth == 0) {
                            text += c;
                            Advance();
                            return text;
                        }
                    }
                    text += c;
                    Advance();
                }
            }

            /// Basic ("...") strings get escape processing, literal ('...') strings do not.
            std::string ReadQuotedString() {
                const char quote = Peek();
                Advance();

                std::string text;
                for (;;) {
                    if (AtEnd()) Fail("unterminated string");
                    const char c = Peek();
                    if (c == quote) {
                        Advance();
                        return text;
                    }
                    if (c == '\n') Fail("unterminated string");
                    if (quote != '"' || c != '\\') {
                        text += c;
                        Advance();
                        continue;
                    }

                    Advance();  // '\'
                    if (AtEnd()) Fail("unterminated escape");
                    const char escape = Peek();
                    switch (escape) {
                        case 'b':  text += '\b'; break;
                        case 't':  text += '\t'; break;
                        case 'n':  text += '\n'; break;
                        case 'f':  text += '\f'; break;
                        case 'r':  text += '\r'; break;
                        case 'e':  text += '\x1b'; break;
                        case '"':  text += '"';  break;
                        case '\\': text += '\\'; break;
                        case 'u':
                        case 'U': {
                            const size_t digits = escape == 'u' ? 4 : 8;
                            Advance();
                            unsigned long codepoint = 0;
                            for (size_t i = 0; i < digits; ++i) {
                                const char hex = Peek();
                                if (!std::isxdigit(static_cast<unsigned char>(hex)))
                                    Fail("invalid unicode escape");
                                codepoint = codepoint * 16 + static_cast<unsigned long>(HexValue(hex));
                                Advance();
                            }
                            AppendCodepoint(text, codepoint);
                            continue;  // digits already consumed
                        }
                        default:
                            Fail(std::string("unknown escape \\") + escape);
                    }
                    Advance();
                }
            }

            static std::string JoinItems(const std::vector<std::string>& items) {
                std::string text = "[";
                for (size_t i = 0; i < items.size(); ++i) {
                    if (i > 0) text += ", ";
                    text += items[i];
                }
                text += "]";
                return text;
            }

            const std::string& path_;
            const std::string& text_;
            size_t pos_ = 0;
            int line_ = 1;
    };
}

Toml toml_parser::parse_toml(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open())
        throw std::runtime_error("Could not open: " + path);

    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // A UTF-8 BOM would otherwise be read as part of the first key.
    if (text.size() >= 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0)
        text.erase(0, 3);

    Reader reader(path, text);
    return reader.Parse();
}

std::vector<std::string> toml_parser::parse_array(const std::string& val) {
    // Kept for callers holding raw text; parse_toml() already splits arrays properly.
    Toml parser;
    return parser.parse_array(val);
}
