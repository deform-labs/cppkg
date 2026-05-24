#include "lock_file.h"
#include <sstream>

// format: fmtlib/fmt = "10.1.0" # a1b2c3d
// there you happy? you have the only line of documentation of this whole project here.
static std::string serialize(const LockMap& entries) {
    std::string out;
    for (const auto& [name, entry] : entries)
        out += name + " = \"" + entry.version + "\" # " + entry.hash + "\n";
    return out;
}

static LockMap deserialize(const std::string& content) {
    LockMap result;
    std::istringstream ss(content);
    std::string line;

    while (std::getline(ss, line)) {
        size_t eq = line.find(" = ");
        if (eq == std::string::npos) continue;

        std::string name = line.substr(0, eq);

        size_t hash_pos = line.find(" # ");
        std::string version, hash;

        if (hash_pos != std::string::npos) {
            // get a sick cut from the barber for the quotes
            std::string raw_ver = line.substr(eq + 3, hash_pos - eq - 3);
            size_t a = raw_ver.find_first_not_of(" \t\"");
            size_t b = raw_ver.find_last_not_of(" \t\"");
            version = (a != std::string::npos) ? raw_ver.substr(a, b - a + 1) : "";
            hash = line.substr(hash_pos + 3);
            // get a sick cut from the barber but this time for the hash
            size_t ha = hash.find_first_not_of(" \t\r\n");
            size_t hb = hash.find_last_not_of(" \t\r\n");
            hash = (ha != std::string::npos) ? hash.substr(ha, hb - ha + 1) : "";
        } else {
            std::string raw_ver = line.substr(eq + 3);
            size_t a = raw_ver.find_first_not_of(" \t\"");
            size_t b = raw_ver.find_last_not_of(" \t\"");
            version = (a != std::string::npos) ? raw_ver.substr(a, b - a + 1) : "";
        }

        result[name] = { version, hash };
    }

    return result;
}

bool LockFile::exists() const {
    return std::filesystem::exists(path);
}

LockMap LockFile::read() const {
    if (!exists()) return {};
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return deserialize(content);
}

void LockFile::write(const LockMap& entries) const {
    std::ofstream file(path);
    if (file.is_open())
        file << serialize(entries);
}

void LockFile::upsert(const std::string& name, const std::string& version, const std::string& hash) const {
    auto entries = read();
    entries[name] = { version, hash };
    write(entries);
}

void LockFile::remove_entry(const std::string& name) const {
    auto entries = read();
    entries.erase(name);
    write(entries);
}