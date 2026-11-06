#include "../structs/toml_struct.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cppkg;

namespace {
    std::string Trim(const std::string& text) {
        const size_t first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return std::string();
        const size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    /// Textual form of an array, kept so callers of get() still see something array-shaped.
    std::string JoinQuoted(const std::vector<std::string>& items) {
        std::string text = "[";
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) text += ", ";
            text += "\"" + items[i] + "\"";
        }
        text += "]";
        return text;
    }
}

std::string Toml::get(const std::string& section, const std::string& key) const {
    auto s = sections.find(section);
    if (s == sections.end())
        throw std::runtime_error(source + ": missing [" + section + "] table (needed for \"" + key + "\")");
    auto k = s->second.keys.find(key);
    if (k == s->second.keys.end())
        throw std::runtime_error(source + ": missing key \"" + key + "\" in [" + section + "]");
    return k->second.is_array ? JoinQuoted(k->second.items) : k->second.value;
}

std::string Toml::get_or(const std::string& section, const std::string& key, const std::string& fallback) const {
    return has(section, key) ? get(section, key) : fallback;
}

bool Toml::has(const std::string& section, const std::string& key) const {
    auto s = sections.find(section);
    if (s == sections.end()) return false;
    return s->second.keys.count(key) > 0;
}

bool Toml::has_section(const std::string& section) const {
    return sections.count(section) > 0;
}

std::vector<std::string> Toml::keys(const std::string& section) const {
    std::vector<std::string> names;
    auto s = sections.find(section);
    if (s == sections.end()) return names;
    for (const auto& entry : s->second.keys)
        names.push_back(entry.first);
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> Toml::array(const std::string& section, const std::string& key) const {
    std::vector<std::string> items;
    auto s = sections.find(section);
    if (s == sections.end()) return items;
    auto k = s->second.keys.find(key);
    if (k == s->second.keys.end() || !k->second.is_array) return items;
    return k->second.items;
}

std::vector<std::string> Toml::parse_array(const std::string& val) {
    std::vector<std::string> result;
    const size_t open  = val.find('[');
    const size_t close = val.rfind(']');

    if (open == std::string::npos || close == std::string::npos || close <= open) return result;

    // Split on commas only outside quotes, so ["a,b"] stays one element.
    std::string item;
    bool quoted = false;
    char quote = '"';

    auto flush = [&]() {
        const std::string trimmed = Trim(item);
        if (!trimmed.empty()) result.push_back(trimmed);
        item.clear();
    };

    for (size_t i = open + 1; i < close; ++i) {
        const char c = val[i];
        if (quoted) {
            if (c == '\\' && quote == '"' && i + 1 < close) {
                item += val[++i];
                continue;
            }
            if (c == quote) {
                quoted = false;
                continue;
            }
            item += c;
            continue;
        }
        if (c == '"' || c == '\'') {
            quoted = true;
            quote = c;
            continue;
        }
        if (c == ',') {
            flush();
            continue;
        }
        item += c;
    }
    flush();

    return result;
}
