#include "../include/handle_package/handle_package.h"
#include "../include/toml/toml_parser.h"
#include "../helpers/create_file.h"
#include "../helpers/color.h"
#include <filesystem>
#include <iostream>


void HandlePackage::create_package(const std::string& name) {
    namespace fs = std::filesystem;

    bool in_workspace = fs::exists("cppkg.toml");
    if (in_workspace) {
        try {
            auto toml = parse_toml("cppkg.toml");
            in_workspace = toml.has("workspace", "members");
        } catch (...) {
            in_workspace = false;
        }
    }

    fs::path root = fs::path(name);

    fs::create_directories(root / "src");
    fs::create_directories(root / "build");

    create_file("cppkg.toml", root.string(),
        "[package]\n"
        "name = \"" + name + "\"\n"
        "version = \"0.1.0\"\n"
        "cpp_std = \"c++20\"\n"
        "\n[dependencies]\n"
    );

    create_file(".gitignore", root.string(), "build/\n");

    create_file("main.cpp", (root / "src").string(),
        "#include <iostream>\n\n"
        "int main() {\n"
        "    std::cout << \"Hello from " + name + "!\\n\";\n"
        "    return 0;\n"
        "}\n"
    );

    if (in_workspace) {
        std::fstream ws_toml("cppkg.toml", std::ios::in);
        std::string content((std::istreambuf_iterator<char>(ws_toml)),
                             std::istreambuf_iterator<char>());
        ws_toml.close();

        size_t pos = content.find("members = [");
        if (pos != std::string::npos) {
            size_t close = content.find(']', pos);
            std::string entry = "    \"" + name + "\",\n";
            content.insert(close, entry);

            std::ofstream out("cppkg.toml");
            out << content;
        }
        std::cout << Color::cyan << "Added to workspace: " << name << Color::reset << "\n";
    }

    std::cout << Color::green << "Created package: " << name << Color::reset << "\n";
}

void HandlePackage::create_workspace(const std::string& name) {
    namespace fs = std::filesystem;
    fs::create_directories(name);

    create_file("cppkg.toml", name,
        "[workspace]\n"
        "members = [\n"
        "]\n\n"
        "[dependencies]\n"
        ""
    );

    create_file(".gitignore", name, "build/\n");

    std::cout << Color::green << "Created workspace: " << name << Color::reset << "\n";
}

void HandlePackage::add_dependency(const std::string& package) {
    size_t at = package.find('@');
    if (at == std::string::npos)
        throw std::runtime_error("Invalid format, expected: <package>@<version>");

    std::string pkg_name = package.substr(0, at);
    std::string version  = package.substr(at + 1);

    if (!std::filesystem::exists("cppkg.toml"))
        throw std::runtime_error("No cppkg.toml found in current directory");

    std::ifstream in("cppkg.toml");
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    in.close();

    size_t pos = content.find("[dependencies]");
    if (pos == std::string::npos)
        throw std::runtime_error("No [dependencies] section found in cppkg.toml");

    size_t insert_pos = content.find('\n', pos) + 1;
    std::string entry = pkg_name + " = \"" + version + "\"\n";
    content.insert(insert_pos, entry);

    std::ofstream out("cppkg.toml");
    out << content;

    std::cout << Color::green << "Added: " << pkg_name << " @ " << version << Color::reset << "\n";
}

void HandlePackage::remove_dependency(const std::string& package) {
    std::string pkg_name = package;
    size_t at = package.find('@');
    if (at != std::string::npos)
        pkg_name = package.substr(0, at);

    if (!std::filesystem::exists("cppkg.toml"))
        throw std::runtime_error("No cppkg.toml found in current directory");

    std::ifstream in("cppkg.toml");
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    in.close();

    size_t dep_section = content.find("[dependencies]");
    if (dep_section == std::string::npos)
        throw std::runtime_error("No [dependencies] section found");

    std::string search = pkg_name + " = ";
    size_t pos = content.find(search, dep_section);
    if (pos == std::string::npos)
        throw std::runtime_error("Dependency not found: " + pkg_name);

    size_t line_start = content.rfind('\n', pos) + 1;
    size_t line_end = content.find('\n', pos);
    if (line_end != std::string::npos) line_end++;

    content.erase(line_start, line_end - line_start);

    std::ofstream out("cppkg.toml");
    out << content;

    std::cout << Color::green << "Removed: " << pkg_name << Color::reset << "\n";
}
