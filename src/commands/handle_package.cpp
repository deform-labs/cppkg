#include "handle_package/handle_package.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

void HandlePackage::create_package(const std::string& name) {
    fs::path root = name;

    fs::create_directories(root / "src");
    fs::create_directories(root / "build");

    std::ofstream(root / "cppkg.toml")
        << "[package]\n"
        << "name = \"" << name << "\"\n"
        << "version = \"0.1.0\"\n"
        << "cpp_std = \"c++20\"\n"
        << "\n[dependencies]\n";

    std::ofstream(root / ".gitignore")
        << "build/\n";

    std::ofstream(root / "src" / "main.cpp")
        << "#include <iostream>\n\n"
        << "int main() {\n"
        << "    std::cout << \"Hello from " << name << "!\\n\";\n"
        << "    return 0;\n"
        << "}\n";

    std::cout << "Created package: " << name << "\n";
}
